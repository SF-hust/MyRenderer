#pragma once

#include "Shader.h"
#include "PipelineState.h"

class Pipeline
{
public:
    void renderToTarget();

    Texture2D3F& getRenderTarget() { return renderTarget; }

    void setMultiSampleSState(const std::vector<Vec2f>& coords)
    {
        sampleCoords = coords;
        if (sampleCoords.size() > 32)
        {
            sampleCoords.resize(32);
        }
        multiSampleCount = sampleCoords.size();
    }

    void setRenderTargetState(int w, int h, bool enableDepth)
    {
        enableDepthTest = enableDepth;
        if (width != w || height != h)
        {
            width = w;
            height = h;
            renderTarget = Texture2D3F(width, height);
            depthBuffer = Texture2D1F(width, height);
        }
    }

    void clearRenderTarget(Vec3f color, float depth)
    {
        for (int i = 0; i < width * height; ++i)
        {
            renderTarget.data[i] = color;
            depthBuffer.data[i] = depth;
        }
        lastClearColor = color;
        lastClearDepth = depth;
        resetMSAARenderTarget();
    }

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
        if (multiSampleCount != (int)tmpColorBuffer.size())
        {
            tmpColorBuffer.clear();
            tmpDepthBuffer.clear();
            tmpColorBuffer.reserve(multiSampleCount);
            tmpDepthBuffer.reserve(multiSampleCount);
        }
        // clear all render targets
        for (int i = 0; i < multiSampleCount; ++i)
        {
            tmpColorBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearColor);
            tmpDepthBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearDepth);
        }
        // clear masks to 0
        msMask.clear();
        msMask.resize(renderTarget.width * renderTarget.height, 0);
    }

protected:
    std::vector<Vec2f> sampleCoords = {Vec2f(0.5f, 0.5f)};
    int multiSampleCount = 1;
    Texture2D3F renderTarget;
    Texture2D1F depthBuffer;
    std::vector<ShaderContext> vertex;
    std::vector<int> index;
    VertexShader* pVertexShader;
    PixelShader* pPixelShader;
    ShaderUniform uniforms;

    std::vector<Texture2D3F> tmpColorBuffer;
    std::vector<Texture2D1F> tmpDepthBuffer;
    std::vector<uint32_t> msMask;

    PipelineState state;

    int width = 0;
    int height = 0;
    bool enableDepthTest = true;

    Vec3f lastClearColor;
    float lastClearDepth;
};

bool pointInTriangle(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2);

Vec3f getPerspectiveCorrectFactor(const Vec2f& q, const Vec4f& p0, const Vec4f& p1, const Vec4f& p2);

void shaderContextLerp(ShaderContext& out, Vec3f factor, const ShaderContext& in0, const ShaderContext& in1, const ShaderContext& in2);

Vec3f getFactor(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2);
