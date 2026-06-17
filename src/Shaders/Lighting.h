#pragma once

#include "Shaders/ShaderData.h"
#include <immintrin.h>

struct InParams
{
    glm::vec2 tcoord;
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 binormal;
};

void LightGridFillShader(const LightGridData* data, uint32_t x, uint32_t y) noexcept;
void LightingVertexShader(const SceneData* data, const Vertex* in, float* out) noexcept;
void vectorcall LightingPixelShader(const SceneData* data,
                                    uint32_t pid, uint32_t tx, uint32_t ty,
                                    const InParams* attrib_a, const InParams* attrib_b, const InParams* attrib_c,
                                    __m256 u, __m256 v, __m256 w, __m256 rz,
                                    __m256& r, __m256& g, __m256& b, __m256& a) noexcept;