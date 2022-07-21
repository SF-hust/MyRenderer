#include "Pipeline.h"

void Pipeline::renderToTarget()
{
    // create tmp render-targets, for multi-sample
    int i, c, x, y;
    float d;
    Vec3f color;
    tmpColorBuffer.reserve(multiSampleCount);
    tmpDepthBuffer.reserve(multiSampleCount);
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

}

Vec3f getPerspectiveCorrectFactor(const Vec4f& p0, const Vec4f& p1, const Vec4f& p2, const Vec3f& q)
{
    
    return Vec3f();
}
