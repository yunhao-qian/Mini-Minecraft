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
    auto timeMin{0.0f};
    auto timeMax{hitTime};
    glm::vec3 firstHitNormal;

    const auto displacementMin{other.minP() - _maxP};
    const auto displacementMax{other.maxP() - _minP};
    for (const auto i : std::views::iota(0, 3)) {
        if (velocity[i] > 0.0f) {
            if (const auto axisTimeMin{displacementMin[i] / velocity[i]}; axisTimeMin > timeMin) {
                timeMin = axisTimeMin;
                firstHitNormal = {0.0f, 0.0f, 0.0f};
                firstHitNormal[i] = 1.0f;
            }
            timeMax = std::min(timeMax, displacementMax[i] / velocity[i]);
        } else if (velocity[i] < 0.0f) {
            if (const auto axisTimeMin{displacementMax[i] / velocity[i]}; axisTimeMin > timeMin) {
                timeMin = axisTimeMin;
                firstHitNormal = {0.0f, 0.0f, 0.0f};
                firstHitNormal[i] = -1.0f;
            }
            timeMax = std::min(timeMax, displacementMin[i] / velocity[i]);
        } else {
            if (displacementMin[i] <= 0.0f && 0.0f <= displacementMax[i]) {
                continue;
            }
            return false;
        }
    }

    if (0.0f < timeMin && timeMin <= timeMax && timeMin <= hitTime) {
        hitTime = timeMin;
        hitNormal = firstHitNormal;
        return true;
    }
    return false;
}
