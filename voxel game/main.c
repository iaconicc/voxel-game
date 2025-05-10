#include "App.h"
#include "ReturnCodes.h"
#include "Exceptions.h"

WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	 int returnStatus = ApplicationStart(hInstance);

	

	 switch (returnStatus)
	 {
	 case RC_NORMAL:
		 return 0;
	 case RC_WNDException:
		 MessageBoxW(NULL, GetExceptionMessage(), L"WndException", MB_OK);
		 return returnStatus;
	 default:
		 -1;
	 }
	 
}
