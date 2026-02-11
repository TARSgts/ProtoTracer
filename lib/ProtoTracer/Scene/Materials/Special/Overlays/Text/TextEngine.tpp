#include "TextEngine.h"

template<uint8_t lineCount, uint8_t characterWidth>
TextEngine<lineCount, characterWidth>::TextEngine(bool isEfficient) {
    this->isEfficient = isEfficient;

    ClearText(); //init to spaces
}

template<uint8_t lineCount, uint8_t characterWidth>
TextEngine<lineCount, characterWidth>::TextEngine(Vector2D size, Vector2D position, uint16_t blinkTime, bool isEfficient) {
    this->size = size;
    this->positionOffset = position;
    this->blinkTime = blinkTime;
    this->isEfficient = isEfficient;

    ClearText(); //init to spaces
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetMaterial(Material* material) {
    this->material = material;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetSize(Vector2D size) {
    this->size = size;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetPositionOffset(Vector2D positionOffset) {
    this->positionOffset = positionOffset;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetRotationOffset(Vector2D rotationOffset) {
    this->rotationOffset = rotationOffset;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetRotationAngle(float rotationAngle) {
    this->rotationAngle = rotationAngle;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetBlinkTime(uint16_t blinkTime) {
    this->blinkTime = blinkTime;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetAntiAliasing(bool enabled) {
    useAntiAliasing = enabled;
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::SetText(uint8_t line, String value, bool centerText) {
    uint8_t length = value.length();
    int spacing = centerText ? (length - characterWidth) / 2 : 0;

    if (line > lineCount || spacing < 0 || length > characterWidth) return; //break when outside of bounds

    for (int i = spacing; i < length + spacing; i++) {
        lines[line][i] = value[i - spacing];
    }
}

template<uint8_t lineCount, uint8_t characterWidth>
void TextEngine<lineCount, characterWidth>::ClearText() {
    for (uint8_t i = 0; i < lineCount; i++) {
        for (uint8_t j = 0; j < characterWidth; j++) {
            lines[i][j] = ' ';
        }
    }
}

template<uint8_t lineCount, uint8_t characterWidth>
RGBColor TextEngine<lineCount, characterWidth>::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector3D positionL = position;
    
    if(rotationAngle != 0 && !isEfficient){
        positionL = positionL - Vector3D(rotationOffset.X, rotationOffset.Y, 0);

        Quaternion temp = Rotation(EulerAngles(Vector3D(0, 0, rotationAngle), EulerConstants::EulerOrderXYZS)).GetQuaternion();
        
        positionL = temp.RotateVector(positionL);// rotate

        positionL = positionL + Vector3D(rotationOffset.X, rotationOffset.Y, 0);
    }
    else{
        Vector2D tempPos = positionL;

        if(Mathematics::IsClose(int(rotationAngle) % 360, 90.0f, 45.0f)){
            positionL.X = tempPos.Y;
            positionL.Y = -tempPos.X;
        }
        else if (Mathematics::IsClose(int(rotationAngle) % 360, 180.0f, 45.0f)){
            positionL.X = -tempPos.X;
            positionL.Y = -tempPos.Y;
        }
        else if (Mathematics::IsClose(int(rotationAngle) % 360, 270.0f, 45.0f)){
            positionL.X = -tempPos.Y;
            positionL.Y = tempPos.X;
        }
    }

    positionL = positionL - Vector3D(positionOffset.X, positionOffset.Y, 0);//offset position
    
    float xF = Mathematics::Map(positionL.X, 0.0f, size.X, characterWidth * 10.0f, 0.0f);
    float yF = Mathematics::Map(positionL.Y, 0.0f, size.Y, lineCount * 10.0f, 0.0f);

    int x = floorf(xF);
    int y = floorf(yF);

    if(x < 0 || x >= characterWidth * 10 || y < 0 || y >= lineCount * 10) return black;

    const int charColumn = x / 10;
    const int charRow = y / 10;
    char searchChar = lines[charRow][charColumn];
    bool blink = millis() % (blinkTime * 2) > blinkTime;
    bool selected = searchChar > 90;

    RGBColor baseColor = material->GetRGB(positionL, normal, uvw);
    RGBColor selectedColor = baseColor.HueShift(180);
    RGBColor foregroundColor = selected ? selectedColor : baseColor;
    RGBColor backgroundColor = black;

    if (selected && blink) {
        foregroundColor = black;
        backgroundColor = selectedColor;
    }

    float coverage = 0.0f;

    if (useAntiAliasing) {
        const uint8_t* glyph = Characters::GetCharacter(searchChar);
        const float sampleOffsets[4][2] = {
            {-0.25f, -0.25f},
            { 0.25f, -0.25f},
            {-0.25f,  0.25f},
            { 0.25f,  0.25f}
        };

        float localXCenter = xF - (charColumn * 10.0f);
        float localYCenter = yF - (charRow * 10.0f);
        float coverageAccum = 0.0f;

        for (uint8_t i = 0; i < 4; i++) {
            float sampleX = localXCenter + sampleOffsets[i][0];
            float sampleY = localYCenter + sampleOffsets[i][1];

            if (sampleX < 1.0f || sampleX >= 9.0f || sampleY < 1.0f || sampleY >= 9.0f) {
                continue;
            }

            uint8_t glyphX = (uint8_t)floorf(sampleX) - 1;
            uint8_t glyphY = (uint8_t)floorf(sampleY) - 1;
            uint8_t rowBits = glyph[glyphY];
            bool bitOn = (rowBits >> (7 - glyphX)) & 0x01;

            coverageAccum += bitOn ? 1.0f : 0.0f;
        }

        coverage = coverageAccum * 0.25f;
    } else {
        //bit
        uint8_t charXBit = 9 - (x % 10);
        uint8_t charYBit = y % 10;

        if(charYBit == 0 || charYBit == 9 || charXBit == 0 || charXBit == 9){//margin
            coverage = 0.0f;
        }
        else{
            uint8_t yCharacter = Characters::GetCharacter(searchChar)[charYBit - 1];//get character
            bool xBit = 1 & (yCharacter >> (charXBit - 1));
            coverage = xBit ? 1.0f : 0.0f;
        }
    }

    return RGBColor::InterpolateColors(backgroundColor, foregroundColor, coverage);
}
