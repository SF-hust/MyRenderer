#include "Texture.h"

Texture2D::Texture2D(int w, int h) : width(w), height(h), data(w * h) { }

Texture2D::~Texture2D() { }

void Texture2D::toBitmap(uint8_t* pDest)
{
	for (int i = 0; i < width * height; ++i)
	{
		pDest[3 * i] = MyMathHelper::floatToByte(data[i].b);
		pDest[3 * i + 1] = MyMathHelper::floatToByte(data[i].g);
		pDest[3 * i + 2] = MyMathHelper::floatToByte(data[i].r);
	}
}

