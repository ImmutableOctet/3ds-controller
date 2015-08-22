#pragma once

#ifndef _3DS_NETINPUT_SHARED_H
	#define _3DS_NETINPUT_SHARED_H
	
	// The number of frames required to wait before sending a packet.
	#define FRAMES_PER_SEND 8 // 1

	// Includes:
	#ifdef _3DS_NETINPUT_SHARED_SUPPORT_LIB
		#include "support.h"
	#else
		#include <3ds.h>
	#endif

	// This is a debug-table that maps bitwise positions to human-readable text.
	const char* debug_keyNames[32] =
	{
		"KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
		"KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
		"KEY_R", "KEY_L", "KEY_X", "KEY_Y",
		"", "", "KEY_ZL", "KEY_ZR",
		"", "", "", "",
		"KEY_TOUCH", "", "", "",
		"KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
		"KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
	};

	// Structures:
	struct inputData
	{
		u32 kDown;
		u32 kHeld;
		u32 kUp;
	};

	// Used for extended data, such as
	// touch position, gyro data, acceleration, etc.
	struct extInputData
	{
		// The primary analog stick. (CirclePad)
		circlePosition left_analog; // analog;

		// The secondary analog stick. (C-stick)
		//circlePosition right_analog;

		// The touch coordinates of the last event.
		// If no event has occurred, this will be undefined/zero.
		touchPosition touch;

		// The accelerometer output from the device.
		//accelVector acceleration;

		// The angle the device's gyroscope is pointing.
		//angularRate gyro; // angle;
	};

	struct metaDeviceData
	{
		u8 volume;
	};

	// This contains both extended input data, and basic input-data.
	struct fullInputData
	{
		inputData data;
		extInputData ext;
		metaDeviceData meta;
	};
#endif