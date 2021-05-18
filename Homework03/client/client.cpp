// client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "string"
#include "iostream"
#include "vector"
#include "stdlib.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define BUFF_SIZE 2048
#pragma comment (lib, "Ws2_32.lib")

using namespace std;

SOCKET client;
sockaddr_in serverAddr;
char buff[BUFF_SIZE], buff2[BUFF_SIZE];
int clientPort, ret, messageLen, serverAddrLen = sizeof(serverAddr);
bool isLogin = false;
string resultCode;


void initWinsock() {
	//Step 1: Inittiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		exit(0);
	}
};

void constructSocket() {
	//Step 2: Construct socket
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		exit(0);
	}
};

void specifyServerAddress(char* clientIP) {
	//Step 3: Specify server address 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(clientPort);
	inet_pton(AF_INET, clientIP, &serverAddr.sin_addr);
};

void connectServer() {
	//Step 4: Request to connect server 
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		exit(0);
	}
	printf("Connect server!\n");
};

// Send message to server
void sendMessage() {
	ret = send(client, buff, strlen(buff), 0);
	if (ret == SOCKET_ERROR) {
		printf("Error %d: Cannot send data.\n", WSAGetLastError());
	}
};

// Receive message from server
void receiveMessage() {
	ret = recv(client, buff, BUFF_SIZE, 0);
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT)
			printf("Time out!\n");
		else {
			printf("Error %d: Cannot receive message. \n", WSAGetLastError());
		}
	}
	else if (strlen(buff) > 0) {
		buff[ret] = 0;
	}
};

// Resolve request to login from client
void loginClient(char* buff2) {
	string accountName;
	if (isLogin == false) {
		memset(buff, 0, BUFF_SIZE);
		memset(buff2, 0, BUFF_SIZE);
		printf("\nPress ENTER to escape");
		printf("\nPlease enter account name:\n");
		gets_s(buff2, BUFF_SIZE);
		accountName = (string)buff2;
		if (accountName.length() == 0) {
			printf("\nEscape LOG IN");
			return;
		}
		accountName = "1" + accountName;
		buff2 = const_cast<char*>(accountName.c_str());
		strcat_s(buff, buff2);

		sendMessage();
		receiveMessage();

		resultCode = (string)buff;
		if (resultCode == "10") {
			printf("\nLogin successful");
			isLogin = true;
		}
		else if (resultCode == "11") {
			printf("\nYour account can not access to server");	
		}
		else if (resultCode == "12") {
			printf("\nWrong account");
		}
		else if (resultCode == "13") {
			printf("\nThis account is logged in another account.");
		}
	}	
};

void sendToServer(char* buff2) {
	string message;
	if (isLogin) {
		memset(buff, 0, BUFF_SIZE);
		memset(buff2, 0, BUFF_SIZE);
		printf("\nPress ENTER to escape");
		printf("\nSend to server:\n");
		gets_s(buff2, BUFF_SIZE);
		message = (string)buff2;
		if (message.length() == 0) {
			printf("\nEscape SEND MESSAGE");
			return;
		}
		message = "2" + message;
		buff2 = const_cast<char*>(message.c_str());
		strcat_s(buff, buff2);
		sendMessage();
		receiveMessage();
		resultCode = (string)buff;
		if (resultCode == "20") {
			printf("\nPost succesfully");
		}
	}	
};

void logOutClient(char* buff2) {
	memset(buff2, 0, BUFF_SIZE);
	string message = "3";
	buff2 = const_cast<char*>(message.c_str());
	strcat_s(buff, buff2);
	sendMessage();
	receiveMessage();
	string resultCode = (string)buff;
	if (resultCode == "30") {
		printf("\nYou are logged out");
		isLogin = false;
	}	
};

// Display menu, send and receive request with server
void menuAndGetRequest() {
	memset(buff, 0, BUFF_SIZE);
	if (isLogin == false) {
		printf("\n----------------------------------------------");
		printf("\n\t1. LOG IN");
		printf("\n\t2. EXIT");
		printf("\nYOUR CHOOSE IS: ");
		string chooseString;
		int choose;
		gets_s(buff, BUFF_SIZE);
		chooseString = (string)buff;
		if (chooseString.length() == 0) {
			exit(0);
		}
		try {
			choose = stoi(chooseString);
		}
		catch (exception &err) {
			printf("\nPlease enter a number");
			return;
		}
		switch (choose) {
		case 1:
			loginClient(buff2);
			break;
		case 2:
			exit(0);
			break;
		default: 
			printf("\nInvalid choose !");
			break;
		}
	}
	else {
		printf("\n----------------------------------------------");
		//printf("\n\t1. LOG IN");
		printf("\n\t2. SEND MESSAGE");
		printf("\n\t3. LOG OUT");
		printf("\n\t4. EXIT");
		printf("\nYOUR CHOOSE IS: ");
		string chooseString;
		int choose;
		gets_s(buff, BUFF_SIZE);
		chooseString = (string)buff;
		if (chooseString.length() == 0) {
			exit(0);
		}
		try {
			choose = stoi(chooseString);
		}
		catch (exception &err) {
			printf("\nPlease enter a number");
			return;
		}
		switch (choose) {
		case 2:
			sendToServer(buff2);
			break;
		case 3:
			logOutClient(buff2);
			break;
		case 4:
			exit(0);
			break;
		default:
			printf("\nInvalid choose");
		}
	}
};
int main(int argc, char* argv[])
{
	//Use command arguments	
	in_addr SERVER_ADDR;
	if (argc < 3) {
		printf("Not enough arguments !!!");
		return -1;
	}
	else if (argc > 3) {
		printf("Too many arguments !!!");
		return -1;
	}
	else {
		//Check port
		try {
			clientPort = stoi(argv[2]);
		}
		catch (exception &err) {
			printf("Invalid Port !!!");
			return -1;
		}
		//Check address
		if (inet_pton(AF_INET, argv[1], &SERVER_ADDR) != 1) {
			printf("Invalid Address !!!");
			return -1;
		}
	}
	//Step 1: Inittiate Winsock
	initWinsock();

	//Step 2: Construct socket
	constructSocket();

	//Step 3: Specify server address 
	specifyServerAddress(argv[1]);

	//Step 4: Request to connect server 
	connectServer();

	//Step 5: Communicate with server

	printf("\nTHIS PROGRAM CAN PROVIDE YOU SOME OPTIONS AS FOLLOW:");

	while (1) {
		menuAndGetRequest();
	}

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();
	return 0;
}
