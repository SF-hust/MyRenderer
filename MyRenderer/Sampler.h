#pragma once

#include "Texture.h"
#include "PipelineState.h"

#undef min
#undef max

enum MipMapMode
{
    MIPMAP_MODE_NO_MIPMAP,
    MIPMAP_MODE_NEAREST,
    MIPMAP_MODE_LINEAR,
};

enum AddressMode
{
    ADDRESS_MODE_REPEAT,
    ADDRESS_MODE_MIRRORED_REPEAT,
    ADDRESS_MODE_CLAMP_TO_EDGE,
    ADDRESS_MODE_CLAMP_TO_BORDER,
};

enum FilterMode
{
    FILTER_MODE_POINT,
    FILTER_MODE_LINEAR,
    FILTER_MODE_ANISOTROPIC
};


template<class T>
class Sampler2D
{
public:

    T sample(const Texture2D<T>& tex, Vec2f uv, Vec2f ddxUV, Vec2f ddyUV, const PipelineState& pipelineState) const;

    void setAddressMode(AddressMode a) { addressMode = a; }

    void setMipMapMode(MipMapMode m) { mipmapMode = m; }

    void setFilterMode(FilterMode f, int a) { filterMode = f; this->anisotropicLevel = a; }

    void setBorderColor(Vec4f b) { borderColor = b; }

protected:
    T sampleFromMipmapLevel(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const;

    T sampleAnisotropic(const Texture2D<T>& tex, Vec2f rawUV, Vec2f ddxUV, Vec2f ddyUV) const;

    T samplePoint(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const;

    T sampleLinear(const Texture2D<T>& tex, Vec2f uv, int mipmapLevel) const;
    
    Vec2f getSampleUV(Vec2f rawUV) const;

protected:
    AddressMode addressMode = ADDRESS_MODE_CLAMP_TO_BORDER;
    MipMapMode mipmapMode = MIPMAP_MODE_NO_MIPMAP;
    FilterMode filterMode = FILTER_MODE_POINT;
    T borderColor = {};
    int anisotropicLevel = 4;
};