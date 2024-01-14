#ifndef UPDATE_CPP
#define UPDATE_CPP
#include "update.h"
#include "log.h"
#include "connect.h"
#include "settings.h"


int update_db(const std::string& folder_path) {
	//download the databases from the server
	for (char firstChar = '0'; firstChar <= 'f'; ++firstChar) {
		for (char secondChar = '0'; secondChar <= 'f'; ++secondChar) {
			// Ensure that the characters are valid hexadecimal digits
			if (!std::isxdigit(firstChar) || !std::isxdigit(secondChar) or std::isupper(firstChar) or std::isupper(secondChar)) {
				continue;
			}

			// Create the filename based on the naming convention
			std::string file_path = folder_path + "\\" + firstChar + secondChar + ".jdbf";
			std::string file_name = firstChar + secondChar + ".jdbf";
			//create the strings to download the files
			char*url=new char[300];
			char*output_path=new char[300];
			get_setting("server:server_url", url);
			strcat_s(url, 295,"/database/");
			strcat_s(url, 295,file_name.c_str() );
			strcpy_s(output_path, 295, file_path.c_str());

			int res = download_file_from_srv(url, output_path);
			if (res != 0) {
				log(LOGLEVEL::ERR, "[update_db()]: Error downloading database file from server", url);
				return 1;
			}


			delete[] url;
			delete[] output_path;

		}
	}
	return 0;
}
int update_settings(const std::string& folder_path) {
	//create the strings to download the files
	char* url = new char[300];
	get_setting("server:server_url", url);
	strcat_s(url, 295, "/database/");
	strcat_s(url, 295, "settings_db.txt");
	int res = download_file_from_srv(url, SETTINGS_DB);
	if (res != 0) {
		log(LOGLEVEL::ERR, "[update_db()]: Error downloading settings database file from server", url);
		return 1;
	}

	delete[] url;
	return 0;
}
#endif