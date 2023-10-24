#ifndef TRANSFORMS
#define TRANSFORMS
#include <iostream>
#include "geometry.h"

float deg2rad(const float &deg);
float rad2deg(const float &rad);

mat4 get_scale(const vec3 &scale);

mat4 get_translation(const vec3 &translation);

mat4 get_rotation(vec3 axis, float angle);

mat4 get_lookAt(vec3 eye, vec3 center, vec3 up);

mat4 get_perspective(const float &aspect_ratio, const float &fov, const float &n, const float &f);

mat4 get_viewport(const int &width, const int &height, const int &depth);

mat4 get_ortho_projection(float top, float bot, float left, float right, float near, float far);
#endif