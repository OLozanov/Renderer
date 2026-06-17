#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Simple.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"

#include <algorithm>

void SkyVertexShader(const SkyData* data, const SimpleVertex* in, float* out) noexcept
{
    __m128 x = _mm_set_ps1(in->position.x);
    __m128 y = _mm_set_ps1(in->position.y);
    __m128 z = _mm_set_ps1(in->position.z);

    __m128 pos = _mm_load_ps(&data->projView[3][0]);
    pos = _mm_fmadd_ps(_mm_load_ps(&data->projView[0][0]), x, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&data->projView[1][0]), y, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&data->projView[2][0]), z, pos);

    _mm_store_ps(out, pos);

    glm::vec2* attrib = reinterpret_cast<glm::vec2*>(out + 4);
    out[4] = in->uv.x;
    out[5] = in->uv.y;
}

void vectorcall SkyPixelShader(const SkyData* data,
                               uint32_t pid, uint32_t tx, uint32_t ty,
                               const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                               __m256 u, __m256 v, __m256 w, __m256 rz,
                               __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t face = pid / 2;
    const Render::Image* texture = data->faces[face];

    __m256 attribx[3];
    __m256 attriby[3];

    attribx[0] = _mm256_set1_ps(attrib_a->x);
    attribx[1] = _mm256_set1_ps(attrib_b->x);
    attribx[2] = _mm256_set1_ps(attrib_c->x);

    attriby[0] = _mm256_set1_ps(attrib_a->y);
    attriby[1] = _mm256_set1_ps(attrib_b->y);
    attriby[2] = _mm256_set1_ps(attrib_c->y);

    __m256 tcoordx = _mm256_mul_ps(attribx[0], u);
    __m256 tcoordy = _mm256_mul_ps(attriby[0], u);

    tcoordx = _mm256_fmadd_ps(attribx[1], v, tcoordx);
    tcoordy = _mm256_fmadd_ps(attriby[1], v, tcoordy);

    tcoordx = _mm256_fmadd_ps(attribx[2], w, tcoordx);
    tcoordy = _mm256_fmadd_ps(attriby[2], w, tcoordy);

    tcoordx = _mm256_mul_ps(tcoordx, rz);
    tcoordy = _mm256_mul_ps(tcoordy, rz);

    __m256 min = _mm256_setzero_ps();
    __m256 xmax = _mm256_set1_ps(1.0f - 1.0f / texture->width());
    __m256 ymax = _mm256_set1_ps(1.0f - 1.0f / texture->height());

    // clamp
    tcoordx = _mm256_max_ps(_mm256_min_ps(tcoordx, xmax), min);
    tcoordy = _mm256_max_ps(_mm256_min_ps(tcoordy, ymax), min);

    SampleLod0(texture, tcoordx, tcoordy, r, g, b, a);
}