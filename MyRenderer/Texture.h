#pragma once

#include "MathHelper.h"

// the left top of the texture is point (0.0, 0.0)

class Texture2D3F
{
public:
	Texture2D3F(int w, int h) : width(w), height(h), data(w * h) { }

	~Texture2D3F() { }

	void toBitmap(uint8_t* pDest) const
	{
		for (int i = 0; i < width * height; ++i)
		{
			pDest[3 * i] = floatToByte(data[i].b);
			pDest[3 * i + 1] = floatToByte(data[i].g);
			pDest[3 * i + 2] = floatToByte(data[i].r);
		}
	}

	Vec3f& at(int x, int y) { return data[x + y * height]; }
	const Vec3f& at(int x, int y) const { return data[x + y * height]; }

public:
	std::vector<Vec3f> data;
	int width;
	int height;
	
};

class Texture2D1F
{
public:
	Texture2D1F(int w, int h) : width(w), height(h), data(w * h) { }

	~Texture2D1F() { }

	void toBitmap(uint8_t* pDest) const
	{
		for (int i = 0; i < width * height; ++i)
		{
			pDest[i] = floatToByte(data[i]);
		}
	}

	float& at(int x, int y) { return data[x + y * height]; }
	const float& at(int x, int y) const { return data[x + y * height]; }

public:
	std::vector<float> data;
	int width;
	int height;

};

