#pragma once

#include "..\..\Materials\Image.h"

class CherrySprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	CherrySprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 12, 12, 16) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t CherrySprite::rgbMemory[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,12,15,15,15,15,15,15,15,15,15,15,7,2,4,2,3,7,15,15,15,15,15,15,15,13,1,7,15,15,15,15,15,15,15,15,15,2,15,12,15,15,15,15,15,15,11,0,4,15,15,11,6,13,15,15,15,15,8,5,5,6,0,0,5,10,15,15,15,15,15,9,9,15,8,8,10,10,15,15,15,15,15,15,15,15,15,8,10,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

const uint8_t CherrySprite::rgbColors[] PROGMEM = {253,165,0,22,172,0,9,165,0,2,141,6,0,77,26,245,29,0,145,17,0,0,49,13,211,7,0,189,7,0,158,3,0,88,3,0,0,8,5,0,4,9,26,0,0,0,0,0};
