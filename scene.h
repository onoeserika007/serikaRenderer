#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include "model.h"

struct Vertex
{
    vec3 pos;
    vec3 norm;
    vec2 tex_coord;
    vec3 screen_coord;
};

struct Triangle
{
    Vertex *vertices[3];
    mat3 TBN;
    vec3 normal;
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    TGAImage texture;
    TGAImage normalMap;
    TGAImage specularMap;

    Mesh(std::shared_ptr<Model> model)
    {
        vertices.resize(model->nverts());
        for (int i = 0; i < model->nfaces(); i++)
        {
            Triangle t;
            for (int j = 0; j < 3; j++)
            {
                Vertex vertex;
                auto vert_key = model->facet_vrt[i * 3 + j];
                vertex.pos = model->vert(i, j);
                vertex.tex_coord = model->uv(i, j);
                vertex.norm = model->normal(i, j);
                vertices[vert_key] = vertex;
                t.vertices[j] = &vertices[vert_key];
            }
            // face normal
            t.normal = ((t.vertices[1]->pos - t.vertices[0]->pos) ^ (t.vertices[2]->pos - t.vertices[0]->pos)).normalized();
            // 计算TBN矩阵
            // 在三角形所在平面建立局部坐标系TBN，用uv作为参数来表达空间向量（TB是位于空间的基向量）
            mat<2, 3> E = {
                {t.vertices[1]->pos - t.vertices[0]->pos,
                 t.vertices[2]->pos - t.vertices[0]->pos}};
            mat2 delta_uv = {{t.vertices[1]->tex_coord - t.vertices[0]->tex_coord,
                              t.vertices[2]->tex_coord - t.vertices[0]->tex_coord}};
            auto TB = delta_uv.invert() * E;
            t.TBN = {{TB[0].normalized(), TB[1].normalized(), t.normal}};
            triangles.emplace_back(t);
        }
        texture = model->diffuse();
        normalMap = model->normalmap;
        specularMap = model->specularmap;
    }

    vec3 normal(const vec2 &uvf) const
    {
        TGAColor c = normalMap.sample2D(uvf[0], uvf[1]);
        return vec3{(double)c[2], (double)c[1], (double)c[0]} * 2. / 255. - vec3{1, 1, 1};
    }

    float specular(const vec2 &uv) const
    {
        TGAColor c = specularMap.sample2D(uv.x, uv.y);
        return c[0];
    }
};

struct Camera
{
    vec3 eye;
    vec3 focus;
    vec3 up;
};

struct Light
{
    enum class Type
    {
        DirectionalLight,
        PointLight
    };
    Type type;
    std::shared_ptr<TGAImage> shadowmap;
    mat4 MVP_viewport;
    vec3 intensity;
    Light() = default;
    Light(const vec3 _intensity, std::shared_ptr<TGAImage> _shadowmap, Type _type) : intensity(_intensity), shadowmap(_shadowmap), type(_type) {}

    virtual void dummyFunc() {}
};

struct DirectionalLight : public Light
{
    vec3 lightDir;
    DirectionalLight() = default;
    DirectionalLight(const vec3 &_lightDir, const vec3 _intensity = {1, 1, 1}, std::shared_ptr<TGAImage> _shadowmap = nullptr)
        : lightDir(_lightDir.normalized()), Light(_intensity, _shadowmap, Type::DirectionalLight) {}
};

struct PointLight : public Light
{
};

class Scene
{
public:
    std::vector<std::shared_ptr<Model>> models;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<DirectionalLight>> dirlights;

    void addModel(std::shared_ptr<Model> model)
    {
        models.push_back(model);
    }

    void addMesh(std::shared_ptr<Mesh> &mesh)
    {
        meshes.push_back(std::move(mesh));
    }

    void addMesh(std::shared_ptr<Mesh> &&mesh)
    {
        meshes.push_back(std::move(mesh));
    }

    void addLight(std::shared_ptr<Light> light)
    {
        if (light->type == Light::Type::DirectionalLight)
        {
            dirlights.push_back(std::dynamic_pointer_cast<DirectionalLight>(light));
        }
    }
};