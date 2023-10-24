#include "transforms.h"

float deg2rad(const float &deg)
{
    return deg / 180.0f * M_PI;
}

float rad2deg(const float &rad)
{
    return rad * 180.0f / M_PI;
}

mat4 get_scale(const vec3 &scale)
{
    mat4 ret = {
        {{scale.x, 0, 0, 0},
         {0, scale.y, 0, 0},
         {0, 0, scale.z, 0},
         {0, 0, 0, 1}}};
    return ret;
}

mat4 get_translation(const vec3 &translation)
{
    // 用三个大括号，最外面一个表示用列表初始化？
    mat4 ret = {{{1, 0, 0, translation.x},
                 {0, 1, 0, translation.y},
                 {0, 0, 1, translation.z},
                 {0, 0, 0, 1}}};
    return ret;
}

// mat4 get_rotation(vec3 axis, float angle)
// {
//     mat4 Rk = getCrossMat4fromVec3(axis.normalized());
//     // printf("s\n");
//     auto rad = deg2rad(angle);
//     // printf("s\n");
//     mat4 M = mat4::identity() + Rk * Rk * (1 - cos(rad)) + Rk * sin(rad);
//     // printf("s\n");
//     return M;
// }
mat4 get_rotation(vec3 axis, float angle)
{
    // 不能像上面那样算，会导致1项（即齐次坐标的w项）被加大，造成缩小
    mat3 Rk = getCrossMat(axis.normalized());
    auto rad = deg2rad(angle);
    mat3 M = mat3::identity() + Rk * Rk * (1 - cos(rad)) + Rk * sin(rad);
    mat4 ret = {
        {{M[0][0], M[0][1], M[0][2], 0},
         {M[1][0], M[1][1], M[1][2], 0},
         {M[2][0], M[2][1], M[2][2], 0},
         {0, 0, 0, 1}}};
    return ret;
}

mat4 get_lookAt(vec3 eye, vec3 center, vec3 up)
{
    // 那么eye看向的始终是z轴负方向
    vec3 z = (eye - center).normalized();
    vec3 x = cross(up, z).normalized();
    vec3 y = cross(z, x).normalized();
    // 先将相机移动到原点处，此时物体的坐标就是[x y z]T - [O'x O'y O'z]T项
    // 实际上把向量减法翻译过来就是乘以一个tr矩阵
    mat4 tr = mat4::identity();
    // [x' y' z'] = [x y z]M
    // M_inv = M^(-1)
    // M_inv = [
    //     x' projected on xyz, 0
    //     y' projected on xyz, 0
    //     z' projected on xyz, 0
    //     0, 0, 0, 1
    // ]
    mat4 M_inv = mat4::identity();
    for (int i = 0; i < 3; i++)
    {
        M_inv[0][i] = x[i];
        M_inv[1][i] = y[i];
        M_inv[2][i] = z[i];
        tr[i][3] = -eye[i];
    }
    return M_inv * tr;
}

mat4 get_perspective(const float &aspect_ratio, const float &fov, const float &n, const float &f)
{
    // 模型z在-1~1范围内，且z轴负方向是远距离方向; eye在0 0 0处，且看向z轴负方向
    // 但是传入的n f都是正数
    // 这个透的没问题，3.7对于0.2~80的空间来说，就是会被透到很后面去
    float near = -n, far = -f;
    mat4 perspective_mat = {
        {{near, 0, 0, 0},
         {0, near, 0, 0},
         {0, 0, near + far, -near * far},
         {0, 0, 1, 0}}};
    // mat4 perspective_mat = mat4::identity();
    // std::cout << "perspective\n"
    //           << perspective_mat << std::endl;

    // 右手系中，在压缩到ndf前n是大于f的
    // int abs(int)可能会造成截断问题导致top为0！！
    float top = fabs(n) * tan(deg2rad(fov / 2.0)), bot = -top;
    // std::cout << "top: " << top << std::endl;
    float left = aspect_ratio * top, right = -left;
    // cao orthomat也没有像你这样写在一起的
    // mat4 ortho_mat = {
    //     {{2 / (left - right), 0, 0, -(left + right) / 2},
    //      {0, 2 / (top - bot), 0, -(top + bot) / 2},
    //      {0, 0, 2 / (near - far), -(near + far) / 2},
    //      {0, 0, 0, 1}}};
    mat4 trans = {
        {{1, 0, 0, -(left + right) / 2},
         {0, 1, 0, -(top + bot) / 2},
         {0, 0, 1, -(near + far) / 2},
         {0, 0, 0, 1}}};

    mat4 scale = {
        {{2 / (left - right), 0, 0, 0},
         {0, 2 / (top - bot), 0, 0},
         {0, 0, 2 / (near - far), 0},
         {0, 0, 0, 1}}};

    mat4 ortho_mat = scale * trans;
    // mat4 ortho_mat = mat4::identity();

    // 压缩到ndc后，1是near，-1是far
    // 大多数点被压缩到-1附近，但是上面的原则仍然是成立的
    // std::cout << "ortho\n"
    //           << ortho_mat << std::endl;
    return ortho_mat * perspective_mat;
}

mat4 get_viewport(const int &width, const int &height, const int &depth)
{
    // 不能写在一起，写在一起的话就是先scale再translate
    // 现在是z值越大越近，需要反转过来
    mat4 scale = get_scale({width / 2.0, height / 2.0, -depth / 2.0});
    mat4 trans = get_translation({1.0, 1.0, -1.0});
    return scale * trans;
}

mat4 get_ortho_projection(float top, float bot, float left, float right, float near, float far)
{
    bot = -bot;
    right = -right;
    near = -near;
    far = -far;
    mat4 trans = {
        {{1, 0, 0, -(left + right) / 2},
         {0, 1, 0, -(top + bot) / 2},
         {0, 0, 1, -(near + far) / 2},
         {0, 0, 0, 1}}};
    mat4 scale = {
        {{2 / (left - right), 0, 0, 0},
         {0, 2 / (top - bot), 0, 0},
         {0, 0, 2 / (near - far), 0},
         {0, 0, 0, 1}}};
    return scale * trans;
}