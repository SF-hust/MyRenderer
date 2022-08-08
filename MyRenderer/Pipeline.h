#pragma once

#include "Shader.h"
#include "PipelineState.h"


/*
* class Pipeline
* usage :
* 1. set pipeline sim_pipelineState, vertex and index input, shaders, uniforms
* 2. call clear render target
* 3. call renderToTarget to draw objects to the msaa buffer
* 4. repeat 3
* 5. call presentToScreen to merge the msaa buffer to the outer buffer
* 6. repeat 2
*/
class Pipeline
{
public:
    void renderToTarget();

    void presentToScreen(uint8_t* buffer);

    void setPipelineState(const PipelineState& state);

    void clearRenderTarget(Vec3f color, float depth);

    void setVertexBuffer(const std::vector<ShaderContext>& v) { vertex = v; }

    void setIndexBuffer(const std::vector<int>& i) { index = i; }

    void setShaders(VertexShader* pVS, PixelShader* pPS) { pVertexShader = pVS; pPixelShader = pPS; }

    void setUniforms(const ShaderUniform& uni) { uniforms = uni; }

protected:
    void rasterTriangle(const ShaderContext& v0, const ShaderContext& v1, const ShaderContext& v2);

    void mergeMSAARenderTarget();

    void resetMSAARenderTarget()
    {
        // reset msaa render targets count
        if (state.msCount != (int)msaaColorBuffer.size())
        {
            msaaColorBuffer.clear();
            msaaDepthBuffer.clear();
            msaaColorBuffer.reserve(state.msCount);
            msaaDepthBuffer.reserve(state.msCount);
        }
        // clear all render targets
        for (int i = 0; i < state.msCount; ++i)
        {
            msaaColorBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearColor);
            msaaDepthBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearDepth);
        }
        // clear masks to 0
        msaaMask.clear();
        msaaMask.resize(renderTarget.width * renderTarget.height, 0);
    }

    void resetRenderTargetState()
    {
        if (renderTarget.width != state.width || renderTarget.height != state.height)
        {
            renderTarget = Texture2D3F(state.width, state.height);
            depthBuffer = Texture2D1F(state.width, state.height);
        }
    }


protected:
    Texture2D3F renderTarget;
    Texture2D1F depthBuffer;
    std::vector<ShaderContext> vertex;
    std::vector<int> index;
    VertexShader* pVertexShader;
    PixelShader* pPixelShader;
    ShaderUniform uniforms;

    std::vector<Texture2D3F> msaaColorBuffer;
    std::vector<Texture2D1F> msaaDepthBuffer;
    std::vector<uint32_t> msaaMask;

    PipelineState state;

    Vec3f lastClearColor;
    float lastClearDepth;
};

bool pointInTriangle(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2);

Vec3f getPerspectiveCorrectFactor(const Vec2f& q, const Vec4f& p0, const Vec4f& p1, const Vec4f& p2);

void shaderContextLerp(ShaderContext& out, Vec3f factor, const ShaderContext& in0, const ShaderContext& in1, const ShaderContext& in2);

Vec3f getFactor(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2);
