#include "terrain_chunk_generation_task.h"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>

namespace {

auto random2D(const glm::vec2 position) -> glm::vec2
{
    return glm::fract(glm::sin(glm::vec2{glm::dot(position, glm::vec2{127.1f, 311.7f}),
                                         glm::dot(position, glm::vec2{269.5f, 183.3f})})
                      * 43758.5453f);
}

auto worleyNoise(const glm::vec2 position) -> float
{
    const auto floorPosition{glm::floor(position)};
    auto d1{3.0f};
    auto d2{3.0f};
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
    return (d2 - d1) / std::numbers::sqrt2_v<float>;
}

auto getGrasslandHeight(const glm::vec2 position) -> float
{
    const auto scaledPosition{position * 0.02f};
    const auto perturbedPosition{
        position
        + 10.0f
              * glm::vec2{glm::perlin(scaledPosition),
                          glm::perlin(scaledPosition + glm::vec2{1.5f, 6.7f})}};
    const auto scaledPerturbedPosition{perturbedPosition * 0.01f};
    const auto noise{std::lerp(glm::perlin(scaledPerturbedPosition) * 0.5f + 0.5f,
                               worleyNoise(scaledPerturbedPosition),
                               1.0f / 3.0f)};
    return noise * 36.0f + 128.0f;
}

auto getMountainHeight(const glm::vec2 position) -> float
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

minecraft::TerrainChunkGenerationTask::TerrainChunkGenerationTask(
    std::unique_ptr<TerrainChunk> chunk,
    std::mutex *const mutex,
    std::unordered_set<std::pair<int, int>, IntPairHash> *const pendingChunks,
    std::vector<std::unique_ptr<TerrainChunk>> *const finishedChunks)
    : _chunk{std::move(chunk)}
    , _mutex{mutex}
    , _pendingChunks{pendingChunks}
    , _finishedChunks{finishedChunks}
{}

auto minecraft::TerrainChunkGenerationTask::run() -> void
{
    for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
            generateColumn(x, z);
        }
    }
    {
        std::lock_guard lock{*_mutex};
        _pendingChunks->erase({_chunk->minX(), _chunk->minZ()});
        _finishedChunks->push_back(std::move(_chunk));
    }
}

auto minecraft::TerrainChunkGenerationTask::generateColumn(const int localX, const int localZ)
    -> void
{
    for (const auto y : std::views::iota(0, 128)) {
        _chunk->setBlockLocal(localX, y, localZ, BlockType::Stone);
    }

    const glm::vec2 center{static_cast<float>(_chunk->minX() + localX) + 0.5f,
                           static_cast<float>(_chunk->minZ() + localZ) + 0.5f};

    const auto biomeInterpolation{glm::smoothstep(0.2f, 0.6f, glm::perlin(center * 0.005f))};

    const auto floatHeight{
        std::lerp(getGrasslandHeight(center), getMountainHeight(center), biomeInterpolation)};
    const auto intHeight{std::clamp(static_cast<int>(std::round(floatHeight)), 128, 256)};

    if (biomeInterpolation < 0.15f) {
        // Grassland biome
        for (const auto y : std::views::iota(128, intHeight)) {
            _chunk->setBlockLocal(localX, y, localZ, BlockType::Dirt);
        }
        if (intHeight > 128) {
            _chunk->setBlockLocal(localX, intHeight - 1, localZ, BlockType::Grass);
        }
    } else {
        // Mountain biome
        for (const auto y : std::views::iota(128, intHeight)) {
            _chunk->setBlockLocal(localX, y, localZ, BlockType::Stone);
        }
        if (intHeight > 200) {
            _chunk->setBlockLocal(localX, intHeight - 1, localZ, BlockType::Snow);
        }
    }

    if (intHeight < 138) {
        for (const auto y : std::views::iota(intHeight, 138)) {
            _chunk->setBlockLocal(localX, y, localZ, minecraft::BlockType::Water);
        }
    }
}
