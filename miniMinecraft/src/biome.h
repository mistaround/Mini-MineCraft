#ifndef BIOME_H
#define BIOME_H

#include "glm_includes.h"

enum BiomeType : unsigned char
{
    PLAIN, MOUNTAIN, DESERT, SNOWPEAK, MARSH, OAK_FOREST, DARK_FOREST, BIRCH_FOREST, LAKE
};

/* ------------- Noise Functions -------------- */
float PerlinNoise(glm::vec2 uv);
float PerlinNoise(glm::vec3 pos);
float SimplexNoise(glm::vec2 uv);
float SimplexNoise(glm::vec3 pos);
float WorleyNoise(glm::vec2 uv);
glm::vec2 randomizeUV(glm::vec2 uv, float amp, float freq);

float peakHeight(glm::vec2 xz);
float midHeight(glm::vec2 xz);
float lowHeight(glm::vec2 xz);

#endif // BIOME_H
