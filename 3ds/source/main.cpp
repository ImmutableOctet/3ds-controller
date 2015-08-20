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
#define REMOTE_ADDRESS "192.168.1.6"

#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)

#define IPPROTO_TCP 0 // 6
//#define AF_UNSPEC 0

#define SD_SEND 1

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000 // 0x800000

//#define lstat stat

// Includes:
#include <3ds.h>
#include <3ds/types.h>
#include <3ds/services/soc.h>

#include <errno.h> // cerror
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <malloc.h>

// Networking related:
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <netdb.h>

// Typedefs:
typedef int SOCKET;

// Structures:
struct inputData
{
	circlePosition analog;

	u32 kDown;
	u32 kHeld;
	u32 kUp;
};

// Global variable(s):
u32* SOC_buffer = nullptr;

// Functions:
int stop()
{
	printf("Exiting...\n");

	// Deinitialize services:
	SOC_Shutdown();

	free(SOC_buffer); SOC_buffer = nullptr;

	gfxExit();

    return -1;
}

int main(int argc, char** argv)
{
	static const size_t packetSize = sizeof(inputData); // auto
	static const u16 port = 4865;

	// This is a debug-table, mapping bitwise positions to human-readable text.
	static const char* keysNames[32] =
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

	// Initialize services:
	gfxInitDefault();

	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_Initialize(SOC_buffer, SOC_BUFFERSIZE) != 0)
	{
	    printf("Unable to initialize SOC.\n");

	    return stop();
	}

	//printf("SOC Context: %d\n", context);

	// Initialize a console on the top screen.
	consoleInit(GFX_TOP, NULL);

	// Networking related:

	// Address related:
	SOCKET connection = INVALID_SOCKET;

    // Network initialization:
    sockaddr_in remoteAddress = {};

    //printf("Creating socket...\n");

    connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCKET_STREAM

    if (connection == INVALID_SOCKET)
    {
    	return stop();
    }

    //printf("Socket created.\n");

    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(port);
    remoteAddress.sin_addr.s_addr = inet_addr(REMOTE_ADDRESS);

    //printf("Connecting to remote host...\n");//: %s, %d\n", REMOTE_ADDRESS, remoteAddress.sin_addr.s_addr);
	
    int connectResult = connect(connection, (sockaddr*)&remoteAddress, sizeof(remoteAddress));

    if (connectResult != 0)
    {
    	printf("Connection failed: %i", connectResult);
    	//printf("Unable to connect %d\n", connectResult);

    	closesocket(connection);

    	return stop();
    }

    int errCode = 0;

	// This will represent the input-state.
	inputData state = {};

	// Output initial information:
	consoleClear();

	printf("\x1b[0;0HPress Start and Select to exit.");
	printf("\x1b[1;0HCirclePad position:");
	printf("\x1b[2;0H%d]", packetSize);

	// Begin the application-loop:
	while (aptMainLoop())
	{
		// Wait for the system to clear the back-buffer.
		gspWaitForVBlank();
		
		// Scan for HID changes.
		hidScanInput();

		inputData newState; // = {}

		// 'hidKeysDown' returns information about buttons that have just been pressed (And weren't in the previous frame).
		newState.kDown = hidKeysDown();
		
		// 'hidKeysHeld' returns information about buttons that have are held down in this frame.
		newState.kHeld = hidKeysHeld();

		// 'hidKeysUp' returns information about the buttons that have been released this frame.
		newState.kUp = hidKeysUp();

		// Read the CirclePad's position.
		hidCircleRead(&newState.analog);

		// Check for an exit operation:
		if ((newState.kDown & KEY_START) && ((newState.kDown & KEY_SELECT) || (newState.kDown & KEY_L)))
		{
			break;
		}

		// This will check for input, and output accordingly:
		if (memcmp(&state, &newState, sizeof(inputData)) != 0)
		{
			// Clear the console.
			consoleClear();

			// These lines must be written each time, because we clear the console:
			printf("\x1b[0;0HPress Start to exit.");
			printf("\x1b[1;0HCirclePad position:");

			printf("\x1b[3;0H");

			// Output the key that was pressed:
			for (int i = 0; i < 32; i++)
			{
				const auto mask = BIT(i);

				// Check for input:
				if (newState.kDown & mask) printf("%s down\n", keysNames[i]);
				if (newState.kHeld & mask) printf("%s held\n", keysNames[i]);
				if (newState.kUp & mask) printf("%s up\n", keysNames[i]);
			}

			// Print the CirclePad's position.
			printf("\x1b[2;0H%04d; %04d", newState.analog.dx, newState.analog.dy);

			if (send(connection, &newState, sizeof(inputData), 0) == SOCKET_ERROR)
			{
				errCode = -1;

				break;
			}
		}

		// Update our current input-state.
		state = newState;

		// Flush and swap the font and back buffers.
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	// Deinitialize networking functionality:
	shutdown(connection, SD_SEND);
	closesocket(connection);

	stop();

	// Return the calculated response to the system.
	return errCode;
}
