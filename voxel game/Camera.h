#pragma once
#include <cglm.h>

void initialiseCamera();
void SetCamPos(vec3 newPos);
void getCameraTargetAndPosition(vec3* pos, vec3* target);
void MoveCameraForward(float displacement);
void MoveCameraBack(float displacement);
void StrafeCameraLeft(float displacement);
void StrafeCameraRight(float displacement);
void RotateCam(float deltaX, float deltaY);