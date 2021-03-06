#include "Pipeline.h"

void Pipeline::renderToTarget()
{
    // create tmp render-targets, for multi-sample
    int i, c, x, y;
    float d;
    Vec3f color;
    tmpColorBuffer.reserve(multiSampleCount);
    tmpDepthBuffer.reserve(multiSampleCount);
    msMask.clear();
    msMask.resize(renderTarget.width * renderTarget.height, 0);
    for (i = 0; i < multiSampleCount; ++i)
    {
        tmpColorBuffer.emplace_back(renderTarget.width, renderTarget.height);
        tmpDepthBuffer.emplace_back(renderTarget.width, renderTarget.height);
    }

    // traverse all vertices, assemble every 3 vertices as 1 triangle
    for (i = 0; i < vertex.size() - 2; i += 3)
    {
        ShaderContext vOut0, vOut1, vOut2;
        pVertexShader->excute(vertex[i], vOut0, uniforms);
        pVertexShader->excute(vertex[i + 1], vOut1, uniforms);
        pVertexShader->excute(vertex[i + 2], vOut2, uniforms);
        // TODO: clipping triangle

        // perspective division
        vOut0.v4f[SV_Position].x /= vOut0.v4f[SV_Position].w;
        vOut0.v4f[SV_Position].y /= vOut0.v4f[SV_Position].w;
        vOut0.v4f[SV_Position].z /= vOut0.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].x /= vOut0.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].y /= vOut0.v4f[SV_Position].w;
        vOut1.v4f[SV_Position].z /= vOut0.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].x /= vOut0.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].y /= vOut0.v4f[SV_Position].w;
        vOut2.v4f[SV_Position].z /= vOut0.v4f[SV_Position].w;
        // raster
        rasterTriangle(vOut0, vOut1, vOut2);
    }

    // merge the tmp render-targets and tmp depth buffers
    for (x = 0; x < width; ++x)
    {
        for (y = 0; y < height; ++y)
        {
            c = 0;
            d = 2.0f;
            color = { 0.0f, 0.0f, 0.0f };
            for (i = 0; i < multiSampleCount; ++i)
            {
                if ((msMask[x + y * width] & (1U << i)) != 0)
                {
                    ++c;
                    if (d < tmpDepthBuffer[i].at(x, y))
                    {
                        d = tmpDepthBuffer[i].at(x, y);
                    }
                    color += tmpColorBuffer[i].at(x, y);
                }
            }
            if (c > 0)
            {
                renderTarget.at(x, y) = color / float(c);
            }
        }
    }
}

void Pipeline::rasterTriangle(const ShaderContext& v0, const ShaderContext& v1, const ShaderContext& v2)
{
    int xstart, xend, ystart, yend;
    int x0, x1, x2, y0, y1, y2;
    int x, y, i;
    x0 = int((v0.v4f.at(SV_Position).x + 1.0f) / 2.0f * width);
    x1 = int((v1.v4f.at(SV_Position).x + 1.0f) / 2.0f * width);
    x2 = int((v2.v4f.at(SV_Position).x + 1.0f) / 2.0f * width);
    y0 = int((v0.v4f.at(SV_Position).y + 1.0f) / 2.0f * height);
    y1 = int((v1.v4f.at(SV_Position).y + 1.0f) / 2.0f * height);
    y2 = int((v2.v4f.at(SV_Position).y + 1.0f) / 2.0f * height);
    xstart =std::max(std::min({ x0, x1, x2 }), 0);
    xend = std::min(std::max({ x0, x1, x2 }) + 1, width);
    ystart = std::max(std::min({ y0, y1, y2 }), 0);
    yend = std::min(std::max({ y0, y1, y2 }) + 1, height);
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
            for (i = 0; i < multiSampleCount; ++i)
            {
                // convert form screen to NDC
                Vec2f p = {
                    (float(x) + sampleCoords[i].x) / width * 2.0f - 1.0f,
                    (float(y) + sampleCoords[i].y) / height * 2.0f - 1.0f };
                if (pointInTriangle(p,
                    v0.v4f.at(SV_Position).xy(),
                    v1.v4f.at(SV_Position).xy(),
                    v2.v4f.at(SV_Position).xy() ))
                {
                    avgCenter += sampleCoords[i];
                    ++coverCount;
                    msMask[x + y * width] |= (1 << i);
                }
            }
            avgCenter /= float(coverCount);
            // if this triangle covers at least 1 sample, call pixel shader to calculate color of this pixel
            if (coverCount > 0)
            {
                Vec3f factor;
                ShaderContext pixelIn;
                Vec2f q = {
                    (float(x) + avgCenter.x) / width * 2.0f - 1.0f,
                    (float(y) + avgCenter.y) / height * 2.0f - 1.0f };
                factor = getPerspectiveCorrectFactor(
                    v0.v4f.at(SV_Position),
                    v1.v4f.at(SV_Position),
                    v2.v4f.at(SV_Position),
                    q);
                shaderContextLerp(pixelIn, factor, v0, v1, v2);
                depth = pixelIn.v4f[SV_Position].z;
                result = pPixelShader->excute(pixelIn, uniforms, textures);
            }
            // check which sample is used, set value to the tmp target
            for (i = 0; i < multiSampleCount; ++i)
            {
                if ((msMask[x + y * width] & (1 << i)) != 0)
                {
                    if (tmpDepthBuffer[i].data[x + y * width] < depth)
                    {
                        tmpColorBuffer[i].data[x + y * width] = result.xyz();
                        tmpDepthBuffer[i].data[x + y * width] = depth;
                    }
                }
            }

            // operating for pixel end
        }
    }
}

bool pointInTriangle(Vec2f p, Vec2f v0, Vec2f v1, Vec2f v2)
{

    return false;
}

Vec3f getPerspectiveCorrectFactor(const Vec4f& p0, const Vec4f& p1, const Vec4f& p2, const Vec2f& q)
{
    
    return Vec3f();
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
