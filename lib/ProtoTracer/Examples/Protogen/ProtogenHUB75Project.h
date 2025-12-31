#pragma once
#include <cmath>

#include "../Templates/ProtogenProjectTemplate.h"
#include "../../Assets/Models/OBJ/DeltaDisplayBackground.h"
#include "../../Assets/Models/FBX/NukudeFlat.h"
#include "../../Assets/Models/OBJ/DVD.h"
#include "../../Assets/Models/OBJ/SolidCube.h"
#include "../../Assets/Models/OBJ/TexturedQuad.h"
#include "../../Assets/Models/OBJ/BSOD_3.h"
#include "../../Assets/Textures/Static/DVDLogoImage.h"
#include "../../Scene/Materials/Static/SimpleMaterial.h"
#include "../../Scene/Materials/Animated/ConfettiParticles.h"

#include "../../Camera/CameraManager/Implementations/HUB75DeltaCameras.h"
#include "../../Controller/HUB75Controller.h"

// E 001 TARS: Add in a plane to the corner of the display to illuminate side panel ---
// ASTRALTODO: Update and use a flat circle for this!
#include "../../Assets/Models/OBJ/SolidCube.h"
// E 001 TARS -------------------------------------------------------------------------

class ProtogenHUB75Project : public ProtogenProject {
private:
    enum class DvdRenderMode { Sprite, Mesh, Cube };
    // Render the real DVD logo sprite by default (falls back to cube/mesh via kDvdRenderMode).
    static constexpr DvdRenderMode kDvdRenderMode = DvdRenderMode::Sprite;
    static constexpr uint8_t kFaceCount =
        #ifdef ENABLE_BAD_APPLE_FACE
        14
        #else
        13
        #endif
    ;
    static constexpr bool kUseDvdDebugCube = false; ///< Set true to render the simple cube for troubleshooting geometry issues.
    HUB75DeltaCameraManager cameras;
    HUB75Controller controller = HUB75Controller(&cameras, 50, 50);
    NukudeFace pM;
    DeltaDisplayBackground deltaDisplayBackground;
    DVD dvdMesh; // DVD logo mesh that carries the bouncing transform
    SolidCube dvdDebugCube; ///< Optional fallback shape when diagnosing rendering glitches.
    TexturedQuad dvdQuad; ///< Flat quad used when rendering the sprite-based DVD image.
    DVD_logo dvdImage = DVD_logo(Vector2D(), Vector2D()); ///< Sprite that emulates the printed DVD logo.
    SimpleMaterial dvdMaterial = SimpleMaterial(RGBColor(0, 255, 170));
    ConfettiParticles dvdConfetti = ConfettiParticles(Vector2D(192.0f, 94.0f), Vector2D(96.0f, 47.0f));
    TexturedQuad bsodQuad; ///< Quad used for the BSOD static image.
    BSOD_3 bsodImage = BSOD_3(Vector2D(), Vector2D()); ///< Static BSOD image material.
    Vector2D dvdOffset = Vector2D(); ///< Logo offset relative to the face center.
    Vector2D dvdVelocity = Vector2D(65.0f, 55.0f); ///< Motion vector (units per second) that drives the screensaver bounce.
    uint32_t lastDvdUpdateMs = 0; ///< Timestamp of the previous update for delta-time integration.
    const float dvdEdgeMargin = 0.0f; ///< No bezel so the square can sweep the entire face.
    const float dvdScale = 0.35f; ///< Relative size of the square after alignment (1.0 fills the canvas).
    const float dvdAlignMargin = 0.0f; ///< No additional margin during alignment.
    const float dvdCornerTolerance = 0.0f; ///< How close (in face units) we must be to both edges to count as a corner hit.
    Vector2D dvdTravelPadding = Vector2D(); ///< Optional padding (per axis) to keep the square off the walls.
    Vector2D dvdCustomTravelMin = Vector2D(); ///< Explicit min travel bounds when fine calibration is needed.
    Vector2D dvdCustomTravelMax = Vector2D(); ///< Explicit max travel bounds when fine calibration is needed.
    bool dvdUseCustomTravelBounds = false; ///< True if SetDVDTravelBounds() has been called.
    bool dvdConfettiEnabled = true; ///< Toggle to enable/disable confetti bursts on corner hits.
    bool dvdConfettiActive = false; ///< Tracks whether the confetti overlay is currently active.
    uint32_t dvdConfettiEndMs = 0; ///< Timestamp (ms) when the confetti overlay should stop.
    const uint32_t dvdConfettiDurationMs = 1000; ///< How long (ms) the confetti overlay should remain visible.
    const float dvdConfettiOpacity = 0.8f; ///< Foreground confetti intensity.
    const float dvdConfettiBackgroundOpacity = 0.5f; ///< Background confetti intensity.
    const RGBColor dvdBouncePalette[6] = {
        RGBColor(0, 255, 170),
        RGBColor(255, 105, 180),
        RGBColor(135, 206, 235),
        RGBColor(255, 215, 0),
        RGBColor(255, 64, 64),
        RGBColor(180, 160, 255)
    };
    uint8_t dvdBouncePaletteIndex = 0;

    // E 001 TARS : Add in a plane to the corner of the display to illuminate side panel
    SolidCube sideIllum;
    // E 001 TARS ----------------------------------------------------------------------
    
	const __FlashStringHelper* faceArray[kFaceCount] = {
        F("DEFAULT"),
        F("ANGRY"),
        F("DOUBT"),
        F("FROWN"),
        F("LOOKUP"),
        F("SAD"),
        F("AUD-GRD"),
        F("OSCIL"),
        F("SPECTRUM"),
        F("DVDLOGO"),
        F("MERGESRT"),
        F("PONG"),
        F("INVADER")
        #ifdef ENABLE_BAD_APPLE_FACE
        ,F("BADAPPLE")
        #endif
    };

    void LinkControlParameters() override {//Called from parent
        AddParameter(NukudeFace::Anger, pM.GetMorphWeightReference(NukudeFace::Anger), 15);
        AddParameter(NukudeFace::Sadness, pM.GetMorphWeightReference(NukudeFace::Sadness), 15, IEasyEaseAnimator::InterpolationMethod::Cosine);
        AddParameter(NukudeFace::Surprised, pM.GetMorphWeightReference(NukudeFace::Surprised), 15);
        AddParameter(NukudeFace::Doubt, pM.GetMorphWeightReference(NukudeFace::Doubt), 15);
        AddParameter(NukudeFace::Frown, pM.GetMorphWeightReference(NukudeFace::Frown), 15);
        AddParameter(NukudeFace::LookUp, pM.GetMorphWeightReference(NukudeFace::LookUp), 15);
        AddParameter(NukudeFace::LookDown, pM.GetMorphWeightReference(NukudeFace::LookDown), 15);

        AddParameter(NukudeFace::HideBlush, pM.GetMorphWeightReference(NukudeFace::HideBlush), 15, IEasyEaseAnimator::InterpolationMethod::Cosine, true);

        AddViseme(Viseme::MouthShape::EE, pM.GetMorphWeightReference(NukudeFace::vrc_v_ee));
        AddViseme(Viseme::MouthShape::AH, pM.GetMorphWeightReference(NukudeFace::vrc_v_aa));
        AddViseme(Viseme::MouthShape::UH, pM.GetMorphWeightReference(NukudeFace::vrc_v_dd));
        AddViseme(Viseme::MouthShape::AR, pM.GetMorphWeightReference(NukudeFace::vrc_v_rr));
        AddViseme(Viseme::MouthShape::ER, pM.GetMorphWeightReference(NukudeFace::vrc_v_ch));
        AddViseme(Viseme::MouthShape::OO, pM.GetMorphWeightReference(NukudeFace::vrc_v_oh));
        AddViseme(Viseme::MouthShape::SS, pM.GetMorphWeightReference(NukudeFace::vrc_v_ss));

        AddBlinkParameter(pM.GetMorphWeightReference(NukudeFace::Blink));
    }

    void Default(){
        ApplyMenuOrDefaultColor(Color::CWHITE);
    }

    void Angry(){
        AddParameterFrame(NukudeFace::Anger, 1.0f);
        ApplyMenuOrDefaultColor(Color::CWHITE);
    } 

    void Sad(){
        AddParameterFrame(NukudeFace::Sadness, 1.0f);
        AddParameterFrame(NukudeFace::Frown, 1.0f);
        ApplyMenuOrDefaultColor(Color::CWHITE);
    }

    void Surprised(){
        AddParameterFrame(NukudeFace::Surprised, 1.0f);
        AddParameterFrame(NukudeFace::HideBlush, 0.0f);
        AddMaterialFrame(Color::CRAINBOW);
        SetStripColorOverride(Color::CRAINBOW);
    }
    
    void Doubt(){
        AddParameterFrame(NukudeFace::Doubt, 1.0f);
    }
    
    void Frown(){
        AddParameterFrame(NukudeFace::Frown, 1.0f);
    }

    void LookUp(){
        AddParameterFrame(NukudeFace::LookUp, 1.0f);
    }

    void LookDown(){
        AddParameterFrame(NukudeFace::LookDown, 1.0f);
    }

    /**
     * @brief Initialize the DVD logo motion so it starts centered and moves at a predictable rate.
     */
    void ResetDvdMotion() {
        dvdOffset = Vector2D();
        float baseSpeed = GetCameraSize().X * 0.35f;
        dvdVelocity = Vector2D(baseSpeed * 0.6f, baseSpeed * 0.48f);
        lastDvdUpdateMs = millis();
    }

    /**
     * @brief Cycle the logo color to the next palette entry (called on every bounce).
     */
    void AdvanceDvdColor() {
        dvdBouncePaletteIndex = (dvdBouncePaletteIndex + 1) % (sizeof(dvdBouncePalette) / sizeof(dvdBouncePalette[0]));
        RGBColor color = dvdBouncePalette[dvdBouncePaletteIndex];
        if (kDvdRenderMode == DvdRenderMode::Sprite) {
            dvdImage.SetTintColor(color);
        } else {
            dvdMaterial.SetRGB(color);
        }
    }

    void TriggerDvdConfetti(const Vector2D& origin, const Vector2D& direction) {
        if (!dvdConfettiEnabled) return;
        dvdConfetti.Trigger(origin, direction, 70.0f, 18);
        dvdConfettiActive = true;
        dvdConfettiEndMs = millis() + dvdConfettiDurationMs;
    }

    void UpdateDvdConfetti() {
        dvdConfetti.Update();
        if (!dvdConfettiEnabled) return;
        if (dvdConfettiActive && millis() <= dvdConfettiEndMs) {
            AddMaterialFrame(dvdConfetti, dvdConfettiOpacity);
            AddBackgroundMaterialFrame(dvdConfetti, dvdConfettiBackgroundOpacity);
            SetStripColorOverride(Color::CRAINBOW);
        } else if (dvdConfettiActive) {
            AddMaterialFrame(dvdConfetti, 0.0f);
            AddBackgroundMaterialFrame(dvdConfetti, 0.0f);
            dvdConfettiActive = false;
        }
    }

    void SetDVDConfettiEnabled(bool enabled) {
        dvdConfettiEnabled = enabled;
    }

    bool IsDVDConfettiEnabled() const {
        return dvdConfettiEnabled;
    }

    /**
     * @brief Adds symmetric padding so the logo never quite reaches the calculated edges.
     */
    void SetDVDTravelPadding(Vector2D padding) {
        dvdTravelPadding = padding;
    }

    /**
     * @brief Overrides the automatically computed bounds with explicit offsets.
     *
     * Handy for aligning to the physical pixels when the modeled canvas does not perfectly
     * match the LED panel (or when a bezel extends farther on one side).
     */
    void SetDVDTravelBounds(Vector2D minBounds, Vector2D maxBounds) {
        float minX = (minBounds.X <= maxBounds.X) ? minBounds.X : maxBounds.X;
        float maxX = (maxBounds.X >= minBounds.X) ? maxBounds.X : minBounds.X;
        float minY = (minBounds.Y <= maxBounds.Y) ? minBounds.Y : maxBounds.Y;
        float maxY = (maxBounds.Y >= minBounds.Y) ? maxBounds.Y : minBounds.Y;
        dvdCustomTravelMin = Vector2D(minX, minY);
        dvdCustomTravelMax = Vector2D(maxX, maxY);
        dvdUseCustomTravelBounds = true;
    }

    /**
     * @brief Returns to automatically sized bounds (removes any manual overrides).
     */
    void ClearDVDTravelBounds() {
        dvdUseCustomTravelBounds = false;
    }

    /**
     * @brief Drives and renders the bouncing DVD logo face.
     *
     * The logo uses a simple velocity integrator with edge checks sized to the current
     * camera bounds, mirroring the classic DVD screensaver behavior.
     */
    Object3D* GetDvdObject() {
        switch (kDvdRenderMode) {
            case DvdRenderMode::Sprite:
                return dvdQuad.GetObject();
            case DvdRenderMode::Cube:
                return dvdDebugCube.GetObject();
            case DvdRenderMode::Mesh:
            default:
                return dvdMesh.GetObject();
        }
    }

    Material* GetDvdMaterial() {
        return (kDvdRenderMode == DvdRenderMode::Sprite) ? static_cast<Material*>(&dvdImage)
                                                        : static_cast<Material*>(&dvdMaterial);
    }

    float RGBToHue(const RGBColor& color) const {
        float r = color.R / 255.0f;
        float g = color.G / 255.0f;
        float b = color.B / 255.0f;
        float maxC = Mathematics::Max(r, Mathematics::Max(g, b));
        float minC = Mathematics::Min(r, Mathematics::Min(g, b));
        float delta = maxC - minC;
        if (delta == 0.0f) {
            return 0.0f;
        }
        float hue;
        if (maxC == r) {
            hue = 60.0f * fmodf(((g - b) / delta), 6.0f);
        } else if (maxC == g) {
            hue = 60.0f * (((b - r) / delta) + 2.0f);
        } else {
            hue = 60.0f * (((r - g) / delta) + 4.0f);
        }
        if (hue < 0.0f) {
            hue += 360.0f;
        }
        return hue;
    }

    void DVDLogoFace(float ratio) {
        GetDvdObject()->Enable();
        pM.GetObject()->Disable();
        (void)ratio; // The motion is time-based; the loop ratio is unused here.

        uint32_t now = millis();
        float deltaTime = (lastDvdUpdateMs == 0) ? 0.016f : (now - lastDvdUpdateMs) / 1000.0f;
        lastDvdUpdateMs = now;

        dvdOffset = dvdOffset + dvdVelocity * deltaTime;

        GetDvdObject()->ResetVertices();
        AlignObjectFace(GetDvdObject(), 0.0f, dvdAlignMargin, false);

        Vector3D canvasSize = GetDvdObject()->GetSize();
        Vector2D canvasHalf(canvasSize.X * 0.5f, canvasSize.Y * 0.5f);
        Vector2D objHalf(canvasSize.X * dvdScale * 0.5f, canvasSize.Y * dvdScale * 0.5f);
        if (kDvdRenderMode == DvdRenderMode::Sprite) {
            Vector2D scaledSize(canvasSize.X * dvdScale, canvasSize.Y * dvdScale);
            dvdImage.SetSize(scaledSize);
        }

        Vector2D edge = canvasHalf - objHalf - (Vector2D(dvdEdgeMargin, dvdEdgeMargin) + dvdTravelPadding);
        if (edge.X < 0.0f) edge.X = 0.0f;
        if (edge.Y < 0.0f) edge.Y = 0.0f;
        Vector2D minEdge(-edge.X, -edge.Y);
        Vector2D maxEdge(edge.X, edge.Y);

        if (dvdUseCustomTravelBounds) {
            minEdge = dvdCustomTravelMin;
            maxEdge = dvdCustomTravelMax;
        }

        bool bounced = false;

        if (dvdOffset.X > maxEdge.X) {
            dvdOffset.X = maxEdge.X;
            dvdVelocity.X = -fabs(dvdVelocity.X);
            bounced = true;
        } else if (dvdOffset.X < minEdge.X) {
            dvdOffset.X = minEdge.X;
            dvdVelocity.X = fabs(dvdVelocity.X);
            bounced = true;
        }

        if (dvdOffset.Y > maxEdge.Y) {
            dvdOffset.Y = maxEdge.Y;
            dvdVelocity.Y = -fabs(dvdVelocity.Y);
            bounced = true;
        } else if (dvdOffset.Y < minEdge.Y) {
            dvdOffset.Y = minEdge.Y;
            dvdVelocity.Y = fabs(dvdVelocity.Y);
            bounced = true;
        }

        if (bounced) {
            bool nearMinX = fabsf(dvdOffset.X - minEdge.X) <= dvdCornerTolerance;
            bool nearMaxX = fabsf(dvdOffset.X - maxEdge.X) <= dvdCornerTolerance;
            bool nearMinY = fabsf(dvdOffset.Y - minEdge.Y) <= dvdCornerTolerance;
            bool nearMaxY = fabsf(dvdOffset.Y - maxEdge.Y) <= dvdCornerTolerance;
            if ((nearMinX || nearMaxX) && (nearMinY || nearMaxY)) {
                Vector2D cornerLocal(nearMinX ? -canvasHalf.X : canvasHalf.X,
                                     nearMinY ? -canvasHalf.Y : canvasHalf.Y);
                Vector2D burstDir(nearMinX ? 1.0f : -1.0f, nearMinY ? 1.0f : -1.0f);
                TriggerDvdConfetti(cornerLocal, burstDir);
            }
            AdvanceDvdColor();
        }

        auto* logoTransform = GetDvdObject()->GetTransform();
        Vector3D centerOffset = GetDvdObject()->GetCenterOffset();

        logoTransform->SetScale(Vector3D(1.0f, 1.0f, 1.0f));
        logoTransform->SetScaleOffset(Vector3D());
        logoTransform->SetRotation(Vector3D());
        logoTransform->SetRotationOffset(Vector3D());
        logoTransform->SetPosition(Vector3D());

        logoTransform->SetScale(Vector3D(dvdScale, dvdScale, 1.0f));
        logoTransform->SetScaleOffset(centerOffset);
        logoTransform->SetRotationOffset(centerOffset);
        logoTransform->SetPosition(Vector3D(dvdOffset.X, dvdOffset.Y, 0.0f));
        GetDvdObject()->UpdateTransform();

        if (kDvdRenderMode == DvdRenderMode::Sprite) {
            Vector3D worldCenter = GetDvdObject()->GetCenterOffset();
            dvdImage.SetPosition(Vector2D(worldCenter.X, worldCenter.Y));
        }

        UpdateDvdConfetti();
        ApplyMenuOrDefaultColor(Color::CWHITE, 0.9f);
    }

    void BSOD3Face() {
        bsodQuad.GetObject()->Enable();
        pM.GetObject()->Disable();
        GetDvdObject()->Disable();

        bsodQuad.GetObject()->ResetVertices();
        AlignObjectFace(bsodQuad.GetObject(), 0.0f, 0.0f, false);

        Vector3D canvasSize = bsodQuad.GetObject()->GetSize();
        bsodImage.SetSize(Vector2D(canvasSize.X, canvasSize.Y));

        Vector3D worldCenter = bsodQuad.GetObject()->GetCenterOffset();
        bsodImage.SetPosition(Vector2D(worldCenter.X, worldCenter.Y));
    }

    void SpectrumAnalyzerCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
        SetStripColorOverride(Color::CHORIZONTALRAINBOW);
    }

    void AudioReactiveGradientCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
        SetStripColorOverride(Color::CHORIZONTALRAINBOW);
    }

    void OscilloscopeCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
        SetStripColorOverride(Color::CHORIZONTALRAINBOW);
    }

    void MergeSortCallback() override {
        AddMaterialFrame(Color::CBLUE, 0.6f);
        SetStripColorOverride(Color::CBLUE);
    }

public:
    ProtogenHUB75Project() : ProtogenProject(&cameras, &controller, 3, Vector2D(), Vector2D(192.0f, 94.0f), 22, 23, kFaceCount, 21){
        scene.AddObject(pM.GetObject());
        scene.AddObject(deltaDisplayBackground.GetObject());
        scene.AddObject(GetDvdObject());
        scene.AddObject(bsodQuad.GetObject());

        // E 001 TARS : Add in a plane to the corner of the display to illuminate side panel
        const float sideIllum_Scale = 0.65;
        sideIllum.GetObject()->GetTransform()->SetPosition(Vector3D(192, 0, 0));
        sideIllum.GetObject()->GetTransform()->SetScale(Vector3D(sideIllum_Scale, sideIllum_Scale, sideIllum_Scale));
        sideIllum.GetObject()->UpdateTransform();
        // scene.AddObject(sideIllum.GetObject());
        // E 001 TARS ----------------------------------------------------------------------

        pM.GetObject()->SetMaterial(GetFaceMaterial());
        deltaDisplayBackground.GetObject()->SetMaterial(GetFaceMaterial());
        GetDvdObject()->SetMaterial(GetDvdMaterial());
        bsodQuad.GetObject()->SetMaterial(&bsodImage);
        dvdConfetti.SetSize(GetCameraSize());
        dvdConfetti.SetPosition(GetCameraSize().Divide(2.0f));
        AddMaterial(Material::Add, &dvdConfetti, 12, 0.0f, 1.0f);
        AddBackgroundMaterial(Material::Add, &dvdConfetti, 12, 0.0f, 1.0f);
        AdvanceDvdColor();
        SetDVDConfettiEnabled(true);
        GetDvdObject()->Disable(); // hidden until the DVD face is selected
        bsodQuad.GetObject()->Disable(); // hidden until the BSOD face is selected
        ResetDvdMotion();

        hud.SetFaceArray(faceArray);

        LinkControlParameters();
        
        SetWiggleSpeed(5.0f);
        SetMenuWiggleSpeed(0.0f, 0.0f, 0.0f);
        SetMenuOffset(Vector2D(17.5f, -3.0f));
        SetMenuSize(Vector2D(192, 56));
    }

    void Update(float ratio) override {
        pM.Reset();
        GetDvdObject()->Disable();
        bsodQuad.GetObject()->Disable();
        pM.GetObject()->Enable();
#ifdef ENABLE_BAD_APPLE_FACE
        ResetBadAppleUsage();
#endif

        uint8_t mode = Menu::GetFaceState();//change by button press

        controller.SetBrightness(Menu::GetBrightness());
        controller.SetAccentBrightness(Menu::GetAccentBrightness());

#ifdef MORSEBUTTON
        if (IsBoopBSODActive()) {
            BSOD3Face();
        } else {
            SelectFaceFromMorse(mode);
        }
#else
        if (IsBoopBSODActive()) {
            BSOD3Face();
        } else {
            SelectFace(mode);
        }
#endif

        UpdateFace(ratio);

        pM.Update();

        if (pM.GetObject()->IsEnabled()) {
            AlignObjectFace(pM.GetObject(), -7.5f);

            pM.GetObject()->GetTransform()->SetPosition(GetWiggleOffset());
            pM.GetObject()->UpdateTransform();
        }
    }

    void SelectFace(uint8_t code) {
        ClearStripColorOverride();
        if (IsBooped() && code != 6 && code != 9) {
            Surprised();
            return;
        }

        switch(code) {
            case 0: Default();  break;
            case 1: Angry();    break;
            case 2: Doubt();    break;
            case 3: Frown();    break;
            case 4: LookUp();   break;
            case 5: Sad();      break;
            case 6: AudioReactiveGradientFace();    break;
            case 7: OscilloscopeFace();             break;
            case 8: SpectrumAnalyzerFace();         break;
            case 9: DVDLogoFace(0.0f);              break;
            case 10: MergeSortFace();               break;
            case 11: PongAutoFace();                break;
            case 12: SpaceInvadersAutoFace();       break;
            #ifdef ENABLE_BAD_APPLE_FACE
            case 13: BadAppleFace();                break;
            #endif
            default: SpectrumAnalyzerFace();        break;
        }
    }

    void SelectFaceFromMorse(uint8_t code) {
        if (IsBooped() && code != 24) {
            Surprised();
            return;
        }

        switch(code) {
            case 1: Angry();        break; // [A]ngry
            case 2: Surprised();    break; // [B]lush
            case 4: Doubt();        break; // [D]oubt
            case 6: Frown();        break; // [F]rown
            case 19: Sad();         break; // [S]ad
            case 21: LookUp();      break; // Look [U]p
            case 22: LookDown();    break; // Look [V] Down
            case 13: MergeSortFace(); break; // [M] Merge sort
            case 24: AudioReactiveGradientFace();   break; // [X] X.X
            case 25: OscilloscopeFace();            break; // [Y] Oscilloscope
            case 26: SpectrumAnalyzerFace();        break; // [Z] Spectrum
            default: Default();     break; // [H] Happy
        }
    }
};
