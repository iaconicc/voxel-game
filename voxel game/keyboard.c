#include "keyboard.h"
#include "bitfield.h"
#include "FIFO.h"

#define MODULE L"kbd"
#include "Logger.h"

bool keyboardIsOwned = false;
bool AutoRepeatEnabled = false;

#define KEYSTATESSIZE 256
#define KEYEVENTSSIZE 128
#define CHARACTERFIFOSIZE 128

BitField* keystates = NULL;
FIFO* keyevents = NULL;
FIFO* characterFifo = NULL;

typedef struct
{
	void (*OnKeyPressed)(uint8_t virtualKey);
	void (*OnKeyReleased)(uint8_t virtualKey);
	void (*OnChar)(WCHAR character);
	void (*ClearState)();
}keyboardOps;

keyboardOps ops;

static void OnKeyPressed(uint8_t virtualKey);
static void OnKeyReleased(uint8_t virtualKey);
static void OnChar(WCHAR character);
static void ClearState();

void* InitKeyboardModuleAndGetOwnership()
{
	if (keyboardIsOwned)
		return NULL;

	InitBitField(&keystates, KEYSTATESSIZE);
	InitFIFO(&characterFifo, CHARACTERFIFOSIZE, sizeof(WCHAR));
	InitFIFO(&keyevents, KEYEVENTSSIZE, sizeof(KeyEvent));

	if (!keystates)
	{
		if (characterFifo)
			DestroyFIFO(&characterFifo);

		if (keyevents)
			DestroyFIFO(&keyevents);

		LogException(RC_KBD_EXCEPTION, L"An exception occured while creating keystates bitmap");
		return NULL;
	}

	if (!characterFifo)
	{
		DestroyBitField(&keystates);

		if (keyevents)
			DestroyFIFO(&keyevents);

		LogException(RC_KBD_EXCEPTION, L"An exception occured while creating character buffer");
		return NULL;
	}

	if (!keyevents)
	{		
		DestroyBitField(&keystates);
		DestroyFIFO(&characterFifo);

		LogException(RC_KBD_EXCEPTION, L"An exception occured while creating key events buffer");
		return NULL;
	}

	keyboardIsOwned = true;

	ops.OnKeyPressed = OnKeyPressed;
	ops.OnKeyReleased = OnKeyReleased;
	ops.OnChar = OnChar;
	ops.ClearState = ClearState;

	LogInfo(L"keyboard module initiliased with no issues.");
	return &ops;
}

static void OnKeyPressed(uint8_t virtualKey)
{
	int index = virtualKey % KEYSTATESSIZE;
	SetBit(&keystates, index);
	KeyEvent event = {
		.type = EVENT_KEY_Press,
		.vkcode = virtualKey
	};
	PushElement(&keyevents, &event);
	LogDebug(L"Key pressed: %u", virtualKey);
}

static void OnKeyReleased(uint8_t virtualKey)
{
	int index = virtualKey % KEYSTATESSIZE;
	UnsetBit(&keystates, index);
	KeyEvent event = {
	.type = EVENT_KEY_Release,
	.vkcode = virtualKey
	};
	PushElement(&keyevents, &event);
	LogDebug(L"Key released: %u", virtualKey);
}

static void OnChar(WCHAR character)
{
	PushElement(&characterFifo, &character);
	LogDebug(L"Character input: %lc", character);
}

static void ClearState()
{
	ClearField(&keystates);
	LogDebug(L"Keyboard state cleared.");
}

bool keyIsPressed(uint8_t virtualkey)
{
	int index = virtualkey % KEYSTATESSIZE;
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

bool isAutoRepeatEnabled()
{
	return AutoRepeatEnabled;
}

void AutoRepeatEnable(bool enable)
{
	AutoRepeatEnabled = enable;
	LogInfo(L"Auto-repeat %s.", enable ? L"enabled" : L"disabled");
}

void FlushCharacters()
{
	FlushFIFO(&characterFifo);
	LogDebug(L"Character buffer flushed.");
}

void FlushKeys()
{
	FlushFIFO(&keyevents);
	LogDebug(L"Key events buffer flushed.");
}

void DestroyKeyboardModuleAndRevokeOwnership(void** keyboardOpsPtr)
{
	if (!keyboardIsOwned)
		return;

	if (*keyboardOpsPtr != &ops)
		return;

	keyboardIsOwned = false;
	*keyboardOpsPtr = NULL;

	DestroyBitField(&keystates);
	DestroyFIFO(&characterFifo);
	DestroyFIFO(&keyevents);

	LogInfo(L"Keyboard module destroyed and ownership revoked.");
}