#include "mouse.h"
#include "FIFO.h"

#define MODULE L"Mouse"
#include "Logger.h"

#define MOUSEEVENTSSIZE 128

bool mouseIsOwned = false;

uint8_t MouseState;

FIFO* MouseEvents = NULL;

uint16_t g_x;
uint16_t g_y;

typedef struct {
	void (*OnMouseMove)(uint16_t x, uint16_t y);
	void (*OnLeftPressed)(uint16_t x, uint16_t y);
	void (*OnLeftReleased)(uint16_t x, uint16_t y);
	void (*OnRightPressed)(uint16_t x, uint16_t y);
	void (*OnRightReleased)(uint16_t x, uint16_t y);
	void (*OnWheelUp)(uint16_t x, uint16_t y);
	void (*OnWheelDown)(uint16_t x, uint16_t y);
	void (*OnMiddlePressed)(uint16_t x, uint16_t y);
	void (*OnMiddleReleased)(uint16_t x, uint16_t y);
}MouseOps;

MouseOps ops;

void OnMouseMove(uint16_t x, uint16_t y);
void OnLeftPressed(uint16_t x, uint16_t y);
void OnLeftReleased(uint16_t x, uint16_t y);
void OnRightPressed(uint16_t x, uint16_t y);
void OnRightReleased(uint16_t x, uint16_t y);
void OnWheelUp(uint16_t x, uint16_t y);
void OnWheelDown(uint16_t x, uint16_t y);
void OnMiddlePressed(uint16_t x, uint16_t y);
void OnMiddleReleased(uint16_t x, uint16_t y);


void* InitMouseModuleAndGetOwnership()
{
	if (mouseIsOwned)
		return NULL;

	InitFIFO(&MouseEvents, MOUSEEVENTSSIZE, sizeof(MouseEvent));

	if (!MouseEvents)
	{
		LogException(RC_MOUSE_EXCEPTION, L"An exception occured while creating mouse events buffer");
		return NULL;
	}

	ops.OnMouseMove = OnMouseMove;
	ops.OnLeftPressed = OnLeftPressed;
	ops.OnLeftReleased = OnLeftReleased;
	ops.OnRightPressed = OnRightPressed;
	ops.OnRightReleased = OnRightReleased;
	ops.OnWheelUp = OnWheelUp;
	ops.OnWheelDown = OnWheelDown;
	ops.OnMiddlePressed = OnMiddlePressed;
	ops.OnMiddleReleased = OnMiddleReleased;

	mouseIsOwned = true;
	LogInfo(L"Mouse module initiliased with no issues.");
	return;
}

uint8_t GetMouseStates()
{
	return MouseState;
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

void OnMouseMove(uint16_t x, uint16_t y)
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

void OnLeftPressed(uint16_t x, uint16_t y)
{
	MouseState |= Mouse_Left;
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

void OnLeftReleased(uint16_t x, uint16_t y)
{
	MouseState &= ~(Mouse_Left);
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

void OnRightPressed(uint16_t x, uint16_t y)
{
	MouseState |= Mouse_Right;
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

void OnRightReleased(uint16_t x, uint16_t y)
{
	MouseState &= ~(Mouse_Right);
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

void OnWheelUp(uint16_t x, uint16_t y)
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

void OnWheelDown(uint16_t x, uint16_t y)
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

void OnMiddlePressed(uint16_t x, uint16_t y)
{
	MouseState |= Mouse_Middle;
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

void OnMiddleReleased(uint16_t x, uint16_t y)
{
	MouseState &= ~(Mouse_Middle);
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

void DestroyMouseModuleAndRevokeOwnership(void** MouseOpsPtr)
{
	if (!mouseIsOwned)
		return;

	if (*MouseOpsPtr != &ops)
		return;

	mouseIsOwned = false;
	*MouseOpsPtr = NULL;

	DestroyFIFO(&MouseEvents);

	LogInfo(L"Mouse module destroyed and ownership revoked.");
}