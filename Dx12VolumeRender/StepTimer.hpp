#pragma once

#include <pch.h>

namespace MyDirectX12 {
    // Helper class for animation and simulation timing.
    class StepTimer {
    public:
        StepTimer() noexcept(false) : elapsedTicks(0), totalTicks(0), leftOverTicks(0), frameCount(0), framesPerSecond(0), framesThisSecond(0), qpcSecondCounter(0), isFixedTimeStep(false), targetElapsedTicks(TicksPerSecond / 60) {
            if (!QueryPerformanceFrequency(&qpcFrequency)) {
                throw std::exception();
            }

            if (!QueryPerformanceCounter(&qpcLastTime)) {
                throw std::exception();
            }

            // Initialize max delta to 1/10 of a second.
            qpcMaxDelta = static_cast<uint64_t>(qpcFrequency.QuadPart / 10);
        }

        // Get elapsed time since the previous Update call.
        uint64_t GetElapsedTicks() const noexcept { return elapsedTicks; }
        double GetElapsedSeconds() const noexcept { return TicksToSeconds(elapsedTicks); }

        // Get total time since the start of the program.
        uint64_t GetTotalTicks() const noexcept { return totalTicks; }
        double GetTotalSeconds() const noexcept { return TicksToSeconds(totalTicks); }

        // Get total number of updates since start of the program.
        uint32_t GetFrameCount() const noexcept { return frameCount; }

        // Get the current framerate.
        uint32_t GetFramesPerSecond() const noexcept { return framesPerSecond; }

        // Set whether to use fixed or variable timestep mode.
        void SetFixedTimeStep(bool isFixedTimestep) noexcept { isFixedTimeStep = isFixedTimestep; }

        // Set how often to call Update when in fixed timestep mode.
        void SetTargetElapsedTicks(uint64_t targetElapsed) noexcept { targetElapsedTicks = targetElapsed; }
        void SetTargetElapsedSeconds(double targetElapsed) noexcept { targetElapsedTicks = SecondsToTicks(targetElapsed); }

        // Integer format represents time using 10,000,000 ticks per second.
        static constexpr uint64_t TicksPerSecond = 10000000;

        static constexpr double TicksToSeconds(uint64_t ticks) noexcept { return static_cast<double>(ticks) / TicksPerSecond; }
        static constexpr uint64_t SecondsToTicks(double seconds) noexcept { return static_cast<uint64_t>(seconds * TicksPerSecond); }

        // After an intentional timing discontinuity (for instance a blocking IO operation)
        // call this to avoid having the fixed timestep logic attempt a set of catch-up
        // Update calls.

        void ResetElapsedTime() {
            if (!QueryPerformanceCounter(&qpcLastTime)) {
                throw std::exception();
            }

            leftOverTicks = 0;
            framesPerSecond = 0;
            framesThisSecond = 0;
            qpcSecondCounter = 0;
        }

        // Update timer state, calling the specified Update function the appropriate number of times.
        template<typename TUpdate>
        void Tick(const TUpdate& update) {
            // Query the current time.
            LARGE_INTEGER currentTime;

            if (!QueryPerformanceCounter(&currentTime)) {
                throw std::exception();
            }

            uint64_t timeDelta = static_cast<uint64_t>(currentTime.QuadPart - qpcLastTime.QuadPart);

            qpcLastTime = currentTime;
            qpcSecondCounter += timeDelta;

            // Clamp excessively large time deltas (e.g. after paused in the debugger).
            if (timeDelta > qpcMaxDelta) {
                timeDelta = qpcMaxDelta;
            }

            // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
            timeDelta *= TicksPerSecond;
            timeDelta /= static_cast<uint64_t>(qpcFrequency.QuadPart);

            const uint32_t lastFrameCount = frameCount;

            if (isFixedTimeStep) {
                // Fixed timestep update logic

                // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
                // the clock to exactly match the target value. This prevents tiny and irrelevant errors
                // from accumulating over time. Without this clamping, a game that requested a 60 fps
                // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
                // accumulate enough tiny errors that it would drop a frame. It is better to just round
                // small deviations down to zero to leave things running smoothly.

                if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(timeDelta - targetElapsedTicks))) < TicksPerSecond / 4000) {
                    timeDelta = targetElapsedTicks;
                }

                leftOverTicks += timeDelta;

                while (leftOverTicks >= targetElapsedTicks) {
                    elapsedTicks = targetElapsedTicks;
                    totalTicks += targetElapsedTicks;
                    leftOverTicks -= targetElapsedTicks;
                    frameCount++;

                    update();
                }
            }
            else {
                // Variable timestep update logic.
                elapsedTicks = timeDelta;
                totalTicks += timeDelta;
                leftOverTicks = 0;
                frameCount++;

                update();
            }

            // Track the current framerate.
            if (frameCount != lastFrameCount) {
                framesThisSecond++;
            }

            if (qpcSecondCounter >= static_cast<uint64_t>(qpcFrequency.QuadPart)) {
                framesPerSecond = framesThisSecond;
                framesThisSecond = 0;
                qpcSecondCounter %= static_cast<uint64_t>(qpcFrequency.QuadPart);
            }
        }

    private:
        // Source timing data uses QPC units.
        LARGE_INTEGER qpcFrequency;
        LARGE_INTEGER qpcLastTime;
        uint64_t qpcMaxDelta;

        // Derived timing data uses a canonical tick format.
        uint64_t elapsedTicks;
        uint64_t totalTicks;
        uint64_t leftOverTicks;

        // Members for tracking the framerate.
        uint32_t frameCount;
        uint32_t framesPerSecond;
        uint32_t framesThisSecond;
        uint64_t qpcSecondCounter;

        // Members for configuring fixed timestep mode.
        bool isFixedTimeStep;
        uint64_t targetElapsedTicks;
    };
}