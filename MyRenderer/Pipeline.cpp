#include "Pipeline.h"

const Vec2i Pipeline::pixel2x2Steps[4] = {Vec2i(0, 0), Vec2i(1, 0), Vec2i(0, 1), Vec2i(1, 1)};


void Pipeline::renderToTarget()
{
    // traverse all vertices, assemble every 3 vertices as 1 triangle
    //for (int i = 0; i < vertices.size() - 2; i += 3)
    for (int i = 0; i < indecies.size() - 2; i += 3)
    {
        ShaderContext vOut0, vOut1, vOut2;
        // excute vertex shader for 3 vertices, tranform to clipping space
        pVertexShader->excute(vertices[indecies[i + 0]], vOut0, uniforms, state);
        pVertexShader->excute(vertices[indecies[i + 1]], vOut1, uniforms, state);
        pVertexShader->excute(vertices[indecies[i + 2]], vOut2, uniforms, state);
        if(shouldClip(vOut0.v4f[SV_Position]) || shouldClip(vOut1.v4f[SV_Position]) || shouldClip(vOut2.v4f[SV_Position]))
        {
            // TODO: clippingTriangle() has no implementation
            std::vector<ShaderContext> clippedVertex = clippingTriangle(vOut0, vOut1, vOut2);
            for(int j = 0; j < clippedVertex.size() - 2; j += 3)
            {
                // do perspective division
                // and SV_Position will be transformed to NDC space( x: -1~1, y: -1~1, z: 0~1 )
                doPerspectiveDivision(clippedVertex[i + 0].v4f[SV_Position]);
                doPerspectiveDivision(clippedVertex[i + 1].v4f[SV_Position]);
                doPerspectiveDivision(clippedVertex[i + 2].v4f[SV_Position]);
                // raster, and shade the pixels
                rasterTriangle(clippedVertex[i], clippedVertex[i + 1], clippedVertex[i + 2]);
            }
        }
        else
        {
            // do perspective division
            // and SV_Position will be transformed to NDC space( x: -1~1, y: -1~1, z: 0~1 )
            doPerspectiveDivision(vOut0.v4f[SV_Position]);
            doPerspectiveDivision(vOut1.v4f[SV_Position]);
            doPerspectiveDivision(vOut2.v4f[SV_Position]);
            // raster, and shade the pixels
            rasterTriangle(vOut0, vOut1, vOut2);
        }
    }// END of loop
}

void Pipeline::presentToScreen(uint8_t* buffer)
{
    mergeMSAARenderTarget();
    renderTarget.toBitmap(buffer);
}

void Pipeline::setPipelineState(const PipelineState& state)
{
    this->state = state;
    resetRenderTargetState();
    resetMSAARenderTarget();
}

void Pipeline::clearRenderTarget(Vec3f color, float depth)
{
    // clear msaa render target
    for (int m = 0; m < state.msCount; ++m)
    {
        for (int i = 0; i < state.width * state.height; ++i)
        {
            msaaColorBuffer[m].data[i] = color;
            msaaDepthBuffer[m].data[i] = depth;
        }
    }
    // clear present render target
    for (int i = 0; i < state.width * state.height; ++i)
    {
        renderTarget.data[i] = color;
        depthBuffer.data[i] = depth;
    }
    lastClearColor = color;
    lastClearDepth = depth;
}

void Pipeline::rasterTriangle(const ShaderContext& v0, const ShaderContext& v1, const ShaderContext& v2)
{
    // TODO?: test per tile of 4x8 pixels
    int xstart, xend, ystart, yend;
    int x, y, i, j;
    // get the SV_Position of all input
    const Vec4f& pos0 = v0.v4f.at(SV_Position);
    const Vec4f& pos1 = v1.v4f.at(SV_Position);
    const Vec4f& pos2 = v2.v4f.at(SV_Position);
    // transform input positions to screen space
    Vec2f p0 = (pos0.xy() + Vec2f(1.0f, 1.0f)) * Vec2f(0.5f, 0.5f) * Vec2f((float)state.width, (float)state.height);
    Vec2f p1 = (pos1.xy() + Vec2f(1.0f, 1.0f)) * Vec2f(0.5f, 0.5f) * Vec2f((float)state.width, (float)state.height);
    Vec2f p2 = (pos2.xy() + Vec2f(1.0f, 1.0f)) * Vec2f(0.5f, 0.5f) * Vec2f((float)state.width, (float)state.height);
    // if triangle in screen space or NDC space is 0 in size, clip
    if(triangleIsZeroInSize(p0, p1, p2) || triangleIsZeroInSize(pos0.xy(), pos1.xy(), pos2.xy()))
    {
        return;
    }
    // get the bounding box of triangle, from (xstart, ystart) to (xend, yend)(not include)
    xstart = std::max((int)std::min({ p0.x, p1.x, p2.x }), 0);
    xend = std::min((int)std::max({ p0.x, p1.x, p2.x }) + 1, state.width);
    ystart = std::max((int)std::min({ p0.y, p1.y, p2.y }), 0);
    yend = std::min((int)std::max({ p0.y, p1.y, p2.y }) + 1, state.height);
    // for processing 2x2 pixels
    xstart = xstart & (~1);
    xend = (xend + 1) & (~1);
    ystart = ystart & (~1);
    yend = (yend + 1) & (~1);
    // traverse all possible pixels
    for (x = xstart; x < xend; x += 2)
    {
        for (y = ystart; y < yend; y += 2)
        {
            Vec2f avgCenters[4] = { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} };
            // if one of the 2x2 pixels should shade, the bit would be set to 1
            uint32_t shadingMask = 0;
            uint32_t newMasks[4] = {0U, 0U, 0U, 0U};
            for(j = 0; j < 4; j++)
            {
                int coverCount = 0;
                // (px, py) is the real coord of this very pixel
                int px = x + pixel2x2Steps[j].x;
                int py = y + pixel2x2Steps[j].y;
                for (i = 0; i < state.msCount; ++i)
                {
                    Vec2f p = Vec2f(float(px) + state.sampleCoords[i].x, float(py) + state.sampleCoords[i].y);
                    // test if triangle covers this sample
                    if (pointInTriangle(p, p0, p1, p2))
                    {
                        avgCenters[j] += state.sampleCoords[i];
                        ++coverCount;
                        newMasks[j] |= (1U << i);
                    }
                }
                // get the average center of covered msaa sample points
                // if triangle covers no sample, set the center to (0.5f, 0.5f)
                if(coverCount > 0)
                {
                    avgCenters[j] /= float(coverCount);
                    // if triangle covers at list 1 sample in 4 pixels, shade the 4 pixels
                    shadingMask |= (1U << j);
                }
                else
                {
                    avgCenters[j] = Vec2f(0.5f, 0.5f);
                }
                avgCenters[j] += Vec2f((float)px, (float)py);
                // to ndc space
                avgCenters[j].x /= (float)state.width;
                avgCenters[j].y /= (float)state.height;
                avgCenters[j] *= Vec2f(2.0f, 2.0f);
                avgCenters[j] -= Vec2f(1.0f, 1.0f);
            }
            if(shadingMask == 0U)
            {
                continue;
            }
            // gen pixel input for 2x2 pixels
            ShaderContext pIn[4];
            Vec3f f00 = getPerspectiveCorrectFactor(avgCenters[0], pos0, pos1, pos2);
            Vec3f f10 = getPerspectiveCorrectFactor(avgCenters[1], pos0, pos1, pos2);
            Vec3f f01 = getPerspectiveCorrectFactor(avgCenters[2], pos0, pos1, pos2);
            Vec3f f11 = getPerspectiveCorrectFactor(avgCenters[3], pos0, pos1, pos2);
            shaderContextLerp(pIn[0], f00, v0, v1, v2);
            shaderContextLerp(pIn[1], f10, v0, v1, v2);
            shaderContextLerp(pIn[2], f01, v0, v1, v2);
            shaderContextLerp(pIn[3], f11, v0, v1, v2);
            // set ddxUV and ddyUV
            pIn[0].v2f[SV_ddxUV] = pIn[1].v2f[SV_ddxUV] = pIn[1].v2f[SV_uv] - pIn[0].v2f[SV_uv];
            pIn[2].v2f[SV_ddxUV] = pIn[3].v2f[SV_ddxUV] = pIn[3].v2f[SV_uv] - pIn[2].v2f[SV_uv];
            pIn[0].v2f[SV_ddyUV] = pIn[2].v2f[SV_ddyUV] = pIn[2].v2f[SV_uv] - pIn[0].v2f[SV_uv];
            pIn[1].v2f[SV_ddyUV] = pIn[3].v2f[SV_ddyUV] = pIn[3].v2f[SV_uv] - pIn[1].v2f[SV_uv];
            // shade the covered pixel, and get the depth
            for(j = 0; j < 4; j++)
            {
                if((shadingMask & (1U << j))  != 0)
                {
                    float newDepth = pIn[j].v4f[SV_Position].z;
                    Vec4f color = pPixelShader->excute(pIn[j], uniforms, state);
                    // (px, py) is the real coord of this very pixel
                    int px = x + pixel2x2Steps[j].x;
                    int py = y + pixel2x2Steps[j].y;
                    for (i = 0; i < state.msCount; ++i)
                    {
                        if ((newMasks[j] & (1U << i)) != 0U)
                        {
                            // if the new depth is smaller,
                            // or this sample have not been writen
                            if (msaaDepthBuffer[i].data[px + py * state.width] > newDepth)
                            {
                                msaaColorBuffer[i].data[px + py * state.width] = color.xyz();
                                msaaDepthBuffer[i].data[px + py * state.width] = newDepth;
                            }
                        }
                    }
                    // refresh the msaa sample mask
                    msaaMask[px + py * state.width] |= newMasks[j];
                }
            }
            // END OF operation for pixels
        }
    }
}

std::vector<ShaderContext> Pipeline::clippingTriangle(ShaderContext& v0, ShaderContext& v1, ShaderContext& v2)
{
    // TODO: implementing clipping
    return std::vector<ShaderContext>();
}

void Pipeline::resetMSAARenderTarget()
{
    // reset msaa render targets count
    if (state.msCount != (int)msaaColorBuffer.size())
    {
        msaaColorBuffer.clear();
        msaaDepthBuffer.clear();
        msaaColorBuffer.reserve(state.msCount);
        msaaDepthBuffer.reserve(state.msCount);
    }
    // clear all msaa render targets
    for (int i = 0; i < state.msCount; ++i)
    {
        msaaColorBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearColor);
        msaaDepthBuffer.emplace_back(renderTarget.width, renderTarget.height, lastClearDepth);
    }
    // clear masks to 0
    msaaMask.clear();
    msaaMask.resize(renderTarget.width * renderTarget.height, 0);
}

void Pipeline::resetRenderTargetState()
{
    // if size of the render target changed, recreate it
    if (renderTarget.width != state.width || renderTarget.height != state.height)
    {
        renderTarget = Texture2D3F(state.width, state.height);
        depthBuffer = Texture2D1F(state.width, state.height);
    }
}

void Pipeline::mergeMSAARenderTarget()
{
    int i, c, x, y;
    float d;
    Vec3f color;
    for (x = 0; x < state.width; ++x)
    {
        for (y = 0; y < state.height; ++y)
        {
            c = 0;
            d = 2.0f;
            color = { 0.0f, 0.0f, 0.0f };
            for (i = 0; i < state.msCount; ++i)
            {
                if ((msaaMask[x + y * state.width] & (1U << i)) != 0)
                {
                    ++c;
                    if (d > msaaDepthBuffer[i].get(x, y))
                    {
                        d = msaaDepthBuffer[i].get(x, y);
                    }
                    color += msaaColorBuffer[i].get(x, y);
                }
            }
            if (c > 0)
            {
                renderTarget.get(x, y) = color / float(state.msCount);
                depthBuffer.get(x, y) = d;
            }
        }
    }
}

bool Pipeline::shouldClip(Vec4f& v)
{
    return v.w < state.near || v.w > state.far ||
        v.x < -v.w || v.x > v.w ||
        v.y < -v.w || v.y > v.w;
}

void shaderContextLerp(ShaderContext& out, Vec3f factor, const ShaderContext& in0, const ShaderContext& in1, const ShaderContext& in2)
{
    int key;
    for (auto itr = in0.f.begin(); itr != in0.f.end(); ++itr)
    {
        key = itr->first;
        out.f[key] = factor.x * in0.f.at(key) + factor.y * in1.f.at(key) + factor.z * in2.f.at(key);
    }
    for (auto itr = in0.v2f.begin(); itr != in0.v2f.end(); ++itr)
    {
        key = itr->first;
        out.v2f[key] = factor.x * in0.v2f.at(key) + factor.y * in1.v2f.at(key) + factor.z * in2.v2f.at(key);
    }
    for (auto itr = in0.v3f.begin(); itr != in0.v3f.end(); ++itr)
    {
        key = itr->first;
        out.v3f[key] = factor.x * in0.v3f.at(key) + factor.y * in1.v3f.at(key) + factor.z * in2.v3f.at(key);
    }
    for (auto itr = in0.v4f.begin(); itr != in0.v4f.end(); ++itr)
    {
        key = itr->first;
        out.v4f[key] = factor.x * in0.v4f.at(key) + factor.y * in1.v4f.at(key) + factor.z * in2.v4f.at(key);
    }
    for (auto itr = in0.m4x4.begin(); itr != in0.m4x4.end(); ++itr)
    {
        key = itr->first;
        out.m4x4[key] = factor.x * in0.m4x4.at(key) + factor.y * in1.m4x4.at(key) + factor.z * in2.m4x4.at(key);
    }
}
