/*
This file contains utility functions that are used by the main program. All funcitons that do not belong to one single action / process but are used widely in the whole system are here

Functions:
    - split(): This function splits a string at the delimiter. The delimiter only occurs once. The first part is stored in out1 and the second part in out2.
    - is_valid_path(): This function checks if the path is valid. It checks if the file exists and if the current process has read access to the file.
    - startup(): This function starts the program.
    - get_filename(): This function returns the filename from a path.
    - strcasecmp(): This function compares two strings case-insensitive.
    - kill_process(): This function kills a process.
    - file_exists(): This function checks if a file exists.
    - get_num_running_threads(): This function returns the number of running threads.
    - has_read_access(): This function checks if the current process has read access to the file.
    - delete_all_files(): This function deletes all files in a directory.
    - get_num_threads(): This function returns the number of threads.
    - set_num_threads(): This function sets the number of threads.
    - thread_safety(): This function checks if the thread safety is enabled.
    - to_lower(): This function converts a string to lowercase.
    - matches_pattern(): This function checks if a string matches a pattern.
*/

#include "utils.h"
#include <windows.h>
#include <string.h>
#include <iostream>
#include "log.h"
#include <tlhelp32.h>
#include <winternl.h>
#include <regex>
#include <filesystem>
#include <regex>
#include <mutex>

namespace fs = std::filesystem;

int num_threads = 0;
std::mutex numThreadsMutex;
void split(const std::string& input, char delimiter, std::string& out1, std::string& out2) {
    // Split a string at the delimiter. The delimiter only occurs once. 
    // The first part is stored in out1 and the second part in out2.
    size_t pos = input.find(delimiter);
    if (pos != std::string::npos) {
        out1 = input.substr(0, pos);
        out2 = input.substr(pos + 1);
    }
}

// Check if the path is valid. It checks if the file exists and if the current process has read access to the file.
bool is_valid_path(const std::string& filename) {
    if (!has_read_access(filename)) {//this also fails if the file does not exist
		return 0; // No read access
	}
    return 1; // No special character found
}

// Check if a string matches a pattern
bool matches_pattern(const std::string& str, const std::string& pattern) {
    std::string::const_iterator str_it = str.begin();
    std::string::const_iterator pattern_it = pattern.begin();

    while (str_it != str.end() && pattern_it != pattern.end()) {
        if (*pattern_it == '*') {
            // Skip consecutive '*' in the pattern
            while (pattern_it != pattern.end() && *pattern_it == '*') {
                ++pattern_it;
            }
            if (pattern_it == pattern.end()) {
                return true; // Trailing '*' matches everything remaining
            }
            // Find the next matching character in the str
            while (str_it != str.end() && tolower(*str_it) != tolower(*pattern_it)) {
                ++str_it;
            }
        }
        else if (tolower(*str_it) == tolower(*pattern_it)) {
            ++str_it;
            ++pattern_it;
        }
        else {
            return false;
        }
    }

    // Skip trailing '*' in the pattern
    while (pattern_it != pattern.end() && *pattern_it == '*') {
        ++pattern_it;
    }

    // If we've reached the end of the pattern and str, or the pattern ends with '*', it's a match
    return pattern_it == pattern.end();
}

// Convert a string to lowercase
std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

// Starts a process in a second, completly decoupled thread. used for the update process
void startup(LPCTSTR lpApplicationName)
{
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    CreateProcess(lpApplicationName,   // the path
        NULL,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure
    );
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// Get the filename from a path
std::string get_filename(const std::string& path) {
    auto pos = path.find_last_of("\\");
    if (pos == std::string::npos) {
        // No directory separator found, return the original path
        return path;
    }
    else {
        // Return the substring after the last directory separator
        return path.substr(pos + 1);
    }
}

// Compare two strings case-insensitive
int strcasecmp(const std::string& s1, const std::string& s2) {
    auto it1 = s1.begin();
    auto it2 = s2.begin();
    while (it1 != s1.end() && it2 != s2.end()) {
        int diff = std::tolower(*it1) - std::tolower(*it2);
        if (diff != 0)
            return diff;
        ++it1;
        ++it2;
    }
    return 0;
}


// Kill a process, used by RTP proccess scanner
void kill_process(const std::string& path) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes)
    {
        if (strcasecmp(pEntry.szExeFile, get_filename(path).c_str()) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, static_cast<DWORD>(pEntry.th32ProcessID));
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
            else
                log(LOGLEVEL::ERR, "[kill_process()]: Error while killing process: ", path);
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}

// Check if a file exists! this function is prety slow, so not used extensivly. Normaly is_valid_path is used instead
bool file_exists(const std::string& filePath) {
    DWORD fileAttributes = GetFileAttributes(filePath.c_str());

    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        // The file does not exist or there was an error
        return false;
    }

    // Check if it's a regular file and not a directory
    return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

// Get the number of running threads! This function is prety slow, so not used extensivly
int get_num_running_threads() {
    DWORD runningThreads = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(hSnapshot, &te)) {
            do {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
                    sizeof(te.th32OwnerProcessID)) {
                    if (te.th32OwnerProcessID == GetCurrentProcessId()) {
                        runningThreads++;
                    }
                }
                te.dwSize = sizeof(THREADENTRY32);
            } while (Thread32Next(hSnapshot, &te));
        }

        CloseHandle(hSnapshot);
    }

    return runningThreads;
}

// Check if the current process has read access to the file
bool has_read_access(const std::string &path) {
	// Check if the current process has read access to the file
    FILE* fp;
    if(fopen_s(&fp, path.c_str(), "r")==0){
        fclose(fp);
		return true;
	}
	return false;
}

// Delete all files in a directory
void delete_all_files(const std::string& directoryPath) {
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry)) {
            try {
                fs::remove(entry.path());
            }
            catch (const std::exception& e) {
            }
        }
    }
}

// Get the number of threads of the system. measured by a state machine, so maybe incorect
int get_num_threads() {
    std::lock_guard<std::mutex> lock(numThreadsMutex);
    return num_threads;
}

// Set the number of threads of the system
int set_num_threads(int num) {
    std::lock_guard<std::mutex> lock(numThreadsMutex);
    num_threads = num;
    return 0;
}

// Check if the thread safety is enabled
bool thread_safety() { //if this is set to false the deepscan funcitons will utilize up to thousands of threads and completely destroy your machine. but it will be fast.
    return true;
}