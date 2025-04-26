#ifndef MINECRAFT_SCENE_SETTINGS_H
#define MINECRAFT_SCENE_SETTINGS_H

#include <cstdint>
#include <mutex>
#include <utility>

namespace minecraft {

struct SceneSettingsData
{
    float sunAltitude{30.0f};
    float sunAzimuth{60.0f};
    float waterRefractiveIndex{1.1f};
    float waterWaveAmplitudeScale{1.0f};
};

class SceneSettings
{
public:
    SceneSettings()
        : _mutex{}
        , _version{0}
        , _data{}
    {}

    std::mutex &mutex() { return _mutex; }

    std::pair<std::int32_t, SceneSettingsData> get()
    {
        const std::lock_guard lock{_mutex};
        return {_version, _data};
    }

    void set(const SceneSettingsData &data)
    {
        const std::lock_guard lock{_mutex};
        ++_version;
        _data = data;
    }

private:
    std::mutex _mutex;
    std::int32_t _version;
    SceneSettingsData _data;
};

} // namespace minecraft

#endif // MINECRAFT_SCENE_SETTINGS_H
