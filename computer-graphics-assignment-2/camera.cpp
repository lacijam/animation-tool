#include "camera.h"

#include <assert.h>

void camera_init(Camera *cam)
{
	cam->pos = { 0.f, 0.f, 0.f };
	cam->front = { 0.f, 0.f, -1.f };
	cam->up = { 0.f, 1.f, 0.f };
	mat4_identity(cam->view);
	mat4_identity(cam->frustrum);
	mat4_identity(cam->ortho);
	cam->yaw = 0;
	cam->pitch = 0;
	cam->roll = 0;
	cam->fly_speed = 20.f;
	cam->walk_speed = 3.f;
	cam->look_speed = 100.f;
	cam->fov = 45.f;
	cam->flying = true;
}

void camera_frustrum(Camera *cam, unsigned int cx, unsigned int cy)
{
	float near_clip = .05f;
	float far_clip = 1000.f;
	float fov_y = (cam->fov * (float)M_PI / 180.f);
	float aspect = (float)cx / cy;
	float top = near_clip * tanf(fov_y / 2);
	float bottom = -1 * top;
	float left = bottom * aspect;
	float right = top * aspect;
	mat4_frustrum(cam->frustrum, left, right, bottom, top, near_clip, far_clip);
}

void camera_ortho(Camera* cam, unsigned int cx, unsigned int cy)
{
	assert(cam && cx > 0 && cy > 0);
	mat4_ortho(cam->ortho, 0, cx, cy, 0, -1.f, 1.f);
}

void camera_update(Camera *cam)
{
	if (cam->pitch >= 89.f) {
		cam->pitch = 89.f;
	} else if (cam->pitch <= -89.f) {
		cam->pitch = -89.f;
	}

	V3 direction;
	direction.x = cosf(radians(cam->yaw)) * cosf(radians(cam->pitch));
	direction.y = sinf(radians(cam->pitch));
	direction.z = sinf(radians(cam->yaw)) * cosf(radians(cam->pitch));
	cam->front = v3_normalise(direction);
}

inline void camera_move_forward(Camera *cam, float dt)
{
	float vel = cam->walk_speed;

	if (cam->flying) {
		vel = cam->fly_speed;
	}

	cam->pos += vel * dt * cam->front;
}

inline void camera_move_backward(Camera *cam, float dt)
{
	float vel = cam->walk_speed;

	if (cam->flying) {
		vel = cam->fly_speed;
	}

	cam->pos -= vel * dt * cam->front;
}

inline void camera_move_left(Camera *cam, float dt)
{
	float vel = cam->walk_speed;

	if (cam->flying) {
		vel = cam->fly_speed;
	}

	cam->pos -= v3_normalise(v3_cross(cam->front, cam->up)) * vel * dt;
}

inline void camera_move_right(Camera *cam, float dt)
{
	float vel = cam->walk_speed;

	if (cam->flying) {
		vel = cam->fly_speed;
	}

	cam->pos += v3_normalise(v3_cross(cam->front, cam->up)) * vel * dt;
}

inline void camera_look_at(Camera *cam)
{
	mat4_look_at(cam->view, cam->pos, cam->pos + cam->front, cam->up);
}