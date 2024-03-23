#ifndef SCAN_CPP
#define SCAN_CPP
#include "scan.h"
#include <windows.h>
#include <iostream>
#include <openssl/md5.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <time.h>
#include "md5hash.h"
#include <string>
#include "well_known.h"
#include "log.h"
#include "virus_ctrl.h"
#include "app_ctrl.h"
#include <mutex> // Include the mutex header
#include <filesystem>
#include "utils.h"

// Define mutexes for thread synchronization
std::mutex fileHandlesMutex;
std::mutex mappingHandlesMutex;
std::mutex fileDataMutex;
std::mutex cntMutex;
std::mutex numThreadsMutex;

std::unordered_map<std::string, HANDLE> fileHandles;
std::unordered_map<std::string, HANDLE> mappingHandles;
std::unordered_map<std::string, char*> fileData;

int cnt = 0;
int num_threads = 0;
int all_files = 0;

//load all the db files into memory
int initialize(const std::string& folderPath) {
    for (char firstChar = '0'; firstChar <= 'f'; ++firstChar) {
        for (char secondChar = '0'; secondChar <= 'f'; ++secondChar) {
            // Ensure that the characters are valid hexadecimal digits
            if (!std::isxdigit(firstChar) || !std::isxdigit(secondChar) or std::isupper(firstChar) or std::isupper(secondChar)) {
                continue;
            }

            // Create the filename based on the naming convention
            std::string filename = folderPath + "\\" + firstChar + secondChar + ".jdbf";
            //printf("Loading %s\n", filename.c_str());

            // Open the file
            HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                //log(LOGLEVEL::ERR, "[initialize()]: Error opening database file: ", filename);
                return 1;
            }

            // Create the file mapping
            HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMapping == NULL) {
                // log(LOGLEVEL::ERR, "[initialize()]: Error creating database file mapping: ", filename);
                CloseHandle(hFile);
                return 2;
            }

            // Map the file into memory
            char* fileDataPtr = static_cast<char*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
            if (fileDataPtr == NULL) {
                //log(LOGLEVEL::ERR, "[initialize()]: Error mapping database file: ", filename);
                CloseHandle(hMapping);
                CloseHandle(hFile);
                return 3;
            }

            // Store the handles in the global maps
            {
                std::lock_guard<std::mutex> lock(fileHandlesMutex);
                fileHandles[filename] = hFile;
            }
            {
                std::lock_guard<std::mutex> lock(mappingHandlesMutex);
                mappingHandles[filename] = hMapping;
            }
            {
                std::lock_guard<std::mutex> lock(fileDataMutex);
                fileData[filename] = fileDataPtr;
            }
        }
    }
    return 0;
}

// Call this function when you are done using the file mappings
void cleanup() {
    for (const auto& entry : fileHandles) {
        UnmapViewOfFile(fileData[entry.first]);
        CloseHandle(mappingHandles[entry.first]);
        CloseHandle(entry.second);
    }

    // Clear the global maps
    {
        std::lock_guard<std::mutex> lock(fileHandlesMutex);
        fileHandles.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mappingHandlesMutex);
        mappingHandles.clear();
    }
    {
        std::lock_guard<std::mutex> lock(fileDataMutex);
        fileData.clear();
    }
}

//the latest and fastest version of searching a hash by now
int search_hash(const std::string& dbname_, const std::string& hash_, const std::string& filepath_) {
    // Check if the file mapping is already open for the given filename
    std::string dbname;
    std::string hash;
    std::string filepath;
    {
        std::lock_guard<std::mutex> lock(fileHandlesMutex);
        dbname = dbname_;
    }
    {
        std::lock_guard<std::mutex> lock(fileDataMutex);
        hash = hash_;
    }
    {
        std::lock_guard<std::mutex> lock(mappingHandlesMutex);
        filepath = filepath_;
    }

    auto fileIter = fileHandles.find(dbname);
    if (fileIter == fileHandles.end() && dbname_.find("c:.jdbf") == std::string::npos) {
        log(LOGLEVEL::ERR_NOSEND, "[search_hash()]: File mapping not initialized for ", dbname);
        return 2;
    }
    else if (fileIter == fileHandles.end()) {
        return 2;
    }

    // Use fileData for subsequent searches
    DWORD fileSize;
    std::string fileContent;
    {
        std::lock_guard<std::mutex> lock(fileDataMutex);
        fileSize = GetFileSize(fileHandles[dbname], NULL);
        fileContent = std::string(fileData[dbname], fileSize);
    }

    // Search for the specific string in the file content
    size_t foundPos = fileContent.find(hash);
    if (foundPos != std::string::npos) {
        //log(LOGLEVEL::VIRUS, "[search_hash()]: Found virus: ", hash, " in file: ", filepath);
        virus_ctrl_store(filepath.c_str(), hash.c_str(), hash.c_str());
        //afterwards do the processing with that file
        virus_ctrl_process(hash.c_str());
        return 1; // Found
    }
    return 0; // Not found
}

//function to get num of files in idr and its subdirs etc
int get_num_files(const std::string& directory) {
	std::string search_path = directory + "\\*.*";
	WIN32_FIND_DATA find_file_data;
	HANDLE hFind = FindFirstFile(search_path.c_str(), &find_file_data);
	int num_files = 0;
    if (hFind == INVALID_HANDLE_VALUE) {
		log(LOGLEVEL::ERR_NOSEND, "[get_num_files()]: Could not open directory: ", search_path.c_str(), " while scanning files inside directory.");
		return 0;
	}

    do {
        if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) {
			continue; // Skip the current and parent directories
		}

		const std::string full_path = directory + "\\" + find_file_data.cFileName;
        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // If it's a directory, recurse into it
                    num_files += get_num_files(full_path);
        }
        else {
            num_files++;
         }
    } while (FindNextFile(hFind, &find_file_data) != 0);
    FindClose(hFind);
    return num_files;
}

//this is the main function to scan folders. it will then start multuiple threads based on the number of cores / settings
void scan_folder(const std::string& directory) {
    std::string search_path = directory + "\\*.*";
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind = FindFirstFile(search_path.c_str(), &find_file_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        log(LOGLEVEL::WARN, "[scan_folder()]: Could not open directory: ", search_path.c_str(), " while scanning files inside directory.");
        return;
    }

    do {
        if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) {
            continue; // Skip the current and parent directories
        }


        const std::string full_path = directory + "\\" + find_file_data.cFileName;
        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // If it's a directory, recurse into it
            scan_folder(full_path);
        }
        else {
            //action scanfile_t will start the trheads for scanning the hashes
             //action_scanfile_t(full_path.c_str());
             //do multithreading here
            int thread_timeout = 0;
            while (num_threads >= std::thread::hardware_concurrency()) {
                Sleep(10);
                thread_timeout++;
                if (thread_timeout == 100 * 60) {//if there is for more than 30 seconds no thread available, chances are high, that the threads did not temrinate correctly but aren t running anymore. so set the counter to 0 because else it might just stop the scan.
                    num_threads = 0;
                }
            }

            if (is_valid_path(full_path)) { //filter out invalid paths and paths with weird characters
                std::uintmax_t fileSize = std::filesystem::file_size(full_path);
                if (fileSize > 4000000000) {//4gb
                    log(LOGLEVEL::INFO_NOSEND, "[scan_folder()]: File too large to scan: ", full_path);
                }
                else {
                    std::thread scan_thread(scan_file_t, full_path);
                    scan_thread.detach();
                }
            }else
                log(LOGLEVEL::INFO_NOSEND, "[scan_folder()]: Invalid path: ", full_path);
            cnt++;
            if (cnt % 100 == 0) {
                printf("Processed %d files;\n", cnt);
                //printf("Number of threads: %d\n", num_threads);
            }
            if (cnt % 1000 == 0) {
                //send progress to com file
                std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
                if (answer_com.is_open()) {
					answer_com << "progress " <<  (cnt*100/(all_files+1)) << "\n";
					answer_com.close();
				}
            }
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    FindClose(hFind);
}



//for singlethreaded scans
void action_scanfile(const std::string& filepath_) {
    thread_init();
    const std::string filepath(filepath_);
    char* db_path = new char[300];
    char* hash = new char[300];
    if (is_valid_path(filepath_)) { //filter out invalid paths and paths with weird characters
        std::string hash_(md5_file_t(filepath));
        if (strlen(hash_.c_str()) < 290)
            strcpy_s(hash, 295, hash_.c_str());
        else {
            strcpy_s(hash, 295, "");
            log(LOGLEVEL::ERR_NOSEND, "[scan_file_t()]: Could not calculate hash for file: ", filepath);
        }
        sprintf_s(db_path, 295, "%s\\%c%c.jdbf", DB_DIR, hash[0], hash[1]);
        if (search_hash(db_path, hash, filepath) != 1) {
            //notify desktop client by writing to answer_com file
            //if there is now virus, we notify here. if there is a virus we only notify in the virus_ctrl_process function
            std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
            if (answer_com.is_open()) {
                answer_com << "not_found " << "\"" << filepath_ << "\"" << " " << hash << " " << "no_action_taken" << "\n";
                answer_com.close();
            }
        }
    }
	else
		log(LOGLEVEL::INFO_NOSEND, "[action_scanfile()]: Invalid path: ", filepath_);
    thread_shutdown();
}
void action_scanfolder(const std::string& folderpath) {
    thread_init();
    thread_local std::string folderpath_(folderpath);
    cnt = 0;
    all_files = get_num_files(folderpath_);
    //tell the desktop client that the scan has started
    std::ofstream answer_com1(ANSWER_COM_PATH, std::ios::app);
    if (answer_com1.is_open()) {
		answer_com1 << "start " << all_files << "\n";
		answer_com1.close();
	}
    scan_folder(folderpath_);
    std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
    if (answer_com.is_open()) {
        answer_com << "end " << "\"" << "nothing" << "\"" << " " << "nothing" << " " << "nothing" << "\n";
        answer_com.close();
    }
    thread_shutdown();
}

void scan_file_t(const std::string& filepath_) {
    num_threads++;
    thread_local const std::string filepath(filepath_);
    thread_local char* db_path = new char[300];
    thread_local char* hash = new char[300];
    thread_local std::string hash_(md5_file_t(filepath));
    if (strlen(hash_.c_str()) < 290)
        strcpy_s(hash, 295, hash_.c_str());
    else{
        strcpy_s(hash, 295, "");
        log(LOGLEVEL::ERR_NOSEND, "[scan_file_t()]: Could not calculate hash for file: ", filepath);
    }
    sprintf_s(db_path, 295, "%s\\%c%c.jdbf", DB_DIR, hash[0], hash[1]);
    search_hash(db_path, hash, filepath);
    num_threads--;
}
void scan_process_t(const std::string& filepath_) {
    thread_local const std::string filepath(filepath_);
    thread_local char* db_path = new char[300];
    thread_local char* hash = new char[300];
    strcpy_s(hash, 295, md5_file_t(filepath).c_str());
    sprintf_s(db_path, 295, "%s\\%c%c.jdbf", DB_DIR, hash[0], hash[1]);
    if (search_hash(db_path, hash, filepath) == 1) {
        //check if need to kill process
        if (get_setting("virus_ctrl:virus_process_found:kill") == 1) {
            //kill the process
            kill_process(filepath.c_str());
            log(LOGLEVEL::VIRUS, "[scan_process_t()]: Killing process: ", filepath);
        }
    }
}
#endif