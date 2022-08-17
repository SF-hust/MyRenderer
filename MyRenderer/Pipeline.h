#pragma once

#include "Shader.h"


/*
* class Pipeline
* usage :
* 1. set pipeline sim_pipelineState, vertices and indecies input, shaders, uniforms
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

    void clearRenderTarget(Vec3f color, float depth);

    void setPipelineState(const PipelineState& state);

    void setVertexBuffer(const std::vector<ShaderContext>& v) { vertices = v; }

    void setIndexBuffer(const std::vector<int>& i) { indecies = i; }

    void setShaders(VertexShader* pVS, PixelShader* pPS) { pVertexShader = pVS; pPixelShader = pPS; }

    void setUniforms(const ShaderUniform& uni) { uniforms = uni; }

protected:
    void rasterTriangle(const ShaderContext& v0, const ShaderContext& v1, const ShaderContext& v2);

    std::vector<ShaderContext> clippingTriangle(ShaderContext& v0, ShaderContext& v1, ShaderContext& v2);

    void mergeMSAARenderTarget();

    void resetMSAARenderTarget();

    void resetRenderTargetState();

protected:
    Texture2D3F renderTarget;
    Texture2D1F depthBuffer;
    std::vector<ShaderContext> vertices;
    std::vector<int> indecies;
    VertexShader* pVertexShader;
    PixelShader* pPixelShader;
    ShaderUniform uniforms;

    std::vector<Texture2D3F> msaaColorBuffer;
    std::vector<Texture2D1F> msaaDepthBuffer;
    std::vector<uint32_t> msaaMask;

    PipelineState state;

    Vec3f lastClearColor;
    float lastClearDepth;

    static const Vec2i pixel2x2Steps[4];
};
