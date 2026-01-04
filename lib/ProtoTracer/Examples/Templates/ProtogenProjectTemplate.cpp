#include "ProtogenProjectTemplate.h"

void ProtogenProject::LinkParameters(){
    eEA.AddParameter(&offsetFace, offsetFaceInd, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceSA, offsetFaceIndSA, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceARG, offsetFaceIndARG, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceOSC, offsetFaceIndOSC, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceSort, offsetFaceIndSort, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFacePong, offsetFaceIndPong, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceInvaders, offsetFaceIndInvaders, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceFlappy, offsetFaceIndFlappy, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFaceSnake, offsetFaceIndSnake, 40, 0.0f, 1.0f);
    eEA.AddParameter(&offsetFacePacman, offsetFaceIndPacman, 40, 0.0f, 1.0f);
#ifdef ENABLE_BAD_APPLE_FACE
    eEA.AddParameter(&offsetFaceBadApple, offsetFaceIndBadApple, 40, 0.0f, 1.0f);
#endif
}

void ProtogenProject::SetBaseMaterial(Material* material){
    materialAnimator.SetBaseMaterial(Material::Add, material);
}

void ProtogenProject::SetMaterialLayers(){
    materialAnimator.SetBaseMaterial(Material::Add, &gradientMat);
    materialAnimator.AddMaterial(Material::Replace, &yellowMaterial, 40, 0.0f, 1.0f);//layer 1
    materialAnimator.AddMaterial(Material::Replace, &orangeMaterial, 40, 0.0f, 1.0f);//layer 2
    materialAnimator.AddMaterial(Material::Replace, &whiteMaterial, 40, 0.0f, 1.0f);//layer 3
    materialAnimator.AddMaterial(Material::Replace, &greenMaterial, 40, 0.0f, 1.0f);//layer 4
    materialAnimator.AddMaterial(Material::Replace, &purpleMaterial, 40, 0.0f, 1.0f);//layer 5
    materialAnimator.AddMaterial(Material::Replace, &redMaterial, 40, 0.0f, 1.0f);//layer 6
    materialAnimator.AddMaterial(Material::Replace, &blueMaterial, 40, 0.0f, 1.0f);//layer 7
    materialAnimator.AddMaterial(Material::Replace, &flowNoise, 40, 0.15f, 1.0f);//layer 8
    materialAnimator.AddMaterial(Material::Replace, &rainbowSpiral, 40, 0.0f, 1.0f);//layer 9
    materialAnimator.AddMaterial(Material::Replace, &hRainbow, 40, 0.0f, 1.0f);//layer 10
    materialAnimator.AddMaterial(Material::Replace, &blackMaterial, 40, 0.0f, 1.0f);//layer 11
    materialAnimator.AddMaterial(Material::Replace, &sA, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &aRG, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &oSC, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &mergeSort, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &pong, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &spaceInvaders, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &flappyBird, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &snake, 20, 0.0f, 1.0f);
    materialAnimator.AddMaterial(Material::Replace, &pacman, 20, 0.0f, 1.0f);
#ifdef ENABLE_BAD_APPLE_FACE
    materialAnimator.AddMaterial(Material::Replace, &badApple, 20, 0.0f, 1.0f);
#endif

    backgroundMaterial.SetBaseMaterial(Material::Add, Menu::GetMaterial());
    backgroundMaterial.AddMaterial(Material::Replace, &yellowMaterial, 40, 0.0f, 1.0f);//layer 1
    backgroundMaterial.AddMaterial(Material::Replace, &orangeMaterial, 40, 0.0f, 1.0f);//layer 2
    backgroundMaterial.AddMaterial(Material::Replace, &whiteMaterial, 40, 0.0f, 1.0f);//layer 3
    backgroundMaterial.AddMaterial(Material::Replace, &greenMaterial, 40, 0.0f, 1.0f);//layer 4
    backgroundMaterial.AddMaterial(Material::Replace, &purpleMaterial, 40, 0.0f, 1.0f);//layer 5
    backgroundMaterial.AddMaterial(Material::Replace, &redMaterial, 40, 0.0f, 1.0f);//layer 6
    backgroundMaterial.AddMaterial(Material::Replace, &blueMaterial, 40, 0.0f, 1.0f);//layer 7
    backgroundMaterial.AddMaterial(Material::Replace, &flowNoise, 40, 0.0f, 1.0f);//layer 8
    backgroundMaterial.AddMaterial(Material::Replace, &rainbowSpiral, 40, 0.0f, 1.0f);//layer 9
    backgroundMaterial.AddMaterial(Material::Replace, &hRainbow, 40, 0.0f, 1.0f);//layer 10
    backgroundMaterial.AddMaterial(Material::Replace, &blackMaterial, 40, 0.0f, 1.0f);//layer 11
    backgroundMaterial.AddMaterial(Material::Add, &sA, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &aRG, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &oSC, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &mergeSort, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &pong, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &spaceInvaders, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &flappyBird, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &snake, 20, 0.0f, 1.0f);
    backgroundMaterial.AddMaterial(Material::Add, &pacman, 20, 0.0f, 1.0f);
#ifdef ENABLE_BAD_APPLE_FACE
    backgroundMaterial.AddMaterial(Material::Add, &badApple, 20, 0.0f, 1.0f);
#endif
}

void ProtogenProject::UpdateKeyFrameTracks(){
    blink.Update();
}

void ProtogenProject::UpdateFFTVisemes(){
    if(Menu::UseMicrophone()){
        float* fftData = MicrophoneFourier::GetFourierFiltered();

        constexpr uint8_t lowBandBins = 5;   // ~0-250 Hz
        constexpr uint8_t speechBandEnd = 64;

        float lowBandAvg = 0.0f;
        float lowBandPeak = 0.0f;
        for(uint8_t i = 0; i < lowBandBins; ++i){
            float bin = fftData[i];
            lowBandAvg += bin;
            if(bin > lowBandPeak)
                lowBandPeak = bin;
        }
        lowBandAvg /= lowBandBins;

        float speechBandAvg = 0.0f;
        float speechBandPeak = 0.0f;
        uint8_t speechBins = speechBandEnd - lowBandBins;
        for(uint8_t i = lowBandBins; i < speechBandEnd; ++i){
            float bin = fftData[i];
            speechBandAvg += bin;
            if(bin > speechBandPeak)
                speechBandPeak = bin;
        }
        speechBandAvg /= float(speechBins);

        float sensitivity = static_cast<float>(Menu::GetMicLevel()) / 10.0f;
        float lowBandWeight = 0.35f + (1.0f - sensitivity) * 0.5f;
        float speechEnergy = speechBandPeak * 0.7f + speechBandAvg * 0.3f;
        float noiseEnergy = lowBandPeak * 0.6f + lowBandAvg * 0.4f;
        float mouthMagnitude = speechEnergy - noiseEnergy * lowBandWeight;
        if(mouthMagnitude < 0.0f)
            mouthMagnitude = 0.0f;

        mouthMagnitude *= 1.2f + sensitivity * 0.6f;
        if(mouthMagnitude > 1.0f)
            mouthMagnitude = 1.0f;

        static float mouthFiltered = 0.0f;
        float attack = 0.35f + sensitivity * 0.4f;
        float release = 0.08f + (1.0f - sensitivity) * 0.2f;
        if(mouthMagnitude > mouthFiltered)
            mouthFiltered += (mouthMagnitude - mouthFiltered) * attack;
        else
            mouthFiltered += (mouthMagnitude - mouthFiltered) * release;
        if(mouthFiltered < 0.0f)
            mouthFiltered = 0.0f;
        if(mouthFiltered > 1.0f)
            mouthFiltered = 1.0f;

        eEA.AddParameterFrame(Viseme::SS + 100, mouthFiltered);

        if(mouthFiltered > 0.08f){
            voiceDetection.Update(MicrophoneFourier::GetFourierFiltered(), MicrophoneFourier::GetSampleRate());
    
            eEA.AddParameterFrame(Viseme::EE + 100, voiceDetection.GetViseme(Viseme::EE));
            eEA.AddParameterFrame(Viseme::AH + 100, voiceDetection.GetViseme(Viseme::AH));
            eEA.AddParameterFrame(Viseme::UH + 100, voiceDetection.GetViseme(Viseme::UH));
            eEA.AddParameterFrame(Viseme::AR + 100, voiceDetection.GetViseme(Viseme::AR));
            eEA.AddParameterFrame(Viseme::ER + 100, voiceDetection.GetViseme(Viseme::ER));
            eEA.AddParameterFrame(Viseme::OO + 100, voiceDetection.GetViseme(Viseme::OO));
        }
    }
}

void ProtogenProject::SetMaterialColor(){
    switch(Menu::GetFaceColor()){
        case 1: materialAnimator.AddMaterialFrame(yellowMaterial, 0.8f); break;
        case 2: materialAnimator.AddMaterialFrame(orangeMaterial, 0.8f); break;
        case 3: materialAnimator.AddMaterialFrame(whiteMaterial, 0.8f); break;
        case 4: materialAnimator.AddMaterialFrame(greenMaterial, 0.8f); break;
        case 5: materialAnimator.AddMaterialFrame(purpleMaterial, 0.8f); break;
        case 6: materialAnimator.AddMaterialFrame(redMaterial, 0.8f); break;
        case 7: materialAnimator.AddMaterialFrame(blueMaterial, 0.8f); break;
        case 8: materialAnimator.AddMaterialFrame(rainbowSpiral, 0.8f); break;
        case 9: materialAnimator.AddMaterialFrame(flowNoise, 0.8f); break;
        case 10: materialAnimator.AddMaterialFrame(hRainbow, 0.8f); break;
        case 11: materialAnimator.AddMaterialFrame(blackMaterial, 0.8f); break;
        default: break;
    }
}

#ifdef ENABLE_FACE_COLOR_STRIP
RGBColor ProtogenProject::GetSolidFaceColor(Color faceColor) const {
    switch(faceColor) {
        case CYELLOW: return RGBColor(255, 255, 0);
        case CORANGE: return RGBColor(255, 165, 0);
        case CWHITE:  return RGBColor(255, 255, 255);
        case CGREEN:  return RGBColor(0, 255, 0);
        case CPURPLE: return RGBColor(255, 0, 255);
        case CRED:    return RGBColor(255, 0, 0);
        case CBLUE:   return RGBColor(0, 0, 255);
        case CBLACK:  return RGBColor(0, 0, 0);
        default:      return RGBColor(0, 0, 0);
    }
}

void ProtogenProject::ApplyFaceStripColor(Color color, float ratio, const RGBColor& hueFront, const RGBColor& hueBack) {
    switch(color) {
        case CBASE:
            faceColorStrip.SetGradient(hueFront, hueBack);
            break;
        case CYELLOW:
        case CORANGE:
        case CWHITE:
        case CGREEN:
        case CPURPLE:
        case CRED:
        case CBLUE:
        case CBLACK:
            faceColorStrip.SetSolidColor(GetSolidFaceColor(color));
            break;
        case CRAINBOW:
            faceColorStrip.ShowRainbow(ratio, 1.0f);
            break;
        case CRAINBOWNOISE:
            faceColorStrip.ShowRainbow(ratio, 0.35f);
            break;
        case CHORIZONTALRAINBOW:
            faceColorStrip.ShowRainbow(ratio, 1.5f);
            break;
        default:
            faceColorStrip.SetGradient(hueFront, hueBack);
            break;
    }
}

ProtogenProject::Color ProtogenProject::ConvertMenuColor(uint8_t menuColor) const {
    if (menuColor > CBLACK) return CBASE;
    return static_cast<Color>(menuColor);
}

void ProtogenProject::SetStripColorOverride(Color color) {
    pendingStripColor = color;
    stripColorOverridePending = true;
}

void ProtogenProject::ClearStripColorOverride() {
    stripColorOverridePending = false;
    stripColorInitialized = false;
}

void ProtogenProject::UpdateFaceColorStrip(float ratio, const RGBColor& hueFront, const RGBColor& hueBack) {
    bool shouldApply = false;
    Color colorSelection = lastAppliedStripColor;

    if (stripColorOverridePending) {
        colorSelection = pendingStripColor;
        stripColorOverridePending = false;
        shouldApply = true;
    } else {
        uint8_t menuColor = Menu::GetFaceColor();
        if (!stripColorInitialized || menuColor != lastMenuFaceColor) {
            colorSelection = ConvertMenuColor(menuColor);
            lastMenuFaceColor = menuColor;
            shouldApply = true;
        }
    }

    if (shouldApply) {
        ApplyFaceStripColor(colorSelection, ratio, hueFront, hueBack);
        lastAppliedStripColor = colorSelection;
        stripColorInitialized = true;
    }
}
#endif

void ProtogenProject::UpdateFace(float ratio) {
    while(!frameLimiter.IsReady()) delay(1);

    Menu::Update(ratio);

    uint8_t fanMenuValue = Menu::GetFanSpeed();
    if (fanMenuValue > 9) fanMenuValue = 9;
    uint8_t fanPwm = static_cast<uint8_t>((static_cast<uint16_t>(fanMenuValue) * 255 + 4) / 9);
    fanController.SetPWM(fanPwm);
    
    xOffset = fGenMatXMove.Update();
    yOffset = fGenMatYMove.Update();
    
    if (Menu::UseBoopSensor()) {
        isBooped = boop.isBooped();

        static bool lastTTPState = false;
        const bool ttpActive = digitalRead(TTP223_PIN) == (TTP223_ACTIVE_HIGH ? HIGH : LOW);

        if (ttpActive != lastTTPState) {
            lastTTPState = ttpActive;

            if (Serial) {
                Serial.print(F("TTP223: "));
                Serial.println(ttpActive ? F("TOUCHED") : F("released"));
            }
        }

        if (ttpActive) {
            isBooped = true;
        }
    }

    uint32_t now = millis();
    if (isBooped) {
        if (!boopHoldActive) {
            boopHoldActive = true;
            boopHoldTriggered = false;
            boopHoldStartMs = now;
        }
        if (!boopHoldTriggered) {
            float heldSeconds = (now - boopHoldStartMs) / 1000.0f;
            if (heldSeconds >= 7.5f) {
                boopHoldTriggered = true;
                boopHoldLatchUntilMs = now + 6000;
            }
        }
    } else {
        boopHoldActive = false;
        boopHoldTriggered = false;
        boopHoldStartMs = 0;
    }

    hud.SetEffect(Menu::GetEffect());// Pull Effect from menu and store reference in hud for observing data
    hud.Update();
    this->scene.SetEffect(&hud);// Use HUD as effect for overlay/data extraction

    voiceDetection.SetThreshold(map(Menu::GetMicLevel(), 0, 10, 1000, 50));
    UpdateFFTVisemes();

    MicrophoneFourier::Update();

    sA.SetHueAngle(ratio * 360.0f * 4.0f);
    sA.SetMirrorYState(Menu::MirrorSpectrumAnalyzer());
    sA.SetFlipYState(!Menu::MirrorSpectrumAnalyzer());
    
    aRG.SetRadius((xOffset + 2.0f) * 2.0f + 25.0f);
    aRG.SetSize(Vector2D((xOffset + 2.0f) * 10.0f + 50.0f, (xOffset + 2.0f) * 10.0f + 50.0f));
    aRG.SetHueAngle(ratio * 360.0f * 8.0f);
    aRG.SetRotation(ratio * 360.0f * 2.0f);

    oSC.SetHueAngle(ratio * 360.0f * 8.0f);
    
    SetMaterialColor();
    RGBColor hueFront = RGBColor(255, 0, 0).HueShift(Menu::GetHueF() * 36);
    RGBColor hueBack  = RGBColor(255, 0, 0).HueShift(Menu::GetHueB() * 36);

    gradientSpectrum[0] = hueFront;
    gradientSpectrum[1] = hueBack;
    gradientMat.UpdateGradient(gradientSpectrum);

    flowNoise.SetGradient(hueFront, 0);
    flowNoise.SetGradient(hueBack, 1);

#ifdef ENABLE_FACE_COLOR_STRIP
    UpdateFaceColorStrip(ratio, hueFront, hueBack);
#endif

    UpdateKeyFrameTracks();

    eEA.Update();
    
    flowNoise.Update(ratio);
    rainbowSpiral.Update(ratio);
    hRainbow.Update(ratio);
    materialAnimator.Update();
    backgroundMaterial.Update();

    uint8_t faceSize = Menu::GetFaceSize();
    float scale = Menu::ShowMenu() * 0.6f + 0.4f;
    float faceSizeOffset = faceSize * (cameraSize.X / 20.0f);// /2 for min of half size, /10 for 10 face size options
    float faceSizeMaxX = cameraSize.X / 2.0f;

    float xMaxCamera = cameraSize.X - faceSizeMaxX + faceSizeOffset;
    
    aRG.SetPosition(Vector2D(xMaxCamera / 2.0f + xOffset * 4.0f, cameraSize.Y / 2.0f + yOffset * 4.0f));

    objA.SetCameraMax(Vector2D(xMaxCamera, cameraSize.Y - cameraSize.Y * offsetFace).Multiply(scale));

#ifdef ENABLE_BAD_APPLE_FACE
    ApplyBadAppleUsage();
#endif
}


void ProtogenProject::SetCameraMain(Vector2D min, Vector2D max){
    this->camMin = min;
    this->camMax = max;

    objA.SetCameraMin(camMin);
    objA.SetCameraMax(camMax);
}

void ProtogenProject::SetCameraRear(Vector2D min, Vector2D max){
    this->camMinRear = min;
    this->camMaxRear = max;

    objARear.SetCameraMin(camMinRear);
    objARear.SetCameraMax(camMaxRear);
}

Transform ProtogenProject::GetAlignmentTransform(Vector2D min, Vector2D max, Object3D* obj, float rotation, float margin){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    
    return objAOther.GetTransform(obj);
}

Transform ProtogenProject::GetAlignmentTransform(Vector2D min, Vector2D max, Object3D** objects, uint8_t objectCount, float rotation, float margin){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    
    return objAOther.GetTransform(objects, objectCount);
}

void ProtogenProject::AlignObject(Vector2D min, Vector2D max, Object3D* obj, float rotation, float margin, bool mirror){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    objAOther.AlignObject(obj);
    
    objAOther.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjects(Vector2D min, Vector2D max, Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    objAOther.AlignObjects(objects, objectCount);
    
    objAOther.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectNoScale(Vector2D min, Vector2D max, Object3D* obj, float rotation, float margin, bool mirror){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    objAOther.AlignObjectNoScale(obj);
    
    objAOther.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectsNoScale(Vector2D min, Vector2D max, Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objAOther.SetCameraMin(min);
    objAOther.SetCameraMax(max);

    objAOther.SetPlaneOffsetAngle(rotation);
    objAOther.SetEdgeMargin(margin);
    objAOther.AlignObjectsNoScale(objects, objectCount);
    
    objAOther.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectFace(Object3D* obj, float rotation, float margin, bool mirror){
    objA.SetPlaneOffsetAngle(rotation);
    objA.SetEdgeMargin(margin);
    objA.AlignObject(obj);
    objA.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectsFace(Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objA.SetPlaneOffsetAngle(rotation);
    objA.SetEdgeMargin(margin);
    objA.AlignObjects(objects, objectCount);
    objA.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectNoScaleFace(Object3D* obj, float rotation, float margin, bool mirror){
    objA.SetPlaneOffsetAngle(rotation);
    objA.SetEdgeMargin(margin);
    objA.AlignObjectNoScale(obj);
    objA.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectsNoScaleFace(Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objA.SetPlaneOffsetAngle(rotation);
    objA.SetEdgeMargin(margin);
    objA.AlignObjectsNoScale(objects, objectCount);
    objA.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectRear(Object3D* obj, float rotation, float margin, bool mirror){
    objARear.SetPlaneOffsetAngle(rotation);
    objARear.SetEdgeMargin(margin);
    objARear.AlignObject(obj);
    objARear.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectsRear(Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objARear.SetPlaneOffsetAngle(rotation);
    objARear.SetEdgeMargin(margin);
    objARear.AlignObjects(objects, objectCount);
    objARear.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectNoScaleRear(Object3D* obj, float rotation, float margin, bool mirror){
    objARear.SetPlaneOffsetAngle(rotation);
    objARear.SetEdgeMargin(margin);
    objARear.AlignObjectNoScale(obj);
    objARear.SetMirrorX(mirror);
}

void ProtogenProject::AlignObjectsNoScaleRear(Object3D** objects, uint8_t objectCount, float rotation, float margin, bool mirror){
    objARear.SetPlaneOffsetAngle(rotation);
    objARear.SetEdgeMargin(margin);
    objARear.AlignObjectsNoScale(objects, objectCount);
    objARear.SetMirrorX(mirror);
}


ObjectAlign* ProtogenProject::GetObjectAlign(){
    return &objAOther;
}

ObjectAlign* ProtogenProject::GetObjectAlignFace(){
    return &objA;
}

ObjectAlign* ProtogenProject::GetObjectAlignRear(){
    return &objARear;
}

Vector2D ProtogenProject::GetCameraSize() const {
    return cameraSize;
}

float ProtogenProject::GetFaceScale(){
    uint8_t faceSize = Menu::GetFaceSize();

    float xSizeRatio = 1.0f - 0.5f + faceSize * (1.0f / 20.0f);

    return xSizeRatio;
}

void ProtogenProject::AddParameter(uint8_t index, float* parameter, uint16_t transitionFrames, IEasyEaseAnimator::InterpolationMethod interpolationMethod, bool invertDirection){
    if(invertDirection){
        eEA.AddParameter(parameter, index, transitionFrames, 1.0f, 0.0f);
    }
    else{
        eEA.AddParameter(parameter, index, transitionFrames, 0.0f, 1.0f);
    }

    eEA.SetInterpolationMethod(index, interpolationMethod);
}

void ProtogenProject::AddViseme(Viseme::MouthShape visemeName, float* parameter){
    constexpr uint16_t visemeTransitionFrames = 6;
    eEA.AddParameter(parameter, visemeName + 100, visemeTransitionFrames, 0.0f, 1.0f);

    eEA.SetInterpolationMethod(visemeName + 100, IEasyEaseAnimator::Cosine);
}

void ProtogenProject::AddBlinkParameter(float* blinkParameter){
    blink.AddParameter(blinkParameter);

    blinkSet = true;
}

void ProtogenProject::AddParameterFrame(uint16_t ProjectIndex, float target){
    eEA.AddParameterFrame(ProjectIndex, target);
}

void ProtogenProject::AddMaterial(Material::Method method, Material* material, uint16_t frames, float minOpacity, float maxOpacity){
    materialAnimator.AddMaterial(method, material, frames, minOpacity, maxOpacity);
}

void ProtogenProject::AddMaterialFrame(Color color, float opacity){
    switch(color){
        case CYELLOW:
            materialAnimator.AddMaterialFrame(yellowMaterial, opacity);
            break;
        case CORANGE:
            materialAnimator.AddMaterialFrame(orangeMaterial, opacity);
            break;
        case CWHITE:
            materialAnimator.AddMaterialFrame(whiteMaterial, opacity);
            break;
        case CGREEN:
            materialAnimator.AddMaterialFrame(greenMaterial, opacity);
            break;
        case CPURPLE:
            materialAnimator.AddMaterialFrame(purpleMaterial, opacity);
            break;
        case CRED:
            materialAnimator.AddMaterialFrame(redMaterial, opacity);
            break;
        case CBLUE:
            materialAnimator.AddMaterialFrame(blueMaterial, opacity);
            break;
        case CRAINBOW:
            materialAnimator.AddMaterialFrame(rainbowSpiral, opacity);
            break;
        case CRAINBOWNOISE:
            materialAnimator.AddMaterialFrame(flowNoise, opacity);
            break;
        case CHORIZONTALRAINBOW:
            materialAnimator.AddMaterialFrame(hRainbow, opacity);
            break;
        case CBLACK:
            materialAnimator.AddMaterialFrame(blackMaterial, opacity);
            break;
        default:
            break;
    }

#ifdef ENABLE_FACE_COLOR_STRIP
    pendingStripColor = color;
    stripColorOverridePending = true;
#endif
}

void ProtogenProject::ApplyMenuOrDefaultColor(Color color, float opacity) {
    if (Menu::GetFaceColor() == 0) {
        AddMaterialFrame(color, opacity);
    }
}

void ProtogenProject::AddMaterialFrame(Material& material, float opacity){
    materialAnimator.AddMaterialFrame(material, opacity);
}

void ProtogenProject::AddBackgroundMaterial(Material::Method method, Material* material, uint16_t frames, float minOpacity, float maxOpacity){
    backgroundMaterial.AddMaterial(method, material, frames, minOpacity, maxOpacity);
}

void ProtogenProject::AddBackgroundMaterialFrame(Color color, float opacity){
    switch(color){
        case CYELLOW:
            backgroundMaterial.AddMaterialFrame(yellowMaterial, opacity);
            break;
        case CORANGE:
            backgroundMaterial.AddMaterialFrame(orangeMaterial, opacity);
            break;
        case CWHITE:
            backgroundMaterial.AddMaterialFrame(whiteMaterial, opacity);
            break;
        case CGREEN:
            backgroundMaterial.AddMaterialFrame(greenMaterial, opacity);
            break;
        case CPURPLE:
            backgroundMaterial.AddMaterialFrame(purpleMaterial, opacity);
            break;
        case CRED:
            backgroundMaterial.AddMaterialFrame(redMaterial, opacity);
            break;
        case CBLUE:
            backgroundMaterial.AddMaterialFrame(blueMaterial, opacity);
            break;
        case CRAINBOW:
            backgroundMaterial.AddMaterialFrame(rainbowSpiral, opacity);
            break;
        case CRAINBOWNOISE:
            backgroundMaterial.AddMaterialFrame(flowNoise, opacity);
            break;
        case CHORIZONTALRAINBOW:
            backgroundMaterial.AddMaterialFrame(hRainbow, opacity);
            break;
        case CBLACK:
            backgroundMaterial.AddMaterialFrame(blackMaterial, opacity);
            break;
        default:
            break;
    }
}

void ProtogenProject::AddBackgroundMaterialFrame(Material& material, float opacity){
    backgroundMaterial.AddMaterialFrame(material, opacity);
}

#ifdef ENABLE_BAD_APPLE_FACE
void ProtogenProject::SetBadAppleActive(bool active) {
    badApple.SetActive(active);
}

void ProtogenProject::ResetBadAppleUsage() {
    badAppleUsedThisFrame = false;
}

void ProtogenProject::ApplyBadAppleUsage() {
    SetBadAppleActive(badAppleUsedThisFrame);
}
#endif

void ProtogenProject::SpectrumAnalyzerFace(){
    sA.Update(MicrophoneFourier::GetWaveform());

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndSA, 1.0f);

    materialAnimator.AddMaterialFrame(sA, offsetFaceSA);
    backgroundMaterial.AddMaterialFrame(sA, offsetFaceSA);

    SpectrumAnalyzerCallback();
}

void ProtogenProject::AudioReactiveGradientFace(){
    aRG.Update(MicrophoneFourier::GetFourierFiltered());

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndARG, 1.0f);

    materialAnimator.AddMaterialFrame(aRG, offsetFaceARG);
    backgroundMaterial.AddMaterialFrame(aRG, offsetFaceARG);

    AudioReactiveGradientCallback();
}

void ProtogenProject::OscilloscopeFace(){
    oSC.Update(MicrophoneFourier::GetSamples());

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndOSC, 1.0f);

    materialAnimator.AddMaterialFrame(oSC, offsetFaceOSC);
    backgroundMaterial.AddMaterialFrame(oSC, offsetFaceOSC);

    OscilloscopeCallback();
}

void ProtogenProject::MergeSortFace(){
    mergeSort.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndSort, 1.0f);

    materialAnimator.AddMaterialFrame(mergeSort, offsetFaceSort);
    backgroundMaterial.AddMaterialFrame(mergeSort, offsetFaceSort);

    MergeSortCallback();
}

void ProtogenProject::PongAutoFace(){
    pong.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndPong, 1.0f);

    materialAnimator.AddMaterialFrame(pong, offsetFacePong);
    backgroundMaterial.AddMaterialFrame(pong, offsetFacePong);
}

void ProtogenProject::SpaceInvadersAutoFace(){
    spaceInvaders.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndInvaders, 1.0f);

    materialAnimator.AddMaterialFrame(spaceInvaders, offsetFaceInvaders);
    backgroundMaterial.AddMaterialFrame(spaceInvaders, offsetFaceInvaders);
}

void ProtogenProject::FlappyBirdAutoFace(){
    flappyBird.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndFlappy, 1.0f);

    materialAnimator.AddMaterialFrame(flappyBird, offsetFaceFlappy);
    backgroundMaterial.AddMaterialFrame(flappyBird, offsetFaceFlappy);
}

void ProtogenProject::SnakeAutoFace(){
    snake.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndSnake, 1.0f);

    materialAnimator.AddMaterialFrame(snake, offsetFaceSnake);
    backgroundMaterial.AddMaterialFrame(snake, offsetFaceSnake);
}

void ProtogenProject::PacmanAutoFace(){
    pacman.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndPacman, 1.0f);

    materialAnimator.AddMaterialFrame(pacman, offsetFacePacman);
    backgroundMaterial.AddMaterialFrame(pacman, offsetFacePacman);
}

#ifdef ENABLE_BAD_APPLE_FACE
void ProtogenProject::BadAppleFace(){
    badAppleUsedThisFrame = true;
    SetBadAppleActive(true);
    badApple.Update();

    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
    eEA.AddParameterFrame(offsetFaceIndBadApple, 1.0f);

    materialAnimator.AddMaterialFrame(badApple, offsetFaceBadApple);
    backgroundMaterial.AddMaterialFrame(badApple, offsetFaceBadApple);
}
#endif

void ProtogenProject::HideFace(){
    eEA.AddParameterFrame(offsetFaceInd, 1.0f);
}

void ProtogenProject::DisableBlinking(){
    blink.Pause();
    blink.Reset();
}

void ProtogenProject::EnableBlinking(){
    blink.Reset();
    blink.Play();
}

bool ProtogenProject::IsBooped(){
    return isBooped;
}

bool ProtogenProject::IsBoopHeldFor(float seconds) const{
    if (!boopHoldActive) return false;
    uint32_t now = millis();
    float heldSeconds = (now - boopHoldStartMs) / 1000.0f;
    return heldSeconds >= seconds;
}

bool ProtogenProject::IsBoopBSODActive() const{
    if (boopHoldLatchUntilMs == 0) return false;
    uint32_t now = millis();
    return now < boopHoldLatchUntilMs;
}

Vector3D ProtogenProject::GetWiggleOffset(){
    return Vector3D(fGenMatXMove.Update(), fGenMatYMove.Update(), 0);
}

void ProtogenProject::SetWiggleSpeed(float multiplier){
    fGenMatXMove.SetPeriod(5.3f / multiplier);
    fGenMatYMove.SetPeriod(6.7f / multiplier);
}

void ProtogenProject::SetMenuWiggleSpeed(float multiplierX, float multiplierY, float multiplierR){
    Menu::SetWiggleSpeed(multiplierX, multiplierY, multiplierR);
}

void ProtogenProject::SetMenuOffset(Vector2D offset){
    Menu::SetPositionOffset(offset);
}

void ProtogenProject::SetMenuSize(Vector2D size){
    Menu::SetSize(size);
}

Material* ProtogenProject::GetFaceMaterial(){
    return &materialAnimator;
}

Material* ProtogenProject::GetBackgroundMaterial(){
    return &backgroundMaterial;
}

ProtogenProject::ProtogenProject(CameraManager* cameras, Controller* controller, uint8_t numObjects, Vector2D camMin, Vector2D camMax, uint8_t microphonePin, uint8_t buttonPin, uint8_t faceCount, uint8_t faceCycleButtonPin) : Project(cameras, controller, numObjects + 1) {
    this->camMin = camMin;
    this->camMax = camMax;
    this->microphonePin = microphonePin;
    this->buttonPin = buttonPin;
    this->faceCount = faceCount;
    this->faceCycleButtonPin = (faceCycleButtonPin == 255 ? buttonPin : faceCycleButtonPin);

    this->scene.AddObject(background.GetObject());
    
    background.GetObject()->SetMaterial(&backgroundMaterial);

    LinkParameters();

    SetMaterialLayers();

    objA.SetCameraMax(camMax);
    objA.SetCameraMin(camMin);
    objA.SetJustification(ObjectAlign::Stretch);
    
    this->scene.EnableEffect();

    cameraSize = camMax - camMin;

    Vector2D analyzerSize = cameraSize;
    Vector2D analyzerPosition = cameraSize.Divide(2.0f);

    sA.SetSize(analyzerSize);
    sA.SetPosition(analyzerPosition);

    oSC.SetSize(analyzerSize);
    oSC.SetPosition(analyzerPosition);
    mergeSort.SetSize(analyzerSize);
    mergeSort.SetPosition(analyzerPosition);
    mergeSort.SetColumnRandomRange(12, 28);
    pong.SetSize(analyzerSize);
    pong.SetPosition(analyzerPosition);
    spaceInvaders.SetSize(analyzerSize);
    spaceInvaders.SetPosition(analyzerPosition);
    flappyBird.SetSize(analyzerSize);
    flappyBird.SetPosition(analyzerPosition);
    snake.SetSize(analyzerSize);
    snake.SetPosition(analyzerPosition);
    pacman.SetSize(analyzerSize);
    pacman.SetPosition(analyzerPosition);
#ifdef ENABLE_BAD_APPLE_FACE
    badApple.SetSize(cameraSize);
    badApple.SetPosition(cameraSize.Divide(2.0f));
#endif

    hud.SetFaceMax(camMax);
    hud.SetFaceMin(camMin);
}

void ProtogenProject::Initialize() {
    controller->Initialize();

    boop.Initialize(5);

    pinMode(TTP223_PIN, TTP223_PIN_MODE);

    hud.Initialize();

    fanController.Initialize();
#ifdef ENABLE_FACE_COLOR_STRIP
    faceColorStrip.Initialize();
#endif

    MicrophoneFourier::Initialize(microphonePin, 8000, 20.0f, 90.0f);//8KHz sample rate, 50dB min, 120dB max
    
#ifdef NEOTRELLISMENU
    Menu::Initialize(faceCount);//NeoTrellis
#else
    Menu::Initialize(faceCount, faceCycleButtonPin, buttonPin, 500);//7 is number of faces
#endif

    Menu::SetCurrentMenu(0);
}
