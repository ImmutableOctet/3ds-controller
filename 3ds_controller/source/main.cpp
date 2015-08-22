/*
	Original Circle Pad example made by Aurelio Mannara for ctrulib
	Please refer to https://github.com/smealum/ctrulib/blob/master/libctru/include/3ds/services/hid.h for more information
	This code was based on the last modified version, created on: 12/13/2014 2:20 UTC+1

	This wouldn't be possible without the amazing work done by:
	-Smealum
	-fincs
	-WinterMute
	-yellows8
	-plutoo
	-mtheall
	-Many others who worked on 3DS homebrew who I'm surely forgetting about.
*/

/*
	Sadly, there isn't support for IPV6/abstract networking, as it's not supported by ctrulib.
*/

// Preprocessor related:

// General:
#define ERROR_CODE 1 // -1

// Networking related:
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)

#define IPPROTO_TCP 0 // 6
//#define AF_UNSPEC 0

#define SD_SEND 1

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000 // 0x800000

// Includes:
#include <3ds.h>
#include <3ds/types.h>
#include <3ds/services/soc.h>

#include <errno.h> // cerror
#include <malloc.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

// Networking related:
#include <sys/socket.h>
//#include <sys/ioctl.h>
//#include <sys/select.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <netdb.h>

// Internal:
#include "shared.h"

// Typedefs:
typedef int SOCKET;

// Global variable(s):
u32* SOC_buffer = nullptr;

// Functions:
bool initGraphics();
void deinitGraphics();
bool initConsole();
void deinitConsole();
bool initInput();
void deinitInput();
bool initSOC();
void deinitSOC();

// This command will always return 'ERROR_CODE',
// regardless of the program's environment.
int stop()
{
	// Tell the user we're exiting.
	printf("Exiting...\n");

	// Deinitialize services:
	deinitSOC();
	deinitInput();
	deinitConsole();

	deinitGraphics();

	// Return the default response.
    return ERROR_CODE;
}

bool initSOC()
{
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	
	if (SOC_Initialize(SOC_buffer, SOC_BUFFERSIZE) != 0)
	{
	    printf("Unable to initialize SOC.\n");

	    return false;
	}

	// Return the default response.
	return true;
}

void deinitSOC()
{
	SOC_Shutdown();

	free(SOC_buffer); SOC_buffer = nullptr;

	return;
}

bool initGraphics()
{
	gfxInitDefault();

	// Return the default response.
	return true;
}

void deinitGraphics()
{
	gfxExit();

	return;
}

bool initConsole(PrintConsole& topScreen, PrintConsole& bottomScreen)
{
	// Initialize a console on the top screen.
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	consoleSelect(&topScreen);
	consoleDebugInit(debugDevice_CONSOLE);

	// Return the default response.
	return true;
}

void deinitConsole()
{
	// Nothing so far.

	return;
}

bool initInput()
{
	hidInit(nullptr);

	//HIDUSER_EnableAccelerometer();
	//HIDUSER_EnableGyroscope();

	// Return the default response.
	return true;
}

void deinitInput()
{
	hidExit();

	return;
}

int main(int argc, char** argv)
{
	// Constant variable(s):
	static const u16 port = 4865;
	
	// Local variable(s):
	PrintConsole topScreen, bottomScreen;

	// Initialize services:
	if (!initGraphics()) return stop();
	if (!initConsole(topScreen, bottomScreen)) return stop();
	if (!initInput()) return stop();
	if (!initSOC()) return stop();
	
	char remoteAddress_str[256]; // INET_ADDRSTRLEN
	
	// Load the remote address:
	{
		// Read the remote address from the file-system.
		FILE* addrFile = fopen("address.txt", "r");

		if (addrFile == nullptr)
		{
			return stop();
		}

		fgets(remoteAddress_str, sizeof(remoteAddress_str), addrFile);

		fclose(addrFile);
	}

	// Networking related:

	// Address related:

	// This will act as our connection to the remote machine.
	SOCKET connection = INVALID_SOCKET;

    // Network initialization:

    // This will be the remote address we use to connect. (Initialized to zero)
    sockaddr_in remoteAddress = {};

    //printf("Creating socket...\n");

    // Create a new TCP socket for networking functionality.
    connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCKET_STREAM

    // Check if creation failed (Service failure):
    if (connection == INVALID_SOCKET)
    {
    	return stop();
    }

    //printf("Socket created.\n");

    // Initialize our remote address:
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(port);
    remoteAddress.sin_addr.s_addr = inet_addr(remoteAddress_str);

    // Tell the user we're going to be connecting:
    printf("Connecting to address:\n");
    printf(remoteAddress_str);
    printf("\n");
	
	// Attempt to connect to the remote server.
    int connectResult = connect(connection, (sockaddr*)&remoteAddress, sizeof(remoteAddress));

    // Check if our connection was successful:
    if (connectResult != 0)
    {
    	printf("Connection failed: %i\n", connectResult);

    	closesocket(connection);

    	return stop();
    }

    // This acts as the error code that
    // will be returned when this program exits.
    int errCode = 0;

	// This will represent the input-state.
	fullInputData states[FRAMES_PER_SEND] = {};

	// The current state in the 'states' buffer.
	unsigned currentState = 0;

	// This is used to count frames. (Ensures input-data is sent)
	unsigned sendTimer = 0;

	// Clear the console thus far. (Debugging information)
	consoleClear();

	// Output initial information:
	printf("Press START and SELECT together to exit.\n\n");

	// Begin the application:
	while (aptMainLoop())
	{
		// Wait for the system to clear the back-buffer.
		gspWaitForVBlank();
		
		// Local variable(s):

		// This will represent the current input-state for this frame. (Copied when different)
		fullInputData newState = {};

		// Scan for HID changes.
		hidScanInput();

		// Basic input:

		// 'hidKeysDown' returns information about buttons that have just been pressed (And weren't in the previous frame).
		newState.data.kDown = hidKeysDown();
		
		// 'hidKeysHeld' returns information about buttons that have are held down in this frame.
		newState.data.kHeld = hidKeysHeld();

		// 'hidKeysUp' returns information about the buttons that have been released this frame.
		newState.data.kUp = hidKeysUp();
		
		// Extended input:

		// Read the CirclePad's position.
		hidCircleRead(&newState.ext.left_analog);

		// Read the current touch-position.
		hidTouchRead(&newState.ext.touch);

		// Read the acceleration of the device.
		//hidAccelRead(&newState.ext.acceleration);

		// Read the rotation of the device.
		//hidGyroRead(&newState.ext.gyro);

		// Meta:

		// Read the device's speaker-volume.
		HIDUSER_GetSoundVolume(&newState.meta.volume);

		// This will check for input, and output accordingly (To be optimized):
		if (memcmp(&states[currentState], &newState, sizeof(newState)) != 0) // newState.kHeld != 0
		{
			// Clear the console.
			//consoleClear();

			//printf("Press Start And Select to exit.\n");

			// Print the CirclePad's position:
			consoleSelect(&bottomScreen);

			consoleClear();

			printf("Press START and SELECT together to exit.\n\n");
			printf("CirclePad position: %04d, %04d\n", newState.ext.left_analog.dx, newState.ext.left_analog.dy);
			printf("Touch position: %d, %d", newState.ext.touch.px, newState.ext.touch.py);

			consoleSelect(&topScreen);

			// Output the key that was pressed:
			for (int i = 0; i < 32; i++)
			{
				const auto mask = BIT(i);

				// Check for input:
				if (newState.data.kDown & mask) printf("%s down\n", debug_keyNames[i]);
				//if (newState.data.kHeld & mask) printf("%s held\n", debug_keyNames[i]);
				if (newState.data.kUp & mask) printf("%s up\n", debug_keyNames[i]);
			}

			// Update our current input-state.
			states[currentState] = newState;

			currentState += 1;
		}

		// Check if it's time to send a packet and begin a new one:
		if ((sendTimer >= FRAMES_PER_SEND && currentState > 0) || currentState > FRAMES_PER_SEND)
		{
			// Send the number of states saved:
			if (send(connection, &states, sizeof(newState)*currentState, 0) == SOCKET_ERROR)
			{
				errCode = -1;

				break;
			}

			// Reset both the state-offset, and the output-timer:
			sendTimer = 0;
			currentState = 0;
		}

		// Flush and swap the font and back buffers:
		gfxFlushBuffers();
		gfxSwapBuffers();

		// Check for an exit operation:
		if ((newState.data.kHeld & KEY_START) && ((newState.data.kHeld & KEY_SELECT) || (newState.data.kHeld & KEY_L)))
		{
			break;
		}

		// Add to the output-timer.
		sendTimer += 1;
	}

	// Deinitialize networking functionality:
	shutdown(connection, SD_SEND);
	closesocket(connection);

	// Stop the application.
	stop();

	// Return the calculated response to the system.
	return errCode;
}
