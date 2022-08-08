#include "Pipeline.h"

void Pipeline::renderToTarget()
{
    // create tmp render-targets, for multi-sample
    int i, c, x, y;
    float d;
    Vec3f color;
    // clear masks to 0
    msaaMask.clear();
    msaaMask.resize(renderTarget.width * renderTarget.height, 0);

    // traverse all vertices, assemble every 3 vertices as 1 triangle
    for (i = 0; i < vertex.size() - 2; i += 3)
    {
        ShaderContext vOut0, vOut1, vOut2;
        // excute vertex shader for 3 vertices, tranform to clipping space
        pVertexShader->excute(vertex[i], vOut0, uniforms, state);
        pVertexShader->excute(vertex[i + 1], vOut1, uniforms, state);
        pVertexShader->excute(vertex[i + 2], vOut2, uniforms, state);
        // TODO: clipping triangle

        // do perspective division
        vOut0.v4f[SV_Position].x /= vOut0.v4f[SV_Position].w;
        vOut0.v4f[SV_Position].y /= vOut0.v4f[SV_Position].w;
        vOut0.v4f[SV_Position].z /= vOut0.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].x /= vOut1.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].y /= vOut1.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].z /= vOut1.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].x /= vOut2.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].y /= vOut2.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].z /= vOut2.v4f[SV_Position].w;
        // TODO: tranform to NDC space
        
        // raster, and shade the pixels
        rasterTriangle(vOut0, vOut1, vOut2);
    }

    // merge the tmp render-targets and tmp depth buffers
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
                    if (d < msaaDepthBuffer[i].at(x, y))
                    {
                        d = msaaDepthBuffer[i].at(x, y);
                    }
                    color += msaaColorBuffer[i].at(x, y);
                }
            }
            if (c > 0)
            {
                renderTarget.at(x, y) = color / float(c);
            }
        }
    }
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
    int xstart, xend, ystart, yend;
    int x0, x1, x2, y0, y1, y2;
    int x, y, i;
    x0 = int((v0.v4f.at(SV_Position).x + 1.0f) / 2.0f * state.width);
    x1 = int((v1.v4f.at(SV_Position).x + 1.0f) / 2.0f * state.width);
    x2 = int((v2.v4f.at(SV_Position).x + 1.0f) / 2.0f * state.width);
    y0 = int((v0.v4f.at(SV_Position).y + 1.0f) / 2.0f * state.height);
    y1 = int((v1.v4f.at(SV_Position).y + 1.0f) / 2.0f * state.height);
    y2 = int((v2.v4f.at(SV_Position).y + 1.0f) / 2.0f * state.height);
    xstart =std::max(std::min({ x0, x1, x2 }), 0);
    xend = std::min(std::max({ x0, x1, x2 }) + 1, state.width);
    ystart = std::max(std::min({ y0, y1, y2 }), 0);
    yend = std::min(std::max({ y0, y1, y2 }) + 1, state.height);
    // traverse all possible pixels
    for (x = xstart; x < xend; ++x)
    {
        for (y = ystart; y < yend; ++y)
        {
            // calculate which samples should be used
            Vec4f result;
            float depth = 1.0f;
            Vec2f avgCenter = { 0.0f, 0.0f };
            int coverCount = 0;
            int mask = 0;
            for (i = 0; i < state.msCount; ++i)
            {
                // convert form screen to NDC
                Vec2f p = {
                    (float(x) + state.sampleCoords[i].x) / float(state.width) * 2.0f - 1.0f,
                    (float(y) + state.sampleCoords[i].y) / float(state.height) * 2.0f - 1.0f };
                if (pointInTriangle(p,
                    v0.v4f.at(SV_Position).xy(),
                    v1.v4f.at(SV_Position).xy(),
                    v2.v4f.at(SV_Position).xy() ))
                {
                    avgCenter += state.sampleCoords[i];
                    ++coverCount;
                    mask |= (1 << i);
                }
            }
            // sample from the center of the covered sample points
            avgCenter /= float(coverCount);
            // if this triangle covers at least 1 sample, call pixel shader to calculate color of this pixel
            if (coverCount > 0)
            {
                Vec3f factor;
                ShaderContext pixelIn;
                Vec2f q = {
                    (float(x) + avgCenter.x) / state.width * 2.0f - 1.0f,
                    (float(y) + avgCenter.y) / state.height * 2.0f - 1.0f };
                factor = getPerspectiveCorrectFactor(
                    q,
                    v0.v4f.at(SV_Position),
                    v1.v4f.at(SV_Position),
                    v2.v4f.at(SV_Position));
                // lerp the vertex properties
                shaderContextLerp(pixelIn, factor, v0, v1, v2);
                depth = pixelIn.v4f[SV_Position].z;
                // call pixel shader
                result = pPixelShader->excute(pixelIn, uniforms, state);
            }
            // check which sample is used, set value to the tmp target
            for (i = 0; i < state.msCount; ++i)
            {
                if ((mask & (1 << i)) != 0)
                {
                    // if the new depth is smaller,
                    // or this sample have not been writen
                    if (msaaDepthBuffer[i].data[x + y * state.width] < depth ||
                        (msaaMask[x + y * state.width] & (1 << i)) == 0)
                    {
                        msaaColorBuffer[i].data[x + y * state.width] = result.xyz();
                        msaaDepthBuffer[i].data[x + y * state.width] = depth;
                    }
                }
            }
            // write the sample mask to buffer
            msaaMask[x + y * state.width] = mask;
            // END OF operation for pixels
        }
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
                    if (d < msaaDepthBuffer[i].at(x, y))
                    {
                        d = msaaDepthBuffer[i].at(x, y);
                    }
                    color += msaaColorBuffer[i].at(x, y);
                }
            }
            if (c > 0)
            {
                renderTarget.at(x, y) = color / float(c);
            }
        }
    }
}

bool pointInTriangle(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2)
{
    Vec3f factor = getFactor(p, v0, v1, v2);
    return factor[0] >= 0.0f && factor[1] >= 0.0f && factor[2] >= 0.0f;
}

Vec3f getPerspectiveCorrectFactor(const Vec2f& q, const Vec4f& p0, const Vec4f& p1, const Vec4f& p2)
{
    Vec3f rawFactor = getFactor(q, p0.xy(), p1.xy(), p2.xy());
    float c0 = rawFactor[0] / p0.w;
    float c1 = rawFactor[1] / p1.w;
    float c2 = rawFactor[2] / p2.w;
    float c = c0 + c1 + c2;
    Vec3f correctedFactor = { c0 / c, c1 / c, c2 / c };
    return correctedFactor;
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

Vec3f getFactor(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2)
{
    Vec2f c = p - v2;
    Vec2f a = v1 - v2;
    Vec2f b = v0 - v2;
    float axb = Vector_cross(a, b);
    Vec3f factor;
    factor[0] = Vector_cross(c, b) / axb;
    factor[1] = Vector_cross(c, a) / (-axb);
    factor[2] = 1.0f - factor[0] - factor[1];
    return factor;
}
