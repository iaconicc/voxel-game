#include "App.h"

#define MODULE L"Main"
#include "Logger.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	
	//initialises the logger the program will not start without it
	if (StartLogger() != 0) {
		MessageBoxW(NULL, L"Failed to initialize the logger. Exiting application.", L"Logger Error", MB_OK | MB_ICONERROR);
		LogWarning(L"failed to start logger");
		return -1;
	}

	LogInfo(L"Application starting with command-line arguments: %S", lpCmdLine);

	//inits program then runs until program exits or has exception
	int returnStatus = ApplicationStartAndRun(800, 600, L"voxel game engine");

	// Handle program exit
	if (returnStatus != 1) {
		MessageBoxW(NULL, L"The program has exited with an exception. Please refer to the logs.", L"Error", MB_OK | MB_ICONERROR);
		LogWarning(L"Application exited with exception code:%u", returnStatus);
	}
	else {
		LogInfo(L"Application exited normally with code: %u", returnStatus);
	}

	 StopLogger();
	 LogInfo(L"Logger stopped. Exiting application.");

	 return returnStatus;
}
