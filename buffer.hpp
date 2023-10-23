#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

template <typename T>
class Buffer
{
    // std::unique_ptr<T> data;
    std::vector<T> data;
    int width;
    int height;

public:
    Buffer() = default;
    Buffer(const int &_width, const int &_height, const T &value)
        : width(_width), height(_height), data(_width * _height, value) {}

    int getIndex(int x, int y)
    {
        return x + y * width;
    }

    T &getElem(int x, int y)
    {
        return data[getIndex(x, y)];
    }

    void setElem(int x, int y, const T &value)
    {
        data[getIndex(x, y)] = value;
    }
};