#include "BouncePhysics.h"

BouncePhysics::BouncePhysics(float gravity, float velocityRatio) :
    velocityFilter(0.4f),
    currentVelocity(0.0f),
    currentPosition(0.0f),
    velocityRatio(velocityRatio),
    gravity(gravity),
    previousMillis(0),
    previousVelocity(0.0f) {
}

float BouncePhysics::Calculate(float velocity, unsigned long currentMillis) {
    float dT = ((float)(currentMillis - previousMillis)) / 1000.0f;

    if (dT > 0.1f && dT < 2.0f) {
        currentVelocity += velocity + gravity * dT;
        currentPosition += velocityRatio * currentVelocity * dT;

        previousMillis = currentMillis;
    }

    return currentPosition;
}

float BouncePhysics::Calculate(float velocity, float dT) {
    velocity = velocityFilter.Filter(velocity);
    float changeRate = (velocity - previousVelocity) / dT;

    changeRate = changeRate < 0.0f ? 0.0f : changeRate;

    currentVelocity = changeRate - gravity * dT;
    currentPosition += velocityRatio * currentVelocity * dT;

    if (currentPosition < 0.0f) {
        currentVelocity = 0.0f;
        currentPosition = 0.0f;
    }
    else if (currentPosition > 1.0f) {
        currentVelocity = 0.0f;
        currentPosition = 1.0f;
    }

    previousVelocity = velocity;

    // currentPosition alone is NOT the visible signal -- with gravity*dT (3.5 at the
    // gravity=35/dT=0.1 values every caller uses) usually exceeding changeRate for
    // ordinary, gradually-varying audio, currentVelocity is negative almost all the time
    // and currentPosition sits clamped at 0 except during sudden onset spikes. The raw
    // `velocity` term carries the actual sustained/baseline level; currentPosition only
    // adds an extra transient "kick" on top. Returning currentPosition alone (a prior,
    // reverted fix attempt) made both audio visualizers go dark since their normal,
    // continuously-varying input rarely produces a nonzero currentPosition at all.
    // The real bug was that this sum was never clamped -- letting it reach up to 2.0 for
    // loud, sustained input and defeat every caller's assumption of a [0,1] magnitude.
    float result = currentPosition + velocity;
    if (result < 0.0f) result = 0.0f;
    if (result > 1.0f) result = 1.0f;
    return result;
}
