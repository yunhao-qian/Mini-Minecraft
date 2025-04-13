#include "terrain_chunk_generation_task.h"

#include "block_face_generation_task.h"
#include "block_type.h"
#include "glm/common.hpp"

#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
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

    auto distance1{3.0f}; // Closest distance
    auto distance2{3.0f}; // Second closest distance

    // Check the 3x3 grid of cells around the point.
    for (const auto dX : {-1.0f, 0.0f, 1.0f}) {
        for (const auto dZ : {-1.0f, 0.0f, 1.0f}) {
            auto neighborPosition{floorPosition};
            neighborPosition += glm::vec2{dX, dZ};
            neighborPosition += random2D(neighborPosition);

            const auto distance{glm::distance(position, neighborPosition)};
            if (distance < distance1) {
                distance2 = distance1;
                distance1 = distance;
            } else if (distance < distance2) {
                distance2 = distance;
            }
        }
    }

    // The maximum difference between d1 and d2 is roughly sqrt(2).
    return (distance2 - distance1) / std::numbers::sqrt2_v<float>;
}

float getPlainElevation(const glm::vec2 position)
{
    return 140.0f + 3.0f * glm::perlin(position * 0.0005f);
}

float getGrasslandElevation(const glm::vec2 position)
{
    const auto scaledPosition{position * 0.015f};
    // Add smooth Perlin noise to the sampling coordinates to deform the straight lines in Worley
    // noise.
    const glm::vec2 perturbation{
        glm::perlin(scaledPosition),
        glm::perlin(scaledPosition + glm::vec2{1.5f, 6.7f}),
    };
    const auto perturbedPosition{position + 40.0f * perturbation};
    const auto scaledPerturbedPosition{perturbedPosition * 0.006f};
    const auto perlin{glm::perlin(scaledPerturbedPosition) * 0.5f + 0.5f};
    const auto worley{worleyNoise(scaledPerturbedPosition)};
    // 3/4 of Perlin noise and 1/4 of Worley noise
    const auto mixedNoise{std::lerp(perlin, worley, 0.25f)};
    return 130.0f + 32.0f * mixedNoise;
}

float getMountainElevation(const glm::vec2 position)
{
    auto total{0.0f};
    auto frequency{0.008f};
    auto amplitude{0.5f};
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 8)) {
        total += std::abs(glm::perlin(position * frequency) * amplitude);
        frequency *= 2.5f;
        amplitude *= 0.4f;
    }
    return 128.0f + 176.0f * total;
}

float getRiverElevation(const glm::vec2 position)
{
    const auto scaledPosition{position * 0.05f};
    const glm::vec2 perturbation{
        glm::perlin(scaledPosition),
        glm::perlin(scaledPosition + glm::vec2{1.5f, 6.7f}),
    };
    const auto perturbedPosition{position + 10.0f * perturbation};
    const auto scaledPerturbedPosition{perturbedPosition * 0.003f};
    const auto perlin{glm::perlin(scaledPerturbedPosition)};
    return 132.0f + 8.0f * glm::smoothstep(0.02f, 0.1f, std::abs(perlin));
}

float getMaxGrassElevation(const glm::vec2 position)
{
    return 160.0f + 8.0f * glm::perlin(position * 0.0005f);
}

float getSnowLineElevation(const glm::vec2 position)
{
    return 200.0f + 6.0f * glm::perlin(position * 0.04f);
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
    const auto centerXZ{glm::vec2{_chunk->originXZ() + localXZ} + 0.5f};

    // Determine the elevation based on interpolating between different biomes.
    const auto biomeValue{glm::perlin(centerXZ * 0.002f) * 0.5f + 0.5f};
    float floatElevation;
    if (const auto grasslandThreshold(0.5f + 0.05f * glm::perlin(centerXZ * 0.008f));
        biomeValue < grasslandThreshold) {
        // Interpolation between plain and grassland
        auto interpolation{biomeValue / grasslandThreshold};
        interpolation = glm::smoothstep(0.2f, 0.8f, interpolation);
        const auto plainElevation{getPlainElevation(centerXZ)};
        const auto grasslandElevation{getGrasslandElevation(centerXZ)};
        floatElevation = std::lerp(plainElevation, grasslandElevation, interpolation);
    } else {
        // Interpolation between grassland and mountain
        auto interpolation{(biomeValue - grasslandThreshold) / (1.0f - grasslandThreshold)};
        interpolation = glm::smoothstep(0.2f, 0.8f, interpolation);
        const auto grasslandElevation{getGrasslandElevation(centerXZ)};
        const auto mountainElevation{getMountainElevation(centerXZ)};
        floatElevation = std::lerp(grasslandElevation, mountainElevation, interpolation);
    }

    // Carve the terrain by rivers.
    const auto riverElevation{getRiverElevation(centerXZ)};
    float riverWeight = 1.0f - glm::smoothstep(0.6f, 0.9f, biomeValue);
    riverWeight *= 1.0f - glm::smoothstep(136.0f, 140.0f, riverElevation);
    floatElevation = std::lerp(floatElevation, riverElevation, riverWeight);

    // Determine the final elevation of the terrain.
    const auto intElevation{std::clamp(static_cast<int>(std::round(floatElevation)), 128, 256)};

    _chunk->setBlockAtLocal(glm::ivec3(localX, 0, localZ), BlockType::Bedrock);

    // y = 1, ..., 127 is the stone layer with caves.
    for (const auto y : std::views::iota(1, 128)) {
        // Cave generation is based on Perlin noise.
        const glm::vec3 centerPosition{centerXZ[0], static_cast<float>(y) + 0.5f, centerXZ[1]};
        const auto noise{glm::perlin(centerPosition * 0.03f)};

        BlockType block;
        if (noise >= 0.0f) {
            block = BlockType::Stone;
        } else if (y < 25) {
            block = BlockType::Lava;
        } else {
            block = BlockType::Air;
        }

        _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, block);
    }

    const glm::ivec3 topPosition{localX, intElevation - 1, localZ};

    if (floatElevation < getMaxGrassElevation(centerXZ)) {
        // Plain or grassland
        for (const auto y : std::views::iota(128, intElevation)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Dirt);
        }
        if (intElevation > 136) {
            _chunk->setBlockAtLocal(topPosition, BlockType::Grass);
        }
    } else {
        // Mountain
        for (const auto y : std::views::iota(128, intElevation)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Stone);
        }
        if (floatElevation > getSnowLineElevation(centerXZ)) {
            _chunk->setBlockAtLocal(topPosition, BlockType::Snow);
        }
    }

    if (intElevation < 138) {
        // Add water
        for (const auto y : std::views::iota(intElevation, 138)) {
            _chunk->setBlockAtLocal(glm::ivec3{localX, y, localZ}, BlockType::Water);
        }
    }
}

} // namespace minecraft
