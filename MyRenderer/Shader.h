#pragma once

#include <unordered_map>
#include "Texture.h"
#include "Sampler.h"
#include "PipelineState.h"

// some constants to use as key in the shader context
constexpr int SV_Position = 0;
constexpr int SV_uv = 1;
constexpr int SV_ddxUV = -3;
constexpr int SV_ddyUV = -4;
constexpr int SV_screenX = -1;
constexpr int SV_screenY = -2;

struct ShaderContext
{
    std::unordered_map<int, float> f;
    std::unordered_map<int, Vec2f> v2f;
    std::unordered_map<int, Vec3f> v3f;
    std::unordered_map<int, Vec4f> v4f;
    std::unordered_map<int, Mat4x4f> m4x4;
    std::unordered_map<int, int> i;
};

struct ShaderUniform : public ShaderContext
{
    std::unordered_map<int, Sampler2D<Vec3f>> sampler2D3F;
    std::vector<Texture2D3F> textures;
};

// class VertexShader
class VertexShader
{
public:
    // DON'T use these functions in the derived class
    // these are for the Pipeline object
    void excute(ShaderContext& input, ShaderContext& output, ShaderUniform& uniform, const PipelineState& pipelineState)
    {
        this->pPipelineState = &pipelineState;
        excute(input, output, uniform);
    }
protected:
    // override this function to imply your own vertex shader
    virtual void excute(ShaderContext& input, ShaderContext& output, ShaderUniform& uniform) = 0;

protected:
    // don't use them in excute() directly
    const PipelineState* pPipelineState;

};

// class PixelShader
class PixelShader
{
public:
    // DON'T use these functions in the derived class
    // these are for the Pipeline object
    Vec4f excute(const ShaderContext& input, const ShaderUniform& uniform, const PipelineState& pipelineState)
    {
        this->pInput = &input;
        this->pUniform = &uniform;
        this->pPipelineState = &pipelineState;
        return excute(input, uniform);
    }

protected:
    // override this function to imply your own shader
    virtual Vec4f excute(
        const ShaderContext& input,
        const ShaderUniform& uniform
    ) = 0;

protected:
    // these are some built-in functions, USE them in the override function
    Vec3f sample(const Sampler2D<Vec3f>& sampler, const Texture2D3F& tex, Vec2f uv)
    {
        sampler.sample(tex, uv, pUniform->v2f.at(SV_ddxUV), pUniform->v2f.at(SV_ddyUV), *pPipelineState);
    }

protected:
    // don't use them in excute() directly
    const PipelineState* pPipelineState;
    const ShaderContext* pInput;
    const ShaderUniform* pUniform;
};
