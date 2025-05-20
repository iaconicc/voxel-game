#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum 
{
	Event_L_Press = 0x01,
	Event_L_Release = 0x02,
	Event_R_Press = 0x03,
	Event_R_Release = 0x04,
	Event_Wheel_Up = 0x05,
	Event_Wheel_Down = 0x06,
	Event_Move = 0x07,
	Event_Midde_Press = 0x08,
	Event_Midde_Down = 0x09,
	Event_Invalid = 0x00,
}MouseEventType;

typedef enum {
	Mouse_Left = 0x01,
	Mouse_Right = 0x02,
	Mouse_Middle = 0x04,
}MouseState;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint8_t type; //this is event type use the enum above
}MouseEvent;

void* InitMouseModuleAndGetOwnership();

uint8_t GetMouseStates(); //this uses the mouse state enum
uint16_t GetMouseX();
uint16_t GetMouseY();
MouseEvent ReadMouseEvent();

bool isMouseBufferFull();
bool isMouseBufferEmpty();

void FlushMouseEvents();

void DestroyMouseModuleAndRevokeOwnership(void** MouseOpsPtr);