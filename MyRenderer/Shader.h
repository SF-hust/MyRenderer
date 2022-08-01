#pragma once

#include <unordered_map>
#include "Texture.h"
#include "Sampler.h"
#include "PipelineState.h"

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

struct ShaderUniform : public ShaderContext
{
    std::unordered_map<int, Sampler2D<Vec3f>> sampler2D3F;
};

// class VertexShader
class VertexShader
{
public:
    // override this function to imply your own vertex shader
    virtual void excute(ShaderContext& input, ShaderContext& output, ShaderUniform& uniform) = 0;

};

// class PixelShader
class PixelShader
{
public: //DON'T use these functions in the derived class
    void setDDUV(float ddu, float ddv) { this->ddu = ddu; this->ddv = ddv; }

    void setPipelineState(PipelineState* p) { pPipelineState = p; }

public:
    // override this function to imply your own shader
    virtual Vec4f excute(
        ShaderContext& input,
        ShaderUniform& uniform,
        const std::vector<Texture2D3F> textures
    ) = 0;

protected:
    // these are some built-in functions, USE them in the override function
    template<class T>
    Vec4f sample2D(const Texture2D<T>& texture, const Vec2f& uv, Sampler2D<T>& sampler)
    { 
        // TODO
    }

protected:
    // don't use them in excute directly
    float ddu = 0.0f, ddv = 0.0f;
    PipelineState* pPipelineState;
};
