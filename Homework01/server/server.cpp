// server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "string"
#include "string.h"
#include "iostream"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#pragma comment (lib, "Ws2_32.lib")

using namespace std;

void resolveIPv4(char* buff, string &message) {
	struct in_addr addr;
	addr.s_addr = inet_addr(buff);
	struct hostent *addressResult = gethostbyaddr((char*)&addr, 4, AF_INET);
	if (addressResult == NULL) {
		printf("Error %d: Cannot resolve IPv4 address.\n", WSAGetLastError());
		int i = WSAGetLastError();
		message.append("-");
		message.append("@");//Client will difference messages
		message.append(WSAGetLastError() + "");
	}
	else {
		char ** pAlias;
		message.append("+");
		message.append(addressResult->h_name);
		message.append("@");
		for (pAlias = addressResult->h_aliases; *pAlias != 0; pAlias++) {
			message.append(*pAlias);
			message.append("@");
		}
	}
};

void resolveHostName(char* buff, string &message) {
	char* hostName = buff;
	char** pAlias;
	struct in_addr addr;
	struct hostent *result;

	result = gethostbyname(hostName);
	if (result == NULL) {
		printf("Error %d: Cannot resolve host name.\n", WSAGetLastError());
		message.append("-");
		message.append("#"); //Client will difference messages
		message.append(WSAGetLastError() + "");
	}
	else {
		if (result->h_addrtype == AF_INET)
		{
			int i = 0;
			message.append("+");
			while (result->h_addr_list[i] != 0) {
				addr.s_addr = *(u_long *)result->h_addr_list[i++];
				message.append(inet_ntoa(addr));
				message.append("#");
			}
		}
	}

}

int main(int argc, char* argv[])
{
	//Use command arguments
	int portInt;

	if (argc < 2) {
		printf("Retype with a port !!\n");
		return -1;
	}
	else if (argc > 2) {
		printf("Too many arguments\n");
		return -1;
	}
	else {
		//Check port
		try {
			portInt = stoi(argv[1]);
		}
		catch (exception &err) {
			printf("Invalid port !!");
			return -1;
		}
	}

	//Step 1: Inittiate winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portInt);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot bind this address.", WSAGetLastError());
		return 0;
	}
	printf("Server started!\n");

	//Step 4: Communicate with client
	sockaddr_in clientAddr;
	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;

	while (1) {
		//Receive Message
		ret = recvfrom(server, buff, BUFF_SIZE, 0, (sockaddr *)&clientAddr, &clientAddrLen);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot receive data.", WSAGetLastError());
		else if (strlen(buff) > 0) {
			buff[ret] = 0;
			in_addr addressTyped;
			string msg = "";
			if (inet_pton(AF_INET, buff, &addressTyped)) {
				resolveIPv4(buff, msg);
			}
			else {
				resolveHostName(buff, msg);
			}

			//convert string to char array and send to client
			char* resultMessage = const_cast<char*>(msg.c_str());

			//Send result to client; 
			ret = sendto(server, resultMessage, strlen(resultMessage), 0, (sockaddr *)&clientAddr, clientAddrLen);
			if (ret == SOCKET_ERROR)
				printf("Error %d: Cannot send data", WSAGetLastError());
		}
	} //end while

	  //Step 5: Close socket
	closesocket(server);

	//Step 6: Terminate Winsock
	WSACleanup();

	return 0;
}

