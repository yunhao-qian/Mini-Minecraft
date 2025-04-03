#include "aligned_box.h"

#include <algorithm>
#include <ranges>

minecraft::AlignedBox::AlignedBox(const glm::vec3 &minP, const glm::vec3 &maxP)
    : _minP{minP}
    , _maxP{maxP}
{}

auto minecraft::AlignedBox::minP() const -> const glm::vec3 &
{
    return _minP;
}

auto minecraft::AlignedBox::maxP() const -> const glm::vec3 &
{
    return _maxP;
}

auto minecraft::AlignedBox::sweep(const glm::vec3 &velocity,
                                  const AlignedBox &other,
                                  float &hitTime,
                                  glm::vec3 &hitNormal) const -> bool
{
    auto minTime{0.0f};
    auto maxTime{hitTime};
    glm::vec3 firstHitNormal;

    const auto minDisplacement{other.minP() - _maxP};
    const auto maxDisplacement{other.maxP() - _minP};

    for (const auto i : std::views::iota(0, 3)) {
        if (velocity[i] > 0.0f) {
            if (const auto axisMinTime{minDisplacement[i] / velocity[i]}; axisMinTime > minTime) {
                minTime = axisMinTime;
                firstHitNormal = {0.0f, 0.0f, 0.0f};
                firstHitNormal[i] = 1.0f;
            }
            maxTime = std::min(maxTime, maxDisplacement[i] / velocity[i]);
        } else if (velocity[i] < 0.0f) {
            if (const auto axisMinTime{maxDisplacement[i] / velocity[i]}; axisMinTime > minTime) {
                minTime = axisMinTime;
                firstHitNormal = {0.0f, 0.0f, 0.0f};
                firstHitNormal[i] = -1.0f;
            }
            maxTime = std::min(maxTime, minDisplacement[i] / velocity[i]);
        } else {
            if (minDisplacement[i] <= 0.0f && 0.0f <= maxDisplacement[i]) {
                continue;
            }
            return false;
        }
    }

    if (0.0f < minTime && minTime <= maxTime && minTime <= hitTime) {
        hitTime = minTime;
        hitNormal = firstHitNormal;
        return true;
    }
    return false;
}
