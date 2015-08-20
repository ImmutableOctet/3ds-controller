/*
	Currently Windows-only.
*/

// Preprocessor related;
#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ERROR_CODE 1 // -1

#define PORT_STR "4865"

// Includes:
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
//#include <cstdio>

#include <stdexcept>
#include <string>
#include <iostream>

#include <chrono>

// Libraries:
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")

// Structures:
struct circlePosition
{
	int16_t x;
	int16_t y;
};

struct inputData
{
	circlePosition analog;

	uint32_t kDown;
	uint32_t kHeld;
	uint32_t kUp;
};

// Functions:
int main()
{
	// Namespaces:
	using namespace std;

	// Constant variable(s):
	static const chrono::seconds waitTime = (chrono::seconds)1;

	// Local variable(s):
	bool running = true;

	WSADATA WSADATA;
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	addrinfo* result = nullptr;
	addrinfo hints = {};

	iResult = WSAStartup(MAKEWORD(2, 2), &WSADATA);

	if (iResult != 0)
	{
		cout << "Unable to initialize WSA: " << iResult << endl;

		return ERROR_CODE;
	}

	try
	{
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iResult = getaddrinfo(NULL, PORT_STR, &hints, &result); // nullptr

		if (iResult != 0)
		{
			throw runtime_error("Enable to resolve internal address: " + to_string(iResult));
		}

		listenSocket = socket(result->ai_family, result->ai_socktype, (int)result->ai_protocol);

		if (listenSocket == INVALID_SOCKET)
		{
			throw runtime_error("Unable to create socket: " + to_string(WSAGetLastError()));
		}

		iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);

		if (iResult == SOCKET_ERROR)
		{
			throw(runtime_error("Unable to bind socket: " + to_string(WSAGetLastError())));
		}

		iResult = listen(listenSocket, SOMAXCONN);

		if (iResult == SOCKET_ERROR)
		{
			throw(runtime_error("Listening failed: " + to_string(WSAGetLastError())));
		}
	}
	catch (const runtime_error& e)
	{
		cout << e.what() << endl;

		running = false;
	}

	if (result != nullptr)
	{
		freeaddrinfo(result); // result = nullptr;
	}

	const auto beginTime = chrono::high_resolution_clock::now();

	sockaddr address;
	int addrlen = sizeof(address);

	while (clientSocket == INVALID_SOCKET)
	{
		if (chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - beginTime) > waitTime)
		{
			running = false;

			break;
		}

		clientSocket = accept(listenSocket, &address, &addrlen);
	}

	if (clientSocket != INVALID_SOCKET)
	{
		cout << "Client found." << endl;
		cout << "Address: " << inet_ntoa(*((in_addr*)&address)) << endl;
	}

	// Since we only need one connection, close the listening-socket:
	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
	}

	while (running)
	{
		cout << "Waiting for an input packet..." << endl;

		inputData state;

		iResult = recv(clientSocket, (char*)&state, sizeof(state), 0);

		if (iResult > 0)
		{
			cout << "Input state found (" << iResult << "bytes)" << endl;
		}
		else if (iResult == 0)
		{
			cout << "Closing connection..." << endl;

			break;
		}
		else
		{
			running = false;

			break;
		}
	}

	cout << "Cleaning up..." << endl;

	// Deinitialize networking functionality:
	if (clientSocket != INVALID_SOCKET)
	{
		if (running)
		{
			shutdown(clientSocket, SD_SEND);
		}

		closesocket(clientSocket);
	}

	// Free up any resources allocated by WSA.
	WSACleanup();

	// Return the calculated response.
	return ((running) ? 0 : ERROR_CODE);
}