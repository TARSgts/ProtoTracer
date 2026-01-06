#pragma once

#include "..\..\Materials\Image.h"

class BellSprite : public Image{
private:
	static const uint8_t rgbMemory[];
	static const uint8_t rgbColors[];

public:
	BellSprite(Vector2D size, Vector2D offset) : Image(rgbMemory, rgbColors, 12, 12, 16) {
		SetSize(size);
		SetPosition(offset);
	}
};

const uint8_t BellSprite::rgbMemory[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,12,0,14,15,15,15,15,15,15,15,15,13,7,2,6,11,15,15,15,15,15,15,15,14,0,1,3,6,15,15,15,15,15,15,15,6,0,1,2,5,15,15,15,15,15,15,12,5,1,2,4,5,14,15,15,15,15,9,3,0,3,3,4,5,6,11,15,15,15,9,3,10,11,0,14,11,10,10,15,15,15,15,14,8,8,4,7,8,8,12,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

const uint8_t BellSprite::rgbColors[] PROGMEM = {254,245,0,252,236,0,252,212,0,249,182,0,236,161,0,226,143,0,141,78,0,46,26,0,23,10,0,3,14,0,10,5,0,6,4,0,2,4,5,1,0,69,1,0,0,0,0,0};
