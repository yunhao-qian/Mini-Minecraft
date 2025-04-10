#include "terrain_chunk_generation_task.h"

#include "block_face_generation_task.h"
#include "block_type.h"

#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <numbers>
#include <ranges>

namespace minecraft {

namespace {

glm::vec2 random2D(const glm::vec2 position)
{
    return glm::fract(glm::sin(glm::vec2{
                          glm::dot(position, glm::vec2{127.1f, 311.7f}),
                          glm::dot(position, glm::vec2{269.5f, 183.3f}),
                      })
                      * 43758.5453f);
}

float worleyNoise(const glm::vec2 position)
{
    const auto floorPosition{glm::floor(position)};

    auto d1{3.0f}; // Closest distance
    auto d2{3.0f}; // Second closest distance

    // Check the 3x3 grid of cells around the point.
    for (const auto dX : {-1.0f, 0.0f, 1.0f}) {
        for (const auto dZ : {-1.0f, 0.0f, 1.0f}) {
            auto neighborPosition{floorPosition};
            neighborPosition += glm::vec2{dX, dZ};
            neighborPosition += random2D(neighborPosition);

            const auto displacement{neighborPosition - position};
            const auto distance{glm::dot(displacement, displacement)};
            if (distance < d1) {
                d2 = d1;
                d1 = distance;
            } else if (distance < d2) {
                d2 = distance;
            }
        }
    }

    // The maximum difference between d1 and d2 is roughly sqrt(2).
    return (d2 - d1) / std::numbers::sqrt2_v<float>;
}

float getGrasslandHeight(const glm::vec2 position)
{
    const auto scaledPosition{position * 0.02f};

    // Add smooth Perlin noise to the sampling coordinates to deform the straight lines in Worley
    // noise.
    const auto perturbedPosition{position
                                 + 10.0f
                                       * glm::vec2{
                                           glm::perlin(scaledPosition),
                                           glm::perlin(scaledPosition + glm::vec2{1.5f, 6.7f}),
                                       }};
    const auto scaledPerturbedPosition{perturbedPosition * 0.01f};

    // 1/3 of Perlin noise and 2/3 of Worley noise
    const auto noise{std::lerp(glm::perlin(scaledPerturbedPosition) * 0.5f + 0.5f,
                               worleyNoise(scaledPerturbedPosition),
                               1.0f / 3.0f)};
    return noise * 36.0f + 128.0f;
}

float getMountainHeight(const glm::vec2 position)
{
    auto total{0.0f};
    auto frequency{0.02f};
    auto amplitude{0.5f};
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 8)) {
        total += std::abs(glm::perlin(position * frequency) * amplitude);
        frequency *= 2.0f;
        amplitude *= 0.4f;
    }
    return total * 96.0f + 160.0f;
}

} // namespace

void TerrainChunkGenerationTask::run()
{
    for (const auto localX : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto localZ : std::views::iota(0, TerrainChunk::SizeZ)) {
            generateColumn(glm::ivec2{localX, localZ});
        }
    }
    {
        // After generating a new terrain chunk, we had better get its instance attributes ready.
        // Otherwise, the chunk will spend a few additional frames to generate the attributes.
        // This task can be a very large object, so we allocate it on the heap to avoid stack
        // overflow errors.
        const auto task{std::make_unique<BlockFaceGenerationTask>(_chunk.get())};
        task->run();
    }
    const std::lock_guard lock{_streamer->_mutex};
    _streamer->_pendingChunks.erase(_chunk->originXZ());
    _streamer->_readyChunks.push_back(std::move(_chunk));
}

void TerrainChunkGenerationTask::generateColumn(const glm::ivec2 localXZ)
{
    const auto localX{localXZ[0]};
    const auto localZ{localXZ[1]};

    for (const auto y : std::views::iota(0, 128)) {
        _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Stone);
    }

    const glm::vec2 centerXZ{glm::vec2{_chunk->originXZ() + localXZ} + 0.5f};

    const auto biomeInterpolation{glm::smoothstep(0.2f, 0.6f, glm::perlin(centerXZ * 0.005f))};

    const auto floatHeight{
        std::lerp(getGrasslandHeight(centerXZ), getMountainHeight(centerXZ), biomeInterpolation)};
    const auto intHeight{std::clamp(static_cast<int>(std::round(floatHeight)), 128, 256)};

    if (biomeInterpolation < 0.15f) {
        // Grassland biome
        for (const auto y : std::views::iota(128, intHeight)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Dirt);
        }
        if (intHeight > 128) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, intHeight - 1, localZ}, BlockType::Grass);
        }
    } else {
        // Mountain biome
        for (const auto y : std::views::iota(128, intHeight)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Stone);
        }
        if (intHeight > 200) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, intHeight - 1, localZ}, BlockType::Snow);
        }
    }

    if (intHeight < 138) {
        for (const auto y : std::views::iota(intHeight, 138)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, minecraft::BlockType::Water);
        }
    }
}

} // namespace minecraft
