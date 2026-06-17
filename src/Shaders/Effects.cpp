#include <glm/glm.hpp>

#include "Effects.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"


void FireVertexShader(const FlameInstance* data, const Vertex* in, float* out) noexcept
{
    const glm::vec2& xdir = data->data->x;
    const glm::vec2& zdir = data->data->z;

    glm::vec3 wpos = { in->position.x * xdir.x + in->position.z * zdir.x + data->pos.x,
                       in->position.y + data->pos.y,
                       in->position.x * xdir.y + in->position.z * zdir.y + data->pos.z };

    __m128 x = _mm_set_ps1(wpos.x);
    __m128 y = _mm_set_ps1(wpos.y);
    __m128 z = _mm_set_ps1(wpos.z);

    const glm::mat4& projView = data->data->projView;

    __m128 pos = _mm_load_ps(&projView[3][0]);
    pos = _mm_fmadd_ps(_mm_load_ps(&projView[0][0]), x, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&projView[1][0]), y, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&projView[2][0]), z, pos);

    _mm_store_ps(out, pos);

    glm::vec2* attrib = reinterpret_cast<glm::vec2*>(out + 4);
    out[4] = in->uv.x;
    out[5] = in->uv.y;
}

void vectorcall FirePixelShader(const FlameInstance* data,
                                uint32_t pid, uint32_t tx, uint32_t ty,
                                const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                                __m256 u, __m256 v, __m256 w, __m256 rz,
                                __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    Render::Image* texture = data->data->fire;

    __m256 tcoordx;
    __m256 tcoordy;

    InterpolateVec2(*attrib_a, *attrib_b, *attrib_c, u, v, w, rz, tcoordx, tcoordy);

    __m256 grad = Render::CalculateGrad(tcoordx, tcoordy);

    float lod1, lod2;
    Render::CalculateLod(texture, grad, lod1, lod2);

    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));

    SampleTrilinear(texture, tcoordx, tcoordy, r, g, b, a, lod1, lod2);

    uint32_t instance = 0;

    r = _mm256_mul_ps(r, _mm256_set1_ps(data->color.r));
    g = _mm256_mul_ps(g, _mm256_set1_ps(data->color.g));
    b = _mm256_mul_ps(b, _mm256_set1_ps(data->color.b));
}