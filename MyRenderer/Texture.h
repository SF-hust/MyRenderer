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
	void toBitmap(uint8_t* pDest);
public:
	std::vector<MyMathHelper::Vec3f> data;

protected:
	int width;
	int height;
	
};
