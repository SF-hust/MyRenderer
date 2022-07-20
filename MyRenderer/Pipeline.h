#pragma once

#include "Shader.h"

class Pipeline
{
public:
	void renderToTarget();

	Texture2D3F& getRenderTarget() { return renderTarget; }

	void setMultiSampleSState(const std::vector<Vec2f>& coords) { sampleCoords = coords; }

	void setRenderTargetState(int w, int h, bool depth)
	{
		width = w;
		height = h;
		enableDepthTest = depth;
	}

	void setVertexBuffer(const std::vector<ShaderContext>& v) { vertex = v; }

	void setIndexBuffer(const std::vector<int>& i) { index = i; }

	void setTextures(const std::vector<Texture2D3F>& t) { textures = t; }

protected:
	std::vector<Vec2f> sampleCoords = {Vec2f(0.5f, 0.5f)};
	Texture2D3F renderTarget;
	std::vector<ShaderContext> vertex;
	std::vector<int> index;
	std::vector<Texture2D3F> textures;
	int width = 0;
	int height = 0;
	bool enableDepthTest = true;
};

