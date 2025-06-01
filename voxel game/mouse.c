#include "mouse.h"
#include "FIFO.h"

#define MODULE L"Mouse"
#include "Logger.h"

#define MOUSEEVENTSSIZE 128

bool mouseIsOwned = false;

uint8_t mouseState;

FIFO* MouseEvents = NULL;

uint16_t g_x;
uint16_t g_y;

static void OnMouseMove(uint16_t x, uint16_t y);
static void OnLeftPressed(uint16_t x, uint16_t y);
static void OnLeftReleased(uint16_t x, uint16_t y);
static void OnRightPressed(uint16_t x, uint16_t y);
static void OnRightReleased(uint16_t x, uint16_t y);
static void OnWheelUp(uint16_t x, uint16_t y);
static void OnWheelDown(uint16_t x, uint16_t y);
static void OnMiddlePressed(uint16_t x, uint16_t y);
static void OnMiddleReleased(uint16_t x, uint16_t y);
static void ClearState();

MouseOps mOps = {
	.OnMouseMove = OnMouseMove,
	.OnLeftPressed = OnLeftPressed,
	.OnLeftReleased = OnLeftReleased,
	.OnRightPressed = OnRightPressed,
	.OnRightReleased = OnRightReleased,
	.OnWheelUp = OnWheelUp,
	.OnWheelDown = OnWheelDown,
	.OnMiddlePressed = OnMiddlePressed,
	.OnMiddleReleased = OnMiddleReleased,
	.ClearState = ClearState,
};

bool InitMouseModuleAndGetOwnership(MouseOps** ops)
{
	if (mouseIsOwned)
		return false;

	InitFIFO(&MouseEvents, MOUSEEVENTSSIZE, sizeof(MouseEvent));

	if (!MouseEvents)
	{
		LogException(L"An exception occured while creating mouse events buffer");
		return false;
	}

	(*ops) = &mOps;

	mouseIsOwned = true;
	LogInfo(L"Mouse module initiliased with no issues.");
	return true;
}

uint8_t GetMouseStates()
{
	return mouseState;
}

uint16_t GetMouseX()
{
	return g_x;
}

uint16_t GetMouseY()
{
	return g_y;
}

MouseEvent ReadMouseEvent()
{
	MouseEvent* event = (MouseEvent*) PopElement(&MouseEvents);
	if (!event)
	{
		MouseEvent invalidEvent = {
			.type = Event_Invalid,
			.x = 0,
			.y = 0,
		};
		return invalidEvent;
	}
	return *event;
}

bool isMouseBufferFull()
{
	return isFIFOFull(&MouseEvents);
}

bool isMouseBufferEmpty()
{
	return isFIFOEmpty(&MouseEvents);
}

void FlushMouseEvents()
{
	FlushFIFO(&MouseEvents);
	LogDebug(L"mouse events buffer flushed.");
}

static void OnMouseMove(uint16_t x, uint16_t y)
{
	MouseEvent event = {
		.type = Event_Move,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	//LogDebug(L"Mouse moved to : %hu, %hu", x, y);
}

static void OnLeftPressed(uint16_t x, uint16_t y)
{
	mouseState |= Mouse_Left;
	MouseEvent event = {
		.type = Event_L_Press,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse L clicked at : %hu, %hu", x, y);
}

static void OnLeftReleased(uint16_t x, uint16_t y)
{
	mouseState &= ~(Mouse_Left);
	MouseEvent event = {
		.type = Event_L_Release,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse L Release at : %hu, %hu", x, y);
}

static void OnRightPressed(uint16_t x, uint16_t y)
{
	mouseState |= Mouse_Right;
	MouseEvent event = {
		.type = Event_R_Press,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse R pressed at : %hu, %hu", x, y);
}

static void OnRightReleased(uint16_t x, uint16_t y)
{
	mouseState &= ~(Mouse_Right);
	MouseEvent event = {
		.type = Event_R_Release,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse R Released at : %hu, %hu", x, y);
}

static void OnWheelUp(uint16_t x, uint16_t y)
{
	MouseEvent event = {
		.type = Event_Wheel_Up,
		.x = x,
		.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse Wheel up at : %hu, %hu", x, y);
}

static void OnWheelDown(uint16_t x, uint16_t y)
{
	MouseEvent event = {
	.type = Event_Wheel_Down,
	.x = x,
	.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse Wheel down at : %hu, %hu", x, y);
}

static void OnMiddlePressed(uint16_t x, uint16_t y)
{
	mouseState |= Mouse_Middle;
	MouseEvent event = {
	.type = Event_Wheel_Down,
	.x = x,
	.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse Middle pressed at : %hu, %hu", x, y);
}

static void OnMiddleReleased(uint16_t x, uint16_t y)
{
	mouseState &= ~(Mouse_Middle);
	MouseEvent event = {
	.type = Event_Wheel_Down,
	.x = x,
	.y = y,
	};
	g_x = x;
	g_y = y;
	PushElement(&MouseEvents, &event);
	LogDebug(L"Mouse Middle released at : %hu, %hu", x, y);
}

static void ClearState()
{
	mouseState = 0;
	LogDebug(L"Mouse state cleared");
}

void DestroyMouseModuleAndRevokeOwnership(MouseOps** MouseOpsPtr)
{
	if (!mouseIsOwned)
		return;

	if (*MouseOpsPtr != &mOps)
		return;

	mouseIsOwned = false;
	*MouseOpsPtr = NULL;

	DestroyFIFO(&MouseEvents);

	LogInfo(L"Mouse module destroyed and ownership revoked.");
}