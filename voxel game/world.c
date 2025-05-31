#include "world.h"
#include "chunk.h"
#include "App.h"
#include <processthreadsapi.h>

static WINAPI WorldThread() {

	while (ProgramIsRunning())
	{

	}

	return 0;
}

void StartWorld()
{
	DWORD id;
	HANDLE handle;
	handle = CreateThread(0,0, WorldThread, 0, 0, &id);
}

