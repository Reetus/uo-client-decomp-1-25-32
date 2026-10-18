// Entry point of the client.
//
// The original's start-up (0x0048ca70) is the Visual C++ runtime's WinMainCRTStartup: it sets the heap
// and the stdio tables up, runs the table of static initialisers, calls WinMain with the command line
// behind the program name, and hands the result to exit, which runs what the initialisers registered
// with atexit. The host runtime does all of that around this function except for the initialisers:
// those are ports, called here in the original's order before the client's WinMain.
#include <windows.h>

#include "../game/static_init_table.h"
#include "../game/winmain.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show)
{
    for (int i = 0; i < g_staticInitialiserCount; i++)
        g_staticInitialisers[i]();
    return WinMain_(instance, previous, commandLine, show);
}
