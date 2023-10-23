#include "renderer.h"

void Renderer::updateMVP()
{
    MVP = project * lookat * model;
    toScreen = viewport * MVP;
}

void Renderer::render(const Scene &scene)
{
    // for (auto model : scene.models)
    // {
    //     render(model.get(), scene);
    // }
    cur_scene = &scene;
    for (auto mesh : scene.meshes)
    {
        cur_mesh = mesh.get();
        render(mesh, scene);
    }

    // 世界坐标系的axis不应该应用Model变换
    drawAxis();
}

void Renderer::render(Model *model, const Scene &scene)
{
    for (int i = 0; i < model->nfaces(); i++)
    {
        vec3 world_coords[3];
        vec3 screen_coords[3];
        vec2 tex_coords[3];
        vec4 colors[3];
        auto texture = model->diffuse();
        for (int j = 0; j < 3; j++)
        {
            auto v = world_coords[j] = model->vert(i, j);
            auto uv = tex_coords[j] = model->uv(i, j);
            auto n = model->normal(i, j).normalized();
            float intensity = 0.0f;
            for (auto light : scene.lights)
            {
                intensity += std::max(0.0, n * light);
            }
            auto tmp = viewport * MVP * embed<4, 3>(world_coords[j], 1.0);
            tmp = tmp / tmp[3];
            screen_coords[j] = proj<3, 4>(tmp);
            colors[j] = sample2D(texture, uv.x, uv.y) * intensity;
        }
        vec3 n = (world_coords[1] - world_coords[0]) ^ (world_coords[2] - world_coords[0]);
        n = n.normalized();

        bool flag = false;
        for (auto light : scene.lights)
        {
            if (n * light > 0)
            {
                flag = true;
                break;
            }
        }
        if (flag)
        {
            triangle(screen_coords, colorBuffer, colors);
        }
    }
}

void Renderer::render(std::shared_ptr<Mesh> mesh, const Scene &scene)
{
    // vertex shader
    for (auto &v : mesh->vertices)
    {
        // std::cout << "vertex shading:" << std::endl;
        auto scpos = toScreen * embed<4, 3>(v.pos, 1.0);
        scpos = scpos / scpos[3];
        v.screen_coord = proj<3, 4>(scpos);
        // std::cout << scpos << std::endl;
        float intensity = 0.0f;
        for (auto light : scene.lights)
        {
            intensity += std::max(0.0, light * v.norm);
        }
        v.intensity = intensity;
    }

    for (auto t : mesh->triangles)
    {
        // cull
        // 这样cull三角形会导致缺少三角形,不是用光线去cull，而是用视线去cull
        // vec3 sight = (camera.eye - camera.focus).normalized();
        // vec3 face_norm = (t.vertices[1]->pos - t.vertices[0]->pos) ^ (t.vertices[2]->pos - t.vertices[0]->pos);
        // face_norm = face_norm.normalized();
        // if (sight * face_norm < 0)
        //     continue;
        // 用视线去cull仍然会有黑线问题

        // rasterization
        render(t);
    }
}

void Renderer::render(const Triangle &t)
{
    vec2 bbox_min = {width - 1, height - 1};
    vec2 bbox_max = {0, 0};
    vec2 limits = {width - 1, height - 1};
    vec3 pts[3];
    for (int i = 0; i < 3; i++)
    {
        pts[i] = t.vertices[i]->screen_coord;
        bbox_max.x = std::min(limits.x, std::max(bbox_max.x, pts[i].x));
        bbox_max.y = std::min(limits.y, std::max(bbox_max.y, pts[i].y));

        bbox_min.x = std::max(0.0, std::min(bbox_min.x, pts[i].x));
        bbox_min.y = std::max(0.0, std::min(bbox_min.y, pts[i].y));
    }

    for (int x = bbox_min.x; x <= bbox_max.x; x++)
    {
        for (int y = bbox_min.y; y <= bbox_max.y; y++)
        {
            // fragment shader
            vec3 P = {x, y, 0};
            auto bcs = getBarycentric(pts, P);
            // std::cout << other << " " << u << " " << v << std::endl;
            if (bcs[0] < 0 || bcs[1] < 0 || bcs[2] < 0 || bcs[0] > 1 || bcs[1] > 1 || bcs[2] > 1)
            {
                continue;
            }
            // std::cout << pts[0] << std::endl;
            // 因为depth仍然是一个平面三角形的属性，和对空间三角形的三个顶点的颜色进行插值需要考虑空间变换是两码事
            for (int i = 0; i < 3; i++)
            {
                P.z += pts[i].z * bcs[i];
            }
            // std::cout << P.z << std::endl;

            // 对于三个坐标的点都成立的重心坐标，对于它的三个维度中的两个维度肯定是成立的
            // 对于一点P的重心坐标又是唯一的，那么在投影平面内计算出来的重心坐标就是在空间中的重心坐标

            if (P.z < depthBuffer.getElem(x, y))
            {
                depthBuffer.setElem(x, y, P.z);
                vec2 tex_coord = {0, 0};
                vec3 normal_interpolated = {0, 0, 0};
                float intensity = 0.0f;
                for (int i = 0; i < 3; i++)
                {
                    tex_coord = tex_coord + t.vertices[i]->tex_coord * bcs[i];
                    normal_interpolated = normal_interpolated + t.vertices[i]->norm * bcs[i];
                    // intensity = intensity + t.vertices[i]->intensity * bcs[i];
                }

                // 必须要对normal进行插值，不然扰动就是基于面的，会出现棱角分明，而不是基于fragment的normal进行的扰动
                mat3 TBN = {{t.TBN[0],
                             t.TBN[1],
                             normal_interpolated.normalized()}};
                vec3 normal_gt = cur_mesh->normal(tex_coord);
                // std::cout << "normal_gt: " << normal_gt << std::endl;
                vec3 normal_world = TBN.transpose() * normal_gt;
                // std::cout << "world_norm: " << normal_world << std::endl;
                for (auto light : cur_scene->lights)
                {
                    intensity += std::max(0.0, normal_world * light);
                }

                // 不应该是对顶点颜色进行插值，而是应该对坐标进行插值，否则会严重降低纹理精度
                TGAColor color = cur_mesh->texture.sample2D(tex_coord.x, tex_coord.y) * intensity;
                colorBuffer.set({P.x, P.y}, color);
            }
        }
    }
}

vec4 Renderer::sample2D(const TGAImage &texture, const float &u, const float &v)
{
    auto tex_w = texture.width(), tex_h = texture.height();
    return texture.get(tex_w * u, tex_h * v).toVec4();
}

vec3 Renderer::getBarycentric(vec2 p0, vec2 p1, vec2 p2, const vec2 &P)
{
    vec2 PA = p0 - P;
    vec2 AB = p1 - p0;
    vec2 AC = p2 - p0;
    vec3 v1 = {AB.x, AC.x, PA.x}, v2 = {AB.y, AC.y, PA.y};
    auto u = cross(v1, v2);
    return {1.0 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z};
}

vec3 Renderer::getBarycentric(vec2 *pts, const vec2 &P)
{
    vec2 PA = pts[0] - P;
    vec2 AB = pts[1] - pts[0];
    vec2 AC = pts[2] - pts[0];
    vec3 v1 = {AB.x, AC.x, PA.x}, v2 = {AB.y, AC.y, PA.y};
    auto u = cross(v1, v2);
    return {1.0 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z};

    // vec3 u = vec3(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]) ^ vec3(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]);
    // /* `pts` and `P` has integer value as coordinates
    //    so `abs(u[2])` < 1 means `u[2]` is 0, that means
    //    triangle is degenerate, in this case return something with negative coordinates */
    // if (std::abs(u.z) < 1)
    //     return vec3(-1, 1, 1);
    // return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

vec3 Renderer::getBarycentric(vec3 *pts, const vec3 &P)
{
    return getBarycentric({pts[0].x, pts[0].y}, {pts[1].x, pts[1].y}, {pts[2].x, pts[2].y}, {P.x, P.y});
}

void Renderer::triangle(vec2 *pts, TGAImage &image, vec4 *colors)
{
    // 如何判断一个pixel是否在三角形内？
    // 1. 三次叉乘结果是否相同
    // 2. 直接求重心坐标，有一个值小于0就在三角形外
    vec2 bbox_min = {image.width() - 1, image.height() - 1};
    vec2 bbox_max = {0, 0};
    vec2 limits = {image.width() - 1, image.height() - 1};
    for (int i = 0; i < 3; i++)
    {
        bbox_max.x = std::min(limits.x, std::max(bbox_max.x, pts[i].x));
        bbox_max.y = std::min(limits.y, std::max(bbox_max.y, pts[i].y));

        bbox_min.x = std::max(0.0, std::min(bbox_min.x, pts[i].x));
        bbox_min.y = std::max(0.0, std::min(bbox_min.y, pts[i].y));
    }

    // std::cout << "min: " << bbox_min << std::endl;
    // std::cout << "max: " << bbox_max << std::endl;

    for (int x = bbox_min.x; x <= bbox_max.x; x++)
    {
        for (int y = bbox_min.y; y <= bbox_max.y; y++)
        {
            vec2 P = {x, y};
            auto [other, u, v] = getBarycentric(pts, P);
            // std::cout << other << u << v << std::endl;
            if (other < 0 || u < 0 || v < 0 || other > 1 || u > 1 || v > 1)
            {
                continue;
            }
            vec4 c = colors[0] * other + colors[1] * u + colors[2] * v;
            TGAColor color = {c[0], c[1], c[2], c[3]};
            image.set(P, color);
        }
    }
}

void Renderer::triangle(vec2 *pts, TGAImage &image, const TGAColor &color)
{
    vec4 colors[3];
    for (int i = 0; i < 3; i++)
    {
        colors[i] = color.toVec4();
    }
    triangle(pts, image, colors);
}

void Renderer::triangle(vec3 *pts, TGAImage &image, vec4 *colors)
{
    // 如何判断一个pixel是否在三角形内？
    // 1. 三次叉乘结果是否相同
    // 2. 直接求重心坐标，有一个值小于0就在三角形外
    vec2 bbox_min = {image.width() - 1, image.height() - 1};
    vec2 bbox_max = {0, 0};
    vec2 limits = {image.width() - 1, image.height() - 1};
    for (int i = 0; i < 3; i++)
    {
        bbox_max.x = std::min(limits.x, std::max(bbox_max.x, pts[i].x));
        bbox_max.y = std::min(limits.y, std::max(bbox_max.y, pts[i].y));

        bbox_min.x = std::max(0.0, std::min(bbox_min.x, pts[i].x));
        bbox_min.y = std::max(0.0, std::min(bbox_min.y, pts[i].y));
    }

    // std::cout << "min: " << bbox_min << std::endl;
    // std::cout << "max: " << bbox_max << std::endl;

    for (int x = bbox_min.x; x <= bbox_max.x; x++)
    {
        for (int y = bbox_min.y; y <= bbox_max.y; y++)
        {
            vec3 P = {x, y, 0};
            auto [other, u, v] = getBarycentric(pts, P);
            // std::cout << other << " " << u << " " << v << std::endl;
            if (other < 0 || u < 0 || v < 0 || other > 1 || u > 1 || v > 1)
            {
                continue;
            }
            // 因为depth仍然是一个平面三角形的属性，和对空间三角形的三个顶点的颜色进行插值需要考虑空间变换是两码事
            P.z = pts[0].z * other + pts[1].z * u + pts[2].z * v;
            // std::cout << P.z << std::endl;

            // 对于三个坐标的点都成立的重心坐标，对于它的三个维度中的两个维度肯定是成立的
            // 对于一点P的重心坐标又是唯一的，那么在投影平面内计算出来的重心坐标就是在空间中的重心坐标

            if (P.z < depthBuffer.getElem(x, y))
            {
                depthBuffer.setElem(x, y, P.z);
                vec4 c = colors[0] * other + colors[1] * u + colors[2] * v;
                TGAColor color = {c[0], c[1], c[2], 255};
                image.set({P.x, P.y}, color);
            }
        }
    }
}

void Renderer::triangle(vec3 *pts, TGAImage &image, const TGAColor &color)
{
    vec4 colors[3];
    for (int i = 0; i < 3; i++)
    {
        colors[i] = color.toVec4();
    }
    triangle(pts, image, colors);
}

// 由于只考虑斜率0~1的情况，所以不需要考虑垂直的情况
void line(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor &color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    // float derror = std::abs(dy / float(dx));
    // // error代表以上一个像素中心为原点，下一个x坐标处，y值的绝对值大小，大于0.5则选择偏远的像素点
    // float error = 0.0f;

    // 同乘以2dx, 取消浮点数
    int derror = std::abs(dy) * 2;
    float error = 0;
    int ycur = y0;
    for (int xcur = x0; xcur <= x1; xcur++)
    {
        if (steep)
        {
            image.set(ycur, xcur, color);
        }
        else
        {
            image.set(xcur, ycur, color);
        }

        error += derror;
        // if (error > 0.5f)
        // {
        //     ycur += dy > 0 ? 1 : -1;
        //     error -= 1.0f;
        // }
        if (error > dx)
        {
            ycur += dy > 0 ? 1 : -1;
            error -= 2 * dx;
        }
    }
    //     // 朴素的插值方法
    //     for (int xcur = x0; xcur <= x1; xcur++)
    //     {
    //         float t = (xcur - x0) / float(x1 - x0);
    //         // 插值出ycur
    //         float ycur = (1.0f - t) * y0 + t * y1;
    //         if (steep)
    //         {
    //             image.set(ycur, xcur, color);
    //         }
    //         else
    //         {
    //             image.set(xcur, ycur, color);
    //         }
    //     }
}

void line(vec2 t0, vec2 t1, TGAImage &image, const TGAColor &color)
{
    line(t0.x, t0.y, t1.x, t1.y, image, color);
}

void triangle_line(vec2 t0, vec2 t1, vec2 t2, TGAImage &image, const TGAColor &color)
{
    line(t0, t1, image, color);
    line(t1, t2, image, color);
    line(t2, t0, image, color);
}

void triangle_oldSchool(vec2 t0, vec2 t1, vec2 t2, TGAImage &image, const TGAColor &color)
{
    // miny t0, maxy t2
    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);

    int total_height = t2.y - t0.y;
    int segment_height = t1.y - t0.y;
    // bottum up
    for (int y = t0.y; y <= t1.y; y++)
    {
        float alpha = (y - t0.y) / float(total_height);
        float beta = (y - t0.y) / float(segment_height + 1);
        auto A = t0 + (t2 - t0) * alpha;
        // 这个插值并不需要很精确
        auto B = t0 + (t1 - t0) * beta;
        image.set(A, red);
        image.set(B, green);
        int x_start = std::min(A.x, B.x);
        int x_end = std::max(A.x, B.x);
        for (int x = x_start; x <= x_end; x++)
        {
            image.set(x, y, color);
        }
    }

    // top down
    segment_height = t2.y - t1.y;
    for (int y = t2.y; y >= t1.y; y--)
    {
        float alpha = (t2.y - y) / float(total_height);
        float beta = (t2.y - y) / float(segment_height + 1);
        auto A = t2 + (t0 - t2) * alpha;
        auto B = t2 + (t1 - t2) * beta;
        image.set(A, red);
        image.set(B, green);
        int x_start = std::min(A.x, B.x);
        int x_end = std::max(A.x, B.x);
        for (int x = x_start; x <= x_end; x++)
        {
            image.set(x, y, color);
        }
    }
}

void Renderer::drawAxis()
{
    vec4 points[4] = {
        {0, 0, 0, 1},
        {1, 0, 0, 1},
        {0, 1, 0, 1},
        {0, 0, 1, 1}};
    for (int i = 0; i < 4; i++)
    {
        // points[i] = viewport * project * lookat * model * points[i];
        points[i] = viewport * project * lookat * points[i];
        points[i] = points[i] / points[i][3];
    }
    line(proj<2, 4>(points[0]), proj<2, 4>(points[1]), colorBuffer, red);
    line(proj<2, 4>(points[0]), proj<2, 4>(points[2]), colorBuffer, green);
    line(proj<2, 4>(points[0]), proj<2, 4>(points[3]), colorBuffer, blue);
}