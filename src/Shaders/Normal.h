#pragma once

#include "Shaders/ShaderData.h"
#include <immintrin.h>

struct NormInParams
{
    glm::vec2 tcoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 binormal;
};

void NormalVertexShader(const SceneData* data, const float* in, float* out) noexcept;
void vectorcall NormalPixelShader(const SceneData* data,
                                  uint32_t pid, uint32_t tx, uint32_t ty,
                                  const NormInParams* attrib_a, const NormInParams* attrib_b, const NormInParams* attrib_c,
                                  __m256 u, __m256 v, __m256 w, __m256 rz,
                                  __m256& r, __m256& g, __m256& b, __m256& a) noexcept;