// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#define NUM_THREAD 4
CRITICAL_SECTION critical;
int sharedValue;

unsigned int __stdcall mythread(void*)
{
	EnterCriticalSection(&critical);
	int number = sharedValue++;
	LeaveCriticalSection(&critical);
	printf("Value sharedValue = %d \n",number);
	return 0;
}

int main(int argc, char* argv[])
{
	int i;
	HANDLE myhandle[NUM_THREAD];
	InitializeCriticalSection(&critical);
	for (i = 0; i < NUM_THREAD; i++)
		myhandle[i] = (HANDLE)_beginthreadex(0, 0, &mythread,
		(void *)i, 0, 0);
	WaitForMultipleObjects(NUM_THREAD, myhandle, TRUE, INFINITE);
	DeleteCriticalSection(&critical);
	return 0;
}