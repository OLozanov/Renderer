#pragma once

#include <immintrin.h>

//-----------------------------fast_log2-------------------------------------
// https://www.flipcode.com/archives/Fast_log_Function.shtml
// 
// int* const exp_ptr = reinterpret_cast<int*>(&val);
// int x = *exp_ptr;
// const int log_2 = ((x >> 23) & 255) - 128;
// x &= ~(255 << 23);
// x += 127 << 23;
// *exp_ptr = x;
// 
// val = ((-1.0f / 3) * val + 2) * val - 2.0f / 3;
// 
// return (val + log_2);
//---------------------------------------------------------------------------
inline __m256 fast_log2(__m256 val) noexcept
{
    __m256i ival = _mm256_castps_si256(val);
    __m256i exp = _mm256_and_epi32(_mm256_srli_epi32(ival, 23), _mm256_set1_epi32(255));
    exp = _mm256_sub_epi32(exp, _mm256_set1_epi32(128));

    ival = _mm256_and_epi32(ival, _mm256_set1_epi32(~(255 << 23)));
    ival = _mm256_add_epi32(ival, _mm256_set1_epi32(127 << 23));

    __m256 fexp = _mm256_cvtepi32_ps(exp);
    __m256 fval = _mm256_castsi256_ps(ival);

    __m256 log2 = _mm256_fmadd_ps(fval, _mm256_set1_ps(-1.0f / 3.0f), _mm256_set1_ps(2.0f));
    log2 = _mm256_fmadd_ps(log2, fval, _mm256_set1_ps(-2.0f / 3.0f));
    log2 = _mm256_add_ps(log2, fexp);

    return log2;
}

inline __m128 fast_log2(__m128 val) noexcept
{
    __m128i ival = _mm_castps_si128(val);
    __m128i exp = _mm_and_epi32(_mm_srli_epi32(ival, 23), _mm_set1_epi32(255));
    exp = _mm_sub_epi32(exp, _mm_set1_epi32(128));

    ival = _mm_and_epi32(ival, _mm_set1_epi32(~(255 << 23)));
    ival = _mm_add_epi32(ival, _mm_set1_epi32(127 << 23));

    __m128 fexp = _mm_cvtepi32_ps(exp);
    __m128 fval = _mm_castsi128_ps(ival);

    __m128 log2 = _mm_fmadd_ps(fval, _mm_set1_ps(-1.0f / 3.0f), _mm_set1_ps(2.0f));
    log2 = _mm_fmadd_ps(log2, fval, _mm_set1_ps(-2.0f / 3.0f));
    log2 = _mm_add_ps(log2, fexp);

    return log2;
}

inline __m256 fast_exp2(__m256 val)
{
    val = _mm256_max_ps(val, _mm256_set1_ps(-126.0f));
    val = _mm256_min_ps(val, _mm256_set1_ps(127.0f));

    __m256 ival = _mm256_round_ps(val, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    __m256 fval = _mm256_sub_ps(val, ival);

    __m256i exp = _mm256_add_epi32(_mm256_cvtps_epi32(ival), _mm256_set1_epi32(127));
    __m256 res = _mm256_castsi256_ps(_mm256_slli_epi32(exp, 23));

    // Polynomial approximation for 2^f where f is in [-0.5, 0.5]
    // 2^f ~ 1.0 + 0.693147 * f + 0.240226 * f^2
    __m256 poly = _mm256_set1_ps(1.0f);
    poly = _mm256_fmadd_ps(fval, _mm256_set1_ps(0.69314718f), poly);
    poly = _mm256_fmadd_ps(_mm256_mul_ps(fval, fval), _mm256_set1_ps(0.24022650f), poly);

    // 2^x = 2^n * 2^f
    return _mm256_mul_ps(res, poly);
}

inline __m256 pow(__m256 x, __m256 y)
{
    __m256 xpow = fast_log2(x);
    return fast_exp2(_mm256_mul_ps(xpow, y));
}