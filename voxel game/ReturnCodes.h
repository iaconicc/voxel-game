#pragma once
#include <Windows.h>

#define RC_NORMAL 0x01
#define RC_WND_EXCEPTIOM 0x02
#define RC_KBD_EXCEPTIOM 0x03
#define RC_UNKNOWN_ERROR -1

WCHAR* convertRCtoString(int rc);