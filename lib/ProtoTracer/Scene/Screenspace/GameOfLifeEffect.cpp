#include "GameOfLifeEffect.h"

#include <Arduino.h>
#include <cstring>

GameOfLifeEffect::~GameOfLifeEffect() {
    for (uint8_t i = 0; i < kMaxContexts; ++i) {
        delete[] contexts[i].cells;
        delete[] contexts[i].nextCells;
        contexts[i].cells = nullptr;
        contexts[i].nextCells = nullptr;
        contexts[i].group = nullptr;
        contexts[i].pixelCount = 0;
        contexts[i].bytes = 0;
        contexts[i].lastUpdateMs = 0;
        contexts[i].lastChangeMs = 0;
        contexts[i].historyCount = 0;
        contexts[i].historyIndex = 0;
        contexts[i].stepTimer = 0.0f;
        contexts[i].seeded = false;
    }
}

void GameOfLifeEffect::SetColors(RGBColor alive, RGBColor background) {
    aliveColor = alive;
    backgroundColor = background;
}

void GameOfLifeEffect::SetStepInterval(float seconds) {
    if (seconds < kMinStepInterval) {
        seconds = kMinStepInterval;
    } else if (seconds > kMaxStepInterval) {
        seconds = kMaxStepInterval;
    }

    stepInterval = seconds;
}

GameOfLifeEffect::Context* GameOfLifeEffect::GetContext(IPixelGroup* pixelGroup) {
    if (!pixelGroup) {
        return nullptr;
    }

    for (uint8_t i = 0; i < kMaxContexts; ++i) {
        if (contexts[i].group == pixelGroup) {
            if (contexts[i].pixelCount != pixelGroup->GetPixelCount()) {
                AllocateContext(&contexts[i], pixelGroup->GetPixelCount());
            }
            return &contexts[i];
        }
    }

    for (uint8_t i = 0; i < kMaxContexts; ++i) {
        if (!contexts[i].group) {
            contexts[i].group = pixelGroup;
            AllocateContext(&contexts[i], pixelGroup->GetPixelCount());
            return &contexts[i];
        }
    }

    contexts[0].group = pixelGroup;
    AllocateContext(&contexts[0], pixelGroup->GetPixelCount());
    return &contexts[0];
}

void GameOfLifeEffect::AllocateContext(Context* context, uint16_t pixelCount) {
    if (!context) {
        return;
    }

    if (context->pixelCount == pixelCount && context->cells && context->nextCells) {
        return;
    }

    delete[] context->cells;
    delete[] context->nextCells;
    context->cells = nullptr;
    context->nextCells = nullptr;

    context->pixelCount = pixelCount;
    context->bytes = static_cast<uint16_t>((pixelCount + 7) / 8);

    if (context->bytes == 0) {
        return;
    }

    context->cells = new uint8_t[context->bytes];
    context->nextCells = new uint8_t[context->bytes];

    ClearContext(context);
    ResetContext(context, context->group);
}

void GameOfLifeEffect::ClearContext(Context* context) {
    if (!context || !context->cells || !context->nextCells) {
        return;
    }

    memset(context->cells, 0, context->bytes);
    memset(context->nextCells, 0, context->bytes);
    context->seeded = false;
    context->stepTimer = 0.0f;
    context->lastUpdateMs = millis();
    context->lastChangeMs = context->lastUpdateMs;
    context->historyCount = 0;
    context->historyIndex = 0;
    memset(context->history, 0, sizeof(context->history));
}

void GameOfLifeEffect::ResetContext(Context* context, IPixelGroup* pixelGroup) {
    if (!context || !context->cells || context->bytes == 0 || context->pixelCount == 0) {
        return;
    }

    memset(context->cells, 0, context->bytes);
    memset(context->nextCells, 0, context->bytes);

    uint32_t population = 0;
    for (uint16_t i = 0; i < context->pixelCount; ++i) {
        bool alive = (random(0, 100) < kSpawnChancePercent);
        SetCell(context, i, alive);
        if (alive) {
            ++population;
        }
    }

    if (population < 3 && pixelGroup) {
        uint16_t center = context->pixelCount / 2;
        SetCell(context, center, true);

        uint16_t neighbor = 0;
        if (pixelGroup->GetOffsetXYIndex(center, &neighbor, 1, 0)) {
            SetCell(context, neighbor, true);
        }
        if (pixelGroup->GetOffsetXYIndex(center, &neighbor, 0, 1)) {
            SetCell(context, neighbor, true);
        }
        if (pixelGroup->GetOffsetXYIndex(center, &neighbor, 1, 1)) {
            SetCell(context, neighbor, true);
        }
        if (pixelGroup->GetOffsetXYIndex(center, &neighbor, -1, 1)) {
            SetCell(context, neighbor, true);
        }
    }

    context->seeded = true;
    context->stepTimer = 0.0f;
    context->lastUpdateMs = millis();
    context->lastChangeMs = context->lastUpdateMs;
    context->historyCount = 0;
    context->historyIndex = 0;
    memset(context->history, 0, sizeof(context->history));
    RecordHash(context, ComputeHash(context));
}

bool GameOfLifeEffect::GetCell(const Context* context, uint16_t index) const {
    if (!context || !context->cells) {
        return false;
    }

    uint16_t byteIndex = index >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(index & 7);

    return (context->cells[byteIndex] & (1u << bitIndex)) != 0;
}

void GameOfLifeEffect::SetCell(Context* context, uint16_t index, bool value) {
    if (!context || !context->cells) {
        return;
    }

    uint16_t byteIndex = index >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(index & 7);
    uint8_t mask = static_cast<uint8_t>(1u << bitIndex);

    if (value) {
        context->cells[byteIndex] |= mask;
    } else {
        context->cells[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}

uint32_t GameOfLifeEffect::ComputeHash(const Context* context) const {
    if (!context || !context->cells || context->bytes == 0) {
        return 0;
    }

    uint32_t hash = 2166136261u;
    for (uint16_t i = 0; i < context->bytes; ++i) {
        hash ^= context->cells[i];
        hash *= 16777619u;
    }

    return hash;
}

bool GameOfLifeEffect::IsHashInHistory(const Context* context, uint32_t hash) const {
    if (!context || context->historyCount == 0) {
        return false;
    }

    for (uint8_t i = 0; i < context->historyCount; ++i) {
        if (context->history[i] == hash) {
            return true;
        }
    }

    return false;
}

void GameOfLifeEffect::RecordHash(Context* context, uint32_t hash) {
    if (!context) {
        return;
    }

    context->history[context->historyIndex] = hash;
    context->historyIndex = static_cast<uint8_t>((context->historyIndex + 1) % kHashHistory);
    if (context->historyCount < kHashHistory) {
        context->historyCount++;
    }
}

uint8_t GameOfLifeEffect::CountNeighbors(const Context* context, IPixelGroup* pixelGroup, uint16_t index) const {
    if (!context || !pixelGroup) {
        return 0;
    }

    uint8_t count = 0;
    for (int8_t dy = -1; dy <= 1; ++dy) {
        for (int8_t dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            uint16_t neighbor = 0;
            if (pixelGroup->GetOffsetXYIndex(index, &neighbor, dx, dy)) {
                if (GetCell(context, neighbor)) {
                    ++count;
                }
            }
        }
    }

    return count;
}

void GameOfLifeEffect::Step(Context* context, IPixelGroup* pixelGroup) {
    if (!context || !pixelGroup || !context->cells || !context->nextCells) {
        return;
    }

    for (uint16_t i = 0; i < context->pixelCount; ++i) {
        uint8_t neighbors = CountNeighbors(context, pixelGroup, i);
        bool alive = GetCell(context, i);
        bool nextAlive = (neighbors == 3) || (alive && neighbors == 2);

        uint16_t byteIndex = i >> 3;
        uint8_t bitIndex = static_cast<uint8_t>(i & 7);
        uint8_t mask = static_cast<uint8_t>(1u << bitIndex);

        if (nextAlive) {
            context->nextCells[byteIndex] |= mask;
        } else {
            context->nextCells[byteIndex] &= static_cast<uint8_t>(~mask);
        }
    }

    uint8_t* temp = context->cells;
    context->cells = context->nextCells;
    context->nextCells = temp;

    uint32_t now = millis();
    uint32_t hash = ComputeHash(context);
    bool isRepeat = IsHashInHistory(context, hash);
    if (!isRepeat) {
        context->lastChangeMs = now;
    }

    RecordHash(context, hash);

    if (now - context->lastChangeMs >= kStableResetMs) {
        ResetContext(context, pixelGroup);
    }
}

void GameOfLifeEffect::Update(Context* context, IPixelGroup* pixelGroup) {
    if (!context || !pixelGroup) {
        return;
    }

    uint32_t now = millis();
    if (context->lastUpdateMs == 0) {
        context->lastUpdateMs = now;
        return;
    }

    float delta = (now - context->lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) {
        delta = kMaxDelta;
    }
    context->lastUpdateMs = now;

    context->stepTimer += delta;
    while (context->stepTimer >= stepInterval) {
        context->stepTimer -= stepInterval;
        Step(context, pixelGroup);
    }
}

void GameOfLifeEffect::ApplyEffect(IPixelGroup* pixelGroup) {
    if (!pixelGroup) {
        return;
    }

    Context* context = GetContext(pixelGroup);
    if (!context || context->pixelCount == 0) {
        return;
    }

    if (!context->seeded) {
        ResetContext(context, pixelGroup);
    }

    Update(context, pixelGroup);

    RGBColor* pixelColors = pixelGroup->GetColors();
    for (uint16_t i = 0; i < context->pixelCount; ++i) {
        pixelColors[i] = GetCell(context, i) ? aliveColor : backgroundColor;
    }
}
