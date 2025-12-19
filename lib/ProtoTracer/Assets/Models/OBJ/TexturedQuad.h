#pragma once

#include "../../../Scene/Materials/Static/SimpleMaterial.h"
#include "../../../Scene/Objects/Object3D.h"

/**
 * @brief Thin quad used for billboarded textures (like the DVD logo sprite).
 *
 * Geometry is centered around the origin so AlignObjectFace() can scale/offset
 * it just like the previous SolidCube placeholder.
 */
class TexturedQuad {
private:
    Vector3D basisVertices[4] = {
        Vector3D(-50.0f, -22.0f, 0.5f),
        Vector3D(50.0f, -22.0f, 0.5f),
        Vector3D(50.0f, 22.0f, 0.5f),
        Vector3D(-50.0f, 22.0f, 0.5f)
    };
    IndexGroup basisIndexes[2] = {IndexGroup(0, 1, 2), IndexGroup(0, 2, 3)};
    StaticTriangleGroup<4, 2> triangleGroup =
        StaticTriangleGroup<4, 2>(&basisVertices[0], &basisIndexes[0]);
    TriangleGroup<4, 2> triangleGroupMemory = TriangleGroup<4, 2>(&triangleGroup);
    SimpleMaterial simpleMaterial = SimpleMaterial(RGBColor(255, 255, 255));
    Object3D basisObj =
        Object3D(&triangleGroup, &triangleGroupMemory, &simpleMaterial);

public:
    TexturedQuad() {}

    Object3D* GetObject() { return &basisObj; }
};
