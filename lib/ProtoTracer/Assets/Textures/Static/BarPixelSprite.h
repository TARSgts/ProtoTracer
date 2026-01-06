#pragma once

#include "..\..\Materials\Image.h"

class BarPixelSprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	BarPixelSprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 15, 7, 3) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t BarPixelSprite::rgbMemory[] PROGMEM = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,0,2,2,2,0,2,2,0,0,2,2,1,1,2,0,2,0,2,0,2,0,2,0,2,0,2,1,1,2,0,0,2,2,0,0,0,2,0,0,2,2,1,1,2,0,2,0,2,0,2,0,2,0,2,0,2,1,1,2,0,0,2,2,0,2,0,2,0,2,0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

const uint8_t BarPixelSprite::rgbColors[] PROGMEM = {255,255,255,255,242,0,0,0,0};
