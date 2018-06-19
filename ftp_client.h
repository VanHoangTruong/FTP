#pragma once

#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include "stdafx.h"
#include <sstream>
#include <vector>
#include <afxsock.h>
#include <iostream>
#include <string>
#include <atlconv.h>
#include <conio.h>
#include <fstream>

#include "direct.h"
#define PATH_SEP '\\'

using namespace std;
class ftp_client {
private:
	CSocket m_socket;
	string m_ip;
	string m_client_ip;
	int m_port;
	int m_ftp_code = 0;
	bool m_passive_mode = false;
	bool m_connected = false;

	void replylogcode(int code)
	{
		switch (code) {
		case 200:
			printf("Command okay");
			break;
		case 500:
			printf("Syntax error, command unrecognized.");
			printf("This may include errors such as command line too long.");
			break;
		case 501:
			printf("Syntax error in parameters or arguments.");
			break;
		case 202:
			printf("Command not implemented, superfluous at this site.");
			break;
		case 502:
			printf("Command not implemented.");
			break;
		case 503:
			printf("Bad sequence of commands.");
			break;
		case 530:
			printf("Not logged in.");
			break;
		}
		printf("\n");
	}
	char * SendCommand(char *str){
		char * buf = new char[BUFSIZ + 1];
		m_socket.Send(str, strlen(str), 0);
		int size = m_socket.Receive(buf, BUFSIZ, 0);
		sscanf(buf, "%d", &m_ftp_code);
		buf[size] = '\0';
		return buf;
	}
	bool CheckCode(int compare_code, char * msg){
		if (m_ftp_code != compare_code){
			printf("%s\n", msg);
			return false;
		}
		return true;
	}
	LPCTSTR StrToIP(string T) {
		return CA2CT(T.c_str());
	}
	string GetPort(string IP, int port) {
		int a = port / 256;
		int b = port % 256;
		char buff[4];
		_itoa(a, buff, 10);
		string a1 = string(buff);

		_itoa(b, buff, 10);
		string b1 = string(buff);
		IP += "," + a1 + "," + b1;
		for (int i = 0; i < IP.size(); i++) {
			if (IP[i] == '.') {
				IP[i] = ',';
			}
		}
		return IP;
	}
	string GetFilename(const string path) {
		return path.substr(path.find_last_of("/\\") + 1);
	}
	int GetPortFromServer(char buf[BUFSIZ + 1]) {
		int a, b;
		char c;
		sscanf(buf, "%*s %*s %*s %*s %c%d,%d,%d,%d,%d,%d", &c, &a, &a, &a, &a, &a, &b);
		return a * 256 + b;
	}
	void Open();
	void Quit();
	void Dir_P(char*);
	void Dir_A(char*);
	void Ls_P();
	void Ls_A();
	void Cd(string);
	void Lcd(string);
	void Mkdir(string);
	void Rmdir(string);
	void Pwd();
	void Upload_File_P(string);
	void Upload_File_A(string);
	void Download_File_P(string);
	void Download_File_A(string);
	void Delete_File(string);
public:
	void Start();
};