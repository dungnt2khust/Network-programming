// server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "string"
#include "iostream"
#include "winsock2.h"
#include "ws2tcpip.h"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#pragma comment (lib, "Ws2_32.lib")

using namespace std;


string connectClient(SOCKET &connSock, SOCKET &listenSock, sockaddr_in &clientAddr, int &clientPort) {
	//This function generate a connection from a connection in queue
	//get into parameters pass by reference (Sockets use to generate connection between client and server,
	//client address and client port)
	//This function return ip address of client

	int clientAddrLen = sizeof(clientAddr);
	char client_ip[INET_ADDRSTRLEN];
	//accept request
	connSock = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
	if (connSock == SOCKET_ERROR) {
		printf("Error %d: Cannot permit incoming connection.", WSAGetLastError());
	}
	else {
		inet_ntop(AF_INET, &clientAddr.sin_addr, client_ip, sizeof(client_ip));
		clientPort = ntohs(clientAddr.sin_port);
		printf("Accept incoming connection from %s:%d\n", client_ip, clientPort);
	}
	string s(client_ip);
	return s;
}

void resolveMessage(char* buff, string &message) {
	//This function resolves message 
	//(add prefix, split a valid string to two strings: digit and alphabet,
	//link results into a message to send to client)
	//Input are data of buff (received message from client) and a result string pass by reference 
	string msgString = buff;
	string alphabet, digit;
	for (int i = 0; i < msgString.length(); i++) {
		char temp = msgString[i];
		if (temp >= 48 && temp <= 57) {
			digit += temp;
		}
		else if ((temp >= 'a' && temp <= 'z') || (temp >= 'A' && temp <= 'Z')) {
			alphabet += temp;
		}
		else {
			message.append("-Error: Invalid message ! ");
			return;
		}
	}
	message.append("+");
	message.append(alphabet);
	message.append("@");
	message.append(digit);
}

int main(int argc, char* argv[])
{
	//Use command arguments
	int serverPort;
	if (argc < 2) {
		printf("Retype with a port !!!\n");
		return -1;
	}
	else if (argc > 2) {
		printf("Too many arguments !!!\n");
		return -1;
	}
	else {
		//Check port
		try {
			serverPort = stoi(argv[1]);
		}
		catch (exception &err) {
			printf("Invalid port !!!");
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
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return -1;
	}


	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return -1;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return -1;
	}
	printf("Server started!\n");

	//Step 5: Communicate with client 
	sockaddr_in clientAddr;
	SOCKET connSock;
	char buff[BUFF_SIZE];
	memset(buff, 0, BUFF_SIZE);
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;

	string clientIP = connectClient(connSock, listenSock, clientAddr, clientPort);
	while (1) {
		//receive message from client
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.", WSAGetLastError());
			if (listenSock != INVALID_SOCKET) {
				memset(buff, 0, BUFF_SIZE);
				clientIP = connectClient(connSock, listenSock, clientAddr, clientPort);
			}
		}
		else if (ret == 0) {
			printf("\nClient disconnects.\n\n");
			memset(buff, 0, BUFF_SIZE);
			clientIP = connectClient(connSock, listenSock, clientAddr, clientPort);
		}
		else {

			buff[ret] = 0;
			cout << "Receive from client[" + clientIP + ":" + to_string(clientPort) + "]" + " " + buff;
			string msg;
			resolveMessage(buff, msg);
			//Convert string to char*
			char* resultMessage = const_cast<char*>(msg.c_str());
			//Echo to client
			ret = send(connSock, resultMessage, strlen(resultMessage), 0);
			if (ret == SOCKET_ERROR) {
				printf("Error %d: Cannot send data.", WSAGetLastError());
				memset(buff, 0, BUFF_SIZE);
				clientIP = connectClient(connSock, listenSock, clientAddr, clientPort);
			}
		}
	}

	//end communicating

	//Step 6: Close socket
	closesocket(connSock);
	closesocket(listenSock);

	//Step 7: Terminate Winsock
	WSACleanup();
	return 0;
}

