#pragma once

#include "Geometry.h"
#include "Render/Pipeline/Surface.h"

static constexpr uint32_t LtGridXSz = 32;
static constexpr uint32_t LtGridYSz = 24;
static constexpr uint32_t GridMaxLights = 32;

struct Light
{
    glm::vec3 pos;
    float radius;
    float falloff;

    glm::vec3 color;
};

struct SceneData
{
    glm::mat4 projView;
    glm::mat4 shadowMat;

    glm::vec3 viewPos;
    glm::vec3 lightDir;

    Render::Image* shadow;

    const Material* materials;
    const uint32_t* faceData;

    Light* lights;
    uint32_t* viewLights;
    uint32_t ltnum;

    uint32_t ltgridx;
    uint32_t ltgridy;

    uint32_t* ltgrid;
};

struct LightGridData
{
    const Render::DepthBuffer* depth;

    glm::mat4 projView;

    uint32_t xblocks;
    float zfactor;

    Light* lights;
    uint32_t* viewLights;
    uint32_t ltnum;

    uint32_t ltgridx;
    uint32_t ltgridy;

    uint32_t* ltgrid;
};

struct SkyData
{
    glm::mat4 projView;
    const Render::Image* faces[6];
};

struct EffectsData
{
    glm::mat4 projView;
    glm::vec2 x, z;

    Render::Image* fire;
};

struct FlameInstance
{
    EffectsData* data;

    glm::vec3 pos;
    glm::vec3 color;
};