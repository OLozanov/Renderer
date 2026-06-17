#pragma once

#include "Shaders/ShaderData.h"
#include <immintrin.h>

void FireVertexShader(const FlameInstance* data, const Vertex* in, float* out) noexcept;
void vectorcall FirePixelShader(const FlameInstance* data,
                                uint32_t pid, uint32_t tx, uint32_t ty,
                                const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                                __m256 u, __m256 v, __m256 w, __m256 rz,
                                __m256& r, __m256& g, __m256& b, __m256& a) noexcept;