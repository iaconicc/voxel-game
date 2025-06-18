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

BitField keystates;
FIFO* keyevents = NULL;
FIFO* characterFifo = NULL;

static void OnKeyPressed(uint8_t virtualKey);
static void OnKeyReleased(uint8_t virtualKey);
static void OnChar(WCHAR character);
static void ClearState();

keyboardOps keyOps = {
	.ClearState = ClearState,
	.OnChar = OnChar,
	.OnKeyPressed = OnKeyPressed,
	.OnKeyReleased = OnKeyReleased,
};

bool InitKeyboardModuleAndGetOwnership(keyboardOps** ops)
{
	if (keyboardIsOwned)
		return false;

	InitBitField(&keystates, KEYSTATESSIZE);
	InitFIFO(&characterFifo, CHARACTERFIFOSIZE, sizeof(WCHAR));
	InitFIFO(&keyevents, KEYEVENTSSIZE, sizeof(KeyEvent));

	if (!InitBitField(&keystates, KEYSTATESSIZE))
	{
		if (characterFifo)
			DestroyFIFO(&characterFifo);

		if (keyevents)
			DestroyFIFO(&keyevents);

		LogException(L"An exception occured while creating keystates bitmap");
		return false;
	}

	if (!characterFifo)
	{
		DestroyBitField(&keystates);

		if (keyevents)
			DestroyFIFO(&keyevents);

		LogException(L"An exception occured while creating character buffer");
		return false;
	}

	if (!keyevents)
	{		
		DestroyBitField(&keystates);
		DestroyFIFO(&characterFifo);

		LogException(L"An exception occured while creating key events buffer");
		return false;
	}

	keyboardIsOwned = true;

	keyOps.OnKeyPressed = OnKeyPressed;
	keyOps.OnKeyReleased = OnKeyReleased;
	keyOps.OnChar = OnChar;
	keyOps.ClearState = ClearState;

	(*ops) = &keyOps;

	LogInfo(L"keyboard module initiliased with no issues.");
	return true;
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
	//LogDebug(L"Key pressed: %u", virtualKey);
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
	//LogDebug(L"Key released: %u", virtualKey);
}

static void OnChar(WCHAR character)
{
	PushElement(&characterFifo, &character);
	//LogDebug(L"Character input: %lc", character);
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

void DestroyKeyboardModuleAndRevokeOwnership(keyboardOps** keyboardOpsPtr)
{
	if (!keyboardIsOwned)
		return;

	if (*keyboardOpsPtr != &keyOps)
		return;

	keyboardIsOwned = false;
	*keyboardOpsPtr = NULL;

	DestroyBitField(&keystates);
	DestroyFIFO(&characterFifo);
	DestroyFIFO(&keyevents);

	LogInfo(L"Keyboard module destroyed and ownership revoked.");
}