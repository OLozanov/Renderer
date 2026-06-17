#include <glm/glm.hpp>

#include "Normal.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"

void NormalVertexShader(const SceneData* data, const Vertex* in, float* out) noexcept
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

    glm::vec3* normal = reinterpret_cast<glm::vec3*>(out + 6);
    glm::vec3* tangent = reinterpret_cast<glm::vec3*>(out + 9);
    glm::vec3* binormal = reinterpret_cast<glm::vec3*>(out + 12);

    *normal = in->normal;
    *tangent = in->tangent;
    *binormal = in->binormal;
}

void vectorcall NormalPixelShader(const SceneData* data,
                                  uint32_t pid, uint32_t tx, uint32_t ty,
                                  const NormInParams* attrib_a, const NormInParams* attrib_b, const NormInParams* attrib_c,
                                  __m256 u, __m256 v, __m256 w, __m256 rz,
                                  __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t texid = data->faceData[pid];
    Render::Image* diffuse = data->materials[texid].diffuseMap;
    Render::Image* normalmap = data->materials[texid].normalMap;

    if (!normalmap)
    {
        r = _mm256_set1_ps(127.0f);
        g = _mm256_set1_ps(127.0f);
        b = _mm256_set1_ps(255.0f);
        a = _mm256_set1_ps(255.0f);

        return;
    }

    __m256 normalx = _mm256_set1_ps(attrib_a->normal.x);
    __m256 normaly = _mm256_set1_ps(attrib_a->normal.y);
    __m256 normalz = _mm256_set1_ps(attrib_a->normal.z);

    __m256 tangentx = _mm256_set1_ps(attrib_a->tangent.x);
    __m256 tangenty = _mm256_set1_ps(attrib_a->tangent.y);
    __m256 tangentz = _mm256_set1_ps(attrib_a->tangent.z);

    __m256 binormalx = _mm256_set1_ps(attrib_a->binormal.x);
    __m256 binormaly = _mm256_set1_ps(attrib_a->binormal.y);
    __m256 binormalz = _mm256_set1_ps(attrib_a->binormal.z);

    __m256 tcoordx;
    __m256 tcoordy;

    InterpolateVec2(attrib_a->tcoord, attrib_b->tcoord, attrib_c->tcoord, u, v, w, rz, tcoordx, tcoordy);

    InterpolateVec3(attrib_a->normal, attrib_b->normal, attrib_c->normal, u, v, w, rz, normalx, normaly, normalz);
    InterpolateVec3(attrib_a->tangent, attrib_b->tangent, attrib_c->tangent, u, v, w, rz, tangentx, tangenty, tangentz);
    InterpolateVec3(attrib_a->binormal, attrib_b->binormal, attrib_c->binormal, u, v, w, rz, binormalx, binormaly, binormalz);

    __m256 grad = Render::CalculateGrad(tcoordx, tcoordy);

    float lod1, lod2;
    Render::CalculateLod(normalmap, grad, lod1, lod2);

    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));

    __m256 local_normx;
    __m256 local_normy;
    __m256 local_normz;
    __m256 local_normw; // not used

    SampleTrilinear(normalmap, tcoordx, tcoordy, local_normx, local_normy, local_normz, local_normw, lod1, lod2);

    if (diffuse->format() == Render::PixelFormat::RGBA8)
    {
        if (diffuse->width() != normalmap->width() ||
            diffuse->height() != normalmap->height())
        {
            Render::CalculateLod(diffuse, grad, lod1, lod2);
        }

        a = SampleAlphaTrilinear(diffuse, tcoordx, tcoordy, lod1, lod2);
    }
    else
        a = _mm256_set1_ps(255.0f);

    __m256 frac = _mm256_set1_ps(1.0f / 255.0f);

    local_normx = _mm256_mul_ps(local_normx, frac);
    local_normy = _mm256_mul_ps(local_normy, frac);
    local_normz = _mm256_mul_ps(local_normz, frac);

    __m256 mone = _mm256_set1_ps(-1.0f);
    __m256 two = _mm256_set1_ps(2.0f);

    local_normx = _mm256_fmadd_ps(local_normx, two, mone);
    local_normy = _mm256_fmadd_ps(local_normy, two, mone);
    local_normz = _mm256_fmadd_ps(local_normz, two, mone);

    __m256 world_normx = _mm256_mul_ps(normalx, local_normz);
    __m256 world_normy = _mm256_mul_ps(normaly, local_normz);
    __m256 world_normz = _mm256_mul_ps(normalz, local_normz);

    world_normx = _mm256_fmadd_ps(tangentx, local_normx, world_normx);
    world_normy = _mm256_fmadd_ps(tangenty, local_normx, world_normy);
    world_normz = _mm256_fmadd_ps(tangentz, local_normx, world_normz);

    world_normx = _mm256_fmadd_ps(binormalx, local_normy, world_normx);
    world_normy = _mm256_fmadd_ps(binormaly, local_normy, world_normy);
    world_normz = _mm256_fmadd_ps(binormalz, local_normy, world_normz);

    NormalizeVec3(world_normx, world_normy, world_normz);

    __m256 maxv = _mm256_set1_ps(255.0f);
    __m256 half = _mm256_set1_ps(0.5f);

    r = _mm256_fmadd_ps(world_normx, half, half);
    g = _mm256_fmadd_ps(world_normy, half, half);
    b = _mm256_fmadd_ps(world_normz, half, half);

    r = _mm256_mul_ps(r, maxv);
    g = _mm256_mul_ps(g, maxv);
    b = _mm256_mul_ps(b, maxv);
}