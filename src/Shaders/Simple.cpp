#include <glm/glm.hpp>

#include "Simple.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"

void SimpleVertexShader(const SceneData* data, const OutVertex* in, float* out) noexcept
{
    __m128 pos = _mm_load_ps((float*)&in->position);
    _mm_store_ps(out, pos);

    glm::vec2* attrib = reinterpret_cast<glm::vec2*>(out + 4);
    out[4] = in->uv.x;
    out[5] = in->uv.y;
}

void vectorcall SimplePixelShader(const SceneData* data,
                                  uint32_t pid, uint32_t tx, uint32_t ty,
                                  const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                                  __m256 u, __m256 v, __m256 w, __m256 rz,
                                  __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t texid = data->faceData[pid];
    Render::Image* texture = data->materials[texid].diffuseMap;

    if (!texture)
    {
        r = _mm256_set1_ps(255.0f);
        g = _mm256_set1_ps(255.0f);
        b = _mm256_set1_ps(255.0f);
        a = _mm256_set1_ps(255.0f);

        return;
    }

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

    __m256 grad = Render::CalculateGrad(tcoordx, tcoordy);

    float lod1, lod2;
    Render::CalculateLod(texture, grad, lod1, lod2);

    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));

    SampleTrilinear(texture, tcoordx, tcoordy, r, g, b, a, lod1, lod2);
}