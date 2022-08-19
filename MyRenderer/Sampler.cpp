#include "Sampler.h"

template <class T>
T Sampler2D<T>::sample(const Texture2D<T> &tex, Vec2f uv, Vec2f ddxUV, Vec2f ddyUV, const PipelineState &pipelineState) const
{
    // clamp to border
    if(addressMode == ADDRESS_MODE_CLAMP_TO_BORDER && (uv.u > 0.0f || uv.u < 1.0f || uv.v < 0.0f || uv.v > 1.0f))
    {
        return borderColor;
    }
    if(filterMode == FILTER_MODE_ANISOTROPIC)
    {
        return sampleAnistrophic(tex, uv, ddxUV, ddyUV);
    }
    Vec2f sampleUV = getSampleUV(uv);
    T result = {};
    switch (mipmapMode)
    {
    case MIPMAP_MODE_NO_MIPMAP:
        result = sampleFromMipmapLevel(tex, sampleUV, 0, 0);
        break;
    case MIPMAP_MODE_NEAREST:
        float scale = std::max(ddxUV.x * (float)tex.width, ddyUV.y * (float)tex.height);
        int mip = fmodf(scale, 1.0f) > 0.5f ? (int)scale : (int)scale - 1;
        mip = std::min(std::max(mip, 0), tex.maxMipmapLevel);
        result = sampleFromMipmapLevel(tex, uv, mip, mip);
        break;
    case MIPMAP_MODE_LINEAR:
    {
        float scale = std::max(ddxUV.x * (float)tex.width, ddyUV.y * (float)tex.height);
        float factor = fmodf(scale, 1.0f);
        int mip1 = std::min(std::max((int)scale - 1, 0), tex.maxMipmapLevel);
        int mip2 = std::min(std::max((int)scale, 0), tex.maxMipmapLevel);
        result += (1.0f - factor) * sampleFromMipmapLevel(tex, sampleUV, mip1, mip1);
        result += factor * sampleFromMipmapLevel(tex, sampleUV, mip2, mip2);
    }
    break;
    }
    return result;
}

template <class T>
T Sampler2D<T>::sampleFromMipmapLevel(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    switch (filterMode)
    {
    case FILTER_MODE_LINEAR:
        return samplePoint(tex, uv, mipmapLevel);
    case FILTER_MODE_POINT:
    default:
        return sampleLinear(tex, uv, mipmapLevel);
    }

    int stx = 0, edx = tex.width, sty = 0, edy = tex.height;
    if (tex.maxMipmapLevel != 0)
    {
        stx = (int)tex.rawWidth - (1 << ((int)tex.maxMipmapLevel - ul + 1));
        edx = (int)tex.rawWidth - (1 << ((int)tex.maxMipmapLevel - ul));
        sty = (int)tex.rawHeight - (1 << ((int)tex.maxMipmapLevel - vl + 1));
        edy = (int)tex.rawHeight - (1 << ((int)tex.maxMipmapLevel - vl));
    }

    Vec2f uvInTex = {float(stx) + uv.u * float(edx - stx), float(sty) + uv.v * float(edy - sty)};
    switch (filterMode)
    {
    case FILTER_MODE_POINT:
        result = tex.at(int(uvInTex.x), int(uvInTex.y));
        break;
    case FILTER_MODE_LINEAR:
    {
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        float ku, kv;
        float xf = uvInTex.u * float(edx - stx);
        float yf = uvInTex.v * float(edy - sty);

        // calculate 4 pixel coords and 2 factors to blend
        if (uv.u == 1.0f)
        // if address mode is ADDRESS_MODE_REPEAT, uv.u can't be 1.0f
        {
            ku = 1.0f;
            x0 = x1 = edx - 1;
        }
        else if (fmodf(uvInTex.u, 1.0f) < 0.5f)
        // left half
        {
            ku = 0.5f - fmodf(uvInTex.u, 1.0f);
            x1 = int(uvInTex.u);
            if (addressMode == ADDRESS_MODE_REPEAT && x1 == stx)
            // when x1 is the first left and address mode is ADDRESS_MODE_REPEAT, the x0 should be the first right
            {
                x0 = edx - 1;
            }
            else
            {
                x0 = std::max(x1 - 1, stx);
            }
        }
        else
        // right half
        {
            ku = 1.5f - fmodf(uvInTex.u, 1.0f);
            x0 = int(uvInTex.u);
            if (addressMode == ADDRESS_MODE_REPEAT && x0 == edx - 1)
            // when x0 is the first right and address mode is ADDRESS_MODE_REPEAT, the x1 should be the first left
            {
                x1 = stx;
            }
            else
            {
                x1 = std::min(x0 + 1, int(edx - 1));
            }
        }

        if (uv.v == 1.0f)
        // if address mode is ADDRESS_MODE_REPEAT, uv.v can't be 1.0f
        {
            kv = 1.0f;
            y0 = y1 = edy - 1;
        }
        else if (fmodf(uvInTex.v, 1.0f) < 0.5f)
        // bottom half
        {
            kv = 0.5f - fmodf(uvInTex.v, 1.0f);
            y1 = int(uvInTex.v);
            if (addressMode == ADDRESS_MODE_REPEAT && y1 == sty)
            // when y1 is the first bottom and address mode is ADDRESS_MODE_REPEAT, the y0 should be the first top
            {
                y0 = edy - 1;
            }
            else
            {
                y0 = std::max(y1 - 1, sty);
            }
        }
        else
        // top half
        {
            kv = 1.5f - fmodf(uvInTex.v, 1.0f);
            y0 = int(uvInTex.v);
            if (addressMode == ADDRESS_MODE_REPEAT && y0 == edy - 1)
            // when y0 is the first top and address mode is ADDRESS_MODE_REPEAT, the y1 should be the first bottom
            {
                y1 = sty;
            }
            else
            {
                y1 = std::min(y0 + 1, int(edy - 1));
            }
        }
        // blend the colors
        T r = {};
        r += ku * kv * tex.at(x0, y0);
        r += (1 - ku) * kv * tex.at(x1, y0);
        r += ku * (1 - kv) * tex.at(x0, y1);
        r += (1 - ku) * (1 - kv) * tex.at(x1, y1);
        result = r;
    }
    break;
    case FILTER_MODE_ANISOTROPIC:
    {
        // TODO:
    }
    break;
    }
    return result;
}

template <class T>
T Sampler2D<T>::samplePoint(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    int w = (int)tex.width >> mipmapLevel;
    int h = (int)tex.height >> mipmapLevel;
    int x = std::min((int)(uv.u * w), w - 1);
    int y = std::min((int)(uv.v * h), h - 1);
    return tex.getMipmapped(x, y, mipmapLevel);
}

template <class T>
T Sampler2D<T>::sampleLinear(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const
{
    int w = (int)tex.width >> mipmapLevel;
    int h = (int)tex.height >> mipmapLevel;
    float xf = uv.u * (float)w;
    float yf = uv.v * (float)h;
    int x0, x1, y0, y1;
    float ku, kv;
    // calculate 4 pixel coords and 2 factors to blend
    if (uv.u == 1.0f)
    // if address mode is ADDRESS_MODE_REPEAT, uv.u can't be 1.0f
    {
        ku = 1.0f;
        x0 = x1 = w - 1;
    }
    else if (fmodf(xf, 1.0f) < 0.5f)
    // sample point in left half of texel
    {
        ku = 0.5f - fmodf(xf, 1.0f);
        x1 = int(xf);
        if (addressMode == ADDRESS_MODE_REPEAT && x1 == stx)
        // when x1 is the first left and address mode is ADDRESS_MODE_REPEAT, the x0 should be the first right
        {
            x0 = w - 1;
        }
        else
        {
            x0 = std::max(x1 - 1, 0);
        }
    }
    else
    // sample point in right half of texel
    {
        ku = 1.5f - fmodf(uvInTex.u, 1.0f);
        x0 = int(uvInTex.u);
        if (addressMode == ADDRESS_MODE_REPEAT && x0 == edx - 1)
        // when x0 is the first right and address mode is ADDRESS_MODE_REPEAT, the x1 should be the first left
        {
            x1 = 0;
        }
        else
        {
            x1 = std::min(x0 + 1, int(w - 1));
        }
    }

    if (uv.v == 1.0f)
    // if address mode is ADDRESS_MODE_REPEAT, uv.v can't be 1.0f
    {
        kv = 1.0f;
        y0 = y1 = edy - 1;
    }
    else if (fmodf(uvInTex.v, 1.0f) < 0.5f)
    // bottom half
    {
        kv = 0.5f - fmodf(uvInTex.v, 1.0f);
        y1 = int(uvInTex.v);
        if (addressMode == ADDRESS_MODE_REPEAT && y1 == sty)
        // when y1 is the first bottom and address mode is ADDRESS_MODE_REPEAT, the y0 should be the first top
        {
            y0 = edy - 1;
        }
        else
        {
            y0 = std::max(y1 - 1, sty);
        }
    }
    else
    // top half
    {
        kv = 1.5f - fmodf(uvInTex.v, 1.0f);
        y0 = int(uvInTex.v);
        if (addressMode == ADDRESS_MODE_REPEAT && y0 == edy - 1)
        // when y0 is the first top and address mode is ADDRESS_MODE_REPEAT, the y1 should be the first bottom
        {
            y1 = sty;
        }
        else
        {
            y1 = std::min(y0 + 1, int(edy - 1));
        }
    }
}

template <class T>
Vec2f Sampler2D<T>::getSampleUV(Vec2f rawUV) const
{
    Vec2f uv = {};
    switch (addressMode)
    {
    case ADDRESS_MODE_REPEAT:
        uv.u = rawUV.u >= 0.0f ? fmod(rawUV.u, 1.0f) : fmod(rawUV.u, 1.0f) + 1.0f;
        uv.v = rawUV.v >= 0.0f ? fmod(rawUV.v, 1.0f) : fmod(rawUV.v, 1.0f) + 1.0f;
        break;
    case ADDRESS_MODE_MIRRORED_REPEAT:
        uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.u), 2.0f) - 1.0f);
        uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.v), 2.0f) - 1.0f);
        break;
    case ADDRESS_MODE_CLAMP_TO_EDGE:
        uv.u = clamp(rawUV.u, 0.0f, 1.0f);
        uv.v = clamp(rawUV.v, 0.0f, 1.0f);
        break;
    }
    return uv;
}
