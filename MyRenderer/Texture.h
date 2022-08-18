#pragma once

#include "MathHelper.h"

#undef max
#undef min
// the left top of the texture is point (0.0, 0.0)

template <class T>
class Texture2D
{
public:
    // default : 0 X 0 texture
    Texture2D() : Texture2D(0, 0) {}

    // gen from buffer
    Texture2D(size_t w, size_t h, const std::vector<T>& buffer, int maxMipmapLevel = 0)
        : width(w), height(h)
    {
        if (width == 0 || height == 0)
        {
            this->maxMipmapLevel = 0;
            return;
        }
        // get the max mipmap level possible
        int mmlp = 0;
        data = buffer;
        while (((width >> mmlp) & 1) == 0 && ((height >> mmlp) & 1) == 0)
        {
           mmlp++;
        }
        // gen no mipmap
        if (maxMipmapLevel == 0)
        {
            this->maxMipmapLevel = 0;
            return;
        }
        // gen mipmap level as max as possible
        if (maxMipmapLevel < 0)
        {
            this->maxMipmapLevel = mmlp;
        }
        else
        {
            // gen mipmaps
            this->maxMipmapLevel = std::min(mmlp, maxMipmapLevel);
        }
        data.resize(dataSizeOfMipmapLevel(maxMipmapLevel));
        genMipmaps();
    }

    // gen single color
    Texture2D(size_t w, size_t h, T initColor = {}, int maxMipmapLevel = 0)
        : width(w), height(h)
    {
        if (width == 0 || height == 0)
        {
            this->maxMipmapLevel = 0;
            return;
        }
        // get the max mipmap level possible
        int mmlp = 0;
        while (((width >> mmlp) & 1) == 0 && ((height >> mmlp) & 1) == 0)
        {
           mmlp++;
        }
        // gen no mipmap
        if (maxMipmapLevel == 0)
        {
            this->maxMipmapLevel = 0;
        }
        // gen mipmap level as max as possible
        else if (maxMipmapLevel < 0)
        {
            this->maxMipmapLevel = mmlp;
        }
        // gen mipmaps
        else
        {
            this->maxMipmapLevel = std::min(mmlp, maxMipmapLevel);
        }
        data = std::vector<T>(dataSizeOfMipmapLevel(this->maxMipmapLevel), initColor);
    }

    Texture2D(const Texture2D& tex) = default;

    Texture2D(Texture2D&& tex) noexcept
        : width(tex.width), height(tex.height),
        data(std::move(tex.data)), maxMipmapLevel(tex.maxMipmapLevel) {}

    Texture2D& operator= (const Texture2D& tex) = default;

    Texture2D& operator= (Texture2D&& tex) noexcept
    {
        this->width = tex.width;
        this->height = tex.height;
        this->data = std::move(tex.data);
        this->maxMipmapLevel = tex.maxMipmapLevel;
        return *this;
    }

    ~Texture2D() {}

    // this function only outputs the level0 texture
    void toBitmap(uint8_t* pDest) const
    {
        float* fs = (float*)data.data();
        int fcount = sizeof(T) / sizeof(float);
        int pix_count = width * height;
        for (int i = 0; i < pix_count; ++i)
        {
            for (int j = 0; j < fcount; ++j)
            {
                pDest[i * fcount + j] = floatToByte(fs[i * fcount + fcount - j - 1]);
            }
        }
    }

protected:
    void genMipmaps()
    {
        int lastMipStart = 0;
        int mipStart = width * height;
        T result;
        for(int mip = 1; mip <= maxMipmapLevel; ++mip)
        {
            int mipWidth = width >> (mip * 2);
            int mipHeight = height >> (mip * 2);
            for(int y = 0; y < mipHeight; ++y)
            {
                for(int x = 0; x < mipWidth; ++x)
                {
                    result += data[lastMipStart + x * 2 + 0 + (y * 2 + 0) * mipHeight * 2];
                    result += data[lastMipStart + x * 2 + 0 + (y * 2 + 1) * mipHeight * 2];
                    result += data[lastMipStart + x * 2 + 1 + (y * 2 + 0) * mipHeight * 2];
                    result += data[lastMipStart + x * 2 + 1 + (y * 2 + 1) * mipHeight * 2];
                    data[mipStart + x + y * mipHeight] = result / 4.0f;
                }
            }
            lastMipStart = mipStart;
            mipStart += (width * height) >> (mip * 2);
        }
    }

    int dataSizeOfMipmapLevel(int m)
    {
        int size = width * height;
        while (m--)
        {
            size += (width * height) >> 2;
        }
        return size;
    }

public:
    // get from level0 mipmap (x, y)
    T& get(int x, int y)
    {
        return data[x + y * height];
    }

    const T& get(int x, int y) const
    {
        return data[x + y * height];
    }

    // mipmapped get(x, y, m)
    T& getMipmapped(int x, int y, int m)
    {
        return data[indexMipmapped(x, y, m)];
    }

    const T& getMipmapped(int x, int y, int m) const
    {
        return data[indexMipmapped(x, y, m)];
    }

    inline int indexMipmapped(int x, int y, int mipmapLevel)
    {
        int h = height >> (mipmapLevel * 2);
        int st = 0;
        for(int m = 0; m < mipmapLevel; ++m)
        {
            st += (width * height) >> (m * 2);
        }
        return st + x + y * h;
    }

    void clear(T value)
    {
        for(auto& v : data)
        {
            v = value;
        }
    }

public:
    std::vector<T> data;
    // w&h of the origin texture
    size_t width;
    size_t height;
    // max mipmap level
    size_t maxMipmapLevel;
};

using Texture2D3F = Texture2D<Vec3f>;
using Texture2D2F = Texture2D<Vec2f>;
using Texture2D1F = Texture2D<float>;
