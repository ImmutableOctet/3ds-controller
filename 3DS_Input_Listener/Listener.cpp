/*
	Currently Windows-only.
*/

// Preprocessor related;
#undef UNICODE

#define BIT(n) (1U<<(n))

#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ERROR_CODE 1 // -1

#define PORT_STR "4865"

// Includes:
#include <windows.h>
//#include <winuser.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <climits>
#include <cstring>
//#include <cstddef>
//#include <cstdio>

#include <stdexcept>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <QuickLib\QuickINI\QuickINI.h>

// Internal:
#include "tables.h"

// Libraries:
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")

// Namespace(s):
using namespace std;
using namespace quickLib;

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

// Typedefs:

// This may be used to map a 3DS button-position to a system-native key-code.
typedef map<uint32_t, UINT> keyCodeMap;

// Enumerator(s):
enum keyAction
{
	ACTION_DOWN,
	ACTION_UP,
	//ACTION_HELD,
};

// Constant variable(s):
static const string INI_3DS_SECTION = "3ds";

// Functions:
void mapDefaultKeys(INI::INIVariables<>& keyMap)
{
	auto& section = keyMap[INI_3DS_SECTION];

	// A, B, Select, Start:
	section[debug_keyNames[0]] = "VK_Z";
	section[debug_keyNames[1]] = "VK_X";
	section[debug_keyNames[2]] = "VK_D";
	section[debug_keyNames[3]] = "VK_F";

	// D-pad:
	section[debug_keyNames[4]] = "VK_RIGHT";
	section[debug_keyNames[5]] = "VK_LEFT";
	section[debug_keyNames[6]] = "VK_UP";
	section[debug_keyNames[7]] = "VK_DOWN";

	// R, L, X, Y
	section[debug_keyNames[8]] = "VK_E";
	section[debug_keyNames[9]] = "VK_Q";
	section[debug_keyNames[10]] = "VK_A";
	section[debug_keyNames[11]] = "VK_S";

	// ZL, ZR:
	section[debug_keyNames[14]] = "VK_C";
	section[debug_keyNames[15]] = "VK_V";

	// Touch screen:
	section[debug_keyNames[20]] = "VK_B";

	// C-stick:
	section[debug_keyNames[24]] = "VK_L";
	section[debug_keyNames[25]] = "VK_J";
	section[debug_keyNames[26]] = "VK_I";
	section[debug_keyNames[27]] = "VK_K";

	// Mapped CirclePad:
	section[debug_keyNames[28]] = "VK_H";
	section[debug_keyNames[29]] = "VK_F";
	section[debug_keyNames[30]] = "VK_T";
	section[debug_keyNames[31]] = "VK_G";

	return;
}

UINT toSystemKey(const char* _3DSKey, const INI::INISection<>& keyMap)
{
	return virtualKeyMap.at(keyMap.at(_3DSKey)); // MapVirtualKey(..., MAPVK_VK_TO_VSC);
}

void remapKeys(const INI::INISection<>& keyMap, keyCodeMap& systemKeyMap)
{
	systemKeyMap[0] = toSystemKey("KEY_A", keyMap);
	systemKeyMap[1] = toSystemKey("KEY_B", keyMap);
	systemKeyMap[2] = toSystemKey("KEY_SELECT", keyMap);
	systemKeyMap[3] = toSystemKey("KEY_START", keyMap);
	
	systemKeyMap[4] = toSystemKey("KEY_DRIGHT", keyMap);
	systemKeyMap[5] = toSystemKey("KEY_DLEFT", keyMap);
	systemKeyMap[6] = toSystemKey("KEY_DUP", keyMap);
	systemKeyMap[7] = toSystemKey("KEY_DDOWN", keyMap);
	
	systemKeyMap[8] = toSystemKey("KEY_R", keyMap);
	systemKeyMap[9] = toSystemKey("KEY_L", keyMap);
	systemKeyMap[10] = toSystemKey("KEY_X", keyMap);
	systemKeyMap[11] = toSystemKey("KEY_Y", keyMap);

	systemKeyMap[14] = toSystemKey("KEY_ZL", keyMap);
	systemKeyMap[15] = toSystemKey("KEY_ZR", keyMap);

	systemKeyMap[20] = toSystemKey("KEY_TOUCH", keyMap);

	systemKeyMap[24] = toSystemKey("KEY_CSTICK_RIGHT", keyMap);
	systemKeyMap[25] = toSystemKey("KEY_CSTICK_LEFT", keyMap);
	systemKeyMap[26] = toSystemKey("KEY_CSTICK_UP", keyMap);
	systemKeyMap[27] = toSystemKey("KEY_CSTICK_DOWN", keyMap);

	systemKeyMap[28] = toSystemKey("KEY_CPAD_RIGHT", keyMap);
	systemKeyMap[29] = toSystemKey("KEY_CPAD_LEFT", keyMap);
	systemKeyMap[30] = toSystemKey("KEY_CPAD_UP", keyMap);
	systemKeyMap[31] = toSystemKey("KEY_CPAD_DOWN", keyMap);

	return;
}

size_t simulateKey(UINT nativeKey, keyAction mode)
{
	// Local variable(s):
	INPUT deviceAction;

	// Set up the input-device descriptor:
	deviceAction.type = INPUT_KEYBOARD;
	deviceAction.ki.time = 0;
	deviceAction.ki.wVk = 0; // (DWORD)nativeKey;
	deviceAction.ki.dwExtraInfo = 0;
	deviceAction.ki.wScan = MapVirtualKey(nativeKey, MAPVK_VK_TO_VSC); // MAPVK_VK_TO_VSC_EX// nativeKey

	// Set the initial flags of this operation.
	deviceAction.ki.dwFlags = KEYEVENTF_SCANCODE;

	// Check if this is an "extended key":
	if ((nativeKey > 32 && nativeKey < 47) || (nativeKey > 90 && nativeKey < 94))
		deviceAction.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

	// Check for mode-specific functionality:
	switch (mode)
	{
		case ACTION_UP:
			deviceAction.ki.dwFlags |= KEYEVENTF_KEYUP;

			break;
		case ACTION_DOWN:
			// Nothing so far.

			break;
	}

	// Simulate the input operation.
	return (size_t)SendInput(1, &deviceAction, sizeof(deviceAction));
}

int main()
{
	// Constant variable(s):
	static const string configPath = "keymap.ini";

	// Local variable(s):
	bool running = true;

	INI::INIVariables<> keyMap;

	try
	{
		INI::load(configPath, keyMap);
	}
	catch (const INI::fileNotFound<>&)
	{
		mapDefaultKeys(keyMap);

		// Save our newly generate key-map.
		INI::save(configPath, keyMap);
	}

	keyCodeMap systemKeyMap;

	remapKeys(keyMap[INI_3DS_SECTION], systemKeyMap);

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

		iResult = ::bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);

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

	sockaddr address;
	int addrlen = sizeof(address);

	cout << "Waiting for a 3DS..." << endl;

	for (int i = 1; (i <= 4 && (clientSocket == INVALID_SOCKET)); i++)
	{
		clientSocket = accept(listenSocket, &address, &addrlen);
	}

	if (clientSocket != INVALID_SOCKET)
	{
		cout << "3DS found." << endl;
	}

	// Since we only need one connection, close the listening-socket:
	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
	}

	while (running)
	{
		//cout << "Waiting for an input packet..." << endl;

		inputData states[FRAMES_PER_SEND];

		iResult = recv(clientSocket, (char*)&states, sizeof(states), 0);

		if (iResult > 0)
		{
			auto statesAvailable = (iResult / sizeof(inputData));

			cout << statesAvailable << " input-states found. (" << iResult << "bytes)" << endl;
			//cout << "Analog: " << state.analog.x << ", " << state.analog.y << endl;

			for (unsigned sID = 0; sID < statesAvailable; sID++)
			{
				const auto& state = states[sID];

				for (int i = 0; i < 32; i++)
				{
					const auto mask = BIT(i);

					if ((state.kDown & mask)) // || (state.kHeld & mask)
					{
						simulateKey(systemKeyMap[i], ACTION_DOWN);

						cout << debug_keyNames[i] << " down." << endl;
					}
					else if (state.kUp & mask)
					{
						simulateKey(systemKeyMap[i], ACTION_UP);

						cout << debug_keyNames[i] << " up." << endl;
					}
				}

				if ((sID+1) < statesAvailable)
				{
					this_thread::sleep_for((chrono::milliseconds)16);
				}
			}
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