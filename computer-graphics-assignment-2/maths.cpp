#include "maths.h"

V3& operator+=(V3 &v, V3 w)
{
    v.x += w.x;
    v.y += w.y;
    v.z += w.z;
    return v;
}

V3 &operator+=(V3 &v, float w)
{
	v.x += w;
	v.y += w;
	v.z += w;
	return v;
}

V3& operator-=(V3 &v, V3 w)
{
    v.x -= w.x;
    v.y -= w.y;
    v.z -= w.z;
    return v;
}

bool operator==(V3 v, V3 w)
{
	return v.x == w.x
		&& v.y == w.y
		&& v.z == w.z;
}

V3 operator+(const V3 &v, const V3 &w)
{
	return {
		v.x + w.x,
		v.y + w.y,
		v.z + w.z
	};
}

V3 operator-(const V3 &v, const V3 &w)
{
	return {
		v.x - w.x,
		v.y - w.y,
		v.z - w.z
	};
}

V3 operator*(const V3 &v, const float s)
{
    return {
        v.x * s,
        v.y * s,
        v.z * s
    };
}

V3 operator*(const float s, const V3 &v)
{
    return v * s;
}

V2 operator+(const V2 &v, const V2 &w)
{
	return {
		v.x + w.x,
		v.y + w.y
	};
}

V2 operator-(const V2 &v, const V2 &w)
{
	return {
		v.x - w.x,
		v.y - w.y
	};
}

V2 operator+(const V2 &v, const float &a)
{
	return {
		v.x + a,
		v.y + a
	};
}

V2 operator-(const V2 &v, const float &a)
{
	return {
		v.x - a,
		v.y - a
	};
}

V2 operator*(const V2 &v, const float& a)
{
	return {
		v.x * a,
		v.y * a
	};
}

V2 operator*(const float& a, const V2 &v)
{
	return v * a;
}

float radians(const float degrees)
{
	return degrees * (float)(M_PI / 180.0);
}

V3 v3_normalise(V3 v)
{
	float magnitude = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude,
	};
}

float v3_dot(V3 a, V3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float v2_dot(V2 a, V2 b)
{
	return a.x * b.x + a.y * b.y;
}

V3 v3_cross(V3 a, V3 b)
{
	return {
		(a.y * b.z) - (a.z * b.y),
		(a.z * b.x) - (a.x * b.z),
		(a.x * b.y) - (a.y * b.x)
	};
}

V3 v4_to_v3(V4 v)
{
	return {
		v.x, v.y, v.z
	};
}

void mat4_copy(float* dest, float* src)
{
	for (unsigned int i = 0; i < 16; i++) {
		dest[i] = src[i];
	}
}

void mat4_multiply(float* result, const float* lhs, const float* rhs)
{
	for (unsigned int i = 0; i < 4; ++i) {
		for (unsigned int j = 0; j < 4; ++j) {
			float n = 0.f;
			
			for (unsigned int k = 0; k < 4; ++k) {
				n += lhs[i + k * 4] * rhs[k + j * 4];
			}

			result[i + j * 4] = n;
		}
	}
}

void mat4_translate(float* matrix, const float tx, const float ty, const float tz)
{
	matrix[12] += (matrix[0] * tx) + (matrix[4] * ty) + (matrix[8]  * tz);
	matrix[13] += (matrix[1] * tx) + (matrix[5] * ty) + (matrix[9]  * tz);
	matrix[14] += (matrix[2] * tx) + (matrix[6] * ty) + (matrix[10] * tz);
}

void mat4_remove_translation(float* matrix)
{
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
}

void mat4_scale(float* matrix, const float sx, const float sy, const float sz)
{
	for (unsigned int i = 0; i < 4; ++i) {
		matrix[i]     *= sx;
		matrix[i + 4] *= sy;
		matrix[i + 8] *= sz;
	}
}

void mat4_rotate_x(float* matrix, const float degs)
{
	const float rads = radians(degs);
	const float sin_t = sinf(rads);
	const float cos_t = cosf(rads);

	for (unsigned int i = 0; i < 4; ++i) {
		const float a = matrix[i + 4];
		const float b = matrix[i + 8];
		matrix[i + 4] = a * cos_t + b * sin_t;
		matrix[i + 8] = b * cos_t - a * sin_t;
	}
}

void mat4_rotate_y(float* matrix, const float degs)
{
	const float rads = radians(degs);
	const float sin_t = sinf(rads);
	const float cos_t = cosf(rads);

	for (unsigned int i = 0; i < 4; ++i) {
		const float a = matrix[i];
		const float b = matrix[i + 8];
		matrix[i]     = a * cos_t - b * sin_t;
		matrix[i + 8] = a * sin_t + b * cos_t;
	}
}

void mat4_rotate_z(float* matrix, const float degs)
{
	const float rads = radians(degs);
	const float sin_t = sinf(rads);
	const float cos_t = cosf(rads);

	for (unsigned int i = 0; i < 4; ++i) {
		const float a = matrix[i];
		const float b = matrix[i + 4];
		matrix[i]     = a * cos_t + b * sin_t;
		matrix[i + 4] = b * cos_t - a * sin_t;
	}
}

void mat4_identity(float* matrix)
{
	for (unsigned char i = 0; i < 16; i++) {
		matrix[i] = 0;
	}

	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
}

void mat4_ortho(float* matrix, float left, float right, float bottom, float top, float near, float far)
{
	mat4_identity(matrix);
	matrix[0] = 2.f / (right - left);
	matrix[5] = 2.f / (top - bottom);
	matrix[10] = -1.f * 2.f / (far - near);
	matrix[12] = -1.f * (right + left) / (right - left);
	matrix[13] = -1.f * (top + bottom) / (top - bottom);
	matrix[14] = -1.f * (far + near) / (far - near);
}

void mat4_frustrum(float* matrix, float left, float right, float bottom, float top, float near, float far)
{
	mat4_identity(matrix);
	matrix[0] = (2 * near) / (right - left);
	matrix[5] = (2 * near) / (top - bottom);
	matrix[8] = (right + left) / (right - left);
	matrix[9] = (top + bottom) / (top - bottom);
	matrix[10] = - 1 * (far + near) / (far - near);
	matrix[11] = - 1;
	matrix[14] = - (2 * far * near) / (far - near);
}

void mat4_look_at(float* matrix, V3 eye, V3 centre, V3 up)
{
	V3 F, T, S, U;
	
	F = v3_normalise(centre - eye);
	T = v3_normalise(up);
	S = v3_normalise(v3_cross(F, T));
	U = v3_normalise(v3_cross(S, F));

	mat4_identity(matrix);
	matrix[0] = S.x;
	matrix[1] = U.x;
	matrix[2] = -F.x;
	matrix[4] = S.y;
	matrix[5] = U.y;
	matrix[6] = -F.y;
	matrix[8] = S.z;
	matrix[9] = U.z;
	matrix[10] = -F.z;
	matrix[12] = -v3_dot(S, eye);
	matrix[13] = -v3_dot(U, eye);
	matrix[14] = v3_dot(F, eye);
}