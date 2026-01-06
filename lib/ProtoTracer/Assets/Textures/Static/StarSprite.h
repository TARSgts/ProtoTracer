#pragma once

#include "..\..\Materials\Image.h"

class StarSprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	StarSprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 12, 12, 16) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t StarSprite::rgbMemory[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,8,15,15,15,15,15,15,15,15,15,15,12,2,9,15,15,15,15,15,15,15,15,15,12,0,5,13,15,15,15,15,15,14,5,0,0,0,3,3,2,7,14,15,15,15,15,6,1,1,4,6,8,15,15,15,15,15,15,9,2,8,7,6,15,15,15,15,15,15,13,4,4,11,9,5,9,15,15,15,15,15,11,6,12,15,12,11,8,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

const uint8_t StarSprite::rgbColors[] PROGMEM = {252,247,0,255,236,0,250,226,0,255,219,0,251,210,0,253,198,0,249,184,0,229,157,0,225,133,0,142,88,0,73,38,0,22,14,0,2,4,2,23,0,1,0,0,1,0,0,0};
