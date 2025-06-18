#pragma once
#include <cglm.h>

void initialiseCamera();
void getCameraWorldPos(double out[3]);
void SetCamWorldPos(vec3 newPos);
void getCameraTargetAndForward(vec3* outTarget, vec3* outForward);
void MoveCameraForward(float displacement);
void MoveCameraBack(float displacement);
void StrafeCameraLeft(float displacement);
void StrafeCameraRight(float displacement);
void RotateCam(float deltaX, float deltaY);