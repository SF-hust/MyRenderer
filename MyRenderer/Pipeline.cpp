#include "Pipeline.h"

void Pipeline::renderToTarget()
{
	// traverse all vertices, assemble every 3 vertices as 1 triangle
	for (int i = 0; i < vertex.size() - 2; i += 3)
	{
		ShaderContext vOut0, vOut1, vOut2;
		pVertexShader->excute(vertex[i], vOut0, uniforms);
		pVertexShader->excute(vertex[i + 1], vOut1, uniforms);
		pVertexShader->excute(vertex[i + 2], vOut2, uniforms);
		rasterTriangle(vOut0, vOut1, vOut2);
	}
}

void Pipeline::rasterTriangle(const ShaderContext& v0, const ShaderContext& v1, const ShaderContext& v2)
{

}
