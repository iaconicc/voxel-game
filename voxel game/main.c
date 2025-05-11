#include "App.h"
#include "ReturnCodes.h"
#include "Exceptions.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	 
	 //inits program then runs until program exits or has exception
	 int returnStatus = ApplicationStartAndRun(hInstance, 800, 600, L"voxel game engine");
	 
	 return returnStatus;
}
