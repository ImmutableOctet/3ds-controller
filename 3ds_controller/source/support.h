#pragma once

// Standard "support" header for non-3DS software. (Provides macros and structures)
#ifndef _3DS_NETINPUT_SUPPORT_H
	#define _3DS_NETINPUT_SUPPORT_H

	// Includes:
	//#include <cstddef>
	#include <cstdint>
	
	// Typedefs:
	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

	typedef int8_t s8;
	typedef int16_t s16;
	typedef int32_t s32;
	typedef int64_t s64;

	typedef volatile u8 vu8;
	typedef volatile u16 vu16;
	typedef volatile u32 vu32;
	typedef volatile u64 vu64;

	typedef volatile s8 vs8;
	typedef volatile s16 vs16;
	typedef volatile s32 vs32;
	typedef volatile s64 vs64;

	// Structures:

	// CTRULib definitions:
	typedef enum
	{
		KEY_A       = BIT(0),
		KEY_B       = BIT(1),
		KEY_SELECT  = BIT(2),
		KEY_START   = BIT(3),
		KEY_DRIGHT  = BIT(4),
		KEY_DLEFT   = BIT(5),
		KEY_DUP     = BIT(6),
		KEY_DDOWN   = BIT(7),
		KEY_R       = BIT(8),
		KEY_L       = BIT(9),
		KEY_X       = BIT(10),
		KEY_Y       = BIT(11),
		KEY_ZL      = BIT(14), // (new 3DS only)
		KEY_ZR      = BIT(15), // (new 3DS only)
		KEY_TOUCH   = BIT(20), // Not actually provided by HID
		KEY_CSTICK_RIGHT = BIT(24), // c-stick (new 3DS only)
		KEY_CSTICK_LEFT  = BIT(25), // c-stick (new 3DS only)
		KEY_CSTICK_UP    = BIT(26), // c-stick (new 3DS only)
		KEY_CSTICK_DOWN  = BIT(27), // c-stick (new 3DS only)
		KEY_CPAD_RIGHT = BIT(28), // circle pad
		KEY_CPAD_LEFT  = BIT(29), // circle pad
		KEY_CPAD_UP    = BIT(30), // circle pad
		KEY_CPAD_DOWN  = BIT(31), // circle pad

		// Generic catch-all directions
		KEY_UP    = KEY_DUP    | KEY_CPAD_UP,
		KEY_DOWN  = KEY_DDOWN  | KEY_CPAD_DOWN,
		KEY_LEFT  = KEY_DLEFT  | KEY_CPAD_LEFT,
		KEY_RIGHT = KEY_DRIGHT | KEY_CPAD_RIGHT,
	} PAD_KEY;

	typedef struct
	{
		u16 px, py;
	} touchPosition;

	typedef struct
	{
		s16 dx, dy;
	} circlePosition;

	typedef struct
	{
		s16 x;
		s16 y;
		s16 z;
	} accelVector;

	typedef struct
	{
		s16 x;//roll
		s16 z;//yaw
		s16 y;//pitch
	} angularRate;
#endif