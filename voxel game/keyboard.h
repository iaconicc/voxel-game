#pragma once
#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
   EVENT_KEY_Press = 0x01,
   EVENT_KEY_Release = 0x02,
   EVENT_KEY_Invalid = 0x03
} EventType;

typedef struct {
	uint8_t vkcode;
	EventType type;
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