#include "security.h"

int check_cert(const char*cert,const char*secrets_path) {
	FILE* fp;
	if (fopen_s(&fp, secrets_path, "r") != 0) {
		return 1;
	}
	else {
		char*secrets = new char[505];
		while (!feof(fp)) {
			fscanf_s(fp, "%s", secrets, 500); // get the secret
			if (strcmp("cert", secrets) == 0) {
				fscanf_s(fp, "%s", secrets, 500); // get the secret
				if (strcmp(cert, secrets) == 0) {
					delete[] secrets;
					return 0;
				}
			}
		}
		delete[] secrets;
		return 2;
	}

}
char* get_apikey(const char* secrets_path) {
	FILE* fp;
	if (fopen_s(&fp, secrets_path, "r") != 0) {
		return 0;
	}
	else {
		char*secrets = new char[505];
		while (!feof(fp)) {
			fscanf_s(fp, "%s", secrets, 500); // get the secret
			if (strcmp("apikey", secrets) == 0) {
				fscanf_s(fp, "%s", secrets, 500); // get the secret
				return secrets;
			}
		}
		delete[] secrets;
		return 0;
	}
}
char* get_machineid(const char*secrets_path){
	FILE* fp;
	if (fopen_s(&fp, secrets_path, "r") != 0) {
		return 0;
	}
	else {
		char* secrets = new char[300];
		while (!feof(fp)) {
			fscanf_s(fp, "%s", secrets, 295); // get the secret
			if (strcmp("machineid", secrets) == 0) {
				fscanf_s(fp, "%s", secrets, 295); // get the secret
				return secrets;
			}
		}
		delete[] secrets;
		return 0;
	}
}