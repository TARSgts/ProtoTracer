#pragma once
#include <cmath>

#include "../Templates/ProtogenProjectTemplate.h"
#include "../../Assets/Models/FBX/NukudeFlat.h"
#include "../../Assets/Models/OBJ/DVD.h"
#include "../../Assets/Models/OBJ/SolidCube.h"
#include "../../Assets/Models/OBJ/TexturedQuad.h"
#include "../../Assets/Models/OBJ/BSOD_3.h"
#include "../../Assets/Textures/Static/DVDLogoImage.h"
#include "../../Scene/Materials/Static/SimpleMaterial.h"
#include "../../Scene/Materials/Animated/ConfettiParticles.h"

#include "../../Camera/CameraManager/Implementations/WS35SplitCameras.h"
#include "../../Controller/WS35Controller.h"

class ProtogenWS35Project : public ProtogenProject {
private:
    enum class DvdRenderMode { Sprite, Mesh, Cube };
    // Use the sprite-based render path by default (switch to Cube/Mesh if you need to debug geometry).
    static constexpr DvdRenderMode kDvdRenderMode = DvdRenderMode::Sprite;
    WS35SplitCameraManager cameras;
    WS35Controller controller = WS35Controller(&cameras, 50);
    NukudeFace pM;
    DVD dvdLogo; // actual DVD logo mesh that bounces across the screen.
    SolidCube dvdDebugCube;
    TexturedQuad dvdQuad;
    DVD_logo dvdImage = DVD_logo(Vector2D(), Vector2D());
    SimpleMaterial dvdMaterial = SimpleMaterial(RGBColor(0, 255, 170));
    ConfettiParticles dvdConfetti = ConfettiParticles(Vector2D(192.0f, 105.0f), Vector2D(96.0f, 52.5f));
    TexturedQuad bsodQuad;
    BSOD_3 bsodImage = BSOD_3(Vector2D(), Vector2D());
    Vector2D dvdOffset = Vector2D(); ///< Current logo offset from the center of the face canvas.
    Vector2D dvdVelocity = Vector2D(65.0f, 55.0f); ///< Pixels per second-ish speed along X/Y for the screensaver motion.
    uint32_t lastDvdUpdateMs = 0; ///< Tracks the last time we advanced the DVD logo (for frame-rate independent motion).
    const float dvdEdgeMargin = 0.0f; ///< No bezel so the square can sweep the entire face.
    const float dvdScale = 0.35f; ///< Relative size of the square after alignment (1.0 fills the canvas).
    const float dvdAlignMargin = 0.0f; ///< No additional margin during alignment.
    const float dvdCornerTolerance = 0.0f; ///< Distance from both edges required before triggering confetti.
    Vector2D dvdTravelPadding = Vector2D(); ///< Optional padding (per axis) to keep the logo away from the bounds.
    Vector2D dvdCustomTravelMin = Vector2D(); ///< Optional explicit minimum travel limit for fine calibration.
    Vector2D dvdCustomTravelMax = Vector2D(); ///< Optional explicit maximum travel limit for fine calibration.
    bool dvdUseCustomTravelBounds = false; ///< Tracks whether manual travel limits are active.
    bool dvdConfettiEnabled = true; ///< Toggle to enable/disable the celebratory confetti burst on collisions.
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
    
    #ifdef ENABLE_BAD_APPLE_FACE
	const __FlashStringHelper* faceArray[15] = {F("DEFAULT"), F("ANGRY"), F("DOUBT"), F("FROWN"), F("LOOKUP"), F("SAD"), F("AUDIO1"), F("AUDIO2"), F("AUDIO3"), F("DVDLOGO"), F("PONG"), F("INVADER"), F("FLAPPY"), F("SNAKE"), F("BADAPPLE")};
    #else
	const __FlashStringHelper* faceArray[14] = {F("DEFAULT"), F("ANGRY"), F("DOUBT"), F("FROWN"), F("LOOKUP"), F("SAD"), F("AUDIO1"), F("AUDIO2"), F("AUDIO3"), F("DVDLOGO"), F("PONG"), F("INVADER"), F("FLAPPY"), F("SNAKE")};
    #endif

    void LinkControlParameters() override {
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
     * @brief Resets the bouncing DVD logo to the center with a fresh timer.
     *
     * We call this once during construction so the first frame does not jump.
     */
    void ResetDvdMotion() {
        dvdOffset = Vector2D();
        float baseSpeed = GetCameraSize().X * 0.35f; // Scale motion to panel width so it feels similar on different canvases.
        dvdVelocity = Vector2D(baseSpeed * 0.6f, baseSpeed * 0.48f);
        lastDvdUpdateMs = millis();
    }

    /**
     * @brief Steps the palette forward so every wall hit noticeably changes the logo color.
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

    Object3D* GetDvdObject() {
        switch (kDvdRenderMode) {
            case DvdRenderMode::Sprite:
                return dvdQuad.GetObject();
            case DvdRenderMode::Cube:
                return dvdDebugCube.GetObject();
            case DvdRenderMode::Mesh:
            default:
                return dvdLogo.GetObject();
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

    /**
     * @brief Allows tuning how close to the edges the DVD face may travel.
     *
     * @param padding Amount of padding (in face pixels) to add on each axis.
     */
    void SetDVDTravelPadding(Vector2D padding) {
        dvdTravelPadding = padding;
    }

    /**
     * @brief Allows explicitly defining the minimum/maximum offsets the logo may visit.
     *
     * Supplying manual bounds is helpful when the physical screen is slightly shifted
     * relative to the modeled canvas (or when a bezel crops a portion of the render).
     */
    void SetDVDTravelBounds(Vector2D minBounds, Vector2D maxBounds) {
        // Ensure minBounds <= maxBounds on each axis so the bounce logic stays sane.
        float minX = (minBounds.X <= maxBounds.X) ? minBounds.X : maxBounds.X;
        float maxX = (maxBounds.X >= minBounds.X) ? maxBounds.X : minBounds.X;
        float minY = (minBounds.Y <= maxBounds.Y) ? minBounds.Y : maxBounds.Y;
        float maxY = (maxBounds.Y >= minBounds.Y) ? maxBounds.Y : minBounds.Y;
        dvdCustomTravelMin = Vector2D(minX, minY);
        dvdCustomTravelMax = Vector2D(maxX, maxY);
        dvdUseCustomTravelBounds = true;
    }

    /**
     * @brief Returns control back to the automatically sized bounds.
     */
    void ClearDVDTravelBounds() {
        dvdUseCustomTravelBounds = false;
    }

    /**
     * @brief Renders the bouncing DVD logo face.
     *
     * - Integrates a simple velocity/position pair so the logo travels at a constant speed.
     * - Bounces off virtual walls sized to the current camera, nudging the position back inside.
     * - Flips color on every bounce for an old-school screensaver vibe.
     * - Aligns, scales, and translates the DVD mesh on top of the existing face canvas.
     */
    void DVDLogoFace(float ratio) {
        GetDvdObject()->Enable();
        pM.GetObject()->Disable();
        (void)ratio; // Motion is driven by elapsed millis(), not the normalized animation ratio.

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
        // Keep menu-driven color selection respected on the strip while the logo is active.
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

public:
    ProtogenWS35Project() : ProtogenProject(&cameras, &controller, 2, Vector2D(), Vector2D(192.0f, 105.0f), 22, 23,
        #ifdef ENABLE_BAD_APPLE_FACE
        15
        #else
        14
        #endif
    ){
        scene.AddObject(pM.GetObject());
        scene.AddObject(GetDvdObject()); // add early so we can toggle it on/off per face selection
        scene.AddObject(bsodQuad.GetObject());

        pM.GetObject()->SetMaterial(GetFaceMaterial());
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

        LinkControlParameters();

        hud.SetFaceArray(faceArray);

        SetWiggleSpeed(5.0f);
        SetMenuWiggleSpeed(0.0f, 0.0f, 0.0f);
        SetMenuOffset(Vector2D(2.5f, -3.0f));
        SetMenuSize(Vector2D(240, 64));
    }

    void Update(float ratio) override {
        pM.Reset();
        GetDvdObject()->Disable();
        bsodQuad.GetObject()->Disable();
        pM.GetObject()->Enable();
#ifdef ENABLE_BAD_APPLE_FACE
        ResetBadAppleUsage();
#endif

        ClearStripColorOverride();
        uint8_t mode = Menu::GetFaceState();//change by button press
        
        controller.SetBrightness(Menu::GetBrightness());
        controller.SetAccentBrightness(Menu::GetAccentBrightness());

        if (IsBooped() && mode != 6 && mode != 9){
            Surprised();
        }
        else{
            if (IsBoopBSODActive()) {
                BSOD3Face();
            } else {
                if (mode == 0) Default();
                else if (mode == 1) Angry();
                else if (mode == 2) Doubt();
                else if (mode == 3) Frown();
                else if (mode == 4) LookUp();
                else if (mode == 5) Sad();
                else if (mode == 6) {
                    AudioReactiveGradientFace();
                }
                else if (mode == 7){
                    OscilloscopeFace();
                }
                else if (mode == 8) {
                    SpectrumAnalyzerFace();
                }
                else if (mode == 9) {
                    DVDLogoFace(ratio);
                }
                else if (mode == 10) {
                    PongAutoFace();
                }
                else if (mode == 11) {
                    SpaceInvadersAutoFace();
                }
                else if (mode == 12) {
                    FlappyBirdAutoFace();
                }
                else if (mode == 13) {
                    SnakeAutoFace();
                }
                #ifdef ENABLE_BAD_APPLE_FACE
                else {
                    BadAppleFace();
                }
                #endif 
            }
        }

        UpdateFace(ratio);

        if (pM.GetObject()->IsEnabled()) {
            pM.SetMorphWeight(NukudeFace::BiggerNose, 1.0f);
            pM.SetMorphWeight(NukudeFace::MoveEye, 1.0f);

            pM.Update();

            AlignObjectFace(pM.GetObject(), -7.5f);

            pM.GetObject()->GetTransform()->SetPosition(GetWiggleOffset());
            pM.GetObject()->UpdateTransform();
        }
    }
};
