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

vector<string> resolveReceivedMessage(char* buff) {
	//This function resolves received message from server
	//split linked results, remove separator characters and push results in a vector and return that vector
	string digit, alphabet, message = buff;
	vector<string> result;
	size_t pos = message.find('@');
	digit = message.substr(1, pos - 1);
	alphabet = message.substr(pos + 1, message.length());
	result.push_back(digit);
	result.push_back(alphabet);
	return result;
}

int main(int argc, char* argv[])
{
	//Use command arguments
	int clientPort;
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
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return -1;
	}

	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return -1;
	}

	//Step 3: Specify server address 
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(clientPort);
	inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

	//Step 4: Request to connect server 
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return -1;
	}
	printf("Connect server!\n");

	//Step 5: Communicate with server
	char buff[BUFF_SIZE];
	int ret, messageLen;
	while (1) {
		//Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		messageLen = strlen(buff);
		if (messageLen == 0) break;

		ret = send(client, buff, messageLen, 0);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send data.", WSAGetLastError());
		
		//Receive echo message
		ret = recv(client, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("\nTime-out!\n");
			else printf("Error %d: Cannot receive data.", WSAGetLastError());
		}
		else if (strlen(buff) > 0) {
			buff[ret] = 0;
			//Resolve received message by prefix
			if (buff[0] == '-') {
				string result = ((string)buff).substr(1);
				cout << result << endl;
			}
			else if (buff[0] == '+') {
				vector<string> result = resolveReceivedMessage(buff);
				cout << result[1] << endl;
				cout << result[0] << endl;
			}
			else {
				printf("Cannot resolve message !!!\n");
			}
		}
	}

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();
	return 0;
}

