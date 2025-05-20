#include "ReturnCodes.h"

#define EXCEPTIONS 5

WCHAR* stringRC[EXCEPTIONS] = {
   {L"UNUSED"},
   {L"RC_NORMAL"},
   {L"RC_WND_EXCEPTION"},
   {L"RC_KBD_EXCEPTION"},
   {L"RC_MOUSE_EXCEPTION"},
};

WCHAR* convertRCtoString(int rc)
{
   if (rc < 0 || rc >= EXCEPTIONS) {
       return stringRC[0]; // Return "UNUSED" for invalid indices
   }

   return stringRC[rc];
}
