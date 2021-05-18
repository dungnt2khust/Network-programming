// SelectTCPServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <conio.h>
#include <fstream>
#include <ctime>

#pragma comment (lib,"ws2_32.lib")
#pragma warning (disable:4996)
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

using namespace std;

SOCKET client[FD_SETSIZE], connSock, listenSock;
fd_set readfds, initfds; //use initfds to initiate readfds at the begining of every loop step
sockaddr_in clientAddr,serverAddr;
int ret, nEvents, clientAddrLen;
char rcvBuff[BUFF_SIZE];
int serverPort;
string resultCode;
fstream ifs, ofs;
const size_t accNum = 4;
bool isConnect = false;
int index;
char client_ip[INET_ADDRSTRLEN];
string clientIn4[FD_SETSIZE], accountName[FD_SETSIZE];

// Declare data type account to contain informations of accounts
struct account {
	string accName;
	int allowLogin;
};

account* arrAcc;

int Receive(SOCKET, char *, int, int, string&, string&);
int Send(SOCKET, char *, int, int);
void initiateWinSock();
void constructSocket();
void bindAddressToSocket();
void listenRequest();
void connectWithClient();
void resolveRequest(char* rcvBuff, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void loginServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void receiveFromClient(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void logOutServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
string getCurrentTime();
void writeToLog(string method, string message, string resultcode, string clientInfor);
void readAccountFile();

int _tmain(int argc, _TCHAR* argv[])
{
	//use command arguments
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
			serverPort = stoi(argv[1]);
		}
		catch (exception &err) {
			printf("Invalid port !!!");
			return 0;
		}
	}
	//Step 1: 
	initiateWinSock();
	//Step 2: Construct socket	
	constructSocket();
	//Step 3: Bind address to socket
	bindAddressToSocket();
	//Step 4: Listen request from client
	listenRequest();

	//Step 5: Communicate with clients
	readAccountFile();
	connectWithClient();
	closesocket(listenSock);
	WSACleanup();
	return 0;
}


/* The recv() wrapper function */
// The parameters of this function is connected socket, buff, size of buff, flags, and reference accountUser, clientInfor in arrays contain information
// This function return size of received message
int Receive(SOCKET s, char *buff, int size, int flags, string &accountUser, string &clientInfor) {
	int n;
	memset(buff, 0, BUFF_SIZE);
	n = recv(s, buff, size, flags);
	if (n == SOCKET_ERROR) {
		if (WSAGetLastError() == 10054) {
			if (accountUser.length() != 0) {
				writeToLog("QUIT ", accountUser, resultCode, clientInfor);
				cout << "QUIT " << accountUser << " [" << clientInfor << "]" << endl;
				accountUser = "";
			}
			cout << "Client [" << clientInfor << "] disconnect. " << endl;
			isConnect = false;
		}
		else
			printf("\nError %d: Cannot receive data.", WSAGetLastError());
	}
	return n;
}

/* The send() wrapper function*/
// The parameters of this function is connected socket, buff, size of buff and flags
// This function return size of sent message
int Send(SOCKET s, char *buff, int size, int flags) {
	int n;
	n = send(s, buff, size, flags);
	if (n == SOCKET_ERROR)
		printf("Error %d: Cannot send data.", WSAGetLastError());

	return n;
}

// Initiate Winsock
void initiateWinSock() {
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");
};

// Construct Socket
void constructSocket() {
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
};

// Bind Address To Socket
void bindAddressToSocket() {
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return;
	}
};

// Listen Request
void listenRequest() {
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		_getch();
		return;
	}

	printf("Server started!\n");
};

// This function use to connect with client, listen events and set up connections
void connectWithClient() {
	for (int i = 0; i < FD_SETSIZE; i++)
		client[i] = 0;	// 0 indicates available entry

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {

				int i;
				for (i = 0; i < FD_SETSIZE; i++)
					if (client[i] == 0) {
						client[i] = connSock;
						FD_SET(client[i], &initfds);
						break;
					}

				if (i == FD_SETSIZE) {
					printf("\nToo many clients.\n");
					closesocket(connSock);
				}
				else {
					printf("You got a connection from [%s:%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port)); /* prints client's IP */	
					clientIn4[i] = string(inet_ntoa(clientAddr.sin_addr)) + ':' + to_string(ntohs(clientAddr.sin_port));
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}

		//receive data from clients
		for (int i = 0; i < FD_SETSIZE; i++) {	
			if (client[i] == 0)
				continue;

			if (FD_ISSET(client[i], &readfds)) {
				ret = Receive(client[i], rcvBuff, BUFF_SIZE, 0, accountName[i], clientIn4[i]);
				if (ret <= 0) {
					FD_CLR(client[i], &initfds);
					closesocket(client[i]);
					client[i] = 0;
					if (isConnect == true) {
						if (accountName[i].length() != 0) {
							writeToLog("QUIT ", accountName[i], resultCode, clientIn4[i]);
							cout << "QUIT " << accountName[i] << " [" << clientIn4[i] << "]" << endl;
							accountName[i] = "";
						}
						cout << "Client [" << clientIn4[i] << "] disconnect." << endl;
					}
				}
				else if (ret > 0) {
					rcvBuff[ret] = 0;
					isConnect = true;	
					resolveRequest(rcvBuff, client[i], accountName[i], clientIn4[i]);
				}
			}
			if (--nEvents <= 0)
				continue; //no more event
		}
	}
};

// This function use to resolve requests from client and call other functions such as: loginServer(), receiveFromClient(), logOutServer() and give them datas from received message 
// The parameters of this function is received buff, connected Socket and reference accountUser, clientInfor in arrays contain information
void resolveRequest(char* rcvBuff, SOCKET &connectedSocket, string &accountUser, string &clientInfor) {
	string receive = (string)rcvBuff;
	string choose = receive.substr(0, 1);
	string data = receive.substr(1);
	switch (stoi(choose)) {
	case 1: 
		loginServer(data, connectedSocket, accountUser, clientInfor);
		break;
	case 2: 
		receiveFromClient(data, connectedSocket, accountUser, clientInfor);
		break;
	case 3:
		logOutServer(data, connectedSocket, accountUser, clientInfor);
		break;
	default:
		break;
	}
};

// This function use to resolve "login" request from client. Authenticate account informations and send resultCode to client.
// The parameters of this function is data from received message, connected Socket and reference accountUser, clientInfor in arrays contain information
void loginServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor) {
	bool rightAcc = false;
	char returnMsg[BUFF_SIZE];
	memset(returnMsg, 0, BUFF_SIZE);
	for (size_t i = 0; i < accNum; i++) {
		if (data == arrAcc[i].accName) {
			rightAcc = true;
			accountUser = data;
			if (arrAcc[i].allowLogin == 0) {
				resultCode = "10";
				strcat_s(returnMsg, "10");
				writeToLog("USER ", accountUser, resultCode, clientInfor);
				cout << "USER " << accountUser << " [" << clientInfor << "]" << endl;	
			}
			else {
				resultCode = "11";
				strcat_s(returnMsg, "11");
				writeToLog("USER ", accountUser, resultCode, clientInfor);
			}
		}
	}
	if (rightAcc == false) {
		resultCode = "12";
		strcat_s(returnMsg, "12");
		writeToLog("USER", "", resultCode, clientInfor);
	}
	send(connectedSocket, returnMsg, strlen(returnMsg), 0);
};

// This function use to resolve "send message" request from client. Receive data and send resultCode to client.
// The parameters of this function is data from received message, connected Socket and reference accountUser, clientInfor in arrays contain information
void receiveFromClient(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor) {
	char returnMsg[BUFF_SIZE];
	memset(returnMsg, 0, BUFF_SIZE);
	cout << "POST " << accountUser << " ["<< clientInfor <<  "]" << endl;
	cout << data << endl;
	resultCode = "20";
	strcat_s(returnMsg, "20");
	send(connectedSocket, returnMsg, strlen(returnMsg), 0);
	writeToLog("POST ", data, resultCode, clientInfor);
};

// This function use to resolve "log out" request from client.send resultCode to client when success or fail.
// The parameters of this function is data from received message, connected Socket and reference accountUser, clientInfor in arrays contain information
void logOutServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor) {
	char returnMsg[BUFF_SIZE];
	memset(returnMsg, 0, BUFF_SIZE);
	resultCode = "30";
	writeToLog("QUIT ", accountUser, resultCode, clientInfor);
	cout << "QUIT " << accountUser << " [" << clientInfor << "]" << endl;
	accountUser = "";
	strcat_s(returnMsg, "30");
	send(connectedSocket, returnMsg, strlen(returnMsg), 0);
};

// This function use to get current time to write in log file
// This function return current time has the form [dd/mm/yy hh:mm:ss]
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

// This function use to write user actions to file log
// The parameters of this function is name of methods (USER, POST, QUIT), message of methods, resultCode and clientInfor
void writeToLog(string method, string message, string resultCode, string clientInfor) {
	string log = clientInfor + " " + getCurrentTime() + " $ ";
	ofs.open("log_20183720.txt", ios::out | ios::app);
	log += method + message + " $ " + resultCode;
	ofs << log << endl;
	ofs.close();
}
// This function use to read file account.txt to get informations of accounts and save into array arrAcc
void readAccountFile() {
	arrAcc = new account[accNum];
	ifs.open("account.txt", ios::in);
	for (size_t i = 0; i < accNum; i++) {
		ifs >> arrAcc[i].accName;
		ifs >> arrAcc[i].allowLogin;
	}
	ifs.close();
};
