#ifndef CAMERA_H
#define CAMERA_H

#include "maths.h"


struct Camera {
	V3 pos;
	V3 front;
	V3 up;
	float view[16];
	float frustrum[16];
	float ortho[16];
	float yaw, pitch, roll;
	float fly_speed;
	float walk_speed;
	float look_speed;
	float fov;
	bool flying;
};

extern void camera_init(Camera *cam);
extern void camera_frustrum(Camera* cam, unsigned int cx, unsigned int cy);
extern void camera_ortho(Camera *cam, unsigned int cx, unsigned int cy);
extern void camera_update(Camera *cam);
extern void camera_move_forward(Camera *cam, float dt);
extern void camera_move_backward(Camera *cam, float dt);
extern void camera_move_left(Camera *cam, float dt);
extern void camera_move_right(Camera *cam, float dt);
extern void camera_look_at(Camera *cam);

#endif