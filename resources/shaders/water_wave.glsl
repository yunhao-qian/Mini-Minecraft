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
// The exponent cannot be strictly 1.0, as it may cause NaN values when computing the derivatives.
const float WaterWaveExponents[WaterWaveCount] = float[](1.2, 3.0, 2.0, 2.5, 1.4, 2.2);

// Effective water simulation from physical models:
// https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models

float getWaterWaveOffset(vec2 position, float time, float amplitudeScale)
{
    float offset = 0.0;
    for (int i = 0; i < WaterWaveCount; ++i) {
        float phase = dot(WaterWaveAngularWaveVectors[i], position) * WaterWaveAngularFrequencies[i]
                      + time * WaterWavePhaseOffsets[i];
        float sinPositive = max((sin(phase) + 1.0) * 0.5, 0.0);
        offset += 2.0 * WaterWaveAmplitudes[i] * pow(sinPositive, WaterWaveExponents[i]);
    }
    offset *= amplitudeScale;
    return offset - 0.5;
}

vec3 getWaterWaveNormal(vec2 position, float time, float amplitudeScale)
{
    vec2 derivative = vec2(0.0, 0.0);
    for (int i = 0; i < WaterWaveCount; ++i) {
        float phase = dot(WaterWaveAngularWaveVectors[i], position) * WaterWaveAngularFrequencies[i]
                      + time * WaterWavePhaseOffsets[i];
        float sinPositive = max((sin(phase) + 1.0) * 0.5, 0.0);
        derivative += WaterWaveExponents[i] * WaterWaveAngularWaveVectors[i]
                      * WaterWaveAngularFrequencies[i] * WaterWaveAmplitudes[i]
                      * pow(sinPositive, WaterWaveExponents[i] - 1.0) * cos(phase);
    }
    derivative *= amplitudeScale;
    return normalize(vec3(-derivative.x, 1.0, -derivative.y));
}
