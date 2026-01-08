#include "EffectChain.h"

EffectChain::EffectChain(Effect* firstEffect, Effect* secondEffect)
    : first(firstEffect), second(secondEffect) {}

void EffectChain::SetEffects(Effect* firstEffect, Effect* secondEffect) {
    first = firstEffect;
    second = secondEffect;
}

void EffectChain::ApplyEffect(IPixelGroup* pixelGroup) {
    if (!pixelGroup) {
        return;
    }

    if (first) {
        first->ApplyEffect(pixelGroup);
    }

    if (second) {
        second->ApplyEffect(pixelGroup);
    }
}
