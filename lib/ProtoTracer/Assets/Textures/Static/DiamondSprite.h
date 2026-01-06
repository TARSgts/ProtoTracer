#pragma once

#include "..\..\Materials\Image.h"

class DiamondSprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	DiamondSprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 12, 12, 16) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t DiamondSprite::rgbMemory[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,13,3,3,2,2,4,7,14,15,15,15,11,2,1,1,1,0,2,4,0,13,15,12,4,5,2,5,5,6,6,9,8,9,15,15,12,8,4,6,0,0,9,5,7,12,15,15,15,12,5,6,0,9,5,9,11,15,15,15,15,15,13,7,7,7,10,12,15,15,15,15,15,15,15,14,8,10,15,15,15,15,15,15,15,15,15,15,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

const uint8_t DiamondSprite::rgbColors[] PROGMEM = {0,250,253,0,243,243,0,241,248,0,238,247,0,230,249,0,217,243,0,203,232,0,162,191,0,120,141,0,92,109,0,64,76,0,16,11,0,6,6,0,2,2,0,1,1,0,0,0};
