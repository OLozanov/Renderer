#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Simple.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"

void ShadowVertexShader(const SceneData* data, const SimpleVertex* in, float* out) noexcept
{
    glm::vec4 pos = data->shadowMat * glm::vec4(in->position, 1.0f);

    out[0] = pos.x;
    out[1] = pos.y;
    out[2] = pos.z;
    out[3] = 1.0f;

    glm::vec2* attrib = reinterpret_cast<glm::vec2*>(out + 4);
    out[4] = in->uv.x;
    out[5] = in->uv.y;
}

void DepthVertexShader(const SceneData* data, const OutVertex* in, float* out) noexcept
{
    __m128 pos = _mm_load_ps((float*)&in->position);
    _mm_store_ps(out, pos);

    glm::vec2* attrib = reinterpret_cast<glm::vec2*>(out + 4);
    out[4] = in->uv.x;
    out[5] = in->uv.y;
}

void vectorcall ShadowPixelShader(const SceneData* data,
                                  uint32_t pid, uint32_t tx, uint32_t ty,
                                  const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                                  __m256 u, __m256 v, __m256 w, __m256 rz,
                                  __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t texid = data->faceData[pid];
    Render::Image* texture = data->materials[texid].diffuseMap;

    if (!texture || (texture->format() != Render::PixelFormat::RGBA8))
    {
        a = _mm256_set1_ps(255.0f);
        return;
    }

    __m256 tcoordx;
    __m256 tcoordy;

    InterpolateVec2(*attrib_a, *attrib_b, *attrib_c, u, v, w, rz, tcoordx, tcoordy);

    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));

    a = SampleAlphaLod0(texture, tcoordx, tcoordy);
}

void vectorcall DepthPixelShader(const SceneData* data,
                                 uint32_t pid, uint32_t tx, uint32_t ty,
                                 const glm::vec2* attrib_a, const glm::vec2* attrib_b, const glm::vec2* attrib_c,
                                 __m256 u, __m256 v, __m256 w, __m256 rz,
                                 __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t texid = data->faceData[pid];
    Render::Image* texture = data->materials[texid].diffuseMap;

    if (!texture || (texture->format() != Render::PixelFormat::RGBA8))
    {
        a = _mm256_set1_ps(255.0f);
        return;
    }

    __m256 tcoordx;
    __m256 tcoordy;

    InterpolateVec2(*attrib_a, *attrib_b, *attrib_c, u, v, w, rz, tcoordx, tcoordy);

    __m256 grad = Render::CalculateGrad(tcoordx, tcoordy);
    
    float lod1, lod2;
    Render::CalculateLod(texture, grad, lod1, lod2);
    
    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));
    
    a = SampleAlphaTrilinear(texture, tcoordx, tcoordy, lod1, lod2);
}