/*
	Currently Windows-only.
*/

// Preprocessor related;
#define BIT(n) (1U<<(n))

// Windows-specific:
#define NOMINMAX
#undef UNICODE

#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define _USE_MATH_DEFINES

#define PORT_STR "4865"
#define ERROR_CODE 1 // -1

#define _3DS_CPAD_MAX 160 // 158 // 155 // 128
#define _3DS_PAD_MASK (KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)
#define _3DS_TOUCH_WIDTH 320
#define _3DS_TOUCH_HEIGHT 240

#define _3DS_NETINPUT_SHARED_SUPPORT_LIB

// Includes:
#include <windows.h>
//#include <winuser.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <climits>
#include <cstring>
#include <cmath>
//#include <cstddef>
//#include <cstdio>

#include <stdexcept>
#include <iostream>
#include <string>
#include <algorithm>
//#include <thread>
//#include <chrono>

#include <QuickLib\QuickINI\QuickINI.h>

// External:
#include "external/iosync/winnt/vjoyDriver.h"

// Internal:
#include "tables.h"

// Libraries:
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")

// Macro backups:
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

// Namespace(s):
using namespace std;
using namespace quickLib;

// Typedefs:

// Windows-specific (vJoy):
typedef UINT vJoyButton; // WORD // DWORD

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

// Enumerator(s):

// Windows-specific:
enum vJoyIDs : UINT
{
	VJOY_ID_NONE = 0,
	MAX_VJOY_DEVICES = 16,
};

// Functions:
double joyHat(bool up, bool down, bool left, bool right)
{
	return fmod(360.0 + (atan2(((right) ? 1.0 : ((left) ? -1.0 : 0.0)), ((up) ? 1.0 : ((down) ? -1.0 : 0.0))) * (180.0 / M_PI)), 360.0);
}

double joyHat(const u32 buttons)
{
	return joyHat(((buttons & KEY_DUP) > 0), ((buttons & KEY_DDOWN) > 0), ((buttons & KEY_DLEFT) > 0), ((buttons & KEY_DRIGHT) > 0));
}

bool stringEnabled(const string& str)
{
	const auto fc = str[0];

	return (fc == 'y' || fc == 'Y' || fc == 't' || fc == 'T');
}

bool initSockets(bool output=true)
{
	// Windows-specific:
	
	// This is used to initialize WinSock.
	WSADATA WSADATA;

	auto iResult = WSAStartup(MAKEWORD(2, 2), &WSADATA);

	if (iResult != 0)
	{
		if (output)
		{
			cout << "Unable to initialize WSA: " << iResult << endl;
		}

		return false;
	}

	// Return the default response.
	return true;
}

void deinitSockets()
{
	// Windows-specific:

	// Free up any resources allocated by WSA.
	WSACleanup();

	return;
}

void mapDefaultKeys(INI::INIVariables<>& config)
{
	auto& section = config[INI_3DS_SECTION];

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

UINT toSystemKey(const char* _3DSKey, const INI::INISection<>& config)
{
	return virtualKeyMap.at(config.at(_3DSKey)); // MapVirtualKey(..., MAPVK_VK_TO_VSC);
}

void remapKeys(const INI::INISection<>& config, keyCodeMap& systemKeyMap)
{
	systemKeyMap[0] = toSystemKey("KEY_A", config);
	systemKeyMap[1] = toSystemKey("KEY_B", config);
	systemKeyMap[2] = toSystemKey("KEY_SELECT", config);
	systemKeyMap[3] = toSystemKey("KEY_START", config);
	
	systemKeyMap[4] = toSystemKey("KEY_DRIGHT", config);
	systemKeyMap[5] = toSystemKey("KEY_DLEFT", config);
	systemKeyMap[6] = toSystemKey("KEY_DUP", config);
	systemKeyMap[7] = toSystemKey("KEY_DDOWN", config);
	
	systemKeyMap[8] = toSystemKey("KEY_R", config);
	systemKeyMap[9] = toSystemKey("KEY_L", config);
	systemKeyMap[10] = toSystemKey("KEY_X", config);
	systemKeyMap[11] = toSystemKey("KEY_Y", config);

	systemKeyMap[14] = toSystemKey("KEY_ZL", config);
	systemKeyMap[15] = toSystemKey("KEY_ZR", config);

	systemKeyMap[20] = toSystemKey("KEY_TOUCH", config);

	systemKeyMap[24] = toSystemKey("KEY_CSTICK_RIGHT", config);
	systemKeyMap[25] = toSystemKey("KEY_CSTICK_LEFT", config);
	systemKeyMap[26] = toSystemKey("KEY_CSTICK_UP", config);
	systemKeyMap[27] = toSystemKey("KEY_CSTICK_DOWN", config);

	systemKeyMap[28] = toSystemKey("KEY_CPAD_RIGHT", config);
	systemKeyMap[29] = toSystemKey("KEY_CPAD_LEFT", config);
	systemKeyMap[30] = toSystemKey("KEY_CPAD_UP", config);
	systemKeyMap[31] = toSystemKey("KEY_CPAD_DOWN", config);

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

void clearConsole()
{
	system("CLS");

	return;
}

void loadConfiguration(const string& configPath, INI::INIVariables<>& config, keyCodeMap& systemKeyMap)
{
	try
	{
		INI::load(configPath, config);
	}
	catch (const INI::fileNotFound<>&)
	{
		mapDefaultKeys(config);

		auto& globalSection = config["global"];

		globalSection["vjoy"] = "true";
		globalSection["keyboard"] = "false";

		// Save our newly generate key-map.
		INI::save(configPath, config);
	}

	remapKeys(config[INI_3DS_SECTION], systemKeyMap);

	return;
}

// Windows-specific (vJoy):

// vJoy must be properly initialized before using this command.
bool vJoyWorking()
{
	// Namespace(s):
	using namespace vJoy;

	// Resolve the current "driver" state:
	if (REAL_VJOY::vJoyEnabled() == TRUE)
	{
		WORD DLLVer, DrvVer;

		return (REAL_VJOY::DriverMatch(&DLLVer, &DrvVer) == TRUE);
	}
	
	return false;
}

LONG vJoyMapAxis(vJoy::vJoyDevice& device, const u32 value, const UINT axis, const UINT maximum_value=_3DS_CPAD_MAX) // SHRT_MAX
{
	// Local variable(s):
	const auto axisInfo = device.getAxis(axis)->second;
	const auto axis_mid = ((axisInfo.max - axisInfo.min) / 2);

	return axis_mid + (value * (axis_mid / maximum_value));
}

LONG vJoyCapAxis(vJoy::vJoyDevice& device, const u32 value, const UINT axis, const UINT maximum_value)
{
	// Local variable(s):
	const auto half_max = (maximum_value/2);

	const auto axisInfo = device.getAxis(axis)->second;
	const LONG axis_mid = ((axisInfo.max - axisInfo.min) / 2);
	const double scalar = std::max(((double)axis_mid / (double)half_max), 1.0);
	const LONG svalue = ((LONG)value - half_max);
	const LONG scaled_value = (LONG)((double)(svalue) * scalar);
	const LONG real_value = (axis_mid + scaled_value);

	return std::max(axisInfo.min, std::min(real_value, axisInfo.max));
}

LONG vJoyScaleAxis(vJoy::vJoyDevice& device, const u32 value, const UINT axis, const UINT maximum_value=_3DS_CPAD_MAX)
{
	// Local variable(s):
	const auto axisInfo = device.getAxis(axis)->second;

	return (value * ((axisInfo.max - axisInfo.min) / maximum_value));
}

// The 'value' argument depends on the axis used.
void vJoyTransferAxis(vJoy::vJoyDevice& device, JOYSTICK_POSITION& vState, const u32 value, const UINT axis, bool alt=false)
{
	if (device.hasAxis(axis))
	{
		switch (axis)
		{
			case HID_USAGE_X:
				vState.wAxisX = vJoyMapAxis(device, value, axis);

				break;
			case HID_USAGE_Y:
				vState.wAxisY = vJoyMapAxis(device, value, axis);

				break;
			case HID_USAGE_Z:
				{
					const LONG m = (device.axisMax(axis) / 2);

					vState.wAxisZ = m;

					if ((value & KEY_L) > 0)
					{
						vState.wAxisZ -= m;
					}

					if ((value & KEY_R) > 0)
					{
						vState.wAxisZ += m;
					}
				}

				break;
			case HID_USAGE_SL0:
				vState.wSlider = vJoyScaleAxis(device, value, axis, 63); // 64
			case HID_USAGE_RX:
				if (alt)
				{
					vState.wAxisXRot = vJoyCapAxis(device, value, axis, _3DS_TOUCH_WIDTH);
				}
				else
				{
					// Nothing so far.
				}

				break;
			case HID_USAGE_RY:
				if (alt)
				{
					vState.wAxisYRot = vJoyCapAxis(device, value, axis, _3DS_TOUCH_HEIGHT);
				}
				else
				{
					// Nothing so far.
				}

				break;
			case HID_USAGE_POV:
				{
					if (device.contPOVNumber > 0)
					{
						if ((value & _3DS_PAD_MASK) > 0)
						{
							vState.bHats = (((DWORD)joyHat(value)) * 100);
						}
						else
						{
							// Disable the HAT.
							vState.bHats = 36001;
						}
					}
					else
					{
						if ((value & KEY_DRIGHT) > 0)
							vState.bHats = 1;
						else if ((value & KEY_DLEFT) > 0)
							vState.bHats = 3;
						else if ((value & KEY_DDOWN) > 0)
							vState.bHats = 2;
						else if ((value & KEY_DUP) > 0)
							vState.bHats = 0;
						else
							vState.bHats = 4;
					}
				}
		}

		return;
	}

	return;
}

int main()
{
	// Constant variable(s):
	static const string configPath = "config.ini";

	// Local variable(s):

	// This will be used later to toggle program execution.
	bool running = true;

	// Configuration related:
	INI::INIVariables<> config;

	// Windows-specific (vJoy):
	bool vJoyEnabled = false;
	bool keyboardEnabled = true;

	// This is used to map system keys (Virtual keys) to 3DS button-positions.
	keyCodeMap systemKeyMap;

	// Load our configuration file.
	loadConfiguration(configPath, config, systemKeyMap);

	// Handle function-specific configurations:
	{
		// Global configuration:
		auto& globalSection = config["global"];

		keyboardEnabled = stringEnabled(globalSection["keyboard"]);

		// Windows-specific (vJoy):
		vJoyEnabled = stringEnabled(globalSection["vjoy"]);
	}

	// Windows-specific:
	
	#ifdef GAMEPAD_VJOY_DYNAMIC_LINK
		// This is will be our handle for vJoy's interface DLL.
		HMODULE vJoyModule = NULL; // nullptr;
	#endif

	vJoy::vJoyDevice vJoy_3DS = { VJOY_ID_NONE };

	if (vJoyEnabled)
	{
		// Namespace(s):
		using namespace vJoy;

		#ifdef GAMEPAD_VJOY_DYNAMIC_LINK
			vJoyModule = REAL_VJOY::linkTo();
			
			// Assign the 'vJoyEnabled' flag, and check against it:
			if (vJoyEnabled = (vJoyModule != NULL))
		#endif
		{
			if (vJoyWorking())
			{
				for (vJoy_3DS.deviceIdentifier = 1; vJoy_3DS.deviceIdentifier < MAX_VJOY_DEVICES; vJoy_3DS.deviceIdentifier++) // <=
				{
					//vJoy_3DS.deviceIdentifier
					if (REAL_VJOY::GetVJDStatus(vJoy_3DS.deviceIdentifier) == VJD_STAT_FREE)
					{
						// Acquire control over the device.
						if (REAL_VJOY::AcquireVJD(vJoy_3DS.deviceIdentifier))
						{
							break;
						}
					}
				}

				// Update the device-object.
				vJoy_3DS.update();
			}
		}
	}

	// Networking related:

	// Initialize networking/socket functionality.
	initSockets();

	// This socket will be used to initially connect to a 3DS.
	SOCKET listenSocket = INVALID_SOCKET;

	// This socket will be used to maintain a connection to a 3DS.
	SOCKET clientSocket = INVALID_SOCKET;

	// This will represent an array of internal addresses.
	addrinfo* result = nullptr;

	// This will act as our protocol/networking hints.
	addrinfo hints = {};

	// Setup our protocol hints (TCP, IPV4):
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	try
	{
		// Used for basic error handling.
		int iResult;

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

	// This address will represent a connected 3DS:
	sockaddr address; int addrlen = sizeof(address);

	cout << "Waiting for a 3DS..." << endl;

	// Try to connect to a 3DS, if the system declines us 4 times, don't bother:
	for (int i = 1; (i <= 4 && (clientSocket == INVALID_SOCKET)); i++)
	{
		clientSocket = accept(listenSocket, &address, &addrlen);
	}

	// Check if we were able to connect to a 3DS:
	if (clientSocket != INVALID_SOCKET)
	{
		cout << "3DS found." << endl;
	}
	else
	{
		running = false;
	}

	// Since we only need one connection, close the listening-socket:
	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
	}

	while (running)
	{
		//cout << "Waiting for an input packet..." << endl;

		fullInputData states[FRAMES_PER_SEND] = {};

		const auto iResult = recv(clientSocket, (char*)&states, sizeof(states), 0); // sizeof(fullInputData)*FRAMES_PER_SEND

		if (iResult > 0)
		{
			//cout << "-----------" << endl;
			clearConsole();

			auto statesAvailable = (iResult / sizeof(fullInputData));
			
			cout << statesAvailable << " input-state(s) found. (" << iResult << "bytes)" << endl;

			const auto finalState = (statesAvailable-1);

			cout << "Button states[finalState]: " << states[finalState].data.kHeld << endl;
			cout << "Analog: " << states[finalState].ext.left_analog.dx << ", " << states[finalState].ext.left_analog.dy << endl;
			//cout << "Gyro: " << states[finalState].ext.gyro.x << ", " << states[finalState].ext.gyro.y << ", " << states[finalState].ext.gyro.z << endl;
			//cout << "Acceleration: " << states[finalState].ext.acceleration.x << ", " << states[finalState].ext.acceleration.y << ", " << states[finalState].ext.acceleration.z << endl;
			cout << "Volume: " << (unsigned)states[finalState].meta.volume << endl;
			cout << "Touch: " << states[finalState].ext.touch.px << ", " << states[finalState].ext.touch.py << endl;

			for (unsigned sID = 0; sID < statesAvailable; sID++)
			{
				const auto& state = states[sID];

				for (int i = 0; i < 32; i++)
				{
					const auto mask = BIT(i);

					if ((state.data.kHeld & mask) > 0) // ((state.kDown & mask) > 0)
					{
						if (keyboardEnabled)
						{
							simulateKey(systemKeyMap[i], ACTION_DOWN);
						}

						cout << debug_keyNames[i] << " down." << endl;
					}
					else if ((state.data.kUp & mask) > 0)
					{
						if (keyboardEnabled)
						{
							simulateKey(systemKeyMap[i], ACTION_UP);
						}

						cout << debug_keyNames[i] << " up." << endl;
					}
				}

				if (vJoyEnabled)
				{

					// Local variable(s):
					const auto& ID = vJoy_3DS.deviceIdentifier;

					JOYSTICK_POSITION vState = {};

					vState.bDevice = ID;
					vState.lButtons = (LONG)((state.data.kDown | state.data.kHeld)); // ^ state.data.kUp;

					// CirclePad:
					if (vJoy_3DS.hasAxis(HID_USAGE_X))
					{
						vState.lButtons ^= (vState.lButtons & (KEY_CPAD_LEFT|KEY_CPAD_RIGHT));
						vJoyTransferAxis(vJoy_3DS, vState, state.ext.left_analog.dx, HID_USAGE_X);
					}

					if (vJoy_3DS.hasAxis(HID_USAGE_Y))
					{
						vState.lButtons ^= (vState.lButtons & (KEY_CPAD_UP|KEY_CPAD_DOWN));
						vJoyTransferAxis(vJoy_3DS, vState, state.ext.left_analog.dy, HID_USAGE_Y);
					}

					// D-pad:
					if (vJoy_3DS.hasAxis(HID_USAGE_POV))
					{
						vState.lButtons ^= (vState.lButtons & _3DS_PAD_MASK);
						vJoyTransferAxis(vJoy_3DS, vState, state.data.kHeld, HID_USAGE_POV);
					}

					vJoyTransferAxis(vJoy_3DS, vState, state.meta.volume, HID_USAGE_SL0);

					// Trigger (L+R) axis:
					/*
					if (vJoy_3DS.hasAxis(HID_USAGE_Z))
					{
						vState.lButtons ^= (vState.lButtons & (KEY_L|KEY_R));
						vJoyTransferAxis(vJoy_3DS, vState, state.data.kHeld, HID_USAGE_Z);
					}
					*/

					// Touch screen / C-stick:
					if (vJoy_3DS.hasAxis(HID_USAGE_RX))
					{
						//vJoyTransferAxis(vJoy_3DS, vState, state.ext.right_analog.dx, HID_USAGE_RX);
						vJoyTransferAxis(vJoy_3DS, vState, state.ext.touch.px, HID_USAGE_RX, true);
					}

					if (vJoy_3DS.hasAxis(HID_USAGE_RY))
					{
						vState.lButtons ^= (vState.lButtons & KEY_TOUCH);

						//vJoyTransferAxis(vJoy_3DS, vState, state.ext.right_analog.dy, HID_USAGE_RY);
						vJoyTransferAxis(vJoy_3DS, vState, state.ext.touch.py, HID_USAGE_RY, true);
					}

					// Update the device with the newly calculated state.
					vJoy::REAL_VJOY::UpdateVJD(ID, &vState);
				}

				/*
				if ((sID+1) < statesAvailable)
				{
					this_thread::sleep_for((chrono::milliseconds)16);
				}
				*/
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

	deinitSockets();

	// Windows-specific (vJoy):

	// Relinquish control over our vJoy device:
	if (vJoy_3DS.deviceIdentifier != VJOY_ID_NONE)
	{
		vJoy::REAL_VJOY::RelinquishVJD(vJoy_3DS.deviceIdentifier);
	}

	#ifdef GAMEPAD_VJOY_DYNAMIC_LINK
		// Release our vJoy module:
		if (vJoyModule != NULL) // nullptr
		{
			FreeLibrary(vJoyModule); // vJoyModule = NULL;
			//vJoy::REAL_VJOY::restoreFunctions();
		}
	#endif

	// Return the calculated response.
	return ((running) ? 0 : ERROR_CODE);
}