// WSAAsyncSelectServer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "server.h"
#include "winsock2.h"
#include "windows.h"
#include "winuser.h"
#include "stdio.h"
#include <string>
#include "conio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include <ctime>
#include <fstream>
#include <shellapi.h>

#define SERVER_ADDR "127.0.0.1"
#define WM_SOCKET WM_USER + 1
#define MAX_CLIENT 1024
#define BUFF_SIZE 2048
#pragma comment (lib, "Ws2_32.lib")
using namespace std;


fstream ifs, ofs;
const size_t accNum = 4;
bool isConnect = false;
string resultCode, clientIn4[FD_SETSIZE], accountName[FD_SETSIZE];
int serverPort;

// Declare data type account to contain informations of accounts
struct account {
	string accName;
	int allowLogin;
};

account* arrAcc;

void resolveRequest(char* rcvBuff, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void loginServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void receiveFromClient(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
void logOutServer(string data, SOCKET &connectedSocket, string &accountUser, string &clientInfor);
string getCurrentTime();
void writeToLog(string method, string message, string resultcode, string clientInfor);
void readAccountFile();

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	windowProc(HWND, UINT, WPARAM, LPARAM);

SOCKET client[MAX_CLIENT];
SOCKET listenSock;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPWSTR *szArgList;
	int argCount;	
	MSG msg;
	HWND serverWindow;

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;
	//Use command arguments
	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (argCount > 2) {
		MessageBox(serverWindow, L"Too many arguments. Please retype !!", L"Error", MB_OK);
		return 0;
	}
	else if (argCount < 2) {
		MessageBox(serverWindow, L"Please enter a port !!!", L"Error", MB_OK);
		return 0;
	}
	else {
		//check server port
		try {
			serverPort = stoi(szArgList[1]);
		}
		catch (exception &err) {
			MessageBox(serverWindow, L"Invalid port !!!", L"Error", MB_OK);
			return 0;
		}
	}

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(serverWindow, L"Winsock 2.2 is not supported.", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//requests Windows message-based notification of network events for listenSock
	WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		MessageBox(serverWindow, L"Cannot associate a local address with server socket.", L"Error!", MB_OK);
	}

	//Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		MessageBox(serverWindow, L"Cannot place server socket in state LISTEN.", L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	for (i = 0; i <MAX_CLIENT; i++)
		client[i] = 0;
	hWnd = CreateWindow(L"WindowClass", L"WSAAsyncSelect TCP Server", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKET connSock;
	sockaddr_in clientAddr;
	int ret, clientAddrLen = sizeof(clientAddr), i;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];
	readAccountFile();

	switch (message) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam) {
					closesocket(client[i]);
					client[i] = 0;
					continue;
				}
		}

		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			connSock = accept((SOCKET)wParam, (sockaddr *)&clientAddr, &clientAddrLen);
			if (connSock == INVALID_SOCKET) {
				break;
			}
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == 0) {
					client[i] = connSock;
					//requests Windows message-based notification of network events for listenSock
					WSAAsyncSelect(client[i], hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
					clientIn4[i] = string(inet_ntoa(clientAddr.sin_addr)) + ':' + to_string(ntohs(clientAddr.sin_port));
					break;
				}
			if (i == MAX_CLIENT)
				MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
		}
		break;

		case FD_READ:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam)
					break;

			ret = recv(client[i], rcvBuff, BUFF_SIZE, 0);
			if (ret <= 0) {
				if (isConnect == true) {
					if (accountName[i].length() != 0) {
						writeToLog("QUIT ", accountName[i], resultCode, clientIn4[i]);
						accountName[i] = "";
					}
				}
			}
			if (ret > 0) {
				//echo to client
				rcvBuff[ret] = 0;
				isConnect = true;
				printf("\nReceive from client: %s", rcvBuff);
				send(client[i], rcvBuff, strlen(rcvBuff), 0);
				//resolveRequest(rcvBuff, client[i], accountName[i], clientIn4[i]);
			}
		}
		break;

		case FD_CLOSE:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam) {
					closesocket(client[i]);
					client[i] = 0;
					break;
				}
		}
		break;
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
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
				accountUser = "";
			}
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
