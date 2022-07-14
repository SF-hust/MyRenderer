#pragma once

#include <unordered_map>
#include "Texture.h"

// some constants to use as key in the shader context
constexpr int SV_Position = 0;

struct ShaderContext
{
	std::unordered_map<int, float> f;
	std::unordered_map<int, Vec2f> v2f;
	std::unordered_map<int, Vec3f> v3f;
	std::unordered_map<int, Vec4f> v4f;
	std::unordered_map<int, Mat4x4f> m4x4;
};


enum FilterMode
{
	POINT,
	LINEAR,
	ANISOTROPIC
};

// the sample function
Vec4f sampler(const Texture2D3F& texture, Vec2f uv, FilterMode filterMode, float ddu, float ddv);


// class Shader
// set the shader context, textures
class Shader
{
public:
	// override these two functions to imply your own shader
	virtual void vertexShader() = 0;
	virtual void pixelShader() = 0;

public:
	void setShaderContext() {}

protected:


};

// class VertexShader
class VertexShader
{
protected:
	// override this function to imply your own shader
	virtual void excute(ShaderContext& input, ShaderContext& output, ShaderContext& uniform) = 0;

};

// class PixelShader
class VertexShader
{
public: //DON'T use these functions in the derived class
	void setDDUV(float ddu, float ddv) { this->ddu = ddu; this->ddv = ddv; }

protected:
	// these are some built-in functions, USE them in the override function
	Vec4f sample(const Texture2D3F& texture, const Vec2f& uv, FilterMode filterMode)
	{ sampler(texture, uv, filterMode, ddu, ddv); }

	// override this function to imply your own shader
	virtual Vec4f excute(
		ShaderContext& input,
		ShaderContext& uniform,
		const std::vector<Texture2D3F> textures
	) = 0;

protected:
	// don't use them in the derived class directly
	float ddu = 0.0f, ddv = 0.0f;
};
