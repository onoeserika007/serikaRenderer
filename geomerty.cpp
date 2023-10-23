#include "geometry.h"

vec3 cross(const vec3 &v1, const vec3 &v2)
{
    return vec<3>{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

vec3 operator^(const vec3 &v1, const vec3 &v2)
{
    return cross(v1, v2);
}

mat3 getCrossMat(const vec3 &v)
{
    mat3 ret = {
        {{0, -v.z, v.y},
         {v.z, 0, v.x},
         {-v.y, v.x, 0}}};
    return ret;
}

mat4 getCrossMat4fromVec3(const vec3 &v)
{
    mat4 ret = {
        {{0, -v.z, v.y, 0},
         {v.z, 0, -v.x, 0},
         {-v.y, v.x, 0, 0},
         {0, 0, 0, 1}}};
    // 不return就会出现segmentation fault
    return ret;
}