#include "App.h"
#include "ReturnCodes.h"

#define MODULE L"Main"
#include "Logger.h"


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	 //inits program then runs until program exits or has exception
	if (StartLogger() != 0) {
		LogWarning(L"failed to start logger");
		return -1;
	}

	LogInfo(L"Application start");
	int returnStatus = ApplicationStartAndRun(hInstance, 800, 600, L"voxel game engine");
	
	if (returnStatus != RC_NORMAL)
	{
		LogWarning(L"Application is exiting with the exception code: %u or %s", returnStatus, convertRCtoString(returnStatus));
		goto end;
	}

	 LogInfo(L"Application has exited normally with code: %u or %s", returnStatus, convertRCtoString(returnStatus));
	 StopLogger();

	 end:
	 return returnStatus;
}
