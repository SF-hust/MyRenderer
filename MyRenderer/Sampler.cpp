#include "Sampler.h"

template <class T>
T Sampler2D<T>::sample(const Texture2D<T> &tex, Vec2f uv, Vec2f ddxUV, Vec2f ddyUV, const PipelineState &pipelineState) const
{
    // in address mode, if uv out of border, just return the border color
    if(addressMode == ADDRESS_MODE_CLAMP_TO_BORDER && (uv.u > 0.0f || uv.u < 1.0f || uv.v < 0.0f || uv.v > 1.0f))
    {
        return borderColor;
    }

    // anisotropic filter mode ignores mipmap mode
    if(filterMode == FILTER_MODE_ANISOTROPIC)
    {
        return sampleAnisotropic(tex, uv, ddxUV, ddyUV);
    }

    // get actual sample uv
    Vec2f sampleUV = getSampleUV(uv);

    T result = {};

    switch (mipmapMode)
    {
    case MIPMAP_MODE_NO_MIPMAP:
        // just get from level0 mipmap
        result = sampleFromMipmapLevel(tex, sampleUV, 0);
        break;

    case MIPMAP_MODE_NEAREST:
    {
        // use long edge to calculate mipmap level
        float scale = std::max(ddxUV.x * (float)tex.width, ddyUV.y * (float)tex.height);
        int mip = fmodf(scale, 1.0f) > 0.5f ? (int)scale : (int)scale - 1;
        mip = clamp(mip, 0, tex.maxMipmapLevel);

        result = sampleFromMipmapLevel(tex, sampleUV, mip);
        break;
    }

    case MIPMAP_MODE_LINEAR:
    {
        // use long edge to calculate mipmap level
        float scale = std::max(ddxUV.x * (float)tex.width, ddyUV.y * (float)tex.height);
        int mip1 = clamp((int)scale - 1, 0, tex.maxMipmapLevel);
        int mip2 = clamp((int)scale, 0, tex.maxMipmapLevel);

        // get the blend factor
        float factor = 1.0f - fmodf(scale, 1.0f);

        // sample and blend
        result += factor * sampleFromMipmapLevel(tex, sampleUV, mip1);
        result += (1.0f - factor) * sampleFromMipmapLevel(tex, sampleUV, mip2);
        break;
    }

    }

    return result;
}

template <class T>
T Sampler2D<T>::sampleFromMipmapLevel(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    switch (filterMode)
    {
    case FILTER_MODE_LINEAR:
        return sampleLinear(tex, uv, mipmapLevel);

     // filter mode is point by default
    default:
    case FILTER_MODE_POINT:
        return samplePoint(tex, uv, mipmapLevel);
    }
}

template<class T>
T Sampler2D<T>::sampleAnisotropic(const Texture2D<T>& tex, Vec2f rawUV, Vec2f ddxUV, Vec2f ddyUV) const
{
    // a is the longer edge, b is the shorter edge
    // a and b are in texture space
    Vec2f a = { ddxUV.u * (float)tex.width, ddxUV.v * (float)tex.height };
    Vec2f b = { ddyUV.u * (float)tex.width, ddyUV.v * (float)tex.height };

    // alen and blen are in units of texel
    float alen = Vector_length(a);
    float blen = Vector_length(b);
    if (alen < blen)
    {
        std::swap(a, b);
        std::swap(alen, blen);
    }

    // make b be normal to a
    b -= Vector_dot(a, b) / (alen * blen) * a;
    blen = Vector_length(b);

    // get two mipmap levels and the blend factor
    int mip1 = clamp((int)blen - 1, 0, tex.maxMipmapLevel);
    int mip2 = clamp((int)blen, 0, tex.maxMipmapLevel);
    float mipmapFactor = fmodf(blen, 1.0f);

    // get actual sample points count
    int sampleCount = clamp((int)(alen / blen + 0.5f), 1, anisotropicLevel);

    std::vector<Vec2f> rawSampleUVs(sampleCount);

    // calculate sample points
    if (sampleCount == 1)
    {
        rawSampleUVs[0] = rawUV;
    }
    else
    {
        Vec2f mainDirection = { a.x / (float)tex.width, a.y / (float)tex.height };
        mainDirection -= Vector_normalize(mainDirection) * blen;
        Vec2f sampleStart = rawUV - (mainDirection / 2);
        Vec2f sampleEnd = rawUV + (mainDirection / 2);
        for (int i = 0; i < sampleCount; ++i)
        {
            float f = (float)i / (float)(sampleCount - 1);
            rawSampleUVs[i] = (1.0f - f) * sampleStart + f * sampleEnd;
        }
    }

    // do sample and blend all samples
    T result = {};
    for (auto& rawUV : rawSampleUVs)
    {
        Vec2f uv = getSampleUV(rawUV);
        result += mipmapFactor * sampleLinear(tex, uv, mip1);
        result += (1 - mipmapFactor) * sampleLinear(tex, uv, mip2);
    }
    result /= (float)rawSampleUVs.size();

    return result;
}

template <class T>
T Sampler2D<T>::samplePoint(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    // get width and height of current mipmap level
    int w = (int)tex.width >> mipmapLevel;
    int h = (int)tex.height >> mipmapLevel;
    // get texel coord of current mipmap level
    int x = std::min((int)(uv.u * w), w - 1);
    int y = std::min((int)(uv.v * h), h - 1);

    return tex.getMipmapped(x, y, mipmapLevel);
}

template <class T>
T Sampler2D<T>::sampleLinear(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    // get width and height of current mipmap level
    int w = (int)tex.width >> mipmapLevel;
    int h = (int)tex.height >> mipmapLevel;

    // get sample point in texture space of current mipmap level
    float xf = uv.u * (float)w;
    float yf = uv.v * (float)h;

    // texel coords
    int x0, x1, y0, y1;
    // blend factor
    float ku, kv;
    // the output color
    T result = {};

    // calculate 4 texel coords and 2 factors to blend
    if (uv.u == 1.0f)
    {
        // if address mode is ADDRESS_MODE_REPEAT, uv.u can't be 1.0f
        // so dont call fmod here
        ku = 1.0f;
        x0 = x1 = w - 1;
    }
    else if (fmodf(xf, 1.0f) < 0.5f)
    {
        // sample point in left half of texel
        // when fractional part of xf is in [0.0, 0.5)

        ku = 0.5f - fmodf(xf, 1.0f);
        x1 = int(xf);
        if (addressMode == ADDRESS_MODE_REPEAT && x1 == 0)
        {
            // when x1 is the leftmost and address mode is ADDRESS_MODE_REPEAT
            // the x0 should be the rightmost
            x0 = w - 1;
        }
        else
        {
            // common case, x0 is x1's left
            x0 = std::max(x1 - 1, 0);
        }
    }
    else
    {
        // sample point in right half of texel
        // when fractional part of xf is in [0.5, 1.0)

        ku = 1.5f - fmodf(xf, 1.0f);
        x0 = int(xf);
        if (addressMode == ADDRESS_MODE_REPEAT && x0 == w - 1)
        {
            // when x0 is the rightmost and address mode is ADDRESS_MODE_REPEAT
            // the x1 should be the leftmost
            x1 = 0;
        }
        else
        {
            // common case, x1 is x0's right
            x1 = std::min(x0 + 1, int(w - 1));
        }
    }

    // do the same to vertical
    if (uv.v == 1.0f)
    {
        kv = 1.0f;
        y0 = y1 = h - 1;
    }
    else if (fmodf(yf, 1.0f) < 0.5f)
    {
        kv = 0.5f - fmodf(yf, 1.0f);
        y1 = int(yf);
        if (addressMode == ADDRESS_MODE_REPEAT && y1 == 0)
        {
            y0 = h - 1;
        }
        else
        {
            y0 = std::max(y1 - 1, 0);
        }
    }
    else
    {
        kv = 1.5f - fmodf(yf, 1.0f);
        y0 = int(yf);
        if (addressMode == ADDRESS_MODE_REPEAT && y0 == h - 1)
        {
            y1 = 0;
        }
        else
        {
            y1 = std::min(y0 + 1, int(h - 1));
        }
    }

    // sample and blend the color
    result += (ku + 0) * (kv + 0) * tex.getMipmapped(x0, y0, mipmapLevel);
    result += (1 - ku) * (kv + 0) * tex.getMipmapped(x1, y0, mipmapLevel);
    result += (ku + 0) * (1 - kv) * tex.getMipmapped(x0, y1, mipmapLevel);
    result += (1 - ku) * (1 - kv) * tex.getMipmapped(x1, y1, mipmapLevel);

    return result;
}

template <class T>
Vec2f Sampler2D<T>::getSampleUV(Vec2f rawUV) const
{
    Vec2f uv = {};

    switch (addressMode)
    {
    case ADDRESS_MODE_REPEAT:
        // mapping u, v to [0.0, 1.0)
        uv.u = rawUV.u >= 0.0f ? fmod(rawUV.u, 1.0f) : fmod(rawUV.u, 1.0f) + 1.0f;
        uv.v = rawUV.v >= 0.0f ? fmod(rawUV.v, 1.0f) : fmod(rawUV.v, 1.0f) + 1.0f;
        break;

    case ADDRESS_MODE_MIRRORED_REPEAT:
        // mapping u, v to [0.0, 1.0]
        uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.u), 2.0f) - 1.0f);
        uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.v), 2.0f) - 1.0f);
        break;

    case ADDRESS_MODE_CLAMP_TO_EDGE:
        // clamp u, v to [0.0, 1.0]
        uv.u = clamp(rawUV.u, 0.0f, 1.0f);
        uv.v = clamp(rawUV.v, 0.0f, 1.0f);
        break;
    }

    return uv;
}
