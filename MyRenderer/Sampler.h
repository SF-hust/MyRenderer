#pragma once

#include "Texture.h"
#include "PipelineState.h"

enum MipMapMode
{
    NO_MIPMAP,
    NEAREST,
    LEARNER,
};

enum AddressMode
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
};

enum FilterMode
{
    POINT,
    LINEAR,
    ANISOTROPIC
};


template<class T>
class Sampler2D
{
public:

    T sample(const Texture2D<T>& tex, Vec2f uv, Vec2f ddxUV, Vec2f ddyUV, const PipelineState& pipelineState) const
    {
        T result = {};

        switch (mipmapMode)
        {
        case NO_MIPMAP:
            result = sampleFromUVLevel(tex, uv, 0, 0);
            break;
        case NEAREST:
            /* code */
            break;
        case LEARNER:
            /* code */
            break;
        default:
            break;
        }

        return result;
    }

    void setAddressMode(AddressMode a) { addressMode = a; }

    void setMipMapMode(MipMapMode m) { mipmapMode = m; }

    void setFilterMode(FilterMode f) { filterMode = f; }

    void setBorderColor(Vec4f b) { borderColor = b; }

protected:
    T sampleFromUVLevel(const Texture2D<T>& tex, Vec2f rawUV, int ul, int vl) const
    {
        Vec2f uv;
        T result;
        switch (addressMode)
        {
        case REPEAT:
            uv.u = rawUV.u >= 0.0f ? fmod(rawUV.u, 1.0f) : fmod(rawUV.u, 1.0f) + 1.0f;
            uv.v = rawUV.v >= 0.0f ? fmod(rawUV.v, 1.0f) : fmod(rawUV.v, 1.0f) + 1.0f;
            break;
        case MIRRORED_REPEAT:
            uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.u), 2.0f) - 1.0f);
            uv.u = 1.0f - std::abs(fmod(std::abs(rawUV.v), 2.0f) - 1.0f);
            break;
        case CLAMP_TO_EDGE:
            uv.u = clamp(rawUV.u, 0.0f, 1.0f);
            uv.v = clamp(rawUV.v, 0.0f, 1.0f);
            break;
        case CLAMP_TO_BORDER:
            if (rawUV.u > 1.0f || rawUV.u < 0.0f || rawUV.v > 1.0f || rawUV.v < 0.0f)
            {
                return borderColor;
            }
            uv = rawUV;
            break;
        }

        int stx = 0, edx = tex.width, sty = 0, edy = tex.height;
        if(tex.mipmapLevel != 0)
        {
            stx = tex.rawWidth - (1 << (tex.mipmapLevel - ul + 1));
            edx = tex.rawWidth - (1 << (tex.mipmapLevel - ul));
            sty = tex.rawHeight - (1 << (tex.mipmapLevel - vl + 1));
            edy = tex.rawHeight - (1 << (tex.mipmapLevel - vl));
        }

        Vec2f uvInTex = {float(stx) + uv.u * float(edx - stx), float(sty) + uv.v * float(edy - sty)};
        switch (filterMode)
        {
        case POINT:
            result = tex.rawat(int(uvInTex.x), int(uvInTex.y));
            break;
        case LINEAR:
        {
            int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            float ku, kv;
            float xf = uvInTex.u * float(edx - stx);
            float yf = uvInTex.v * float(edy - sty);

            // calculate 4 pixel coords and 2 factors to blend
            if (uv.u == 1.0f)
            // if address mode is REPEAT, uv.u can't be 1.0f
            {
                ku = 1.0f;
                x0 = x1 = edx - 1;
            }
            else if (fmodf(uvInTex.u, 1.0f) < 0.5f)
            // left half
            {
                ku = 0.5f - fmodf(uvInTex.u, 1.0f);
                x1 = int(uvInTex.u);
                if(addressMode == REPEAT && x1 == stx)
                // when x1 is the first left and address mode is REPEAT, the x0 should be the first right
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
                if(addressMode == REPEAT && x0 == edx - 1)
                // when x0 is the first right and address mode is REPEAT, the x1 should be the first left
                {
                    x1 = stx;
                }
                else
                {
                    x1 = std::min(x0 + 1, int(edx - 1));
                }
            }

            if (uv.v == 1.0f)
            // if address mode is REPEAT, uv.v can't be 1.0f
            {
                kv = 1.0f;
                y0 = y1 = edy - 1;
            }
            else if (fmodf(uvInTex.v, 1.0f) < 0.5f)
            // bottom half
            {
                kv = 0.5f - fmodf(uvInTex.v, 1.0f);
                y1 = int(uvInTex.v);
                if(addressMode == REPEAT && y1 == sty)
                // when y1 is the first bottom and address mode is REPEAT, the y0 should be the first top
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
                if(addressMode == REPEAT && y0 == edy - 1)
                // when y0 is the first top and address mode is REPEAT, the y1 should be the first bottom
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
            r += ku * kv * tex.rawat(x0, y0);
            r += (1 - ku) * kv * tex.rawat(x1, y0);
            r += ku * (1 - kv) * tex.rawat(x0, y1);
            r += (1 - ku) * (1 - kv) * tex.rawat(x1, y1);
            result = r;
        }
            break;
        case ANISOTROPIC:
        {
            // TODO:
            
        }
            break;
        }
        return result;

    }


protected:
    AddressMode addressMode = CLAMP_TO_BORDER;
    MipMapMode mipmapMode = NO_MIPMAP;
    FilterMode filterMode = POINT;
    T borderColor = {};
};