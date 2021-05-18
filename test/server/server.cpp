
// server.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "Winsock2.h"
#include "Ws2tcpip.h"
#include "iostream"
#include "fstream"
#include "string"
#include "process.h"
#include "ctime"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

SOCKET listenSock;
SOCKET acceptSock;
fstream accountFile;
ofstream logFile;
sockaddr_in clientAddr;
int clientLength = sizeof(clientAddr);

// Initiate Winsock
void initiateWinsock() {

	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);
	if (WSAStartup(version, &wsaData)) {
		printf("Winsock 2 is not supported\n");
		exit(0);
	}
}

// Construct socket
void constructSocket() {

	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket", WSAGetLastError());
		exit(0);
	}
}

// Bind address to socket
void bindSocket(char* serverPort) {

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(serverPort));
	inet_pton(AF_INET, SERVER_ADDR, &addr.sin_addr);
	if (bind(listenSock, (sockaddr *)&addr, sizeof(addr))) {
		printf("Error %d: Cannot associate a local  address with server socket.", WSAGetLastError());
		exit(0);
	}
}

// Listen Request from client
void listenRequest() {

	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state Listen", WSAGetLastError());
		exit(0);
	}

	printf("Server Started!");
}

void sendMessage(char *rs, SOCKET &connectedSocket) {
	// Send message
	int ret = send(connectedSocket, rs, strlen(rs), 0);
	if (ret == SOCKET_ERROR) {
		printf("Error %d: Can't send data.\n", WSAGetLastError());
	}
}

// Log file
void writeInLogFile(string log) {
	logFile.open("C:\\Users\\dungg\\Documents\\Visual Studio 2015\\Projects\\test\\files txt\\log_20183720.txt", ios::out | ios::app);
	// Write in file log
	if (logFile.is_open()) {
		logFile << log + "\n";
		logFile.close();
	}
	else cout << "Unable to open file.";
}

// Handle Login function
void logIn(string data, SOCKET &connectedSocket, string &log) {
	char rs[BUFF_SIZE];
	memset(rs, 0, BUFF_SIZE);
	accountFile.open("C:\\Users\\dungg\\Documents\\Visual Studio 2015\\Projects\\test\\files txt\\account.txt", ios::in);

	string line;
	while (!accountFile.eof())
	{
		getline(accountFile, line);
		string username = line.substr(0, line.find(" "));
		if (username == data) {
			string status = line.substr(line.find(" ") + 1);
			if (status == "1") {
				strcat_s(rs, "406 User has been locked");
				log += "406";
			}
			else {
				strcat_s(rs, "200 Login successful!");
				log += "200";
			}
			break;
		}
	}
	if (strlen(rs) == 0) {
		strcat_s(rs, "406 User does not exist!");
		log += "406";
	}
	sendMessage(rs, connectedSocket);
	writeInLogFile(log);
	accountFile.close();
}

// Handle Post Message Function
void postMessage(string data, SOCKET &connectedSocket, string &log) {
	char rs[BUFF_SIZE];
	memset(rs, 0, BUFF_SIZE);

	strcat_s(rs, "200 Post sucessful!");
	log += "200";
	// Send Message to Client
	sendMessage(rs, connectedSocket);
	writeInLogFile(log);
}

void logOut(SOCKET &connectedSocket, string &log) {
	char rs[BUFF_SIZE];
	memset(rs, 0, BUFF_SIZE);
	strcat_s(rs, "200 Logout sucessfull!");
	log += "200";

	// Send message
	sendMessage(rs, connectedSocket);
	writeInLogFile(log);
}

// Selected by the user
void chooseFunction(char *buff, SOCKET &connectedSocket, string &log) {

	string str(buff);
	log += str + "$";
	string key = str.substr(0, 4);
	string data;
	if (str.length() > 4) {
		data = str.substr(5);
	}
	if (key == "USER") {
		logIn(data, connectedSocket, log);
	}
	else if (key == "POST") {
		postMessage(data, connectedSocket, log);
	}
	else if (key == "QUIT") {
		logOut(connectedSocket, log);
	}
}

// Return time
void returnCurrentTime(string &log) {
	log += "[";
	time_t current = time(0);
	tm *ltm = localtime(&current);
	log += to_string(ltm->tm_mday) + "/";
	log += to_string(ltm->tm_mon + 1) + "/";
	log += to_string(ltm->tm_year + 1900) + " ";
	log += to_string(ltm->tm_hour) + ":";
	log += to_string(ltm->tm_min) + ":";
	log += to_string(ltm->tm_sec) + "]" + "$";
}

// receive message from client
void recvMessage(char *buff, char *clientIP, int clientPort, SOCKET &connectedSocket) {
	string log;
	memset(buff, 0, BUFF_SIZE);

	int ret = recv(connectedSocket, buff, BUFF_SIZE, 0);
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == 10054) {
			printf("Client [%s:%d] disconnects. \n", clientIP, clientPort);
			_endthreadex(0);
		}
		else {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
		}
	}
	else if (ret == 0) {
		printf("Client [%s:%d] disconnects. \n", clientIP, clientPort);
		_endthreadex(0);
	}
	else {
		buff[ret] = 0;
		printf("Receive from client [%s:%d]: %s\n", clientIP, clientPort, buff);
		log = clientIP;
		log += ":" + to_string(clientPort);
		returnCurrentTime(log);
		// handle message
		chooseFunction(buff, connectedSocket, log);
	}
}

// Thread to receive and process message from client
unsigned __stdcall echoThread(void *param) {

	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int clientPort;

	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
	clientPort = ntohs(clientAddr.sin_port);
	printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);

	SOCKET connectedSocket = (SOCKET)param;
	// communicate with client
	while (1)
	{
		recvMessage(buff, clientIP, clientPort, connectedSocket);
	}
	closesocket(connectedSocket);
	_endthreadex(0);
	return 0;

}

// accept request
void acceptRequest() {
	clientAddr.sin_family = AF_INET;

	acceptSock = accept(listenSock, (sockaddr *)&clientAddr, &clientLength);
	if (acceptSock == SOCKET_ERROR) {
		printf("Error %d: Cannot permit incoming connection", WSAGetLastError());
		exit(0);
	}
	else {
		_beginthreadex(0, 0, echoThread, (void *)acceptSock, 0, 0);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Enter error, please enter like this: server.exe 5500");
		return 0;
	}
	char* serverPort = argv[1];

	initiateWinsock();
	constructSocket();
	bindSocket(serverPort);
	listenRequest();
	while (1) {
		acceptRequest();
	}
	//	communicateClient();

	// Close Socket
	closesocket(acceptSock);
	closesocket(listenSock);

	// Terminate Winsock
	WSACleanup();
	return 0;
}

