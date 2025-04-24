#include "aligned_box_3d.h"

#include <algorithm>
#include <ranges>

namespace minecraft {

bool AlignedBox3D::sweep(const glm::vec3 &velocity,
                         const AlignedBox3D &other,
                         float &hitTime,
                         glm::vec3 &hitNormal) const
{
    auto minTime{0.0f};
    auto maxTime{hitTime};
    glm::vec3 firstHitNormal{0.0f};

    const auto minDisplacement{other._minPoint - _maxPoint};
    const auto maxDisplacement{other._maxPoint - _minPoint};

    for (const auto i : std::views::iota(0, 3)) {
        if (velocity[i] > 0.0f) {
            if (const auto axisMinTime{minDisplacement[i] / velocity[i]}; axisMinTime > minTime) {
                minTime = axisMinTime;
                firstHitNormal = glm::vec3{0.0f};
                firstHitNormal[i] = 1.0f;
            }
            maxTime = std::min(maxTime, maxDisplacement[i] / velocity[i]);
        } else if (velocity[i] < 0.0f) {
            if (const auto axisMinTime{maxDisplacement[i] / velocity[i]}; axisMinTime > minTime) {
                minTime = axisMinTime;
                firstHitNormal = glm::vec3{0.0f};
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

} // namespace minecraft
