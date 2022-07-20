#pragma once

#include <unordered_map>
#include "Texture.h"

// some constants to use as key in the shader context
constexpr int SV_Position = 0;

extern Vec4f BORDER_COLOR;

struct ShaderContext
{
    std::unordered_map<int, float> f;
    std::unordered_map<int, Vec2f> v2f;
    std::unordered_map<int, Vec3f> v3f;
    std::unordered_map<int, Vec4f> v4f;
    std::unordered_map<int, Mat4x4f> m4x4;
};


//
struct SampleMode
{
    // filter mode, used for sampler
    enum FilterMode
    {
        POINT,
        LINEAR,
        ANISOTROPIC
    };

    // address mode, used for sampler
    enum AddressMode
    {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER,
    };

    FilterMode filterMode;
    AddressMode addressMode;
};

// the sample function
Vec4f sampler(const Texture2D3F& texture, Vec2f uv, const SampleMode& mode, float ddu, float ddv);

// class VertexShader
class VertexShader
{
public:
    // override this function to imply your own vertex shader
    virtual void excute(ShaderContext& input, ShaderContext& output, ShaderContext& uniform) = 0;

};

// class PixelShader
class PixelShader
{
public: //DON'T use these functions in the derived class
    void setDDUV(float ddu, float ddv) { this->ddu = ddu; this->ddv = ddv; }

    void setSampleMode(const SampleMode& mode) { sampleMode = mode; }

public:
    // override this function to imply your own shader
    virtual Vec4f excute(
        ShaderContext& input,
        ShaderContext& uniform,
        const std::vector<Texture2D3F> textures
    ) = 0;

protected:
    // these are some built-in functions, USE them in the override function
    Vec4f sample(const Texture2D3F& texture, const Vec2f& uv)
    { sampler(texture, uv, sampleMode, ddu, ddv); }

protected:
    // don't use them in excute directly
    float ddu = 0.0f, ddv = 0.0f;
    SampleMode sampleMode = {SampleMode::FilterMode::POINT, SampleMode::AddressMode::CLAMP_TO_BORDER};
};
