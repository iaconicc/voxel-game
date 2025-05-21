#pragma once
#include <Windows.h>

#define RC_NORMAL 0x01
#define RC_WND_EXCEPTION 0x02
#define RC_KBD_EXCEPTION 0x03
#define RC_MOUSE_EXCEPTION 0x04
#define RC_DX3D11_EXPCEPTION 0x04
#define RC_UNKNOWN_EXCEPTION -1

//returns a string for a provided return code
WCHAR* convertRCtoString(int rc);