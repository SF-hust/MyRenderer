#pragma once

#include "Pipeline.h"

constexpr int SC_COLOR = 2;

std::vector<ShaderContext> sim_vertices(3);

std::vector<int> sim_indecies = {0, 2, 1};

ShaderUniform uniforms;

PipelineState sim_pipelineState;

Pipeline simPipeline;

uint8_t bitData[800 * 600 * 3 * sizeof(uint8_t)];


class SimpleVS : public VertexShader
{
protected:
    virtual void excute(ShaderContext& input, ShaderContext& output, ShaderUniform& uniform) override
    {
        output.v4f[SV_Position] = input.v4f[SV_Position];
        output.v3f[SC_COLOR] = input.v3f[SC_COLOR];
    }
};

class SimplePS : public PixelShader
{
protected:
    virtual Vec4f excute(const ShaderContext& input, const ShaderUniform& uniform) override
    {
        return Vec4f(input.v3f.at(SC_COLOR), 0.0f);
    }
};

SimpleVS sim_vs;
SimplePS sim_ps;

void initSimplePipeline()
{
    sim_vertices[0].v3f[SV_Position] = { -0.5f, -0.5f, 0.0f };
    sim_vertices[1].v3f[SV_Position] = { 0.0f, 0.5f, 0.0f };
    sim_vertices[2].v3f[SV_Position] = { 0.5f, -0.5f, 0.0f };
    sim_vertices[0].v3f[SC_COLOR] = { 1.0f, 0.0f, 0.0f };
    sim_vertices[1].v3f[SC_COLOR] = { 0.0f, 1.0f, 0.0f };
    sim_vertices[2].v3f[SC_COLOR] = { 0.0f, 0.0f, 1.0f };
    simPipeline.setPipelineState(sim_pipelineState);
    simPipeline.setVertexBuffer(sim_vertices);
    simPipeline.setIndexBuffer(sim_indecies);
    simPipeline.setUniforms(uniforms);
    simPipeline.setShaders(&sim_vs, &sim_ps);
}
