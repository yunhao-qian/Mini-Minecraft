#ifndef MINI_MINECRAFT_WATER_WAVE_H
#define MINI_MINECRAFT_WATER_WAVE_H

#include <glm/glm.hpp>

namespace minecraft {

float getWaterWaveOffset(const glm::vec2 position, const float time);

float getAverageWaterWaveOffset();

} // namespace minecraft

#endif // MINI_MINECRAFT_WATER_WAVE_H
