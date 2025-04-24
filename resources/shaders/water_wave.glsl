const int WaterWaveCount = 6;
const float WaterWaveAmplitudes[WaterWaveCount] = float[](0.016, 0.025, 0.020, 0.022, 0.028, 0.033);
const vec2 WaterWaveAngularWaveVectors[WaterWaveCount] = vec2[](vec2(-0.08, -0.23),
                                                                vec2(-0.23, 0.11),
                                                                vec2(0.23, -0.10),
                                                                vec2(-0.15, 0.20),
                                                                vec2(0.18, -0.18),
                                                                vec2(0.23, 0.10));
const float WaterWaveAngularFrequencies[WaterWaveCount] = float[](4.3, 3.2, 1.2, 4.2, 1.0, 2.4);
const float WaterWavePhaseOffsets[WaterWaveCount] = float[](3.451, 4.134, 4.771, 4.065, 4.776, 5.552);
const float WaterWaveExponents[WaterWaveCount] = float[](1.0, 2.0, 1.5, 2.5, 1.2, 1.6);

// Effective water simulation from physical models:
// https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models

float getWaterWaveOffset(vec2 position, float time)
{
    float offset = -0.5;
    for (int i = 0; i < WaterWaveCount; ++i) {
        float phase = dot(WaterWaveAngularWaveVectors[i], position) * WaterWaveAngularFrequencies[i]
                      + time * WaterWavePhaseOffsets[i];
        offset += 2.0 * WaterWaveAmplitudes[i] * pow((sin(phase) + 1.0) * 0.5, WaterWaveExponents[i]);
    }
    return offset;
}

vec3 getWaterWaveNormal(vec2 position, float time)
{
    vec2 derivative = vec2(0.0, 0.0);
    for (int i = 0; i < WaterWaveCount; ++i) {
        float phase = dot(WaterWaveAngularWaveVectors[i], position) * WaterWaveAngularFrequencies[i]
                      + time * WaterWavePhaseOffsets[i];
        derivative += WaterWaveExponents[i] * WaterWaveAngularWaveVectors[i]
                      * WaterWaveAngularFrequencies[i] * WaterWaveAmplitudes[i]
                      * pow((sin(phase) + 1.0) * 0.5, WaterWaveExponents[i] - 1.0) * cos(phase);
    }
    return normalize(vec3(-derivative.x, 1.0, -derivative.y));
}
