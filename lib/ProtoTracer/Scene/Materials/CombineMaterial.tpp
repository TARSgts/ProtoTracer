#pragma once

template<size_t materialCount>
CombineMaterial<materialCount>::CombineMaterial() {}

template<size_t materialCount>
void CombineMaterial<materialCount>::AddMaterial(Method method, Material* material, float opacity) {
    if (materialsAdded < materialCount) {
        this->method[materialsAdded] = method;
        this->materials[materialsAdded] = material;
        this->opacity[materialsAdded] = opacity;

        materialsAdded++;
    }
}

template<size_t materialCount>
void CombineMaterial<materialCount>::SetMethod(uint8_t index, Method method) {
    if (index < materialsAdded) {
        this->method[index] = method;
    }
}

template<size_t materialCount>
void CombineMaterial<materialCount>::SetOpacity(uint8_t index, float opacity) {
    if (index < materialsAdded) {
        this->opacity[index] = opacity;
    }
}

template<size_t materialCount>
void CombineMaterial<materialCount>::SetMaterial(uint8_t index, Material* material) {
    if (index < materialsAdded) {
        materials[index] = material;
    }
}

template<size_t materialCount>
RGBColor CombineMaterial<materialCount>::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector3D rgb;
    Vector3D tempV;
    RGBColor temp;

    for (int i = 0; i < materialsAdded; i++) {
        if (opacity[i] > 0.025f) {
            switch (method[i]) {
                case Base:
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    rgb.X = temp.R;
                    rgb.Y = temp.G;
                    rgb.Z = temp.B;

                    rgb = rgb * opacity[i];

                    break;
                case Add:
                    // Add all colors to base color
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    rgb.X = (rgb.X + temp.R) * opacity[i] + rgb.X * (1.0f - opacity[i]);
                    rgb.Y = (rgb.Y + temp.G) * opacity[i] + rgb.Y * (1.0f - opacity[i]);
                    rgb.Z = (rgb.Z + temp.B) * opacity[i] + rgb.Z * (1.0f - opacity[i]);

                    break;
                case Subtract:
                    // Subtract from base color
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    rgb.X = (rgb.X - temp.R) * opacity[i] + rgb.X * (1.0f - opacity[i]);
                    rgb.Y = (rgb.Y - temp.G) * opacity[i] + rgb.Y * (1.0f - opacity[i]);
                    rgb.Z = (rgb.Z - temp.B) * opacity[i] + rgb.Z * (1.0f - opacity[i]);

                    break;
                case Multiply:
                    // Multiply with base color -- channels are 0-255, not 0-1, so the raw
                    // product must be rescaled by /255 or it blows out toward white instead
                    // of darkening (e.g. two mid-grays at 128 would multiply to 16384,
                    // clamped to 255, instead of the correct ~64).
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    rgb.X = (rgb.X * temp.R / 255.0f) * opacity[i] + rgb.X * (1.0f - opacity[i]);
                    rgb.Y = (rgb.Y * temp.G / 255.0f) * opacity[i] + rgb.Y * (1.0f - opacity[i]);
                    rgb.Z = (rgb.Z * temp.B / 255.0f) * opacity[i] + rgb.Z * (1.0f - opacity[i]);

                    break;
                case Divide:
                    // Divide from base color -- guarded against a zero denominator (a black
                    // pixel in the other layer), which previously produced Inf/NaN. Also
                    // rescaled by *255: dividing two 0-255 values directly yields a ~0-1
                    // ratio, not a 0-255 result, so the un-rescaled version always rendered
                    // near-black regardless of opacity.
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    rgb.X = (temp.R > 0 ? (rgb.X / temp.R) * 255.0f : 255.0f) * opacity[i] + rgb.X * (1.0f - opacity[i]);
                    rgb.Y = (temp.G > 0 ? (rgb.Y / temp.G) * 255.0f : 255.0f) * opacity[i] + rgb.Y * (1.0f - opacity[i]);
                    rgb.Z = (temp.B > 0 ? (rgb.Z / temp.B) * 255.0f : 255.0f) * opacity[i] + rgb.Z * (1.0f - opacity[i]);

                    break;
                case Darken:
                    // Find minimum color in all cases
                    temp = materials[i]->GetRGB(position, normal, uvw);
                    tempV = Vector3D::Min(Vector3D(temp.R, temp.G, temp.B), rgb);

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                case Lighten:
                    // Find maximum color in all cases
                    temp = materials[i]->GetRGB(position, normal, uvw);
                    tempV = Vector3D::Max(Vector3D(temp.R, temp.G, temp.B), rgb);

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                case Screen:
                    // 1 - (1 - a)(1 - b), rescaled by /255 since channels are 0-255 not 0-1
                    // (without it, two dark colors screen-blend to deeply negative instead
                    // of staying dark).
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    tempV.X = 255.0f - (255.0f - rgb.X) * (255.0f - temp.R) / 255.0f;
                    tempV.Y = 255.0f - (255.0f - rgb.Y) * (255.0f - temp.G) / 255.0f;
                    tempV.Z = 255.0f - (255.0f - rgb.Z) * (255.0f - temp.B) / 255.0f;

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                case Overlay:
                    // if a < 0.5, 2ab; else 1 - 2(1 - a)(1 - b) -- rescaled by /255, same
                    // reason as Multiply/Screen above.
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    if (rgb.X < 128) tempV.X = 2.0f * rgb.X * temp.R / 255.0f;
                    else tempV.X = 255.0f - 2.0f * (255.0f - rgb.X) * (255.0f - temp.R) / 255.0f;

                    if (rgb.Y < 128) tempV.Y = 2.0f * rgb.Y * temp.G / 255.0f;
                    else tempV.Y = 255.0f - 2.0f * (255.0f - rgb.Y) * (255.0f - temp.G) / 255.0f;

                    if (rgb.Z < 128) tempV.Z = 2.0f * rgb.Z * temp.B / 255.0f;
                    else tempV.Z = 255.0f - 2.0f * (255.0f - rgb.Z) * (255.0f - temp.B) / 255.0f;

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                case SoftLight: {
                    // (1 - 2b)a^2 + 2ba, computed in normalized 0-1 space then rescaled by
                    // *255 -- doing the algebra directly in 0-255 space (as before) drops a
                    // /255 factor on the squared term and produces wildly over-scaled results.
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    float aN = rgb.X / 255.0f, bN = temp.R / 255.0f;
                    tempV.X = ((1.0f - 2.0f * bN) * aN * aN + 2.0f * bN * aN) * 255.0f;

                    aN = rgb.Y / 255.0f; bN = temp.G / 255.0f;
                    tempV.Y = ((1.0f - 2.0f * bN) * aN * aN + 2.0f * bN * aN) * 255.0f;

                    aN = rgb.Z / 255.0f; bN = temp.B / 255.0f;
                    tempV.Z = ((1.0f - 2.0f * bN) * aN * aN + 2.0f * bN * aN) * 255.0f;

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                }
                case Replace:
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    tempV.X = temp.R;
                    tempV.Y = temp.G;
                    tempV.Z = temp.B;

                    rgb = Vector3D::LERP(rgb, tempV, opacity[i]);

                    break;
                case EfficientMask:
                    temp = materials[i]->GetRGB(position, normal, uvw);

                    if (temp.R > 128 && temp.G > 128 && temp.B > 128) {
                        rgb.X = temp.R;
                        rgb.Y = temp.G;
                        rgb.Z = temp.B;

                        rgb = rgb * opacity[i];

                        i = materialsAdded;

                        break;
                    }

                    break;
                case Bypass:
                    materials[i]->GetRGB(position, normal, uvw);
                    break;
                default:

                    break;
            }
        }
    }

    return RGBColor(rgb.Constrain(0, 255));
}
