#define GLM_FORCE_SWIZZLE GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>

#include "Lighting.h"
#include "Shaders/Common.h"
#include "Render/Pipeline/Sampling.h"
#include "Utility/Math.h"

///////////////////////////////////////////////////////////////////////////////////
/*
void LightGridFillShader(const LightGridData* data, uint32_t x, uint32_t y) noexcept
{
    --> find tiles min and max depth value

    float tilex = data->ltgridx;
    float tiley = data->ltgridy;

    float zrange = std::max(dmax - dmin, 0.0001f);
    float rzrange = 1.0f / zrange;

    glm::vec3 gridOffset = { -2.0f * x + tilex - 1.0f,
                            -2.0f * y + tiley - 1.0f,
                            -dmin * rzrange };

    glm::mat4 projToTile = { tilex,             0,            0,         0,
                             0,                 tiley,        0,         0,
                             0,                 0,          rzrange,     0,
                             gridOffset.x, -gridOffset.y, gridOffset.z,  1 };

    glm::mat4 proj = projToTile * data->projView;

    glm::vec4 frustum[6];

    frustum[0].x = proj[0].w + proj[0].x;
    frustum[0].y = proj[1].w + proj[1].x;
    frustum[0].z = proj[2].w + proj[2].x;
    frustum[0].w = proj[3].w + proj[3].x;

    // right
    frustum[1].x = proj[0].w - proj[0].x;
    frustum[1].y = proj[1].w - proj[1].x;
    frustum[1].z = proj[2].w - proj[2].x;
    frustum[1].w = proj[3].w - proj[3].x;

    // top
    frustum[2].x = proj[0].w - proj[0].y;
    frustum[2].y = proj[1].w - proj[1].y;
    frustum[2].z = proj[2].w - proj[2].y;
    frustum[2].w = proj[3].w - proj[3].y;

    // bottom
    frustum[3].x = proj[0].w + proj[0].y;
    frustum[3].y = proj[1].w + proj[1].y;
    frustum[3].z = proj[2].w + proj[2].y;
    frustum[3].w = proj[3].w + proj[3].y;

    // near
    frustum[4].x = proj[0].z;
    frustum[4].y = proj[1].z;
    frustum[4].z = proj[2].z;
    frustum[4].w = proj[3].z;

    // far
    frustum[5].x = proj[0].w - proj[0].z;
    frustum[5].y = proj[1].w - proj[1].z;
    frustum[5].z = proj[2].w - proj[2].z;
    frustum[5].w = proj[3].w - proj[3].z;

    for (size_t f = 0; f < 6; f++)
    {
        float rlen = 1.0f / sqrt(glm::dot(frustum[f].xyz(), frustum[f].xyz()));
        frustum[f] *= rlen;
    }

    uint32_t offset = (y * data->ltgridx + x) * GridMaxLights;
    uint32_t lnum = 0;

    for (uint32_t i = 0; i < data->ltnum; i++)
    {
        uint32_t idx = data->viewLights[i];

        float radius = data->lights[idx].radius + data->lights[idx].falloff;
        glm::vec3 pos = data->lights[idx].pos;

        bool intersect = true;
        for (size_t f = 0; f < 6; f++)
        {
            float d = glm::dot(pos, frustum[f].xyz()) + frustum[f].w;
            if (d < -radius) intersect = false;
        }

        if (intersect)
        {
            data->ltgrid[offset + lnum + 1] = idx;
            lnum++;
        }
    }

    data->ltgrid[offset] = lnum;
}
*////////////////////////////////////////////////////////////////////////////////////

void LightGridFillShader(const LightGridData* data, uint32_t x, uint32_t y) noexcept
{
    // Find min, max depth
    __m256 vmin = _mm256_set1_ps(1.0f);
    __m256 vmax = _mm256_setzero_ps();

    uint32_t mintx = x * 8;
    uint32_t minty = y * 12;

    uint32_t maxtx = mintx + 7;
    uint32_t maxty = minty + 11;

    for (uint32_t ty = minty; ty <= maxty; ty++)
        for (uint32_t tx = mintx; tx <= maxtx; tx++)
        {
            const float* addr = data->depth->data() + (ty * data->xblocks + tx) * 8;
            __m256 dval = _mm256_load_ps(addr);

            vmin = _mm256_min_ps(vmin, dval);
            vmax = _mm256_max_ps(vmax, dval);
        }

    __m128 vmin_hi = _mm256_extractf128_ps(vmin, 1);
    __m128 vmin_lo = _mm256_castps256_ps128(vmin);
    __m128 vmax_hi = _mm256_extractf128_ps(vmax, 1);
    __m128 vmax_lo = _mm256_castps256_ps128(vmax);
    
    __m128 min64 = _mm_min_ps(vmin_hi, vmin_lo);
    __m128 max64 = _mm_max_ps(vmax_hi, vmax_lo);
    
    __m128 shift64 = _mm_movehl_ps(min64, min64);
    __m128 min32 = _mm_min_ps(min64, shift64);
    
    shift64 = _mm_movehl_ps(max64, max64);
    __m128 max32 = _mm_max_ps(max64, shift64);
    
    __m128 shift32 = _mm_movehdup_ps(min32);
    float dmin = _mm_cvtss_f32(_mm_min_ss(min32, shift32));
    
    shift32 = _mm_movehdup_ps(max32);
    float dmax = _mm_cvtss_f32(_mm_max_ss(max32, shift32));

    float zrange = std::max(dmax - dmin, 0.0001f);
    float rzrange = 1.0f / zrange;

    float tilex = data->ltgridx;
    float tiley = data->ltgridy;

    glm::vec3 gridOffset = {-2.0f * x + tilex - 1.0f,
                            -2.0f * y + tiley - 1.0f,
                            -dmin * rzrange };

    glm::mat4 projToTile = { tilex,             0,        0,             0,
                             0,                 tiley,    0,             0,
                             0,                 0,        rzrange,       0,
                             gridOffset.x, -gridOffset.y, gridOffset.z,  1 };

    const glm::mat4& projView = data->projView;

    __m128 pview1 = _mm_setr_ps(projView[0].x, projView[1].x, projView[2].x, projView[3].x);
    __m128 pview2 = _mm_setr_ps(projView[0].y, projView[1].y, projView[2].y, projView[3].y);
    __m128 pview3 = _mm_setr_ps(projView[0].z, projView[1].z, projView[2].z, projView[3].z);
    __m128 pview4 = _mm_setr_ps(projView[0].w, projView[1].w, projView[2].w, projView[3].w);

    __m128 row[4];

    // We need to get rows, so we make transposed multiply (projToTile * projView)T
    for (size_t i = 0; i < 4; i++)
    {
        __m128 x = _mm_set_ps1(projToTile[0][i]);
        __m128 y = _mm_set_ps1(projToTile[1][i]);
        __m128 z = _mm_set_ps1(projToTile[2][i]);
        __m128 w = _mm_set_ps1(projToTile[3][i]);

        row[i] = _mm_mul_ps(pview1, x);
        row[i] = _mm_fmadd_ps(pview2, y, row[i]);
        row[i] = _mm_fmadd_ps(pview3, z, row[i]);
        row[i] = _mm_fmadd_ps(pview4, w, row[i]);
    }
    
    __m128 signmask = _mm_set1_ps(-0.f);
    
    __m256 mat[6];
    
    mat[0] = _mm256_set_m128(row[3], row[3]);
    mat[1] = _mm256_set_m128(row[3], row[3]);
    mat[2] = _mm256_set_m128(_mm_setzero_ps(), row[3]);
    mat[3] = _mm256_set_m128(row[0], _mm_xor_ps(row[0], signmask));
    mat[4] = _mm256_set_m128(row[1], _mm_xor_ps(row[1], signmask));
    mat[5] = _mm256_set_m128(row[2], _mm_xor_ps(row[2], signmask));
    
    __m256 frustum[3];
    
    for (size_t f = 0; f < 3; f++)
    {
        frustum[f] = _mm256_add_ps(mat[f], mat[f + 3]);
        __m256 len = _mm256_dp_ps(frustum[f], frustum[f], 0x7F);
        frustum[f] = _mm256_mul_ps(frustum[f], _mm256_rsqrt_ps(len));
    }

    uint32_t offset = (y * data->ltgridx + x) * GridMaxLights;
    uint32_t lnum = 0;

    for (uint32_t i = 0; i < data->ltnum; i++)
    {
        uint32_t idx = data->viewLights[i];
        const Light& light = data->lights[idx];

        float radius = light.radius + light.falloff;

        __m128 poss = _mm_setr_ps(light.pos.x, light.pos.y, light.pos.z, 1.0f);
        __m256 pos = _mm256_set_m128(poss, poss);

        bool intersect = true;
        for (size_t f = 0; f < 3; f++)
        {
            __m256 dp = _mm256_dp_ps(pos, frustum[f], 0xFF);

            if (dp.m256_f32[0] < -radius) intersect = false;
            if (dp.m256_f32[4] < -radius) intersect = false;
        }

        if (intersect)
        {
            data->ltgrid[offset + lnum + 1] = idx;
            lnum++;
        }
    }

    data->ltgrid[offset] = lnum;
}

void LightingVertexShader(const SceneData* data, const Vertex* in, float* out) noexcept
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

    glm::vec3* realpos = reinterpret_cast<glm::vec3*>(out + 6);
    glm::vec3* normal = reinterpret_cast<glm::vec3*>(out + 9);
    glm::vec3* tangent = reinterpret_cast<glm::vec3*>(out + 12);
    glm::vec3* binormal = reinterpret_cast<glm::vec3*>(out + 15);

    *realpos = in->position;
    *normal = in->normal;
    *tangent = in->tangent;
    *binormal = in->binormal;
}

void vectorcall LightingPixelShader(const SceneData* data,
                                    uint32_t pid, uint32_t tx, uint32_t ty,
                                    const InParams* attrib_a, const InParams* attrib_b, const InParams* attrib_c,
                                    __m256 u, __m256 v, __m256 w, __m256 rz,
                                    __m256& r, __m256& g, __m256& b, __m256& a) noexcept
{
    uint32_t texid = data->faceData[pid];
    const Material* material = data->materials + texid;
    Render::Image* diffuse = data->materials[texid].diffuseMap;
    Render::Image* normalmap = data->materials[texid].normalMap;

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 half = _mm256_set1_ps(0.5f);

    __m256 posx;
    __m256 posy;
    __m256 posz;

    __m256 tcoordx;
    __m256 tcoordy;

    InterpolateVec2(attrib_a->tcoord, attrib_b->tcoord, attrib_c->tcoord, u, v, w, rz, tcoordx, tcoordy);

    InterpolateVec3(attrib_a->pos, attrib_b->pos, attrib_c->pos, u, v, w, rz, posx, posy, posz);

    __m256 viewx = _mm256_sub_ps(_mm256_set1_ps(data->viewPos.x), posx);
    __m256 viewy = _mm256_sub_ps(_mm256_set1_ps(data->viewPos.y), posy);
    __m256 viewz = _mm256_sub_ps(_mm256_set1_ps(data->viewPos.z), posz);

    NormalizeVec3(viewx, viewy, viewz);

    __m256 ltdirx = _mm256_set1_ps(data->lightDir.x);
    __m256 ltdiry = _mm256_set1_ps(data->lightDir.y);
    __m256 ltdirz = _mm256_set1_ps(data->lightDir.z);

    __m256 normalx;
    __m256 normaly;
    __m256 normalz;

    __m256 tangentx;
    __m256 tangenty;
    __m256 tangentz;

    __m256 binormalx;
    __m256 binormaly;
    __m256 binormalz;

    InterpolateVec3(attrib_a->normal, attrib_b->normal, attrib_c->normal, u, v, w, rz, normalx, normaly, normalz);
    InterpolateVec3(attrib_a->tangent, attrib_b->tangent, attrib_c->tangent, u, v, w, rz, tangentx, tangenty, tangentz);
    InterpolateVec3(attrib_a->binormal, attrib_b->binormal, attrib_c->binormal, u, v, w, rz, binormalx, binormaly, binormalz);

    // Shadow
    __m256 shx = _mm256_fmadd_ps(posx, _mm256_set1_ps(data->shadowMat[0][0]), _mm256_set1_ps(data->shadowMat[3][0]));
    __m256 shy = _mm256_fmadd_ps(posx, _mm256_set1_ps(data->shadowMat[0][1]), _mm256_set1_ps(data->shadowMat[3][1]));
    __m256 shz = _mm256_fmadd_ps(posx, _mm256_set1_ps(data->shadowMat[0][2]), _mm256_set1_ps(data->shadowMat[3][2]));

    shx = _mm256_fmadd_ps(posy, _mm256_set1_ps(data->shadowMat[1][0]), shx);
    shy = _mm256_fmadd_ps(posy, _mm256_set1_ps(data->shadowMat[1][1]), shy);
    shz = _mm256_fmadd_ps(posy, _mm256_set1_ps(data->shadowMat[1][2]), shz);

    __m256 zbias = _mm256_set1_ps(0.001f);

    shx = _mm256_fmadd_ps(posz, _mm256_set1_ps(data->shadowMat[2][0]), shx);
    shy = _mm256_fmadd_ps(posz, _mm256_set1_ps(data->shadowMat[2][1]), shy);
    shz = _mm256_fmadd_ps(posz, _mm256_set1_ps(data->shadowMat[2][2]), shz);
    shz = _mm256_sub_ps(shz, zbias);

    shy = _mm256_xor_ps(shy, _mm256_set1_ps(-0.0f));

    __m256 shmax = _mm256_set1_ps(1.0f - 1.0f / data->shadow->width());

    shx = _mm256_fmadd_ps(shx, half, half);
    shy = _mm256_fmadd_ps(shy, half, half);

    shx = _mm256_max_ps(_mm256_min_ps(shx, shmax), zero);
    shy = _mm256_max_ps(_mm256_min_ps(shy, shmax), zero);

    __m256 shad = SampleCmp(data->shadow, shx, shy, shz);

    __m256 grad = Render::CalculateGrad(tcoordx, tcoordy);

    // wrap coordinates
    tcoordx = _mm256_sub_ps(tcoordx, _mm256_floor_ps(tcoordx));
    tcoordy = _mm256_sub_ps(tcoordy, _mm256_floor_ps(tcoordy));

    uint32_t ltx = tx / 8;
    uint32_t lty = ty / 12;

    uint32_t ltentry = (lty * data->ltgridx + ltx) * GridMaxLights;
    uint32_t ltnum = data->ltgrid[ltentry];

    __m256i ishad = _mm256_castps_si256(shad);
    if (_mm256_testz_si256(ishad, ishad) && ltnum == 0)   // whole warp completely shadowed
    {
        if (diffuse)
        {
            float lod1, lod2;
            Render::CalculateLod(diffuse, grad, lod1, lod2);

            SampleTrilinear(diffuse, tcoordx, tcoordy, r, g, b, a, lod1, lod2);
        }
        else
        {
            r = _mm256_set1_ps(255.0f);
            g = _mm256_set1_ps(255.0f);
            b = _mm256_set1_ps(255.0f);
            a = _mm256_set1_ps(255.0f);
        }

        __m256 quarter = _mm256_set1_ps(0.25f);

        r = _mm256_mul_ps(r, quarter);
        g = _mm256_mul_ps(g, quarter);
        b = _mm256_mul_ps(b, quarter);
    }
    else
    {
        __m256 local_normx;
        __m256 local_normy;
        __m256 local_normz;
        __m256 local_normw; // not used

        bool coupled = diffuse && normalmap &&
                       (diffuse->width() == normalmap->width() &&
                        diffuse->height() == normalmap->height());

        if (coupled)
        {
            float lod1, lod2;
            Render::CalculateLod(diffuse, grad, lod1, lod2);

            Sample2Trilinear(diffuse, normalmap, tcoordx, tcoordy, 
                             r, g, b, a,
                             local_normx, local_normy, local_normz,
                             lod1, lod2);
        }
        else
        {
            if (diffuse)
            {
                float lod1, lod2;
                Render::CalculateLod(diffuse, grad, lod1, lod2);

                SampleTrilinear(diffuse, tcoordx, tcoordy, r, g, b, a, lod1, lod2);
            }
            else
            {
                r = _mm256_set1_ps(255.0f);
                g = _mm256_set1_ps(255.0f);
                b = _mm256_set1_ps(255.0f);
                a = _mm256_set1_ps(255.0f);
            }

            if (normalmap)
            {
                float lod1, lod2;
                Render::CalculateLod(diffuse, grad, lod1, lod2);

                SampleTrilinear(normalmap, tcoordx, tcoordy, local_normx, local_normy, local_normz, local_normw, lod1, lod2);
            }
            else
            {
                local_normx = _mm256_set1_ps(127.0f);
                local_normy = _mm256_set1_ps(127.0f);
                local_normz = _mm256_set1_ps(255.0f);
            }
        }

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

        __m256 diffr = zero;
        __m256 diffg = zero;
        __m256 diffb = zero;

        __m256 specr = zero;
        __m256 specg = zero;
        __m256 specb = zero;

        // Directional light
        if (!_mm256_testz_si256(ishad, ishad))
        {
            __m256 ltcos = _mm256_mul_ps(world_normx, ltdirx);
            ltcos = _mm256_fmadd_ps(world_normy, ltdiry, ltcos);
            ltcos = _mm256_fmadd_ps(world_normz, ltdirz, ltcos);
            ltcos = _mm256_max_ps(_mm256_setzero_ps(), ltcos);

            __m256 spec;

            __m256 lttest = _mm256_cmp_ps(ltcos, _mm256_setzero_ps(), _CMP_GT_OS);

            // Specular
            if (material->sfactor > 0.001f && _mm256_testz_ps(lttest, lttest) == 0)
            {
                __m256 sintencity = _mm256_and_ps(_mm256_set1_ps(material->sfactor), lttest);

                __m256 halfvecx = _mm256_add_ps(viewx, ltdirx);
                __m256 halfvecy = _mm256_add_ps(viewy, ltdiry);
                __m256 halfvecz = _mm256_add_ps(viewz, ltdirz);

                NormalizeVec3(halfvecx, halfvecy, halfvecz);

                __m256 hvcos = _mm256_mul_ps(world_normx, halfvecx);
                hvcos = _mm256_fmadd_ps(world_normy, halfvecy, hvcos);
                hvcos = _mm256_fmadd_ps(world_normz, halfvecz, hvcos);

                hvcos = _mm256_max_ps(_mm256_setzero_ps(), hvcos);

                spec = pow(hvcos, _mm256_set1_ps(material->sexponent));
                spec = _mm256_mul_ps(spec, sintencity);
                spec = _mm256_mul_ps(spec, shad);
            }
            else
                spec = _mm256_setzero_ps();

            ltcos = _mm256_mul_ps(ltcos, shad);

            diffr = ltcos;
            diffg = ltcos;
            diffb = ltcos;

            specr = spec;
            specg = spec;
            specb = spec;
        }

        // Omni lights
        for (uint32_t i = 0; i < ltnum; i++)
        {
            uint32_t ltid = data->ltgrid[ltentry + i + 1];
            Light* light = data->lights + ltid;

            __m256 ltdirx = _mm256_sub_ps(_mm256_set1_ps(light->pos.x), posx);
            __m256 ltdiry = _mm256_sub_ps(_mm256_set1_ps(light->pos.y), posy);
            __m256 ltdirz = _mm256_sub_ps(_mm256_set1_ps(light->pos.z), posz);

            __m256 len = _mm256_mul_ps(ltdirx, ltdirx);
            len = _mm256_fmadd_ps(ltdiry, ltdiry, len);
            len = _mm256_fmadd_ps(ltdirz, ltdirz, len);
            len = _mm256_sqrt_ps(len);

            __m256 rad = _mm256_set1_ps(light->radius + light->falloff);
            __m256 rfall = _mm256_set1_ps(1.0f / light->falloff);

            __m256 att = _mm256_sub_ps(rad, len);
            att = _mm256_mul_ps(att, rfall);
            att = _mm256_min_ps(one, _mm256_max_ps(zero, att));

            //att = one;

            __m256 test = _mm256_cmp_ps(att, zero, _CMP_GT_OS);
            if (_mm256_testz_ps(test, test)) continue;
            
            __m256 rlen = _mm256_rcp_ps(len);
            
            ltdirx = _mm256_mul_ps(ltdirx, rlen);
            ltdiry = _mm256_mul_ps(ltdiry, rlen);
            ltdirz = _mm256_mul_ps(ltdirz, rlen);

            __m256 ltr = _mm256_set1_ps(light->color.r);
            __m256 ltg = _mm256_set1_ps(light->color.g);
            __m256 ltb = _mm256_set1_ps(light->color.b);

            __m256 ltcos = _mm256_mul_ps(world_normx, ltdirx);
            ltcos = _mm256_fmadd_ps(world_normy, ltdiry, ltcos);
            ltcos = _mm256_fmadd_ps(world_normz, ltdirz, ltcos);
            
            ltcos = _mm256_max_ps(_mm256_setzero_ps(), ltcos);
            ltcos = _mm256_mul_ps(ltcos, att);

            diffr = _mm256_fmadd_ps(ltr, ltcos, diffr);
            diffg = _mm256_fmadd_ps(ltg, ltcos, diffg);
            diffb = _mm256_fmadd_ps(ltb, ltcos, diffb);

            __m256 lttest = _mm256_cmp_ps(ltcos, _mm256_setzero_ps(), _CMP_GT_OS);

            // Specular
            if (material->sfactor > 0.001f && _mm256_testz_ps(lttest, lttest) == 0)
            {
                __m256 sintencity = _mm256_and_ps(_mm256_set1_ps(material->sfactor), lttest);

                __m256 halfvecx = _mm256_add_ps(viewx, ltdirx);
                __m256 halfvecy = _mm256_add_ps(viewy, ltdiry);
                __m256 halfvecz = _mm256_add_ps(viewz, ltdirz);

                NormalizeVec3(halfvecx, halfvecy, halfvecz);

                __m256 hvcos = _mm256_mul_ps(world_normx, halfvecx);
                hvcos = _mm256_fmadd_ps(world_normy, halfvecy, hvcos);
                hvcos = _mm256_fmadd_ps(world_normz, halfvecz, hvcos);

                hvcos = _mm256_max_ps(_mm256_setzero_ps(), hvcos);

                __m256 spec = pow(hvcos, _mm256_set1_ps(material->sexponent));
                spec = _mm256_mul_ps(spec, sintencity);
                spec = _mm256_mul_ps(spec, att);

                specr = _mm256_fmadd_ps(spec, ltr, specr);
                specg = _mm256_fmadd_ps(spec, ltg, specg);
                specb = _mm256_fmadd_ps(spec, ltb, specb);
            }
        }

        diffr = _mm256_fmadd_ps(diffr, half, half);
        diffg = _mm256_fmadd_ps(diffg, half, half);
        diffb = _mm256_fmadd_ps(diffb, half, half);

        diffr = _mm256_mul_ps(diffr, diffr);
        diffg = _mm256_mul_ps(diffg, diffg);
        diffb = _mm256_mul_ps(diffb, diffb);

        r = _mm256_fmadd_ps(r, diffr, specr);
        g = _mm256_fmadd_ps(g, diffg, specg);
        b = _mm256_fmadd_ps(b, diffb, specb);
    }
}