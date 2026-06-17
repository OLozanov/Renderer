#pragma once

#include "Render/Pipeline/Image.h"
#include <immintrin.h>

namespace Render
{

void vectorcall SampleLod0(const Image* image, __m256 u, __m256 v,
                           __m256& rval, __m256& gval, __m256& bval, __m256& aval) noexcept;

void vectorcall Sample2Lod0(const Image* image1, const Image* image2, __m256 u, __m256 v,
                           __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                           __m256& rval2, __m256& gval2, __m256& bval2) noexcept;

__m256 vectorcall SampleAlphaLod0(const Image* image, __m256 u, __m256 v) noexcept;

__m256 vectorcall SampleCmp(const Image* image, __m256 u, __m256 v, __m256 z) noexcept;

void vectorcall Sample(const Image* image, __m256 u, __m256 v, 
                       __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                       float lod1, float lod2) noexcept;

void vectorcall SampleTrilinear(const Image* image, __m256 u, __m256 v,
                                __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                                float lod1, float lod2) noexcept;

void vectorcall Sample2Trilinear(const Image* image1, const Image* image2, __m256 u, __m256 v,
                                __m256& rval, __m256& gval, __m256& bval, __m256& aval,
                                __m256& rval2, __m256& gval2, __m256& bval2,
                                float lod1, float lod2) noexcept;

__m256 vectorcall SampleAlphaTrilinear(const Image* image, __m256 u, __m256 v, float lod1, float lod2) noexcept;

__m256 vectorcall CalculateGrad(__m256 u, __m256 v) noexcept;
void vectorcall CalculateLod(const Image* image, __m256 grad, float& lod1, float& lod2) noexcept;

} // namespace Render