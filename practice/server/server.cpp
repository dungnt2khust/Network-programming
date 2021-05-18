// MultithreadTCPEchoServer.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNING
#include "stdio.h"
#include <iostream>
#include <string>
#include "string.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "process.h"
#include <fstream>
#include <ctime>
#include <Windows.h>
#define SERVER_PORT 5500
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable:4996)
using namespace std;
#define NUM_THREAD 4
CRITICAL_SECTION critical;

SOCKET listenSock, connSocket;
sockaddr_in clientAddr, serverAddr;
char clientIP[INET_ADDRSTRLEN];
string client_ip, resultCode, clientInfor, accountUser;
char buff[BUFF_SIZE], buff2[BUFF_SIZE];
int ret, clientAddrLen = sizeof(clientAddr), clientPort, errorCode, choose;
fstream ifs, ofs;
const size_t accNum = 4;
int num_thread = 0, index;




// Declare data type account to contain informations of accounts
struct account {
	string accName;
	int allowLogin;
	bool isLogin = false;
};
account* arrAcc;


void initWinsock() {
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		exit(0);
	}
};

void constructSocket() {
	//Step 2: Construct socket		
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
};

void bindAddress() {
	//Step 3: Bind address to socket	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		exit(0);
	}
};

void listenClientRequest() {
	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		exit(0);
	}
};

//Get current time to write in log file
string getCurrentTime() {
	time_t now = time(0);
	tm* ltm = localtime(&now);
	auto day = to_string(ltm->tm_mday),
		month = to_string(ltm->tm_mon + 1),
		year = to_string(ltm->tm_year + 1900),
		hour = to_string(ltm->tm_hour),
		min = to_string(ltm->tm_min),
		second = to_string(ltm->tm_sec);
	string time;
	hour = (stoi(hour) < 10) ? "0" + hour : hour;
	min = (stoi(min) < 10) ? "0" + min : min;
	second = (stoi(second) < 10) ? "0" + second : second;
	time = "[" + day + "/" + month + "/" + year
		+ " " + hour + ":" + min + ":" + second + "] ";
	return time;
};

// Write user actions to file log
void writeToLog(string method, string message, string resultCode) {
	string log = clientInfor + " " + getCurrentTime() + " $ ";
	ofs.open("C:\\Users\\dungg\\Documents\\Visual Studio 2015\\Projects\\Homework03\\Files txt\\log_20183720.txt", ios::out | ios::app);
	log += method + message + " $ " + resultCode;
	ofs << log << endl;
	ofs.close();
}

// Send message to client
void sendMessage() {
	ret = send(connSocket, buff, strlen(buff), 0);
	if (ret == SOCKET_ERROR) {
		printf("Error %d: Cannot send data.\n", WSAGetLastError());
	}
};

// Receive messgae from client
void receiveMessage() {
	memset(buff, 0, BUFF_SIZE);
	ret = recv(connSocket, buff, BUFF_SIZE, 0);
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == 10054) {
			if (choose != 3 && accountUser.length() > 0) {
				writeToLog("QUIT ", accountUser, "30");
				cout << "QUIT " << accountUser << endl;
			}
			printf("Client [%s:%d] disconnects.\n", clientIP, clientPort);
			--num_thread;
			_endthreadex(0);
		}
		else {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
		}
	}
	else if (ret == 0) {
		if (choose != 3 && accountUser.length() > 0) {
			writeToLog("QUIT ", accountUser, "30");
			cout << "QUIT " << accountUser << endl;
		}
		--num_thread;
		printf("Client [%s:%d] disconnects.\n", clientIP, clientPort);

		_endthreadex(0);
	}
	else if (strlen(buff) > 0) {
		buff[ret] = 0;
	}
};

// Read file account.txt to get informations of accounts and save into array arrAcc
void readAccountFile() {
	arrAcc = new account[accNum];
	ifs.open("C:\\Users\\dungg\\Documents\\Visual Studio 2015\\Projects\\Homework03\\Files txt\\account.txt", ios::in);
	for (size_t i = 0; i < accNum; i++) {
		ifs >> arrAcc[i].accName;
		ifs >> arrAcc[i].allowLogin;
	}
	ifs.close();
};

// Resolve when client send login request
void loginServer(char* buff2, string data) {
	bool rightAcc = false;
	memset(buff, 0, BUFF_SIZE);
	memset(buff2, 0, BUFF_SIZE);	
	for (size_t i = 0; i < accNum; i++) {
		if (data == arrAcc[i].accName) {
			rightAcc = true;
			accountUser = data;
			if (arrAcc[i].allowLogin == 0) {
				/*
				EnterCriticalSection(&critical);	
				bool isLogin = arrAcc[i].isLogin;
				index = i;
				if (isLogin == true) {
					resultCode = "13";
					writeToLog("USER ", accountUser, resultCode);
				}
				else {
					arrAcc[i].isLogin = true;
					resultCode = "10";
					writeToLog("USER ", accountUser, resultCode);
					cout << "USER " << accountUser << endl;
				}
				LeaveCriticalSection(&critical);	
				resultCode = "10";
				writeToLog("USER ", accountUser, resultCode);
				cout << "USER " << accountUser << endl;

			}
			else {
				resultCode = "11";
				writeToLog("USER ", accountUser, resultCode);
				accountUser = "";
			}
		}
	}
	if (rightAcc == false) {
		resultCode = "12";
		writeToLog("USER", "", resultCode);
	}
	memset(buff, 0, BUFF_SIZE);
	buff2 = const_cast<char*>(resultCode.c_str());
	strcat_s(buff, buff2);
	sendMessage();
};

void receiveFromClient(char* buff2, string data) {
	memset(buff2, 0, BUFF_SIZE);
	cout << "POST " << accountUser << ":" << endl;
	cout << data << endl;
	resultCode = "20";
	buff2 = const_cast<char*>(resultCode.c_str());
	memset(buff, 0, BUFF_SIZE);
	strcat_s(buff, buff2);
	sendMessage();
	writeToLog("POST ", data, resultCode);
};

void setLogOut() {
	EnterCriticalSection(&critical);	
	arrAcc[index].isLogin = false;
	LeaveCriticalSection(&critical);
};

void logOutServer(char* buff2, string data) {
	memset(buff, 0, BUFF_SIZE);
	memset(buff2, 0, BUFF_SIZE);
	resultCode = "30";
	writeToLog("QUIT ", accountUser, resultCode);
	cout << "QUIT " << accountUser << endl;
	buff2 = const_cast<char*>(resultCode.c_str());
	strcat_s(buff, buff2);
	sendMessage();	
	setLogOut();
};

// Resolve request from client
unsigned __stdcall resolveRequest(void *param) {
	memset(buff, 0, BUFF_SIZE);
	int ret;
	SOCKET connectedSocket = (SOCKET)param;	
	EnterCriticalSection(&critical);
	int number = sharedValue++;
	LeaveCriticalSection(&critical);
	printf("SharedValue: %d\n", number);	
	while (1) {
		receiveMessage();
		string message = (string)buff;
		string data = "";
		choose = stoi(message.substr(0, 1));
		if (message.length() > 1) {
			data = message.substr(1);
		}

		switch (choose) {
		case 1:
			loginServer(buff2, data);
			break;
		case 2:
			receiveFromClient(buff2, data);
			break;
		case 3:
			logOutServer(buff2, data);
			break;
		default:
			break;
		}
	}	
	closesocket(connectedSocket);
	_endthreadex(0);
	return 0;
};


int main(int argc, char* argv[])
{
	//Use command arguments
	int server_port;
	if (argc > 2) {
		printf("Too many arguments. Please retype !!!");
		return 0;
	}
	else if (argc < 2) {
		printf("Please enter a port !!!");
		return 0;
	}
	else {
		//check server port
		try {
			server_port = stoi(argv[1]);
		}
		catch (exception &err) {
			printf("Invalid port !!!");
			return 0;
		}
	}
	//Step 1: Initiate WinSock
	initWinsock();
	constructSocket();
	bindAddress();
	listenClientRequest();
	printf("Server started!\n");
	readAccountFile();
	//Step 5: Communicate with client

	HANDLE myHandle[NUM_THREAD];
	InitializeCriticalSection(&critical);
	while(num_thread < NUM_THREAD){
		connSocket = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
		if (connSocket == SOCKET_ERROR)
			printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
			string deminator = ":";
			clientInfor = clientIP + deminator + to_string(clientPort);
			myHandle[num_thread] = (HANDLE)_beginthreadex(0, 0, resolveRequest, (void*)connSocket, 0, 0);
			num_thread++;
			;
		}	
	}
	WaitForMultipleObjects(NUM_THREAD, myHandle, TRUE, INFINITE);
	DeleteCriticalSection(&critical);
	closesocket(listenSock);

	WSACleanup();

	return 0;
}


