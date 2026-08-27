#pragma once

#include "../CameraManager.h"
#include "../../Camera.h"
#include "../../Pixels/PixelGroup.h"
#include "../../Pixels/PixelGroups/DeltaDisplayL.h"
#include "../../Pixels/PixelGroups/DeltaDisplayR.h"
#include "../../../Utils/Math/Transform.h"

class HUB75DeltaCameraManager : public CameraManager {
private:
    CameraLayout cameraLayout = CameraLayout(CameraLayout::ZForward, CameraLayout::YUp);
    Transform camTransform = Transform(Vector3D(), Vector3D(0, 0, -500.0f), Vector3D(1, 1, 1));
    Transform camSideTransformL = Transform(Vector3D(), Vector3D(204.0f, 0, -500.0f), Vector3D(1, 1, 1));
    Transform camSideTransformR = Transform(Vector3D(0, 0, 0), Vector3D(204.0f, 0, -500.0f), Vector3D(1, 1, 1));
    // Height matches ProtogenHUB75Project's declared face canvas (Vector2D(192, 94) --
    // see ProtogenHUB75Project.h) exactly. It used to be declared 96 here vs 94 there, a
    // mismatch that (combined with PixelGroup::GetCoordinate() sampling cell origins
    // instead of centers) thinned the top-edge margin to a fraction of a pixel and left
    // the bottom row sampling exactly on the material boundary with zero margin --
    // together the two issues caused flicker at the edges of the screen. Now that
    // GetCoordinate() centers each sample, keeping this in sync with the material canvas
    // is what gives every edge equal margin instead of clipping the top row outright.
    PixelGroup<2048> camPixels = PixelGroup<2048>(Vector2D(192.0f, 94.0f), Vector2D(0.0f, 0.0f), 64);
    PixelGroup<88> camSidePixelsL = PixelGroup<88>(DeltaDisplayL);
    PixelGroup<88> camSidePixelsR = PixelGroup<88>(DeltaDisplayR);
    Camera<2048> camMain = Camera<2048>(&camTransform, &cameraLayout, &camPixels);
    Camera<88> camSidePanelsL = Camera<88>(&camSideTransformL, &cameraLayout, &camSidePixelsL);
    Camera<88> camSidePanelsR = Camera<88>(&camSideTransformR, &cameraLayout, &camSidePixelsR);

public:
    HUB75DeltaCameraManager() : CameraManager(new CameraBase*[3]{ &camMain, &camSidePanelsL, &camSidePanelsR }, 3) {}
};
