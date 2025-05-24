#include "Camera.h"
#include <memory.h>

vec3 up = {0.0f, 1.0f, 0.0f};

typedef struct {
	vec3 pos;
    vec3 target;
    vec3 forward;
	vec3 right;
}Camera;

Camera camera;

void getCameraTargetAndPosition(vec3* pos, vec3* target)
{
    memcpy(pos, &camera.pos, sizeof(vec3));
    memcpy(target, &camera.target, sizeof(vec3));
}

void initialiseCamera()
{
    glm_vec3_copy((vec3) { 0.0f, 0.0f, 5.0f }, camera.pos);
    glm_vec3_sub(camera.target, camera.pos, camera.forward);
    glm_vec3_normalize(camera.forward);
    glm_vec3_cross(camera.forward, up, camera.right);
    glm_vec3_normalize(camera.right);
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