#ifndef MINI_MINECRAFT_ALIGNED_BOX_H
#define MINI_MINECRAFT_ALIGNED_BOX_H

#include <glm/glm.hpp>

namespace minecraft {

class AlignedBox
{
public:
    AlignedBox(const glm::vec3 &minP, const glm::vec3 &maxP);

    auto minP() const -> const glm::vec3 &;
    auto maxP() const -> const glm::vec3 &;

    auto sweep(const glm::vec3 &velocity,
               const AlignedBox &other,
               float &hitTime,
               glm::vec3 &hitNormal) const -> bool;

private:
    glm::vec3 _minP;
    glm::vec3 _maxP;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_ALIGNED_BOX_H
