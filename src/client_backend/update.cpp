/*
This file contains the functions to update the cyberhex system. And its components

Functions:
    - update_system(): This function updates the cyberhex system.
    - update_yara(): This function updates the yara databases.
    - update_db2(): This function updates the sig databases.
    - update_db(): This function updates the databases. Calls update_db2()
    - update_settings(): This function updates the settings.
    - action_update_settings(): This function updates the settings. if caled by scheduler
    - action_update_db(): This function updates the databases. if called by scheduler

*/

#ifndef UPDATE_CPP
#define UPDATE_CPP

#include "update.h"
#include "log.h"
#include "connect.h"
#include "settings.h"
#include "security.h"
#include "app_ctrl.h"
#include "utils.h"
#include <string>
#include <iostream>
#include <cctype>
#include <filesystem>
#include "zip.h"

int update_system() {
    thread_init();
    log(LOGLEVEL::INFO, "[update_system()]: Updating system");
    //update cyberhex executables
    //we will download them from the jakach server, not the companys server

    //download version of cyberhex. because we do not know, if a new one is available.
    std::string url = JAKACH_UPDATE_SRV;
    url += "version.txt";
    int res = 0;
    char*server_response = new char[100];
    if (connect_to_srv(url, server_response, 100, 0) != 0 ) {
        log(LOGLEVEL::ERR, "[update_system()]: Error downloading cyberhex version file from server. ERROR:");
    }
    else if (atoi(server_response) > CURRENT_VERSION) {
        std::string url = JAKACH_UPDATE_SRV;
        url += "cyberhex_latest.exe";
        std::string output_path = APP_DIR;
        output_path += "\\cyberhex_latest.exe";

        res = download_file_from_srv(url, output_path, 0, 1);
        if (res != 0) {
            log(LOGLEVEL::ERR, "[update_system()]: Error downloading cyberhex update file from server. ERROR:", res);
            return 1; // Error downloading file
        }
        //download the updater
        url = JAKACH_UPDATE_SRV;
        url += "cyberhex_update.exe";
        output_path = APP_DIR;
        output_path += "\\cyberhex_update.exe";

        res = download_file_from_srv(url, output_path, 0, 1);
        if (res != 0) {
            log(LOGLEVEL::ERR, "[update_system()]: Error downloading cyberhex update file from server. ERROR:", res);
            return 1; // Error downloading file
        }
        //start the update executable, which will install the new version of cyberhex
        log(LOGLEVEL::INFO, "[update_system()]: Downloaded update files. Starting the update executable. Cyberhex will now be terminated!");
        startup(output_path.c_str());
        log(LOGLEVEL::INFO, "[update_system()]: Update executable started. Cyberhex will now be terminated!");
        exit(0);
    }
    else {
		log(LOGLEVEL::INFO, "[update_system()]: No new version of cyberhex available.");
	}
    thread_shutdown();
    return 0;
}

int update_yara(const std::string& folder_path) {
    //remove the old databases
    std::string path = folder_path + "\\";
    delete_all_files(folder_path); //remove all files in the folder

    std::string url = get_setting_string("server:server_url");
    if (url.empty() || url == "nan") {
        return 2; // Invalid server URL
    }
    url += "/database_srv/yara.zip";
    std::string output_path = folder_path + "\\" + "yara.zip";
    int res = download_file_from_srv(url, output_path, get_setting("communication:unsafe_tls"), 1);

    if (res != 0) {
        return 10; // Error downloading file
    }

    //unzip the file
    unzip(output_path, folder_path);
    return 0;
}

int update_db2(const std::string&folder_path) {
    //remove the old databases
    std::string path = folder_path + "\\";
    delete_all_files(folder_path); //remove all files in the folder

    std::string url = get_setting_string("server:server_url");
    if (url.empty() || url == "nan") {
        return 2; // Invalid server URL
    }
    url += "/database_srv/sig.zip";
    std::string output_path = folder_path + "\\" + "sig.zip";
    int res = download_file_from_srv(url, output_path, get_setting("communication:unsafe_tls"), 1);

    if (res != 0) {
		return 10; // Error downloading file
	}

    //unzip the file
    unzip(output_path, folder_path);
    return 0;
}

int update_db(const std::string& folder_path) {
    //check if we have connection to the server
    char*buffer=new char [100];
    std::string output_path = folder_path + "\\" + "test.tmp";
    if (download_file_from_srv(get_setting_string("server:server_url"), output_path, get_setting("communication:unsafe_tls"), 1)!=0) {
        log(LOGLEVEL::ERR, "[update_db()]: Error connecting to server.");
		return 1; // Error connecting to server
	}
    delete buffer;
    update_yara(YARA_DB_DIR); //update the yara databases
    return update_db2(folder_path); //redirect to the new update funtion
    return 0;
}

int update_settings(const std::string& settings_type) {
    // Create the strings to download the files
    std::string url = get_setting_string("server:server_url");
    if (url.empty() || url == "nan") {
        return 2; // Invalid server URL
    }
    url += "/api/php/settings/get_settings.php?";
    url += settings_type + "&machine_id=" + get_machineid(SECRETS) + "&apikey=" + get_apikey(SECRETS);

    int res = 1;
    if (settings_type == "settings")
        res = download_file_from_srv(url, SETTINGS_DB, get_setting("communication:unsafe_tls"),0);
    else if (settings_type == "rtp_included")
        res = download_file_from_srv(url, INCLUDED_FOLDERS, get_setting("communication:unsafe_tls"),0);
    else if (settings_type == "rtp_excluded")
        res = download_file_from_srv(url, EXCLUDED_FOLDERS, get_setting("communication:unsafe_tls"),0);
    else if (settings_type == "sched")
        res = download_file_from_srv(url, SCHED_PATH, get_setting("communication:unsafe_tls"),0);
    else if (settings_type == "disallowed_start")
        res = download_file_from_srv(url, DISALLOWED_START_FOLDERS, get_setting("communication:unsafe_tls"), 0);

    if (res != 0) {
        log(LOGLEVEL::ERR, "[update_settings()]: Error downloading settings database file from server. ERROR:", res);
        return 1; // Error downloading file
    }

    return 0;
}

//update the settings. if caled by scheduler
int action_update_settings() {
    // Update the settings
    int err = 0;
    if (update_settings("settings") != 0)
        err = 1;

    // Update the included folders
    if (update_settings("rtp_included") != 0)
        err = 2;

    // Update the excluded folders
    if (update_settings("rtp_excluded") != 0)
        err = 3;

    // Update the schedule
    if (update_settings("sched") != 0)
        err = 4;

    return err;
}

int action_update_db() {
    // Update the databases
    return update_db(DB_DIR);
}

#endif // UPDATE_CPP
