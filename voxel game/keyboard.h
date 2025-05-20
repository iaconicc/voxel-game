#pragma once
#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
   EVENT_KEY_Press = 0x01,
   EVENT_KEY_Release = 0x02,
   EVENT_KEY_Invalid = 0x00
} KeyEventType;

typedef struct {
	uint8_t vkcode;
	uint8_t type; //this is event type use the enum above
}KeyEvent;

void* InitKeyboardModuleAndGetOwnership();

bool keyIsPressed(uint8_t virtualkey);
KeyEvent ReadKey();
WCHAR ReadChar();

bool isCharacterBufferFull();
bool isCharacterBufferEmpty();
bool isKeyEventsBufferFull();
bool isKeyEventsBufferEmpty();

void FlushKeys();
void FlushCharacters();

bool isAutoRepeatEnabled();
void AutoRepeatEnable(bool enable);

void DestroyKeyboardModuleAndRevokeOwnership(void** ops);