#include "Shader.h"

Vec4f BORDER_COLOR = { 0.5f, 0.5f, 0.5f, 0.0f };

Vec4f sampler(const Texture2D3F& texture, Vec2f rawUV, const SampleMode& mode, float ddu, float ddv)
{
    Vec4f result;
    Vec2f uv;
    // calculate the actual uv to sample in the texture
    switch (mode.addressMode)
    {
    case SampleMode::AddressMode::REPEAT:
        uv.u = rawUV.u >= 0.0f ? fmod(rawUV.u, 1.0f) : fmod(rawUV.u, 1.0f) + 1.0f;
        uv.v = rawUV.v >= 0.0f ? fmod(rawUV.v, 1.0f) : fmod(rawUV.v, 1.0f) + 1.0f;
        break;
    case SampleMode::AddressMode::MIRRORED_REPEAT:
        uv.u = rawUV.u >= 0.0f ? fmod(rawUV.u, 1.0f) : fmod(-rawUV.u, 1.0f);
        uv.v = rawUV.v >= 0.0f ? fmod(rawUV.v, 1.0f) : fmod(-rawUV.v, 1.0f);
        break;
    case SampleMode::AddressMode::CLAMP_TO_EDGE:
        uv.u = clamp(rawUV.u, 0.0f, 1.0f);
        uv.v = clamp(rawUV.v, 0.0f, 1.0f);
        break;
    case SampleMode::AddressMode::CLAMP_TO_BORDER:
        if (rawUV.u > 1.0f || rawUV.u < 0.0f || rawUV.v > 1.0f || rawUV.v < 0.0f)
        {
            return BORDER_COLOR;
        }
        break;
    }
    
    // sample from the texture
    switch (mode.filterMode)
    {
    case SampleMode::FilterMode::POINT:
    {
        int x = std::min(int(uv.u) * int(texture.width), int(texture.width - 1));
        int y = std::min(int(uv.v) * int(texture.height), int(texture.height - 1));
        result = texture.at(x, y);
    }
        break;
    case SampleMode::FilterMode::LINEAR:
    {
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        float ku, kv;
        float xf = uv.u * texture.width;
        float yf = uv.v * texture.height;
        // calculate 4 pixel coords and 2 factors to blend
        if (uv.u = 1.0f)
        {
            x0 = x1 = texture.width - 1;
            ku = 1.0f;
        }
        else if (fmodf(xf, 1.0f) < 0.5f)
        {
            x1 = int(xf);
            x0 = std::max(x1 - 1, 0);
            ku = 0.5f - fmodf(xf, 1.0f);
        }
        else
        {
            x0 = int(xf);
            x1 = std::min(x0 + 1, int(texture.width - 1));
            ku = 1.5f - fmodf(xf, 1.0f);
        }
        if (uv.v = 1.0f)
        {
            y0 = y1 = texture.height - 1;
            kv = 1.0f;
        }
        else if (fmodf(yf, 1.0f) < 0.5f)
        {
            y1 = int(yf);
            y0 = std::max(y1 - 1, 0);
            kv = 0.5f - fmodf(yf, 1.0f);
        }
        else
        {
            y0 = int(yf);
            y1 = std::min(y0 + 1, int(texture.height - 1));
            kv = 1.5f - fmodf(yf, 1.0f);
        }
        // blend the colors
        Vec3f r = {0.0f, 0.0f, 0.0f};
        r += ku * kv * texture.at(x0, y0);
        r += (1 - ku) * kv * texture.at(x1, y0);
        r += ku * (1 - kv) * texture.at(x0, y1);
        r += (1 - ku) * (1 - kv) * texture.at(x1, y1);
        result = r;
    }
        break;
    case SampleMode::FilterMode::ANISOTROPIC:
    {
        // TODO:
    }
        break;
    }
    return result;
}

