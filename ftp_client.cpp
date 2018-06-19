#include "ftp_client.h"

void ftp_client::Open(){
	char buf[BUFSIZ + 1];
	char input[20];
	// Create socket
	m_socket.Create();
	// Connect to server
	int flag = this->m_socket.Connect(CA2CT(m_ip.c_str()), m_port);
	if (flag == 0) {
		cout << "Connect To Server Failed" << endl;
	}
	else {
		cout << "Connect To Server Successfully" << endl;
		// Get ip address of client
		SOCKADDR_IN client_sock_addr;
		int len = sizeof client_sock_addr;
		m_socket.GetSockName((SOCKADDR*)&client_sock_addr, &len);
		unsigned int ip = client_sock_addr.sin_addr.S_un.S_addr;
		unsigned char * tmp_ip = (unsigned char*)&ip;
		m_client_ip = to_string((int)tmp_ip[0]) + "." + to_string((int)tmp_ip[1]) + "." + to_string((int)tmp_ip[2]) + "." + to_string((int)tmp_ip[3]);
		memset(buf, 0, sizeof(buf));
		// Getting greeting message
		while (m_socket.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &m_ftp_code);
			if (CheckCode(220, buf) == false) return;
			char * str = strstr(buf, "220");
			if (str != NULL) break;
			//memset(buf, 0, tmp);
		}
		//USER
		m_ftp_code = 0;
		char * res;
		while (m_ftp_code != 230){
			cout << "Name : ";
			scanf("%s", input);
			memset(buf, 0, sizeof buf);
			sprintf(buf, "USER %s\r\n", input);
			res = SendCommand(buf);
			CheckCode(331, res);
			//PASS
			m_ftp_code = 0;
			memset(input, 0, sizeof input);
			memset(buf, 0, sizeof buf);
			cout << "Password: ";
			scanf("%s", input);
			sprintf(buf, "PASS %s\r\n", input);
			res = SendCommand(buf);
			CheckCode(230, res);
		}
		this->m_connected = true;
	}
}
void ftp_client::Quit(){
	SendCommand("QUIT\r\n");
	m_connected = false;
	m_socket.Close();
}
void ftp_client::Pwd(){
	char * buffer = SendCommand("XPWD\r\n");
	if (CheckCode(257, buffer) == false) return;
	char dir[255];
	sscanf(buffer, "%*s %s %*s", dir);
	cout << dir << endl;
}
void ftp_client::Cd(string path){
	char buffer[BUFSIZ];
	sprintf(buffer, "CWD %s\r\n", path.c_str());
	char * res = SendCommand(buffer);
	CheckCode(250, res);
}
void ftp_client::Lcd(string path){
	string exedir = path.substr(0, path.find_last_of(PATH_SEP) + 1).c_str();
	_chdir(exedir.c_str());
}
void ftp_client::Dir_P(char *command){
	// dir: LIST\r\n
	// ls: NLST\r\n
	char buffer[BUFSIZ * 4];
	int data_port = GetPortFromServer(SendCommand("PASV\r\n"));
	CSocket data_socket; // Socket for data stream
	data_socket.Create();
	if (data_socket.Connect(CA2CT(m_ip.c_str()), data_port) == 0) return;
	m_socket.Send(command, strlen(command), 0);

	int size = data_socket.Receive(buffer, sizeof buffer, 0);
	buffer[size] = '\0';
	cout << buffer;
	data_socket.Close();
	m_socket.Receive(buffer, BUFSIZ, 0);
}
void ftp_client::Dir_A(char *command){
	// dir: LIST\r\n
	// ls: NLST\r\n
	char buffer_recv[BUFSIZ * 4 + 1];
	char buffer_send[BUFSIZ];
	char str_port[100];
	
	CString ip(m_client_ip.c_str());
	CSocket cmd_socket;
	UINT data_port;
	cmd_socket.Create();
	cmd_socket.GetSockName(ip, data_port);
	strcpy(str_port, GetPort(m_client_ip, data_port).c_str());
	memset(buffer_send, 0, sizeof buffer_send);
	sprintf(buffer_send, "PORT %s\r\n", str_port);
	SendCommand(buffer_send);
	if (cmd_socket.Listen(1) == FALSE) return;

	m_socket.Send(command, strlen(command), 0);

	CSocket data_socket;
	int size = 0;
	if (cmd_socket.Accept(data_socket))
		size = data_socket.Receive(buffer_recv, sizeof buffer_recv - 1, 0);
	buffer_recv[size] = '\0';
	cout << buffer_recv;

	m_socket.Receive(buffer_send, BUFSIZ, 0);
	cmd_socket.Receive(buffer_send, BUFSIZ, 0);
	cmd_socket.Close();
	data_socket.Close();
}
void ftp_client::Mkdir(string name){
	char buffer[BUFSIZ];
	sprintf(buffer, "XMKD %s\r\n", name.c_str());
	char * res = SendCommand(buffer);
	CheckCode(257, res);
}
void ftp_client::Rmdir(string name){
	char buffer[BUFSIZ];
	sprintf(buffer, "XRMD %s\r\n", name.c_str());
	char * res = SendCommand(buffer);
	CheckCode(250, res);
}


void ftp_client::Delete_File(string name){
	char buffer[BUFSIZ];
	sprintf(buffer, "DELE %s\r\n", name.c_str());
	char * res = SendCommand(buffer);
	CheckCode(250, res);
}
void ftp_client::Upload_File_P(string name){
	string file = GetFilename(name);
	fstream f;
	f.open(name, ios::in | ios::binary);
	f.seekg(0, f.end);
	int size_of_data = f.tellg();
	f.seekg(0, f.beg);

	int data_port = GetPortFromServer(SendCommand("PASV\r\n"));

	CSocket data_socket;
	data_socket.Create();
	if (data_socket.Connect(CA2CT(m_ip.c_str()), data_port) == 0) return;

	char buf[BUFSIZ];
	char data[1024];
	sprintf(buf, "STOR %s\r\n", file.c_str());
	SendCommand(buf);

	while (!f.eof()) {
		if (size_of_data < 1024) {
			f.read(data, size_of_data);
			data_socket.Send(data, size_of_data, 0);

			break;
		}
		else {
			f.read(data, 1024);
			data_socket.Send(data, 1024, 0);
			size_of_data = size_of_data - 1024;
		}
	}
	data_socket.Close();
	m_socket.Receive(buf, BUFSIZ, 0);
}
void ftp_client::Upload_File_A(string name){
	string file = GetFilename(name);
	fstream f;
	f.open(name, ios::in | ios::binary);
	f.seekg(0, f.end);
	int size_of_data = f.tellg();
	f.seekg(0, f.beg);

	char buffer_send[BUFSIZ];
	char str_port[100];
	CString ip(m_client_ip.c_str());
	CSocket cmd_socket;
	UINT data_port;
	cmd_socket.Create();
	cmd_socket.GetSockName(ip, data_port);
	strcpy(str_port, GetPort(m_client_ip, data_port).c_str());
	memset(buffer_send, 0, sizeof buffer_send);
	sprintf(buffer_send, "PORT %s\r\n", str_port);
	SendCommand(buffer_send);
	if (cmd_socket.Listen(1) == FALSE) return;

	sprintf(buffer_send, "STOR %s\r\n", file.c_str());
	m_socket.Send(buffer_send, BUFSIZ, 0);

	CSocket data_socket;
	char data[1024];
	if (cmd_socket.Accept(data_socket))
	{
		while (!f.eof()) {
			if (size_of_data < 1024) {
				f.read(data, size_of_data);
				data_socket.Send(data, size_of_data, 0);
				break;
			}
			else {
				f.read(data, 1024);
				data_socket.Send(data, 1024, 0);
				size_of_data = size_of_data - 1024;
			}
		}
	}
	m_socket.Receive(buffer_send, BUFSIZ, 0);
	cmd_socket.Receive(buffer_send, BUFSIZ, 0);
	cmd_socket.Close();
	data_socket.Close();
	m_socket.Receive(buffer_send, BUFSIZ, 0);
}
void ftp_client::Download_File_P(string name){
	string file = name;
	fstream f;
	f.open(file, ios::out | ios::binary);

	SendCommand("TYPE I\r\n");
	int data_port = GetPortFromServer(SendCommand("PASV\r\n"));

	CSocket data_socket;
	data_socket.Create();
	if (data_socket.Connect(CA2CT(m_ip.c_str()), data_port) == 0) return;

	char buf[BUFSIZ];
	char data[1024];
	sprintf(buf, "RETR %s\r\n", file.c_str());
	char * res = SendCommand(buf);
	if (m_ftp_code == 550) {
		printf("%s\n", res);
		return;
	}
	int size = 0;
	do{
		memset(buf, 0, sizeof buf);
		size = data_socket.Receive(buf, BUFSIZ, 0);
		f.write(buf, size);
	} while (size > 0);
	f.close();
	data_socket.Close();
	m_socket.Receive(buf, BUFSIZ, 0);
}
void ftp_client::Download_File_A(string name){
	string file = name;
	fstream f;
	f.open(name, ios::out | ios::binary);

	char buffer_send[BUFSIZ];
	char str_port[100];
	CString ip(m_client_ip.c_str());
	CSocket cmd_socket;
	UINT data_port;
	cmd_socket.Create();
	cmd_socket.GetSockName(ip, data_port);
	strcpy(str_port, GetPort(m_client_ip, data_port).c_str());
	memset(buffer_send, 0, sizeof buffer_send);
	sprintf(buffer_send, "PORT %s\r\n", str_port);
	SendCommand(buffer_send);
	if (cmd_socket.Listen(1) == FALSE) return;

	sprintf(buffer_send, "RETR %s\r\n", file.c_str());
	m_socket.Send(buffer_send, BUFSIZ, 0);

	CSocket Connector;
	int tmp = 0;
	if (cmd_socket.Accept(Connector))
	{
		do {
			memset(buffer_send, 0, sizeof buffer_send);
			tmp = Connector.Receive(buffer_send, BUFSIZ, 0);
			f.write(buffer_send, tmp);
		} while (tmp > 0);
	}
	f.close();
	cmd_socket.Close();
	Connector.Close();
	m_socket.Receive(buffer_send, BUFSIZ, 0);
}
void ftp_client::Start(){
	bool error = false;
	cout << "IP Server : ";
	cin >> m_ip;
	m_port = 21;
	while (true) {
		error = false;
		string command;
		fflush(stdout);
		cout.flush();
		cout << "> ";
		fflush(stdin);
		if (cin.peek() == '\n') cin.ignore();
		getline(cin, command);
		string buf;
		stringstream str_stream(command);
		vector<string> param;
		while (str_stream >> buf) {
			param.push_back(buf);
		}
		if (param[0] == "open") {
			if (m_connected) cout << "Connected to Server Already!" << endl;
			else Open();
		}
		else {
			if (!m_connected){ // Not connected
				if (param[0] == "quit" || param[0] == "exit")
					return;
				cout << "Not Connected to Server yet! Try \"open\" to connect to Server" << endl;
			}
			else {
				if (param[0] == "quit" || param[0] == "exit") {
					Quit();
					return;
				}
				else if (param[0] == "passive") m_passive_mode = true;
				else if (param[0] == "dir"){
					if (m_passive_mode) Dir_P("LIST\r\n");
					else Dir_A("LIST\r\n");
				}
				else if (param[0] == "ls"){
					if (m_passive_mode) Dir_P("NLST\r\n");
					else Dir_A("NLST\r\n");
				}
				else if (param[0] == "pwd")
					Pwd();
				else if (param[0] == "cd"){
					if (param.size() < 2) error = true;
					else Cd(param[1]);
				}
				else if (param[0] == "lcd"){
					if (param.size() < 2) error = true;
					else Lcd(param[1]);
				}
				else if (param[0] == "mkdir"){
					if (param.size() < 2) error = true;
					else Mkdir(param[1]);
				}
				else if (param[0] == "rmdir"){
					if (param.size() < 2) error = true;
					else Rmdir(param[1]);
				}
				else if (param[0] == "delete"){
					if (param.size() < 2) error = true;
					else Delete_File(param[1]);
				}
				else if (param[0] == "mdelete"){
					if (param.size() < 2) error = true;
					else 
					for (int i = 1; i < param.size(); i++) {
						cout << "Delete file: " << param[i] << endl;
						Delete_File(param[i]);
					}
				}
				else if (param[0] == "put") {
					if (param.size() < 2) error = true;
					else {
						if (m_passive_mode) Upload_File_P(param[1]);
						else Upload_File_A(param[1]);
					}
				}
				else if (param[0] == "mput") {
					if (param.size() < 2) error = true;
					else {
						if (m_passive_mode) 
							for (int i = 1; i < param.size(); i++) {
								cout << "Upload file: " << param[i] << endl;
								Upload_File_P(param[i]);
							}
						else 
							for (int i = 1; i < param.size(); i++) {
								cout << "Upload file: " << param[i] << endl;
								Upload_File_P(param[i]);
							}
					}
				}
				else if (param[0] == "get") {
					if (param.size() < 2) error = true;
					else {
						if (m_passive_mode) Download_File_P(param[1]);
						else Download_File_A(param[1]);
					}
				}
				else if (param[0] == "mget") {
					if (param.size() < 2) error = true;
					else {
						if (m_passive_mode)
						for (int i = 1; i < param.size(); i++) {
							cout << "Download file: " << param[i] << endl;
							Download_File_P(param[i]);
						}
						else
						for (int i = 1; i < param.size(); i++) {
							cout << "Download file: " << param[i] << endl;
							Download_File_P(param[i]);
						}
					}
				}
				else error = true;
				if (error)
					cout << "Invalid Command" << endl;
			}
		}
	}
}