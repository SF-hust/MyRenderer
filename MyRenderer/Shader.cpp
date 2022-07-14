#include "Shader.h"

Vec4f sampler(const Texture2D3F& texture, Vec2f uv, FilterMode filterMode, float ddu, float ddv)
{
	Vec4f result;
	switch (filterMode)
	{
	case POINT:
		break;
	case LINEAR:
		break;
	case ANISOTROPIC:
		break;
	default:
		break;
	}
	return result;
}

