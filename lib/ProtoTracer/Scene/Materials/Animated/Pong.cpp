#include "Pong.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kHitAngleScale = 0.45f;
constexpr float kRightPaddleAimOffset = 0.08f;
}

PongFace::PongFace(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    ResetBall(random(0, 2) == 0);
}

void PongFace::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
}

void PongFace::SetPosition(Vector2D center) {
    offset = center;
}

void PongFace::RecalculateDimensions() {
    float minAxis = Mathematics::Min(size.X, size.Y);
    paddleWidth = Mathematics::Max(2.0f, size.X * 0.06f);
    paddleHeight = Mathematics::Max(10.0f, size.Y * 0.32f);
    ballRadius = Mathematics::Max(2.0f, minAxis * 0.045f);
    paddleInset = paddleWidth * 1.4f;
    ballSpeed = Mathematics::Max(35.0f, size.X * 0.7f);
    paddleSpeed = ballSpeed * 1.1f;
    centerLineWidth = Mathematics::Max(2.0f, paddleWidth * 0.7f);
}

void PongFace::ResetBall(bool serveRight) {
    ballPos = Vector2D();

    float angleDeg = float(random(-35, 36));
    float angleRad = Mathematics::DegreesToRadians(angleDeg);
    float xDir = serveRight ? 1.0f : -1.0f;

    ballVel.X = cosf(angleRad) * ballSpeed * xDir;
    ballVel.Y = sinf(angleRad) * ballSpeed;
}

void PongFace::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    float maxPaddleStep = paddleSpeed * delta;
    float leftTarget = ballPos.Y;
    float rightTarget = ballPos.Y + (ballVel.Y * paddleHeight * kRightPaddleAimOffset);
    float leftDelta = leftTarget - paddleLeftY;
    float rightDelta = rightTarget - paddleRightY;
    if (leftDelta > maxPaddleStep) leftDelta = maxPaddleStep;
    if (leftDelta < -maxPaddleStep) leftDelta = -maxPaddleStep;
    if (rightDelta > maxPaddleStep) rightDelta = maxPaddleStep;
    if (rightDelta < -maxPaddleStep) rightDelta = -maxPaddleStep;
    paddleLeftY += leftDelta;
    paddleRightY += rightDelta;

    float paddleLimit = size.Y - paddleHeight * 0.5f - 1.0f;
    if (paddleLeftY > paddleLimit) paddleLeftY = paddleLimit;
    if (paddleLeftY < -paddleLimit) paddleLeftY = -paddleLimit;
    if (paddleRightY > paddleLimit) paddleRightY = paddleLimit;
    if (paddleRightY < -paddleLimit) paddleRightY = -paddleLimit;

    ballPos = ballPos + ballVel * delta;

    float topBound = size.Y - ballRadius;
    float bottomBound = -size.Y + ballRadius;
    if (ballPos.Y > topBound) {
        ballPos.Y = topBound;
        ballVel.Y = -fabsf(ballVel.Y);
    } else if (ballPos.Y < bottomBound) {
        ballPos.Y = bottomBound;
        ballVel.Y = fabsf(ballVel.Y);
    }

    float leftX = -size.X + paddleInset;
    float rightX = size.X - paddleInset;
    float paddleHalfW = paddleWidth * 0.5f;
    float paddleHalfH = paddleHeight * 0.5f;

    bool bounced = false;

    if (ballVel.X < 0.0f && ballPos.X - ballRadius <= leftX + paddleHalfW) {
        if (fabsf(ballPos.Y - paddleLeftY) <= paddleHalfH + ballRadius) {
            ballPos.X = leftX + paddleHalfW + ballRadius;
            ballVel.X = fabsf(ballVel.X);
            float offsetRatio = (ballPos.Y - paddleLeftY) / paddleHalfH;
            if (offsetRatio > 1.0f) offsetRatio = 1.0f;
            if (offsetRatio < -1.0f) offsetRatio = -1.0f;
            ballVel.Y += offsetRatio * ballSpeed * kHitAngleScale;
            bounced = true;
        } else if (ballPos.X < -size.X - ballRadius) {
            ResetBall(true);
            return;
        }
    }

    if (ballVel.X > 0.0f && ballPos.X + ballRadius >= rightX - paddleHalfW) {
        if (fabsf(ballPos.Y - paddleRightY) <= paddleHalfH + ballRadius) {
            ballPos.X = rightX - paddleHalfW - ballRadius;
            ballVel.X = -fabsf(ballVel.X);
            float offsetRatio = (ballPos.Y - paddleRightY) / paddleHalfH;
            if (offsetRatio > 1.0f) offsetRatio = 1.0f;
            if (offsetRatio < -1.0f) offsetRatio = -1.0f;
            ballVel.Y += offsetRatio * ballSpeed * kHitAngleScale;
            bounced = true;
        } else if (ballPos.X > size.X + ballRadius) {
            ResetBall(false);
            return;
        }
    }

    if (bounced) {
        float speed = sqrtf(ballVel.X * ballVel.X + ballVel.Y * ballVel.Y);
        if (speed > 1.0f) {
            float scale = ballSpeed / speed;
            ballVel.X *= scale;
            ballVel.Y *= scale;
        }
    }
}

RGBColor PongFace::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float leftX = -size.X + paddleInset;
    float rightX = size.X - paddleInset;
    float paddleHalfW = paddleWidth * 0.5f;
    float paddleHalfH = paddleHeight * 0.5f;

    if (fabsf(relative.X - leftX) <= paddleHalfW && fabsf(relative.Y - paddleLeftY) <= paddleHalfH) {
        return paddleColor;
    }
    if (fabsf(relative.X - rightX) <= paddleHalfW && fabsf(relative.Y - paddleRightY) <= paddleHalfH) {
        return paddleColor;
    }

    float dx = relative.X - ballPos.X;
    float dy = relative.Y - ballPos.Y;
    if ((dx * dx + dy * dy) <= (ballRadius * ballRadius)) {
        return ballColor;
    }

    float halfLine = centerLineWidth * 0.5f;
    if (fabsf(relative.X) <= halfLine) {
        return lineColor;
    }

    return backgroundColor;
}
