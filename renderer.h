#pragma once
#include <iostream>
#include "constants.h"
#include "geometry.h"
#include "buffer.hpp"
#include "scene.h"
// #include "transforms.hpp"
#include <string>

enum class AttachmentType
{
    COLOR,
    SHADOWMAP
};

class Renderer
{
    Buffer<float> depthBuffer;
    TGAImage colorBuffer;
    const Scene *cur_scene;
    const Mesh *cur_mesh;
    Camera camera;
    int sample_rate;
    int width;
    int height;

    int shadowmap_resolution = 1024;

    float ambient_intensity = 10;
    float zDepth;

public:
    mat4 toScreen;
    mat4 MVP;
    mat4 model;
    mat4 lookat;
    mat4 project;
    mat4 viewport;

    Renderer(int _width, int _height, int _sample_rate = 1, float _zDepth = 1.0)
        : width(_width),
          height(_height),
          sample_rate(_sample_rate),
          depthBuffer(_width * _sample_rate, _height * _sample_rate, 1.0),
          colorBuffer(_width, _height, TGAImage::RGB),
          zDepth(_zDepth) {}
    vec3 getBarycentric(vec2 p0, vec2 p1, vec2 p2, const vec2 &P);
    vec3 getBarycentric(vec3 *pts, const vec3 &P);
    vec3 getBarycentric(vec2 *pts, const vec2 &P);
    void triangle(vec2 *pts, TGAImage &image, vec4 *colors);
    void triangle(vec3 *pts, TGAImage &image, vec4 *colors);
    void triangle(vec2 *pts, TGAImage &image, const TGAColor &color);
    void triangle(vec3 *pts, TGAImage &image, const TGAColor &color);

    void setMVP(const mat4 &_mvp) { MVP = _mvp; }
    void setViewport(const mat4 &_viewport) { viewport = _viewport; }
    void updateMVP();
    void setCamera(const Camera _camera) { camera = _camera; }
    vec4 sample2D(const TGAImage &texture, const float &u, const float &v);
    void render(const Scene &scene);
    void render(std::shared_ptr<Mesh> mesh, const Scene &scene);

    void render(const Triangle &t, AttachmentType type, TGAImage &renderTarget);
    TGAColor phongShader(const vec3 &fragPos, const vec2 &uv, const vec3 &normal, const TGAColor &color);
    void fragment_shader_color(const vec3 &P, const Triangle &t, const vec3 &bcs, TGAImage &renderTarget);
    void fragment_shader_shadowmap(const vec3 &P, const Triangle &t, const vec3 &bcs, TGAImage &renderTarget);
    void generateShadowMap(const Scene &scene);

    void drawAxis();

    void write_tga_file(const std::string &path) { colorBuffer.write_tga_file(path); }
};

void line(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor &color);
void line(vec2 t0, vec2 t1, TGAImage &image, const TGAColor &color);

void triangle_line(vec2 t0, vec2 t1, vec2 t2, TGAImage &image, const TGAColor &color);
void triangle_oldSchool(vec2 t0, vec2 t1, vec2 t2, TGAImage &image, const TGAColor &color);
