#include "Transform.h"

void VertexTransformShader(const SceneTransformData* data, uint32_t x, uint32_t y) noexcept
{
    __m128 col1 = _mm_load_ps(&data->projView[0][0]);
    __m128 col2 = _mm_load_ps(&data->projView[1][0]);
    __m128 col3 = _mm_load_ps(&data->projView[2][0]);
    __m128 col4 = _mm_load_ps(&data->projView[3][0]);

    __m256 proj1 = _mm256_set_m128(col1, col1);
    __m256 proj2 = _mm256_set_m128(col2, col2);
    __m256 proj3 = _mm256_set_m128(col3, col3);
    __m256 proj4 = _mm256_set_m128(col4, col4);

    uint32_t begin = x * data->batch;
    uint32_t end = std::min(begin + data->batch, data->num);

    for (uint32_t i = begin; i < end; i += 2)
    {
        __m128 x1 = _mm_set_ps1(data->in[i].position.x);
        __m128 y1 = _mm_set_ps1(data->in[i].position.y);
        __m128 z1 = _mm_set_ps1(data->in[i].position.z);

        __m128 x2 = _mm_set_ps1(data->in[i + 1].position.x);
        __m128 y2 = _mm_set_ps1(data->in[i + 1].position.y);
        __m128 z2 = _mm_set_ps1(data->in[i + 1].position.z);

        __m256 x = _mm256_setr_m128(x1, x2);
        __m256 y = _mm256_setr_m128(y1, y2);
        __m256 z = _mm256_setr_m128(z1, z2);

        __m256 pos = proj4;
        pos = _mm256_fmadd_ps(proj1, x, pos);
        pos = _mm256_fmadd_ps(proj2, y, pos);
        pos = _mm256_fmadd_ps(proj3, z, pos);

        __m128 pos1 = _mm256_castps256_ps128(pos);
        __m128 pos2 = _mm256_extractf128_ps(pos, 1);

        _mm_store_ps((float*)&data->out[i].position, pos1);
        _mm_store_ps((float*)&data->out[i + 1].position, pos2);
    }
}