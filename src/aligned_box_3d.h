#ifndef MINI_MINECRAFT_ALIGNED_BOX_3D_H
#define MINI_MINECRAFT_ALIGNED_BOX_3D_H

#include <glm/glm.hpp>

namespace minecraft {

class AlignedBox3D
{
public:
    AlignedBox3D(const glm::vec3 &minPoint, const glm::vec3 &maxPoint);

    const glm::vec3 &minPoint() const;
    const glm::vec3 &maxPoint() const;

    bool sweep(const glm::vec3 &velocity,
               const AlignedBox3D &other,
               float &hitTime,
               glm::vec3 &hitNormal) const;

private:
    glm::vec3 _minPoint;
    glm::vec3 _maxPoint;
};

inline AlignedBox3D::AlignedBox3D(const glm::vec3 &minPoint, const glm::vec3 &maxPoint)
    : _minPoint{minPoint}
    , _maxPoint{maxPoint}
{}

inline const glm::vec3 &AlignedBox3D::minPoint() const
{
    return _minPoint;
}

inline const glm::vec3 &AlignedBox3D::maxPoint() const
{
    return _maxPoint;
}

} // namespace minecraft

#endif // MINI_MINECRAFT_ALIGNED_BOX_3D_H
