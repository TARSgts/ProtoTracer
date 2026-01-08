/**
 * @file EffectChain.h
 * @brief Defines a simple effect chain that applies two effects in order.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "Effect.h"

class EffectChain : public Effect {
private:
    Effect* first = nullptr;
    Effect* second = nullptr;

public:
    EffectChain() = default;
    EffectChain(Effect* first, Effect* second);

    void SetEffects(Effect* firstEffect, Effect* secondEffect);
    void ApplyEffect(IPixelGroup* pixelGroup) override;
};
