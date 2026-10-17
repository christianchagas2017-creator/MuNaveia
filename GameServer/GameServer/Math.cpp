#include "stdafx.h"
#include "Math.h"
#include <cmath>

void AngleMatrix(const vec3_t angles, float matrix[3][4])
{
    const float roll  = angles[2] * (Q_PI * 2.0f / 360.0f);
    const float pitch = angles[1] * (Q_PI * 2.0f / 360.0f);
    const float yaw   = angles[0] * (Q_PI * 2.0f / 360.0f);

    const float sy = std::sin(roll),  cy = std::cos(roll);
    const float sp = std::sin(pitch), cp = std::cos(pitch);
    const float sr = std::sin(yaw),   cr = std::cos(yaw);

    matrix[0][0] = cp * cy;
    matrix[1][0] = cp * sy;
    matrix[2][0] = -sp;
    matrix[0][1] = sr * sp * cy - cr * sy;
    matrix[1][1] = sr * sp * sy + cr * cy;
    matrix[2][1] = sr * cp;
    matrix[0][2] = cr * sp * cy + sr * sy;
    matrix[1][2] = cr * sp * sy - sr * cy;
    matrix[2][2] = cr * cp;
    matrix[0][3] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][3] = 0.0f;
}

void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out)
{
    out[0] = in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2];
    out[1] = in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2];
    out[2] = in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2];
}
