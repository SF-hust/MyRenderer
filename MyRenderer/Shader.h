#pragma once

#include <unordered_map>
#include "Texture.h"

class Shader
{
public:
	virtual void excute(ShaderContext& input, ShaderContext& output, std::vector<Texture2D3F>& textures) = 0;

};

struct ShaderContext
{
	std::unordered_map<int, float> f;
	std::unordered_map<int, MyMathHelper::Vec2f> v2f;
	std::unordered_map<int, MyMathHelper::Vec3f> v3f;
	std::unordered_map<int, MyMathHelper::Vec4f> v4f;
	std::unordered_map<int, MyMathHelper::Mat4x4f> m4x4;
};