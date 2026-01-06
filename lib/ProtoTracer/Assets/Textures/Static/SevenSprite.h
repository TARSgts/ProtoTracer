#pragma once

#include "..\..\Materials\Image.h"

class SevenSprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	SevenSprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 12, 12, 16) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t SevenSprite::rgbMemory[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,10,14,1,1,1,10,15,10,13,15,15,15,7,0,4,4,4,5,5,5,13,15,15,15,7,2,3,3,3,4,3,6,13,15,15,15,8,7,15,15,15,0,7,13,15,15,15,15,15,15,15,12,0,7,9,14,15,15,15,15,15,15,14,8,6,8,15,15,15,15,15,15,15,15,13,0,6,8,15,15,15,15,15,15,15,15,2,0,6,8,15,15,15,15,15,15,15,15,10,10,10,10,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

const uint8_t SevenSprite::rgbColors[] PROGMEM = {253,196,0,194,137,0,228,66,0,254,54,0,253,53,0,251,49,0,252,44,0,247,27,0,229,19,0,207,12,0,184,12,0,105,5,0,9,2,0,28,0,0,2,0,1,0,0,0};
