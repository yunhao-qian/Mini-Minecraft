#ifndef MINECRAFT_ALIGNED_BOX_3D_H
#define MINECRAFT_ALIGNED_BOX_3D_H

#include <glm/glm.hpp>

namespace minecraft {

class AlignedBox3D
{
public:
    AlignedBox3D()
        : AlignedBox3D{glm::vec3{0.0f}, glm::vec3{0.0f}}
    {}

    AlignedBox3D(const glm::vec3 &minPoint, const glm::vec3 &maxPoint)
        : _minPoint{minPoint}
        , _maxPoint{maxPoint}
    {}

    const glm::vec3 &minPoint() const { return _minPoint; }

    const glm::vec3 &maxPoint() const { return _maxPoint; }

    bool isEmpty() const { return !glm::any(glm::lessThan(_minPoint, _maxPoint)); }

    bool sweep(const glm::vec3 &velocity,
               const AlignedBox3D &other,
               float &hitTime,
               glm::vec3 &hitNormal) const;

private:
    glm::vec3 _minPoint;
    glm::vec3 _maxPoint;
};

} // namespace minecraft

#endif // MINECRAFT_ALIGNED_BOX_3D_H
