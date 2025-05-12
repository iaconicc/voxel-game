#include "ReturnCodes.h"

WCHAR* stringRC[4] = {
	{L"UNUSED"},
	{L"RC_NORMAL"},
	{L"RC_WND_EXCEPTIOM"},
	{L"RC_KBD_EXCEPTIOM"},
};

WCHAR* convertRCtoString(int rc)
{
	return stringRC[rc];
}
