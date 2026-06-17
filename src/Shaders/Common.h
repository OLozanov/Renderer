#pragma once

#include <glm/glm.hpp>
#include <immintrin.h>

inline void InterpolateVec2(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                            const __m256& u, const __m256& v, const __m256& w, const __m256& rz,
                            __m256& vectorx, __m256& vectory)
{
    __m256 vecx[3];
    __m256 vecy[3];

    vecx[0] = _mm256_set1_ps(a.x);
    vecx[1] = _mm256_set1_ps(b.x);
    vecx[2] = _mm256_set1_ps(c.x);

    vecy[0] = _mm256_set1_ps(a.y);
    vecy[1] = _mm256_set1_ps(b.y);
    vecy[2] = _mm256_set1_ps(c.y);

    vectorx = _mm256_mul_ps(vecx[0], u);
    vectory = _mm256_mul_ps(vecy[0], u);

    vectorx = _mm256_fmadd_ps(vecx[1], v, vectorx);
    vectory = _mm256_fmadd_ps(vecy[1], v, vectory);

    vectorx = _mm256_fmadd_ps(vecx[2], w, vectorx);
    vectory = _mm256_fmadd_ps(vecy[2], w, vectory);

    vectorx = _mm256_mul_ps(vectorx, rz);
    vectory = _mm256_mul_ps(vectory, rz);
}

inline void InterpolateVec3(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                            const __m256& u, const __m256& v, const __m256& w, const __m256& rz,
                            __m256& vectorx, __m256& vectory, __m256& vectorz)
{
    __m256 vecx[3];
    __m256 vecy[3];
    __m256 vecz[3];

    vecx[0] = _mm256_set1_ps(a.x);
    vecx[1] = _mm256_set1_ps(b.x);
    vecx[2] = _mm256_set1_ps(c.x);

    vecy[0] = _mm256_set1_ps(a.y);
    vecy[1] = _mm256_set1_ps(b.y);
    vecy[2] = _mm256_set1_ps(c.y);

    vecz[0] = _mm256_set1_ps(a.z);
    vecz[1] = _mm256_set1_ps(b.z);
    vecz[2] = _mm256_set1_ps(c.z);

    vectorx = _mm256_mul_ps(vecx[0], u);
    vectory = _mm256_mul_ps(vecy[0], u);
    vectorz = _mm256_mul_ps(vecz[0], u);

    vectorx = _mm256_fmadd_ps(vecx[1], v, vectorx);
    vectory = _mm256_fmadd_ps(vecy[1], v, vectory);
    vectorz = _mm256_fmadd_ps(vecz[1], v, vectorz);

    vectorx = _mm256_fmadd_ps(vecx[2], w, vectorx);
    vectory = _mm256_fmadd_ps(vecy[2], w, vectory);
    vectorz = _mm256_fmadd_ps(vecz[2], w, vectorz);

    vectorx = _mm256_mul_ps(vectorx, rz);
    vectory = _mm256_mul_ps(vectory, rz);
    vectorz = _mm256_mul_ps(vectorz, rz);
}

inline void NormalizeVec3(__m256& x, __m256& y, __m256& z)
{
    __m256 len = _mm256_mul_ps(x, x);
    len = _mm256_fmadd_ps(y, y, len);
    len = _mm256_fmadd_ps(z, z, len);

    __m256 rlen = _mm256_rsqrt_ps(len);

    x = _mm256_mul_ps(x, rlen);
    y = _mm256_mul_ps(y, rlen);
    z = _mm256_mul_ps(z, rlen);
}

inline __m256i ToRGB8(const __m256& r, const __m256& g, const __m256& b)
{
    __m256 maxv = _mm256_set1_ps(255.0f);

    __m256i ir = _mm256_cvttps_epi32(_mm256_min_ps(r, maxv));
    __m256i ig = _mm256_cvttps_epi32(_mm256_min_ps(g, maxv));
    __m256i ib = _mm256_cvttps_epi32(_mm256_min_ps(b, maxv));

    __m256i color = _mm256_set1_epi32(0xFF);
    color = _mm256_or_si256(color, _mm256_slli_epi32(ib, 8));
    color = _mm256_or_si256(color, _mm256_slli_epi32(ig, 16));
    color = _mm256_or_si256(color, _mm256_slli_epi32(ir, 24));

    return color;
}

inline __m128 MatMul(const glm::mat4& mat, const glm::vec3& vec)
{
    __m128 x = _mm_set_ps1(vec.x);
    __m128 y = _mm_set_ps1(vec.y);
    __m128 z = _mm_set_ps1(vec.z);

    __m128 pos = _mm_load_ps(&mat[3][0]);
    pos = _mm_fmadd_ps(_mm_load_ps(&mat[0][0]), x, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&mat[1][0]), y, pos);
    pos = _mm_fmadd_ps(_mm_load_ps(&mat[2][0]), z, pos);

    return pos;
}