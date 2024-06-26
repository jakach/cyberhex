#pragma once
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cctype> // for std::isxdigit
#include <iostream>
#include <future>
#include <vector>
#include <algorithm>
void scan_folder(const std::string& directory);
void action_scanfile(const std::string& filepath);
void action_scanfolder(const std::string& folderpath);
//void action_scanfile_t(const char* filepath);
void scan_file_t(const std::string& filepath_);
int initialize(const std::string& folderPath);
void scan_process_t(const std::string& filepath_);
int get_num_files(const std::string& directory);
int search_hash(const std::string& dbname_, const std::string& hash_, const std::string& filepath_);
void cleanup();
void do_quickscan();
