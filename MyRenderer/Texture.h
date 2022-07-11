#pragma once

#include "MathHelper.h"

class Texture2D3F
{
public:
	Texture2D3F(int w, int h) : width(w), height(h), data(w * h) { }

	~Texture2D3F() { }

	void toBitmap(uint8_t* pDest)
	{
		for (int i = 0; i < width * height; ++i)
		{
			pDest[3 * i] = MyMathHelper::floatToByte(data[i].b);
			pDest[3 * i + 1] = MyMathHelper::floatToByte(data[i].g);
			pDest[3 * i + 2] = MyMathHelper::floatToByte(data[i].r);
		}
	}

public:
	std::vector<MyMathHelper::Vec3f> data;

protected:
	int width;
	int height;
	
};

class Texture2D1F
{
public:
	Texture2D1F(int w, int h) : width(w), height(h), data(w * h) { }

	~Texture2D1F() { }

	void toBitmap(uint8_t* pDest)
	{
		for (int i = 0; i < width * height; ++i)
		{
			pDest[i] = MyMathHelper::floatToByte(data[i]);
		}
	}

public:
	std::vector<float> data;

protected:
	int width;
	int height;

};

