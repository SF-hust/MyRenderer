#pragma once

#include "Pipeline.h"

constexpr int SC_COLOR = 2;

constexpr int screenWidth = 100;
constexpr int screenHeight = 100;
constexpr int screenScale = 4;

std::vector<ShaderContext> sim_vertices(3);

std::vector<int> sim_indecies = {0, 2, 1};

ShaderUniform uniforms;

PipelineState sim_pipelineState;

Pipeline simPipeline;

uint8_t bitData[screenWidth * screenHeight * 3 * sizeof(uint8_t)];
uint8_t screenData[screenWidth * screenHeight * 3 * screenScale * screenScale * sizeof(uint8_t)];



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
        return Vec4f(input.v3f.at(SC_COLOR), 1.0f);
    }
};

SimpleVS sim_vs;
SimplePS sim_ps;

void initSimplePipeline()
{
    /*
    sim_pipelineState.msCount = 1;
    sim_pipelineState.sampleCoords = {
        {0.5f, 0.5f}
    };
    //*/

    //*
    sim_pipelineState.msCount = 4;
    sim_pipelineState.sampleCoords = {
        {0.25f, 0.25f}, {0.25f, 0.75f}, {0.75f, 0.25f}, {0.75f, 0.75f}
    };
    //*/

    /*
    sim_pipelineState.msCount = 16;
    sim_pipelineState.sampleCoords = {
        {0.125f, 0.125f}, {0.125f, 0.375f}, {0.125f, 0.625f}, {0.125f, 0.875f},
        {0.375f, 0.125f}, {0.375f, 0.375f}, {0.375f, 0.625f}, {0.375f, 0.875f},
        {0.625f, 0.125f}, {0.625f, 0.375f}, {0.625f, 0.625f}, {0.625f, 0.875f},
        {0.875f, 0.125f}, {0.875f, 0.375f}, {0.875f, 0.625f}, {0.875f, 0.875f},
    };

    //*/
    sim_pipelineState.width = screenWidth;
    sim_pipelineState.height = screenHeight;
    sim_vertices[0].v4f[SV_Position] = { -0.5f, -0.5f, 0.0f, 1.0f };
    sim_vertices[1].v4f[SV_Position] = { 0.0f, 0.5f, 0.0f, 1.0f };
    sim_vertices[2].v4f[SV_Position] = { 0.5f, -0.5f, 0.0f, 1.0f };
    sim_vertices[0].v3f[SC_COLOR] = { 1.0f, 0.0f, 0.0f };
    sim_vertices[1].v3f[SC_COLOR] = { 0.0f, 1.0f, 0.0f };
    sim_vertices[2].v3f[SC_COLOR] = { 0.0f, 0.0f, 1.0f };
    simPipeline.setPipelineState(sim_pipelineState);
    simPipeline.setVertexBuffer(sim_vertices);
    simPipeline.setIndexBuffer(sim_indecies);
    simPipeline.setUniforms(uniforms);
    simPipeline.setShaders(&sim_vs, &sim_ps);
}
