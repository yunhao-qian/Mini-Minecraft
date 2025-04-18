#include "water_wave.h"

#include <cmath>
#include <ranges>

namespace minecraft {

namespace {

// These wave parameters should be in sync with water_wave.glsl.

constexpr auto WaterWaveCount{6};
constexpr float WaterWaveAmplitudes[WaterWaveCount]{0.016f, 0.025f, 0.020f, 0.022f, 0.028f, 0.033f};
constexpr glm::vec2 WaterWaveAngularWaveVectors[WaterWaveCount]{
    {-0.08f, -0.23f},
    {-0.23f, 0.11f},
    {0.23f, -0.10f},
    {-0.15f, 0.20f},
    {0.18f, -0.18f},
    {0.23f, 0.10f},
};
constexpr float WaterWaveAngularFrequencies[WaterWaveCount]{4.3f, 3.2f, 1.2f, 4.2f, 1.0f, 2.4f};
constexpr float WaterWavePhaseOffsets[WaterWaveCount]{3.451f, 4.134f, 4.771f, 4.065f, 4.776f, 5.552f};
constexpr float WaterWaveExponents[WaterWaveCount]{1.0f, 2.0f, 1.5f, 2.5f, 1.2f, 1.6f};

} // namespace

float getWaterWaveOffset(const glm::vec2 position, const float time)
{
    auto offset{-0.5f};
    for (const auto i : std::views::iota(0, WaterWaveCount)) {
        const auto phase{glm::dot(WaterWaveAngularWaveVectors[i], position)
                             * WaterWaveAngularFrequencies[i]
                         + time * WaterWavePhaseOffsets[i]};
        offset += 2.0f * WaterWaveAmplitudes[i]
                  * std::pow((std::sin(phase) + 1.0f) * 0.5f, WaterWaveExponents[i]);
    }
    return offset;
}

float getAverageWaterWaveOffset()
{
    // std::pow() is not constexpr, so we have to compute it at runtime.

    static auto isFirstCall{true};
    static float averageOffset;

    if (!isFirstCall) {
        return averageOffset;
    }
    isFirstCall = false;

    averageOffset = -0.5f;
    for (const auto i : std::views::iota(0, WaterWaveCount)) {
        averageOffset += 2.0f * WaterWaveAmplitudes[i] * std::pow(0.5f, WaterWaveExponents[i]);
    }
    return averageOffset;
}

} // namespace minecraft
