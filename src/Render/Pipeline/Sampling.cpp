#include "Sampling.h"

#include "Utility/Math.h"

namespace Render
{

void vectorcall SampleLod0(const Image* image, __m256 u, __m256 v, 
                           __m256& rval, __m256& gval, __m256& bval, __m256& aval) noexcept
{
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    uint32_t width = image->width();
    uint32_t height = image->height();

    const int32_t* base = reinterpret_cast<const int32_t*>(image->data());

    __m256i iw = _mm256_set1_epi32(width);
    __m256i ih = _mm256_set1_epi32(height);

    __m256 w = _mm256_set1_ps(width);
    __m256 h = _mm256_set1_ps(height);

    u = _mm256_mul_ps(u, w);
    v = _mm256_mul_ps(v, h);

    __m256 iu = _mm256_floor_ps(u);
    __m256 iv = _mm256_floor_ps(v);

    __m256i x = _mm256_cvttps_epi32(iu);
    __m256i y = _mm256_cvttps_epi32(iv);

    __m256i x1 = _mm256_add_epi32(x, ione);
    __m256i y1 = _mm256_add_epi32(y, ione);

    x1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1, iw), x1);
    y1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1, ih), y1);

    __m256 fu = _mm256_sub_ps(u, iu);
    __m256 fv = _mm256_sub_ps(v, iv);

    __m256 rfu = _mm256_sub_ps(one, fu);
    __m256 rfv = _mm256_sub_ps(one, fv);

    __m256i offsets[4];

    __m256i yspan = _mm256_mullo_epi32(iw, y);
    __m256i y1span = _mm256_mullo_epi32(iw, y1);

    offsets[0] = _mm256_add_epi32(yspan, x);
    offsets[1] = _mm256_add_epi32(yspan, x1);
    offsets[2] = _mm256_add_epi32(y1span, x);
    offsets[3] = _mm256_add_epi32(y1span, x1);

    __m256 weight[4];
    weight[0] = _mm256_mul_ps(rfu, rfv);
    weight[1] = _mm256_mul_ps(fu, rfv);
    weight[2] = _mm256_mul_ps(rfu, fv);
    weight[3] = _mm256_mul_ps(fu, fv);

    __m256i val[4];
    val[0] = _mm256_i32gather_epi32(base, offsets[0], 4);
    val[1] = _mm256_i32gather_epi32(base, offsets[1], 4);
    val[2] = _mm256_i32gather_epi32(base, offsets[2], 4);
    val[3] = _mm256_i32gather_epi32(base, offsets[3], 4);

    __m256 r[4];
    __m256 g[4];
    __m256 b[4];
    __m256 a[4];

    __m256i mask = _mm256_set1_epi32(0xFF);

    bool alpha = image->format() == PixelFormat::RGBA8;

    for (size_t i = 0; i < 4; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));

        if (alpha) a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    rval = _mm256_mul_ps(r[0], weight[0]);
    gval = _mm256_mul_ps(g[0], weight[0]);
    bval = _mm256_mul_ps(b[0], weight[0]);
    aval = alpha ? _mm256_mul_ps(a[0], weight[0]) : _mm256_set1_ps(255.0f);

    for (size_t i = 1; i < 4; i++)
    {
        rval = _mm256_fmadd_ps(r[i], weight[i], rval);
        gval = _mm256_fmadd_ps(g[i], weight[i], gval);
        bval = _mm256_fmadd_ps(b[i], weight[i], bval);

        if (alpha) aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }
}

void vectorcall Sample2Lod0(const Image* image1, const Image* image2, __m256 u, __m256 v,
                           __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                           __m256& rval2, __m256& gval2, __m256& bval2) noexcept
{
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    uint32_t width = image1->width();
    uint32_t height = image1->height();

    __m256i iw = _mm256_set1_epi32(width);
    __m256i ih = _mm256_set1_epi32(height);

    __m256 w = _mm256_set1_ps(width);
    __m256 h = _mm256_set1_ps(height);

    u = _mm256_mul_ps(u, w);
    v = _mm256_mul_ps(v, h);

    __m256 iu = _mm256_floor_ps(u);
    __m256 iv = _mm256_floor_ps(v);

    __m256i x = _mm256_cvttps_epi32(iu);
    __m256i y = _mm256_cvttps_epi32(iv);

    __m256i x1 = _mm256_add_epi32(x, ione);
    __m256i y1 = _mm256_add_epi32(y, ione);

    x1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1, iw), x1);
    y1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1, ih), y1);

    __m256 fu = _mm256_sub_ps(u, iu);
    __m256 fv = _mm256_sub_ps(v, iv);

    __m256 rfu = _mm256_sub_ps(one, fu);
    __m256 rfv = _mm256_sub_ps(one, fv);

    __m256i offsets[4];

    __m256i yspan = _mm256_mullo_epi32(iw, y);
    __m256i y1span = _mm256_mullo_epi32(iw, y1);

    offsets[0] = _mm256_add_epi32(yspan, x);
    offsets[1] = _mm256_add_epi32(yspan, x1);
    offsets[2] = _mm256_add_epi32(y1span, x);
    offsets[3] = _mm256_add_epi32(y1span, x1);

    __m256 weight[4];
    weight[0] = _mm256_mul_ps(rfu, rfv);
    weight[1] = _mm256_mul_ps(fu, rfv);
    weight[2] = _mm256_mul_ps(rfu, fv);
    weight[3] = _mm256_mul_ps(fu, fv);

    const int32_t* base = reinterpret_cast<const int32_t*>(image1->data());

    __m256i val[4];
    val[0] = _mm256_i32gather_epi32(base, offsets[0], 4);
    val[1] = _mm256_i32gather_epi32(base, offsets[1], 4);
    val[2] = _mm256_i32gather_epi32(base, offsets[2], 4);
    val[3] = _mm256_i32gather_epi32(base, offsets[3], 4);

    __m256 r[4];
    __m256 g[4];
    __m256 b[4];
    __m256 a[4];

    __m256i mask = _mm256_set1_epi32(0xFF);

    bool alpha = image1->format() == PixelFormat::RGBA8;

    for (size_t i = 0; i < 4; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));

        if (alpha) a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    rval = _mm256_mul_ps(r[0], weight[0]);
    gval = _mm256_mul_ps(g[0], weight[0]);
    bval = _mm256_mul_ps(b[0], weight[0]);
    aval = alpha ? _mm256_mul_ps(a[0], weight[0]) : _mm256_set1_ps(255.0f);

    for (size_t i = 1; i < 4; i++)
    {
        rval = _mm256_fmadd_ps(r[i], weight[i], rval);
        gval = _mm256_fmadd_ps(g[i], weight[i], gval);
        bval = _mm256_fmadd_ps(b[i], weight[i], bval);

        if (alpha) aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }

    base = reinterpret_cast<const int32_t*>(image2->data());

    val[0] = _mm256_i32gather_epi32(base, offsets[0], 4);
    val[1] = _mm256_i32gather_epi32(base, offsets[1], 4);
    val[2] = _mm256_i32gather_epi32(base, offsets[2], 4);
    val[3] = _mm256_i32gather_epi32(base, offsets[3], 4);

    for (size_t i = 0; i < 4; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));
    }

    rval2 = _mm256_mul_ps(r[0], weight[0]);
    gval2 = _mm256_mul_ps(g[0], weight[0]);
    bval2 = _mm256_mul_ps(b[0], weight[0]);

    for (size_t i = 1; i < 4; i++)
    {
        rval2 = _mm256_fmadd_ps(r[i], weight[i], rval2);
        gval2 = _mm256_fmadd_ps(g[i], weight[i], gval2);
        bval2 = _mm256_fmadd_ps(b[i], weight[i], bval2);
    }
}

__m256 vectorcall SampleAlphaLod0(const Image* image, __m256 u, __m256 v) noexcept
{
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    uint32_t width = image->width();
    uint32_t height = image->height();

    const int32_t* base = reinterpret_cast<const int32_t*>(image->data());

    __m256i iw = _mm256_set1_epi32(width);
    __m256i ih = _mm256_set1_epi32(height);

    __m256 w = _mm256_set1_ps(width);
    __m256 h = _mm256_set1_ps(height);

    u = _mm256_mul_ps(u, w);
    v = _mm256_mul_ps(v, h);

    __m256 iu = _mm256_floor_ps(u);
    __m256 iv = _mm256_floor_ps(v);

    __m256i x = _mm256_cvttps_epi32(iu);
    __m256i y = _mm256_cvttps_epi32(iv);

    __m256i x1 = _mm256_add_epi32(x, ione);
    __m256i y1 = _mm256_add_epi32(y, ione);

    x1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1, iw), x1);
    y1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1, ih), y1);

    __m256 fu = _mm256_sub_ps(u, iu);
    __m256 fv = _mm256_sub_ps(v, iv);

    __m256 rfu = _mm256_sub_ps(one, fu);
    __m256 rfv = _mm256_sub_ps(one, fv);

    __m256i offsets[4];

    __m256i yspan = _mm256_mullo_epi32(iw, y);
    __m256i y1span = _mm256_mullo_epi32(iw, y1);

    offsets[0] = _mm256_add_epi32(yspan, x);
    offsets[1] = _mm256_add_epi32(yspan, x1);
    offsets[2] = _mm256_add_epi32(y1span, x);
    offsets[3] = _mm256_add_epi32(y1span, x1);

    __m256 weight[4];
    weight[0] = _mm256_mul_ps(rfu, rfv);
    weight[1] = _mm256_mul_ps(fu, rfv);
    weight[2] = _mm256_mul_ps(rfu, fv);
    weight[3] = _mm256_mul_ps(fu, fv);

    __m256i val[4];
    val[0] = _mm256_i32gather_epi32(base, offsets[0], 4);
    val[1] = _mm256_i32gather_epi32(base, offsets[1], 4);
    val[2] = _mm256_i32gather_epi32(base, offsets[2], 4);
    val[3] = _mm256_i32gather_epi32(base, offsets[3], 4);

    __m256 a[4];

    __m256i mask = _mm256_set1_epi32(0xFF);

    for (size_t i = 0; i < 4; i++)
    {
        a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    __m256 aval = _mm256_mul_ps(a[0], weight[0]);

    for (size_t i = 1; i < 4; i++)
    {
        aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }

    return aval;
}

__m256 vectorcall SampleCmp(const Image* image, __m256 u, __m256 v, __m256 z) noexcept
{
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    uint32_t width = image->width();
    uint32_t height = image->height();

    const float* base = reinterpret_cast<const float*>(image->data());

    __m256i iw = _mm256_set1_epi32(width);
    __m256i ih = _mm256_set1_epi32(height);

    __m256 w = _mm256_set1_ps(width);
    __m256 h = _mm256_set1_ps(height);

    u = _mm256_mul_ps(u, w);
    v = _mm256_mul_ps(v, h);

    __m256 iu = _mm256_floor_ps(u);
    __m256 iv = _mm256_floor_ps(v);

    __m256i x = _mm256_cvttps_epi32(iu);
    __m256i y = _mm256_cvttps_epi32(iv);

    __m256i x1 = _mm256_add_epi32(x, ione);
    __m256i y1 = _mm256_add_epi32(y, ione);

    x1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1, iw), x1);
    y1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1, ih), y1);

    __m256 fu = _mm256_sub_ps(u, iu);
    __m256 fv = _mm256_sub_ps(v, iv);

    __m256 rfu = _mm256_sub_ps(one, fu);
    __m256 rfv = _mm256_sub_ps(one, fv);

    __m256i offsets[4];

    __m256i yspan = _mm256_mullo_epi32(iw, y);
    __m256i y1span = _mm256_mullo_epi32(iw, y1);

    offsets[0] = _mm256_add_epi32(yspan, x);
    offsets[1] = _mm256_add_epi32(yspan, x1);
    offsets[2] = _mm256_add_epi32(y1span, x);
    offsets[3] = _mm256_add_epi32(y1span, x1);

    __m256 weight[4];
    weight[0] = _mm256_mul_ps(rfu, rfv);
    weight[1] = _mm256_mul_ps(fu, rfv);
    weight[2] = _mm256_mul_ps(rfu, fv);
    weight[3] = _mm256_mul_ps(fu, fv);

    __m256 val[4];
    val[0] = _mm256_i32gather_ps(base, offsets[0], 4);
    val[1] = _mm256_i32gather_ps(base, offsets[1], 4);
    val[2] = _mm256_i32gather_ps(base, offsets[2], 4);
    val[3] = _mm256_i32gather_ps(base, offsets[3], 4);

    for (size_t i = 0; i < 4; i++)
    {
        __m256 cmp = _mm256_cmp_ps(val[i], z, _CMP_GT_OS);
        val[i] = _mm256_and_ps(_mm256_set1_ps(1.0f), cmp);
    }

    __m256 shade = _mm256_mul_ps(val[0], weight[0]);

    for (size_t i = 1; i < 4; i++) shade = _mm256_fmadd_ps(val[i], weight[i], shade);

    return shade;
}

void vectorcall Sample(const Image* image, __m256 u, __m256 v, 
                       __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                       float lod1, float lod2) noexcept
{
    if (lod1 <= 0.0f && lod2 <= 0.0f) return SampleLod0(image, u, v, rval, gval, bval, aval);

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    float maxmip = image->mipmaps() - 1;

    lod1 = std::max(std::min(lod1, maxmip), 0.0f);
    lod2 = std::max(std::min(lod2, maxmip), 0.0f);

    uint32_t width1 = image->width() >> (uint32_t)lod1;
    uint32_t height1 = image->height() >> (uint32_t)lod1;

    uint32_t width2 = image->width() >> (uint32_t)lod2;
    uint32_t height2 = image->height() >> (uint32_t)lod2;

    const int32_t* data = reinterpret_cast<const int32_t*>(image->data());

    uint32_t base1 = image->offset(lod1);
    uint32_t base2 = image->offset(lod2);

    __m256i base = _mm256_set_epi32(base1, base1, base2, base2, base1, base1, base2, base2);

    __m256i iw = _mm256_set_epi32(width1, width1, width2, width2, width1, width1, width2, width2);
    __m256i ih = _mm256_set_epi32(height1, height1, height2, height2, height1, height1, height2, height2);

    __m256 w = _mm256_cvtepi32_ps(iw);
    __m256 h = _mm256_cvtepi32_ps(ih);

    u = _mm256_mul_ps(u, w);
    v = _mm256_mul_ps(v, h);

    __m256 iu = _mm256_floor_ps(u);
    __m256 iv = _mm256_floor_ps(v);

    __m256i x = _mm256_cvttps_epi32(iu);
    __m256i y = _mm256_cvttps_epi32(iv);

    __m256i x1 = _mm256_add_epi32(x, ione);
    __m256i y1 = _mm256_add_epi32(y, ione);

    x1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1, iw), x1);
    y1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1, ih), y1);

    __m256 fu = _mm256_sub_ps(u, iu);
    __m256 fv = _mm256_sub_ps(v, iv);

    __m256 rfu = _mm256_sub_ps(one, fu);
    __m256 rfv = _mm256_sub_ps(one, fv);

    __m256i offsets[4];

    __m256i yspan = _mm256_mullo_epi32(iw, y);
    __m256i y1span = _mm256_mullo_epi32(iw, y1);

    yspan = _mm256_add_epi32(yspan, base);
    y1span = _mm256_add_epi32(y1span, base);

    offsets[0] = _mm256_add_epi32(yspan, x);
    offsets[1] = _mm256_add_epi32(yspan, x1);
    offsets[2] = _mm256_add_epi32(y1span, x);
    offsets[3] = _mm256_add_epi32(y1span, x1);

    __m256 weight[4];
    weight[0] = _mm256_mul_ps(rfu, rfv);
    weight[1] = _mm256_mul_ps(fu, rfv);
    weight[2] = _mm256_mul_ps(rfu, fv);
    weight[3] = _mm256_mul_ps(fu, fv);

    __m256i val[4];
    val[0] = _mm256_i32gather_epi32(data, offsets[0], 4);
    val[1] = _mm256_i32gather_epi32(data, offsets[1], 4);
    val[2] = _mm256_i32gather_epi32(data, offsets[2], 4);
    val[3] = _mm256_i32gather_epi32(data, offsets[3], 4);

    __m256 r[4];
    __m256 g[4];
    __m256 b[4];
    __m256 a[4];

    __m256i mask = _mm256_set1_epi32(0xFF);

    bool alpha = image->format() == PixelFormat::RGBA8;

    for (size_t i = 0; i < 4; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));

        if (alpha) a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    rval = _mm256_mul_ps(r[0], weight[0]);
    gval = _mm256_mul_ps(g[0], weight[0]);
    bval = _mm256_mul_ps(b[0], weight[0]);
    aval = alpha ? _mm256_mul_ps(a[0], weight[0]) : _mm256_set1_ps(255.0f);

    for (size_t i = 1; i < 4; i++)
    {
        rval = _mm256_fmadd_ps(r[i], weight[i], rval);
        gval = _mm256_fmadd_ps(g[i], weight[i], gval);
        bval = _mm256_fmadd_ps(b[i], weight[i], bval);

        if (alpha) aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }
}

void vectorcall SampleTrilinear(const Image* image, __m256 u, __m256 v, 
                                __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                                float lod1, float lod2) noexcept
{
    if (lod1 <= 0.0f && lod2 <= 0.0f)
    {
        SampleLod0(image, u, v, rval, gval, bval, aval);
        return;
    }

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    float maxmip = image->mipmaps() - 1;

    lod1 = std::max(std::min(lod1, maxmip), 0.0f);
    lod2 = std::max(std::min(lod2, maxmip), 0.0f);

    uint32_t width1_1 = image->width() >> (uint32_t)lod1;
    uint32_t height1_1 = image->height() >> (uint32_t)lod1;

    uint32_t width2_1 = image->width() >> (uint32_t)lod2;
    uint32_t height2_1 = image->height() >> (uint32_t)lod2;

    uint32_t width1_2 = image->width() >> (uint32_t)(lod1 + 1);
    uint32_t height1_2 = image->height() >> (uint32_t)(lod1 + 1);

    uint32_t width2_2 = image->width() >> (uint32_t)(lod2 + 1);
    uint32_t height2_2 = image->height() >> (uint32_t)(lod2 + 1);

    const int32_t* data = reinterpret_cast<const int32_t*>(image->data());

    uint32_t base1_1 = image->offset(lod1);
    uint32_t base2_1 = image->offset(lod2);

    uint32_t base1_2 = image->offset(lod1+1);
    uint32_t base2_2 = image->offset(lod2+1);

    __m256i base1 = _mm256_set_epi32(base1_1, base1_1, base2_1, base2_1, base1_1, base1_1, base2_1, base2_1);
    __m256i base2 = _mm256_set_epi32(base1_2, base1_2, base2_2, base2_2, base1_2, base1_2, base2_2, base2_2);

    __m256i iw1 = _mm256_set_epi32(width1_1, width1_1, width2_1, width2_1, width1_1, width1_1, width2_1, width2_1);
    __m256i ih1 = _mm256_set_epi32(height1_1, height1_1, height2_1, height2_1, height1_1, height1_1, height2_1, height2_1);

    __m256i iw2 = _mm256_set_epi32(width1_2, width1_2, width2_2, width2_2, width1_2, width1_2, width2_2, width2_2);
    __m256i ih2 = _mm256_set_epi32(height1_2, height1_2, height2_2, height2_2, height1_2, height1_2, height2_2, height2_2);

    __m256 w1 = _mm256_cvtepi32_ps(iw1);
    __m256 h1 = _mm256_cvtepi32_ps(ih1);

    __m256 w2 = _mm256_cvtepi32_ps(iw2);
    __m256 h2 = _mm256_cvtepi32_ps(ih2);

    __m256 fw = _mm256_setr_ps(lod1, lod1, lod2, lod2, lod1, lod1, lod2, lod2);
    fw = _mm256_sub_ps(fw, _mm256_floor_ps(fw));
    __m256 rfw = _mm256_sub_ps(one, fw);

    __m256 u1 = _mm256_mul_ps(u, w1);
    __m256 v1 = _mm256_mul_ps(v, h1);

    __m256 iu1 = _mm256_floor_ps(u1);
    __m256 iv1 = _mm256_floor_ps(v1);

    __m256i x1 = _mm256_cvttps_epi32(iu1);
    __m256i y1 = _mm256_cvttps_epi32(iv1);

    __m256i x1_1 = _mm256_add_epi32(x1, ione);
    __m256i y1_1 = _mm256_add_epi32(y1, ione);

    x1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1_1, iw1), x1_1);
    y1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1_1, ih1), y1_1);

    __m256 fu1 = _mm256_sub_ps(u1, iu1);
    __m256 fv1 = _mm256_sub_ps(v1, iv1);

    __m256 rfu1 = _mm256_sub_ps(one, fu1);
    __m256 rfv1 = _mm256_sub_ps(one, fv1);

    __m256 u2 = _mm256_mul_ps(u, w2);
    __m256 v2 = _mm256_mul_ps(v, h2);

    __m256 iu2 = _mm256_floor_ps(u2);
    __m256 iv2 = _mm256_floor_ps(v2);

    __m256i x2 = _mm256_cvttps_epi32(iu2);
    __m256i y2 = _mm256_cvttps_epi32(iv2);

    __m256i x2_1 = _mm256_add_epi32(x2, ione);
    __m256i y2_1 = _mm256_add_epi32(y2, ione);

    x2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x2_1, iw2), x2_1);
    y2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y2_1, ih2), y2_1);

    __m256 fu2 = _mm256_sub_ps(u2, iu2);
    __m256 fv2 = _mm256_sub_ps(v2, iv2);

    __m256 rfu2 = _mm256_sub_ps(one, fu2);
    __m256 rfv2 = _mm256_sub_ps(one, fv2);

    __m256i yspan1_1 = _mm256_mullo_epi32(iw1, y1);
    __m256i yspan1_2 = _mm256_mullo_epi32(iw1, y1_1);

    yspan1_1 = _mm256_add_epi32(yspan1_1, base1);
    yspan1_2 = _mm256_add_epi32(yspan1_2, base1);

    __m256i yspan2_1 = _mm256_mullo_epi32(iw2, y2);
    __m256i yspan2_2 = _mm256_mullo_epi32(iw2, y2_1);

    yspan2_1 = _mm256_add_epi32(yspan2_1, base2);
    yspan2_2 = _mm256_add_epi32(yspan2_2, base2);

    __m256i offsets[8];

    offsets[0] = _mm256_add_epi32(yspan1_1, x1);
    offsets[1] = _mm256_add_epi32(yspan1_1, x1_1);
    offsets[2] = _mm256_add_epi32(yspan1_2, x1);
    offsets[3] = _mm256_add_epi32(yspan1_2, x1_1);

    offsets[4] = _mm256_add_epi32(yspan2_1, x2);
    offsets[5] = _mm256_add_epi32(yspan2_1, x2_1);
    offsets[6] = _mm256_add_epi32(yspan2_2, x2);
    offsets[7] = _mm256_add_epi32(yspan2_2, x2_1);

    __m256 weight[8];
    weight[0] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, rfv1));
    weight[1] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, rfv1));
    weight[2] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, fv1));
    weight[3] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, fv1));

    weight[4] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, rfv2));
    weight[5] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, rfv2));
    weight[6] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, fv2));
    weight[7] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, fv2));

    __m256i val[8];

    for (size_t i = 0; i < 8; i++) val[i] = _mm256_i32gather_epi32(data, offsets[i], 4);

    __m256 r[8];
    __m256 g[8];
    __m256 b[8];
    __m256 a[8];

    __m256i mask = _mm256_set1_epi32(0xFF);

    bool alpha = image->format() == PixelFormat::RGBA8;

    for (size_t i = 0; i < 8; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));

        if (alpha) a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    rval = _mm256_mul_ps(r[0], weight[0]);
    gval = _mm256_mul_ps(g[0], weight[0]);
    bval = _mm256_mul_ps(b[0], weight[0]);
    aval = alpha ? _mm256_mul_ps(a[0], weight[0]) : _mm256_set1_ps(255.0f);

    for (size_t i = 1; i < 8; i++)
    {
        rval = _mm256_fmadd_ps(r[i], weight[i], rval);
        gval = _mm256_fmadd_ps(g[i], weight[i], gval);
        bval = _mm256_fmadd_ps(b[i], weight[i], bval);

        if (alpha) aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }
}

void vectorcall Sample2Trilinear(const Image* image1, const Image* image2, __m256 u, __m256 v,
                                __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                                __m256& rval2, __m256& gval2, __m256& bval2,
                                float lod1, float lod2) noexcept
{
    if (lod1 <= 0.0f && lod2 <= 0.0f)
    {
        Sample2Lod0(image1, image2, u, v, rval, gval, bval, aval, rval2, gval2, bval2);
        return;
    }

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    float maxmip = image1->mipmaps() - 1;

    lod1 = std::max(std::min(lod1, maxmip), 0.0f);
    lod2 = std::max(std::min(lod2, maxmip), 0.0f);

    uint32_t width1_1 = image1->width() >> (uint32_t)lod1;
    uint32_t height1_1 = image1->height() >> (uint32_t)lod1;

    uint32_t width2_1 = image1->width() >> (uint32_t)lod2;
    uint32_t height2_1 = image1->height() >> (uint32_t)lod2;

    uint32_t width1_2 = image1->width() >> (uint32_t)(lod1 + 1);
    uint32_t height1_2 = image1->height() >> (uint32_t)(lod1 + 1);

    uint32_t width2_2 = image1->width() >> (uint32_t)(lod2 + 1);
    uint32_t height2_2 = image1->height() >> (uint32_t)(lod2 + 1);

    uint32_t base1_1 = image1->offset(lod1);
    uint32_t base2_1 = image1->offset(lod2);

    uint32_t base1_2 = image1->offset(lod1 + 1);
    uint32_t base2_2 = image1->offset(lod2 + 1);

    __m256i base1 = _mm256_set_epi32(base1_1, base1_1, base2_1, base2_1, base1_1, base1_1, base2_1, base2_1);
    __m256i base2 = _mm256_set_epi32(base1_2, base1_2, base2_2, base2_2, base1_2, base1_2, base2_2, base2_2);

    __m256i iw1 = _mm256_set_epi32(width1_1, width1_1, width2_1, width2_1, width1_1, width1_1, width2_1, width2_1);
    __m256i ih1 = _mm256_set_epi32(height1_1, height1_1, height2_1, height2_1, height1_1, height1_1, height2_1, height2_1);

    __m256i iw2 = _mm256_set_epi32(width1_2, width1_2, width2_2, width2_2, width1_2, width1_2, width2_2, width2_2);
    __m256i ih2 = _mm256_set_epi32(height1_2, height1_2, height2_2, height2_2, height1_2, height1_2, height2_2, height2_2);

    __m256 w1 = _mm256_cvtepi32_ps(iw1);
    __m256 h1 = _mm256_cvtepi32_ps(ih1);

    __m256 w2 = _mm256_cvtepi32_ps(iw2);
    __m256 h2 = _mm256_cvtepi32_ps(ih2);

    __m256 fw = _mm256_setr_ps(lod1, lod1, lod2, lod2, lod1, lod1, lod2, lod2);
    fw = _mm256_sub_ps(fw, _mm256_floor_ps(fw));
    __m256 rfw = _mm256_sub_ps(one, fw);

    __m256 u1 = _mm256_mul_ps(u, w1);
    __m256 v1 = _mm256_mul_ps(v, h1);

    __m256 iu1 = _mm256_floor_ps(u1);
    __m256 iv1 = _mm256_floor_ps(v1);

    __m256i x1 = _mm256_cvttps_epi32(iu1);
    __m256i y1 = _mm256_cvttps_epi32(iv1);

    __m256i x1_1 = _mm256_add_epi32(x1, ione);
    __m256i y1_1 = _mm256_add_epi32(y1, ione);

    x1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1_1, iw1), x1_1);
    y1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1_1, ih1), y1_1);

    __m256 fu1 = _mm256_sub_ps(u1, iu1);
    __m256 fv1 = _mm256_sub_ps(v1, iv1);

    __m256 rfu1 = _mm256_sub_ps(one, fu1);
    __m256 rfv1 = _mm256_sub_ps(one, fv1);

    __m256 u2 = _mm256_mul_ps(u, w2);
    __m256 v2 = _mm256_mul_ps(v, h2);

    __m256 iu2 = _mm256_floor_ps(u2);
    __m256 iv2 = _mm256_floor_ps(v2);

    __m256i x2 = _mm256_cvttps_epi32(iu2);
    __m256i y2 = _mm256_cvttps_epi32(iv2);

    __m256i x2_1 = _mm256_add_epi32(x2, ione);
    __m256i y2_1 = _mm256_add_epi32(y2, ione);

    x2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x2_1, iw2), x2_1);
    y2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y2_1, ih2), y2_1);

    __m256 fu2 = _mm256_sub_ps(u2, iu2);
    __m256 fv2 = _mm256_sub_ps(v2, iv2);

    __m256 rfu2 = _mm256_sub_ps(one, fu2);
    __m256 rfv2 = _mm256_sub_ps(one, fv2);

    __m256i yspan1_1 = _mm256_mullo_epi32(iw1, y1);
    __m256i yspan1_2 = _mm256_mullo_epi32(iw1, y1_1);

    yspan1_1 = _mm256_add_epi32(yspan1_1, base1);
    yspan1_2 = _mm256_add_epi32(yspan1_2, base1);

    __m256i yspan2_1 = _mm256_mullo_epi32(iw2, y2);
    __m256i yspan2_2 = _mm256_mullo_epi32(iw2, y2_1);

    yspan2_1 = _mm256_add_epi32(yspan2_1, base2);
    yspan2_2 = _mm256_add_epi32(yspan2_2, base2);

    __m256i offsets[8];

    offsets[0] = _mm256_add_epi32(yspan1_1, x1);
    offsets[1] = _mm256_add_epi32(yspan1_1, x1_1);
    offsets[2] = _mm256_add_epi32(yspan1_2, x1);
    offsets[3] = _mm256_add_epi32(yspan1_2, x1_1);

    offsets[4] = _mm256_add_epi32(yspan2_1, x2);
    offsets[5] = _mm256_add_epi32(yspan2_1, x2_1);
    offsets[6] = _mm256_add_epi32(yspan2_2, x2);
    offsets[7] = _mm256_add_epi32(yspan2_2, x2_1);

    __m256 weight[8];
    weight[0] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, rfv1));
    weight[1] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, rfv1));
    weight[2] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, fv1));
    weight[3] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, fv1));

    weight[4] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, rfv2));
    weight[5] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, rfv2));
    weight[6] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, fv2));
    weight[7] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, fv2));

    const int32_t* data = reinterpret_cast<const int32_t*>(image1->data());

    __m256i val[8];

    for (size_t i = 0; i < 8; i++) val[i] = _mm256_i32gather_epi32(data, offsets[i], 4);

    __m256 r[8];
    __m256 g[8];
    __m256 b[8];
    __m256 a[8];

    __m256i mask = _mm256_set1_epi32(0xFF);

    bool alpha = image1->format() == PixelFormat::RGBA8;

    for (size_t i = 0; i < 8; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));

        if (alpha) a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    rval = _mm256_mul_ps(r[0], weight[0]);
    gval = _mm256_mul_ps(g[0], weight[0]);
    bval = _mm256_mul_ps(b[0], weight[0]);
    aval = alpha ? _mm256_mul_ps(a[0], weight[0]) : _mm256_set1_ps(255.0f);

    for (size_t i = 1; i < 8; i++)
    {
        rval = _mm256_fmadd_ps(r[i], weight[i], rval);
        gval = _mm256_fmadd_ps(g[i], weight[i], gval);
        bval = _mm256_fmadd_ps(b[i], weight[i], bval);

        if (alpha) aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }

    data = reinterpret_cast<const int32_t*>(image2->data());

    for (size_t i = 0; i < 8; i++) val[i] = _mm256_i32gather_epi32(data, offsets[i], 4);

    for (size_t i = 0; i < 8; i++)
    {
        r[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 24)));
        g[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 16)));
        b[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(val[i], 8)));
    }

    rval2 = _mm256_mul_ps(r[0], weight[0]);
    gval2 = _mm256_mul_ps(g[0], weight[0]);
    bval2 = _mm256_mul_ps(b[0], weight[0]);

    for (size_t i = 1; i < 8; i++)
    {
        rval2 = _mm256_fmadd_ps(r[i], weight[i], rval2);
        gval2 = _mm256_fmadd_ps(g[i], weight[i], gval2);
        bval2 = _mm256_fmadd_ps(b[i], weight[i], bval2);
    }
}

__m256 vectorcall SampleAlphaTrilinear(const Image* image, __m256 u, __m256 v, float lod1, float lod2) noexcept
{
    if (lod1 <= 0.0f && lod2 <= 0.0f) return SampleAlphaLod0(image, u, v);

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256i ione = _mm256_set1_epi32(1);

    float maxmip = image->mipmaps() - 1;

    lod1 = std::max(std::min(lod1, maxmip), 0.0f);
    lod2 = std::max(std::min(lod2, maxmip), 0.0f);

    uint32_t width1_1 = image->width() >> (uint32_t)lod1;
    uint32_t height1_1 = image->height() >> (uint32_t)lod1;

    uint32_t width2_1 = image->width() >> (uint32_t)lod2;
    uint32_t height2_1 = image->height() >> (uint32_t)lod2;

    uint32_t width1_2 = image->width() >> (uint32_t)(lod1 + 1);
    uint32_t height1_2 = image->height() >> (uint32_t)(lod1 + 1);

    uint32_t width2_2 = image->width() >> (uint32_t)(lod2 + 1);
    uint32_t height2_2 = image->height() >> (uint32_t)(lod2 + 1);

    const int32_t* data = reinterpret_cast<const int32_t*>(image->data());

    uint32_t base1_1 = image->offset(lod1);
    uint32_t base2_1 = image->offset(lod2);

    uint32_t base1_2 = image->offset(lod1+1);
    uint32_t base2_2 = image->offset(lod2+1);

    __m256i base1 = _mm256_set_epi32(base1_1, base1_1, base2_1, base2_1, base1_1, base1_1, base2_1, base2_1);
    __m256i base2 = _mm256_set_epi32(base1_2, base1_2, base2_2, base2_2, base1_2, base1_2, base2_2, base2_2);

    __m256i iw1 = _mm256_set_epi32(width1_1, width1_1, width2_1, width2_1, width1_1, width1_1, width2_1, width2_1);
    __m256i ih1 = _mm256_set_epi32(height1_1, height1_1, height2_1, height2_1, height1_1, height1_1, height2_1, height2_1);

    __m256i iw2 = _mm256_set_epi32(width1_2, width1_2, width2_2, width2_2, width1_2, width1_2, width2_2, width2_2);
    __m256i ih2 = _mm256_set_epi32(height1_2, height1_2, height2_2, height2_2, height1_2, height1_2, height2_2, height2_2);

    __m256 w1 = _mm256_cvtepi32_ps(iw1);
    __m256 h1 = _mm256_cvtepi32_ps(ih1);

    __m256 w2 = _mm256_cvtepi32_ps(iw2);
    __m256 h2 = _mm256_cvtepi32_ps(ih2);

    __m256 fw = _mm256_setr_ps(lod1, lod1, lod2, lod2, lod1, lod1, lod2, lod2);
    fw = _mm256_sub_ps(fw, _mm256_floor_ps(fw));
    __m256 rfw = _mm256_sub_ps(one, fw);

    __m256 u1 = _mm256_mul_ps(u, w1);
    __m256 v1 = _mm256_mul_ps(v, h1);

    __m256 iu1 = _mm256_floor_ps(u1);
    __m256 iv1 = _mm256_floor_ps(v1);

    __m256i x1 = _mm256_cvttps_epi32(iu1);
    __m256i y1 = _mm256_cvttps_epi32(iv1);

    __m256i x1_1 = _mm256_add_epi32(x1, ione);
    __m256i y1_1 = _mm256_add_epi32(y1, ione);

    x1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x1_1, iw1), x1_1);
    y1_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y1_1, ih1), y1_1);

    __m256 fu1 = _mm256_sub_ps(u1, iu1);
    __m256 fv1 = _mm256_sub_ps(v1, iv1);

    __m256 rfu1 = _mm256_sub_ps(one, fu1);
    __m256 rfv1 = _mm256_sub_ps(one, fv1);

    __m256 u2 = _mm256_mul_ps(u, w2);
    __m256 v2 = _mm256_mul_ps(v, h2);

    __m256 iu2 = _mm256_floor_ps(u2);
    __m256 iv2 = _mm256_floor_ps(v2);

    __m256i x2 = _mm256_cvttps_epi32(iu2);
    __m256i y2 = _mm256_cvttps_epi32(iv2);

    __m256i x2_1 = _mm256_add_epi32(x2, ione);
    __m256i y2_1 = _mm256_add_epi32(y2, ione);

    x2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(x2_1, iw2), x2_1);
    y2_1 = _mm256_andnot_si256(_mm256_cmpeq_epi32(y2_1, ih2), y2_1);

    __m256 fu2 = _mm256_sub_ps(u2, iu2);
    __m256 fv2 = _mm256_sub_ps(v2, iv2);

    __m256 rfu2 = _mm256_sub_ps(one, fu2);
    __m256 rfv2 = _mm256_sub_ps(one, fv2);

    __m256i yspan1_1 = _mm256_mullo_epi32(iw1, y1);
    __m256i yspan1_2 = _mm256_mullo_epi32(iw1, y1_1);

    yspan1_1 = _mm256_add_epi32(yspan1_1, base1);
    yspan1_2 = _mm256_add_epi32(yspan1_2, base1);

    __m256i yspan2_1 = _mm256_mullo_epi32(iw2, y2);
    __m256i yspan2_2 = _mm256_mullo_epi32(iw2, y2_1);

    yspan2_1 = _mm256_add_epi32(yspan2_1, base2);
    yspan2_2 = _mm256_add_epi32(yspan2_2, base2);

    __m256i offsets[8];

    offsets[0] = _mm256_add_epi32(yspan1_1, x1);
    offsets[1] = _mm256_add_epi32(yspan1_1, x1_1);
    offsets[2] = _mm256_add_epi32(yspan1_2, x1);
    offsets[3] = _mm256_add_epi32(yspan1_2, x1_1);

    offsets[4] = _mm256_add_epi32(yspan2_1, x2);
    offsets[5] = _mm256_add_epi32(yspan2_1, x2_1);
    offsets[6] = _mm256_add_epi32(yspan2_2, x2);
    offsets[7] = _mm256_add_epi32(yspan2_2, x2_1);

    __m256 weight[8];
    weight[0] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, rfv1));
    weight[1] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, rfv1));
    weight[2] = _mm256_mul_ps(rfw, _mm256_mul_ps(rfu1, fv1));
    weight[3] = _mm256_mul_ps(rfw, _mm256_mul_ps(fu1, fv1));

    weight[4] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, rfv2));
    weight[5] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, rfv2));
    weight[6] = _mm256_mul_ps(fw, _mm256_mul_ps(rfu2, fv2));
    weight[7] = _mm256_mul_ps(fw, _mm256_mul_ps(fu2, fv2));

    __m256i val[8];

    for (size_t i = 0; i < 8; i++) val[i] = _mm256_i32gather_epi32(data, offsets[i], 4);

    __m256 a[8];

    __m256i mask = _mm256_set1_epi32(0xFF);

    for (size_t i = 0; i < 8; i++)
    {
        a[i] = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, val[i]));
    }

    __m256 aval = _mm256_mul_ps(a[0], weight[0]);

    for (size_t i = 1; i < 8; i++)
    {
        aval = _mm256_fmadd_ps(a[i], weight[i], aval);
    }

    return aval;
}

__m256 vectorcall CalculateGrad(__m256 u, __m256 v) noexcept
{
    __m256i ind1 = _mm256_setr_epi32(0, 0, 2, 2, 0, 0, 0, 0);
    __m256i ind2 = _mm256_setr_epi32(1, 4, 3, 6, 0, 0, 0, 0);

    __m256 u1 = _mm256_permutevar8x32_ps(u, ind1);
    __m256 u2 = _mm256_permutevar8x32_ps(u, ind2);

    __m256 v1 = _mm256_permutevar8x32_ps(v, ind1);
    __m256 v2 = _mm256_permutevar8x32_ps(v, ind2);

    __m256 grad1 = _mm256_permute2f128_ps(u1, v1, 0x20);
    __m256 grad2 = _mm256_permute2f128_ps(u2, v2, 0x20);

    return _mm256_sub_ps(grad2, grad1);
}

void vectorcall CalculateLod(const Image* image, __m256 grad, float& lod1, float& lod2) noexcept
{
    __m128 w = _mm_set_ps1(image->width());
    __m128 h = _mm_set_ps1(image->height());

    __m256 sz = _mm256_setr_m128(w, h);
    grad = _mm256_mul_ps(grad, sz);
    grad = _mm256_mul_ps(grad, grad);

    __m128 der1 = _mm256_extractf128_ps(grad, 0);
    __m128 der2 = _mm256_extractf128_ps(grad, 1);

    __m128 der = _mm_add_ps(der1, der2);
    der = _mm_sqrt_ps(der);

    der1 = _mm_permute_ps(der, 8);		// 0, 2, 0, 0
    der2 = _mm_permute_ps(der, 13);		// 1, 3, 0, 0

    der = fast_log2(_mm_max_ps(der1, der2));

    lod1 = der.m128_f32[1];
    lod2 = der.m128_f32[0];
}

} // namespace Render