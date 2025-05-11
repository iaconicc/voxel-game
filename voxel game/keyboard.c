#include "keyboard.h"
#include "bitfield.h"
#include "FIFO.h"
#include "Exceptions.h"

bool keyboardIsOwned = false;
bool AutoRepeatEnabled = false;

BitField* keystates;
FIFO* keyevents;
FIFO* characterFifo;



typedef struct
{
	void (*OnKeyPressed)(uint8_t virtualKey);
	void (*OnKeyReleased)(uint8_t virtualKey);
	void (*OnChar)(WCHAR character);
	void (*ClearState)();
}keyboardOps;

keyboardOps ops;


static void OnKeyPressed(uint8_t virtualKey)
{
	int index = virtualKey % 256;
	SetBit(&keystates, index);
	KeyEvent event ={
		.type = EVENT_KEY_Press,
		.vkcode = virtualKey
	};
	PushElement(&keyevents, &event);
}

static void OnKeyReleased(uint8_t virtualKey)
{
	int index = virtualKey % 256;
	UnsetBit(&keystates, index);
	KeyEvent event = {
	.type = EVENT_KEY_Release,
	.vkcode = virtualKey
	};
	PushElement(&keyevents, &event);
}
static void OnChar(WCHAR character)
{
	PushElement(&characterFifo, &character);
}

static void ClearState()
{
	ClearField(&keystates);
}

bool isAutoRepeatEnabled()
{
	return AutoRepeatEnabled;
}

void AutoRepeatEnable(bool enable)
{
	AutoRepeatEnabled = enable;
}

void* InitKeyboardModuleAndGetOwnership()
{
	if (keyboardIsOwned)
		return NULL;

	InitBitField(&keystates, 256);
	InitFIFO(&characterFifo, 128, sizeof(WCHAR));
	InitFIFO(&keyevents, 128, sizeof(KeyEvent));

	if (!keystates)
	{
		Exception(RC_KBD_EXCEPTIOM, L"An exception occured while creating keystates bitmap");
		return NULL;
	}

	if (!characterFifo)
	{
		Exception(RC_KBD_EXCEPTIOM, L"An exception occured while creating character buffer");
		return NULL;
	}

	if (!keyevents)
	{
		Exception(RC_KBD_EXCEPTIOM, L"An exception occured while creating key events buffer");
		return NULL;
	}

	keyboardIsOwned = true;

	ops.OnKeyPressed = OnKeyPressed;
	ops.OnKeyReleased = OnKeyReleased;
	ops.OnChar = OnChar;
	ops.ClearState = ClearState;

	return &ops;
}

bool keyIsPressed(uint8_t virtualkey)
{
	int index = virtualkey % 256;
	return ReadBit(&keystates, index);
}

KeyEvent ReadKey()
{
	KeyEvent* event = (KeyEvent*) PopElement(&keyevents);
	if (!event)
	{
		KeyEvent invalidEvent = {
			.type = EVENT_KEY_Invalid,
			.vkcode = NULL
		};
		return invalidEvent;
	}
	return *event;
}

WCHAR ReadChar()
{
	WCHAR* character = (WCHAR*) PopElement(&characterFifo);
	if (!character)
		return NULL;

	return *character;
}

bool isCharacterBufferFull()
{
	return isFIFOFull(&characterFifo);
}

bool isCharacterBufferEmpty()
{
	return isFIFOEmpty(&characterFifo);
}

bool isKeyEventsBufferFull()
{
	return isFIFOFull(&keyevents);
}

bool isKeyEventsBufferEmpty()
{
	return isFIFOEmpty(&keyevents);
}

void FlushCharacters()
{
	FlushFIFO(&characterFifo);
}

void FlushKeys()
{
	FlushFIFO(&keyevents);
}
void DestroyKeyboardModuleAndRevokeOwnership()
{
	if (!keyboardIsOwned)
		return;

	keyboardIsOwned = false;
	DestroyBitField(&keystates);
	DestroyFIFO(&characterFifo);
	DestroyFIFO(&keyevents);
}