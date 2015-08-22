/*
	This file is a modified version of the IOSync project's "vJoyDriver.h" file. -Anthony Diamond (Developer of IOSync)

	This module ("vJoyDriver.cpp"+"vJoyDriver.h") provides only a subset of the vJoy interface API.
*/

// Preprocessor related:
#pragma once

#ifndef VJOYDRIVER_H
	#define VJOYDRIVER_H

	#define GAMEPAD_VJOY_ENABLED

	#define VJOY_INTERFACE_LIB "VJOYINTERFACE"
	#define VJOY_INTERFACE_DLL "VJOYINTERFACE.DLL"

	#ifdef GAMEPAD_VJOY_ENABLED
		#define GAMEPAD_VJOY_DYNAMIC_LINK
	
		#ifdef _WIN64
			#define VJOY_INTERFACE_DLL_GLOBAL "vJoy\\x64\\vJoyInterface.dll"
		#else
			#define VJOY_INTERFACE_DLL_GLOBAL "vJoy\\x86\\vJoyInterface.dll"
			#define VJOY_INTERFACE_DLL_GLOBAL_ALT "vJoy\\vJoyInterface.dll"
		#endif

		// Includes:
		#include <windows.h>
		
		#include <vJoy/vjoyinterface.h>
		#include <vJoy/public.h>

		// Standard library:
		#include <map>
		//#include <list>
	
		#include <cstdarg>

		// Libraries:
		#ifndef GAMEPAD_VJOY_DYNAMIC_LINK
			#pragma comment(lib, VJOY_INTERFACE_LIB)
		#endif

		// Namespace(s):
		namespace vJoy
		{
			// Namespace(s):
			using namespace std;

			namespace REAL_VJOY
			{
				// Functions:

				// This command restores the internal function-pointers to their original state.
				BOOL restoreFunctions();

				BOOL linkTo(HMODULE hDLL);

				HMODULE linkTo(LPCTSTR path);
				HMODULE linkTo();

				HMODULE globalLinkTo(size_t paths, ...);

				// vJoy API:
				BOOL __cdecl vJoyEnabled();
				BOOL __cdecl DriverMatch(WORD* DLLVer, WORD* DrvVer);
				BOOL __cdecl AcquireVJD(UINT rID);
				VOID __cdecl RelinquishVJD(UINT rID);
				BOOL __cdecl ResetVJD(UINT rID);
				BOOL __cdecl UpdateVJD(UINT rID, PVOID pData);
				enum VjdStat __cdecl GetVJDStatus(UINT rID);
				BOOL __cdecl GetVJDAxisExist(UINT rID, UINT axis);
				BOOL __cdecl GetVJDAxisMin(UINT rID, UINT axis, LONG* min);
				BOOL __cdecl GetVJDAxisMax(UINT rID, UINT axis, LONG* max);
				int __cdecl GetVJDContPovNumber(UINT rID);
			}
				
			// Structures:
			struct vJoyDevice
			{
				// Structures:
				struct axis final
				{
					// Fields:
					LONG min, max;
				};

				// Typedefs:
				typedef map<UINT, axis> axes_t;

				// Fields:
				UINT deviceIdentifier;
				int contPOVNumber;

				axes_t axes;

				// Methods:
				inline axes_t::iterator getAxis(const UINT axis)
				{
					auto axisIterator = axes.find(axis);

					if (axisIterator == axes.end())
					{
						if (REAL_VJOY::GetVJDAxisExist(deviceIdentifier, axis))
						{
							LONG axis_min, axis_max;

							REAL_VJOY::GetVJDAxisMin(deviceIdentifier, axis, &axis_min);
							REAL_VJOY::GetVJDAxisMax(deviceIdentifier, axis, &axis_max);

							axes[axis] = { axis_min, axis_max };

							return axes.find(axis);
						}
					}

					return axisIterator;
				}

				inline bool hasAxis(const UINT axis)
				{
					return (getAxis(axis) != axes.end());
				}
							
				inline LONG axisMin(const UINT axis)
				{
					auto aIterator = getAxis(axis);

					if (aIterator != axes.end())
					{
						return aIterator->second.min;
					}

					//return -LONG_MAX;
					return 0;
				}

				inline LONG axisMax(const UINT axis)
				{
					auto aIterator = getAxis(axis);

					if (aIterator != axes.end())
					{
						return aIterator->second.max;
					}

					//return LONG_MAX;
					return 0;
				}

				// This command should only be called when initializing / "refreshing" this device.
				inline void update()
				{
					// Retrieve proper axis metrics:
					getAxis(HID_USAGE_X);
					getAxis(HID_USAGE_Y);

					getAxis(HID_USAGE_RX);
					getAxis(HID_USAGE_RY);

					getAxis(HID_USAGE_POV);
					getAxis(HID_USAGE_Z);

					// Get the number of continuous POV HATs.
					contPOVNumber = REAL_VJOY::GetVJDContPovNumber(deviceIdentifier);
				}
			};

			// Classes:
			// Nothing so far.
		}
	#endif
#endif