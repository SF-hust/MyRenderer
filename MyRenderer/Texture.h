#pragma once

#include "MathHelper.h"

class Texture
{
};

class Texture2D
{
public:
	Texture2D(int w, int h);
	~Texture2D();
	void* toBitmap();
public:
	std::vector<Vec3f> data;

protected:
	int width;
	int height;
	
};

Texture2D::Texture2D(int w, int h) : width(w), height(h)
{
	data.reserve(w * h);
}

Texture2D::~Texture2D()
{
}

