#ifndef MINECRAFT_WATER_WAVE_H
#define MINECRAFT_WATER_WAVE_H

#include <glm/glm.hpp>

namespace minecraft {

float getWaterWaveOffset(const glm::vec2 position, const float time, const float amplitudeScale);

float getAverageWaterWaveOffset(const float amplitudeScale);

} // namespace minecraft

#endif // MINECRAFT_WATER_WAVE_H
