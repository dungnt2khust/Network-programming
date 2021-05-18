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
char DELIMITER[] = "\r\n";

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
	messageLen = strlen(buff);
	ret = send(client, buff, messageLen, 0);
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT)
			printf("Time out!");
		else
			printf("Error %d: Cannot send data.\n", WSAGetLastError());
	}	
};

// Receive message from server
void receiveMessage() {
	memset(buff, 0, BUFF_SIZE);
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

	while (1) {
		memset(buff, 0, BUFF_SIZE);
		printf("\nSend to server: ");
		gets_s(buff, BUFF_SIZE);
		sendMessage();
		receiveMessage();
		printf("\nReceive from server: %s", buff);
	}

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();
	return 0;
}
