#pragma once

#include <cassert>
#include <cstdio>

class FramesPerSecondCounter
{
public:
    FramesPerSecondCounter(float avgInterval = 0.5f)
        : avgInterval(avgInterval)
    {
        assert(avgInterval > 0.0f);
    }

    bool tick(float deltaSeconds, bool frameRendered = true)
    {
        if (frameRendered) {
            numFrames++;
        }

        accumulatedTime += deltaSeconds;

        if (accumulatedTime > avgInterval) {
            currentFps = static_cast<float>(numFrames / accumulatedTime);

            if (printFps) {
                printf("FPS: %.1f\n", currentFps);
            }

            numFrames = 0;
            accumulatedTime = 0;
            return true;
        }

        return false;
    }

    inline float getFPS() const {
        return currentFps;
    }

    float avgInterval = 0.5f;
    int numFrames = 0;
    double accumulatedTime = 0;
    float currentFps = 0.0f;
    bool printFps = true;
};