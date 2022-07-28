#pragma once

#include "MathHelper.h"

// the left top of the texture is point (0.0, 0.0)

template <class T>
class Texture2D
{
public:
    Texture2D(size_t w, size_t h) : Texture2D(w, h, false) {}

    Texture2D(size_t w, size_t h, bool mipmap) : width(w), height(h)
    {
        mipmapLevel = 0;
        if (!mipmap || width == 1 || height == 1)
        // gen no mipmap
        {
            rawWidth = width;
            rawHeight = height;
            data = std::vector<T>(rawWidth * rawHeight);
        }
        else if (std::__popcount(width) != 1 || std::__popcount(height) != 1)
        // try to gen mipmap but the width or height is not pow(2, n)
        {
            width = 0;
            height = 0;
            rawWidth = 0;
            rawHeight = 0;
            mipmapLevel = 0;
        }
        else
        // gen mipmaps
        {
            rawWidth = width * 2;
            rawHeight = height * 2;
            data = std::vector<T>(rawWidth * rawHeight);
            mipmapLevel = (size_t)std::log2(std::min(width, height));
            genMipmap();
        }
    }

    ~Texture2D() {}

    // this function only outputtings the level0 texture
    void toBitmap(uint8_t *pDest) const
    {
        for (int row = 0; row < height; ++row)
        {
            memcpy(pDest + row * width * sizeof(T), data.data() + row * rawWidth, width * sizeof(T));
        }
    }

protected:
    void genMipmap()
    {
        // gen top half of the mipmapped texture
        for (int ul = 1; ul <= mipmapLevel; ++ul)
        {
            for (int row = 0; row < int(height); ++row)
            {
                int colst = int(rawWidth) - (1 << (mipmapLevel - ul + 1));
                int coled = int(rawWidth) - (1 << (mipmapLevel - ul));
                int pcol = int(rawWidth) - (1 << (mipmapLevel - ul + 2));
                for (int col = colst; col < coled; ++col, pcol += 2)
                {
                    data[rawIndex(col, row)] = (data[rawIndex(pcol, row)] + data[rawIndex(pcol + 1, row)]) / 2.0f;
                }
            }
        }
        // gen botton half of the mipmapped texture
        for (int vl = 1; vl <= mipmapLevel; ++vl)
        {
            int rowst = int(rawHeight) - (1 << (mipmapLevel - vl + 1));
            int rowed = int(rawHeight) - (1 << (mipmapLevel - vl));
            int prow = int(rawHeight) - (1 << (mipmapLevel - vl + 2));
            for (int row = rowst; row < rowed; ++row, prow += 2)
            {
                for (int col = 0; col < int(rawWidth) - 1; ++col)
                {
                    data[rawIndex(col, row)] = (data[rawIndex(col, prow)] + data[rawIndex(col, prow + 1)]) / 2.0f;
                }
            }
        }
        // gen mipmap end
    }

public:
    // rawat(x, y), directly returns the texture data
    T &rawat(int x, int y) { return data[rawIndex(x, y)]; }
    const T &rawat(int x, int y) const { return data[rawIndex(x, y)]; }

    // at(x, y), just used for level0
    T &at(int x, int y) { return get(x, y, 0); }
    const T &at(int x, int y) const { return get(x, y, 0); }

    // mipmapped get(x, y, l, ul, vl)
    T &get(int x, int y, int l, int ul = 0, int vl = 0)
    {
        int rawx, rawy;
        if (ul == 0 && vl == 0)
        // if is not anistropic
        {
            rawx = x + int(rawWidth) - (1 << (mipmapLevel - l + 1));
            rawy = y + int(rawWidth) - (1 << (mipmapLevel - l + 1));
        }
        else
        // anistropic
        {
            rawx = x + int(rawWidth) - (1 << (mipmapLevel - ul + 1));
            rawy = y + int(rawWidth) - (1 << (mipmapLevel - vl + 1));
        }

        return data[rawIndex(rawx, rawy)];
    }

    const T &get(int x, int y, int l, int ul = 0, int vl = 0) const
    {
        int rawx, rawy;
        if (ul == 0 && vl == 0)
        // if is not anistropic
        {
            rawx = x + int(rawWidth) - (1 << (mipmapLevel - l + 1));
            rawy = y + int(rawWidth) - (1 << (mipmapLevel - l + 1));
        }
        else
        // anistropic
        {
            rawx = x + int(rawWidth) - (1 << (mipmapLevel - ul + 1));
            rawy = y + int(rawWidth) - (1 << (mipmapLevel - vl + 1));
        }

        return data[rawIndex(rawx, rawy)];
    }

    inline size_t rawIndex(int x, int y)
    {
        return x + y * rawWidth;
    }

public:
    std::vector<T> data;
    // w&h of the origin texture
    size_t width;
    size_t height;
    // w&h of data
    size_t rawWidth;
    size_t rawHeight;

    size_t mipmapLevel;
};

using Texture2D3F = Texture2D<Vec3f>;
using Texture2D2F = Texture2D<Vec2f>;
using Texture2D1F = Texture2D<float>;
