#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include "geometry.h"

// #pragma pack(show)
#pragma pack(push, 1)
struct TGAHeader
{
    std::uint8_t idlength = 0;
    std::uint8_t colormaptype = 0;
    std::uint8_t datatypecode = 0;
    std::uint16_t colormaporigin = 0;
    std::uint16_t colormaplength = 0;
    std::uint8_t colormapdepth = 0;
    std::uint16_t x_origin = 0;
    std::uint16_t y_origin = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t bitsperpixel = 0;
    std::uint8_t imagedescriptor = 0;
};
#pragma pack(pop)

struct TGAColor
{
    std::uint8_t bgra[4] = {0, 0, 0, 0};
    std::uint8_t bytespp = 4;
    std::uint8_t &operator[](const int i) { return bgra[i]; }

    vec4 toVec4() const
    {
        vec4 ret;
        for (int i = 0; i < 4; i++)
        {
            ret[i] = bgra[i];
        }
        return ret;
    }

    TGAColor &operator*(const double &x)
    {
        for (int i = 0; i < 4; i++)
        {
            bgra[i] = std::max(0.0, std::min(255.0, bgra[i] * x));
        }
        return *this;
    }
};

struct TGAImage
{
    enum Format
    {
        GRAYSCALE = 1,
        RGB = 3,
        RGBA = 4
    };

    TGAImage() = default;
    TGAImage(const int w, const int h, const int bpp);
    bool read_tga_file(const std::string filename);
    bool write_tga_file(const std::string filename, const bool vflip = true,
                        const bool rle = true) const;
    void flip_horizontally();
    void flip_vertically();
    TGAColor get(const int x, const int y) const;
    void set(const int x, const int y, const TGAColor &c);
    void set(const vec2 &point, const TGAColor &c);
    void clear(const TGAColor &color = {0, 0, 0, 255});
    TGAColor sample2D(const float &u, const float &v) const;
    int width() const;
    int height() const;

private:
    bool load_rle_data(std::ifstream &in);
    bool unload_rle_data(std::ofstream &out) const;

    int w = 0;
    int h = 0;
    std::uint8_t bpp = 0;
    std::vector<std::uint8_t> data = {};
};
