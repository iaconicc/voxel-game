#include "Camera.h"
#include <memory.h>

vec3 up = {0.0f, 1.0f, 0.0f};

typedef struct {
	vec3 pos;
    vec3 target;
    vec3 forward;
	vec3 right;
    float rotX;
    float rotY;
}Camera;

Camera camera;

void getCameraTargetAndPosition(vec3* pos, vec3* target)
{
    memcpy(pos, &camera.pos, sizeof(vec3));
    memcpy(target, &camera.target, sizeof(vec3));
}

void initialiseCamera()
{
    glm_vec3_copy((vec3) { 0.0f, 36.0f, -5.0f }, camera.pos);
    glm_vec3_copy((vec3) { 0.0f, 36.0f, 0.0f }, camera.target);
    glm_vec3_sub(camera.target, camera.pos, camera.forward); //calculate forward vector
    glm_vec3_normalize(camera.forward);
    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);

    camera.rotY = 0;
    camera.rotX = 0;

    vec3 rotationAxis = { 0.0f, 1.0f, 0.0f }; // Y-axis for yaw
    glm_vec3_rotate(camera.forward, glm_rad(camera.rotX), rotationAxis);

    vec3 rightAxis;
    glm_vec3_cross(camera.forward, up, rightAxis);
    glm_vec3_normalize(rightAxis);
    glm_vec3_rotate(camera.forward, glm_rad(camera.rotY), rightAxis);

    // Recalculate the right vector after both rotations
    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);

    glm_vec3_add(camera.pos, camera.forward, camera.target);
}

void SetCamPos(vec3 newPos)
{
    glm_vec3_copy(newPos, camera.pos);
}

void MoveCameraForward(float displacement)
{
    vec3 movement = { 0.0f, 0.0f, 0.0f };
    glm_vec3_scale(camera.forward, displacement, movement);
    glm_vec3_add(camera.pos, movement, camera.pos);

    glm_vec3_add(camera.pos, camera.forward, camera.target);
}

void MoveCameraBack(float displacement)
{
    vec3 movement = { 0.0f, 0.0f, 0.0f };
    glm_vec3_scale(camera.forward, -displacement, movement);
    glm_vec3_add(camera.pos, movement, camera.pos);

    glm_vec3_add(camera.pos, camera.forward, camera.target);
}

void StrafeCameraLeft(float displacement)
{
    vec3 movement = { 0.0f, 0.0f, 0.0f };
    glm_vec3_scale(camera.right, -displacement, movement);
    glm_vec3_add(camera.pos, movement, camera.pos);

    glm_vec3_add(camera.pos, camera.forward, camera.target);
}

void StrafeCameraRight(float displacement)
{
    vec3 movement = { 0.0f, 0.0f, 0.0f };
    glm_vec3_scale(camera.right, displacement, movement);
    glm_vec3_add(camera.pos, movement, camera.pos);

    glm_vec3_add(camera.pos, camera.forward, camera.target);
}

void RotateCam(float deltaX, float deltaY)
{
    camera.rotX += deltaX;
    camera.rotY += deltaY;

    // Rotate around the Y-axis (yaw)
    vec3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    glm_vec3_rotate(camera.forward, glm_rad(deltaX), rotationAxis);

    // Rotate around the right axis (pitch)
    vec3 rightAxis;
    glm_vec3_cross(camera.forward, up, rightAxis);
    glm_vec3_normalize(rightAxis);
    glm_vec3_rotate(camera.forward, glm_rad(deltaY), rightAxis);

    // Recalculate the right vector after both rotations
    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);

    // Update the target position
    glm_vec3_add(camera.pos, camera.forward, camera.target);
}