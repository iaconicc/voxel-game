#include "Camera.h"
#include <memory.h>

vec3 up = {0.0f, 1.0f, 0.0f};

typedef struct {
	double Worldpos[3];
    vec3 target;
    vec3 forward;
	vec3 right;
    float rotX;
    float rotY;
}Camera;

Camera camera;

void initialiseCamera()
{
    camera.Worldpos[0] = 0.0f;
    camera.Worldpos[1] = 0.0f;
    camera.Worldpos[2] = 0.0f;

    glm_vec3_copy((vec3) { 0.0f, 0.0f, 1.0f }, camera.forward);
    glm_vec3_normalize(camera.forward);

    vec3 renderTarget;
    renderTarget[0] = (float)(camera.Worldpos[0] + camera.forward[0]);
    renderTarget[1] = (float)(camera.Worldpos[1] + camera.forward[1]);
    renderTarget[2] = (float)(camera.Worldpos[2] + camera.forward[2]);

    glm_vec3_copy(renderTarget, camera.target);

    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);

    camera.rotY = 0;
    camera.rotX = 0;
}

void SetCamWorldPos(vec3 newPos)
{
    camera.Worldpos[0] += (double)newPos[0];
    camera.Worldpos[1] += (double)newPos[1];
    camera.Worldpos[2] += (double)newPos[2];
}

void MoveCameraForward(float displacement)
{
    camera.Worldpos[0] += camera.forward[0] * displacement;
    camera.Worldpos[1] += camera.forward[1] * displacement;
    camera.Worldpos[2] += camera.forward[2] * displacement;
}

void MoveCameraBack(float displacement)
{
    camera.Worldpos[0] -= camera.forward[0] * displacement;
    camera.Worldpos[1] -= camera.forward[1] * displacement;
    camera.Worldpos[2] -= camera.forward[2] * displacement;
}

void StrafeCameraLeft(float displacement)
{
    camera.Worldpos[0] -= camera.right[0] * displacement;
    camera.Worldpos[1] -= camera.right[1] * displacement;
    camera.Worldpos[2] -= camera.right[2] * displacement;
}

void StrafeCameraRight(float displacement)
{
    camera.Worldpos[0] += camera.right[0] * displacement;
    camera.Worldpos[1] += camera.right[1] * displacement;
    camera.Worldpos[2] += camera.right[2] * displacement;
}

void RotateCam(float deltaX, float deltaY)
{
    camera.rotX += deltaX;
    camera.rotY += deltaY;

    // Start with default forward vector
    glm_vec3_copy((vec3) { 0.0f, 0.0f, 1.0f }, camera.forward);

    // Rotate around Y-axis (yaw)
    glm_vec3_rotate(camera.forward, glm_rad(camera.rotX), up);

    // Compute right axis from forward
    vec3 rightAxis;
    glm_vec3_cross(camera.forward, up, rightAxis);
    glm_vec3_normalize(rightAxis);

    // Rotate around right axis (pitch)
    glm_vec3_rotate(camera.forward, glm_rad(camera.rotY), rightAxis);

    // Recalculate right vector
    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);

    // Update target for view matrix use
    glm_vec3_add((vec3) { 0.0f, 0.0f, 0.0f }, camera.forward, camera.target);
}

void getCameraTargetAndForward(vec3* outTarget, vec3* outForward)
{
    if (outTarget) glm_vec3_copy(camera.target, *outTarget);
    if (outForward) glm_vec3_copy(camera.forward, *outForward);
}

void getCameraWorldPos(double out[3]) {
    out[0] = camera.Worldpos[0];
    out[1] = camera.Worldpos[1];
    out[2] = camera.Worldpos[2];
}
