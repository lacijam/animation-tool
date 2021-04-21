#ifndef MATHS_H
#define MATHS_H

#include <math.h>

#define M_PI 3.14159265359

struct V4 {
	union {
		float E[4];
		struct {
    		float x, y, z, w;
		};
	};
};

struct V3 {
	union {
		float E[3];
		struct {
    		float x, y, z;
		};
	};
};

struct V2 {
	union {
		float E[2];
		struct {
			float x,y;
		};
	};
};

// Vector functions.
extern V3& operator+=(V3 &v, V3 w);
extern V3 &operator+=(V3 &v, float w);
extern V3& operator-=(V3 &v, V3 w);
extern bool operator==(V3 v, V3 w);
extern V3 operator+(const V3 &v, const V3 &w);
extern V3 operator-(const V3 &v, const V3 &w);
extern V3 operator*(const V3 &v, const float s);
extern V3 operator*(const float s, const V3 &v);
extern V2 operator+(const V2 &v, const V2 &w);
extern V2 operator-(const V2 &v, const V2 &w);
extern V2 operator+(const V2 &v, const float &a);
extern V2 operator-(const V2 &v, const float &a);
extern V2 operator*(const V2 &v, const float& a);
extern V2 operator*(const float& a, const V2 &v);

extern float radians(const float degrees);

extern V3 v3_normalise(V3 v);
extern float v3_dot(V3 a, V3 b);
extern float v2_dot(V2 a, V2 b);
extern V3 v3_cross(V3 a, V3 b);
extern V3 v4_to_v3(V4 v);

// Matrix functions.
extern void mat4_copy(float* dest, float* src);
extern void mat4_multiply(float* result, const float* lhs, const float* rhs);
extern void mat4_translate(float* matrix, const float tx, const float ty, const float tz);
extern void mat4_remove_translation(float* matrix);
extern void mat4_scale(float* matrix, const float sx, const float sy, const float sz);
extern void mat4_rotate_x(float* matrix, const float degs);
extern void mat4_rotate_y(float* matrix, const float degs);
extern void mat4_rotate_z(float* matrix, const float degs);
extern void mat4_identity(float* matrix);
extern void mat4_ortho(float* matrix, float left, float right, float bottom, float top, float near, float far);
extern void mat4_frustrum(float* matrix, float left, float right, float bottom, float top, float near, float far);
extern void mat4_look_at(float* matrix, V3 eye, V3 centre, V3 up);
#endif