#pragma once

// Minimal vector math used by SkillManager.cpp.
typedef float vec_t;
typedef vec_t vec3_t[3];

#ifndef Q_PI
#define Q_PI 3.14159265358979323846f
#endif

inline void Vector(vec_t x, vec_t y, vec_t z, vec3_t out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void AngleMatrix(const vec3_t angles, float matrix[3][4]);
void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
