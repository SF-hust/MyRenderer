#pragma once
#include "MathHelper.h"
#include <vector>

struct PipelineState
{
    int width = 800;
    int height = 600;
    float fov = 90.0f;
    float near = 0.1f;
    float far = 64.0f;
    std::vector<Vec2f> sampleCoords = {Vec2f(0.5f, 0.5f)};
    int msCount = 1;
    bool enableDepthTest = true;
};