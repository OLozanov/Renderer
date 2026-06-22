#include "Pipeline.h"

#include "Render/Pipeline/CommandList.h"
#include "Utility/Affinity.h"

#include <algorithm>
#include <utility>
#include <cmath>

#include <iostream>

namespace Render
{

Pipeline::Pipeline()
: m_colorBuffer(nullptr)
, m_depthBuffer(nullptr)
, m_width(0)
, m_height(0)
, m_commands(nullptr)
, m_executeBarrier(ThreadNum)
, m_finishBarrier(ThreadNum)
{
    initThreads();
}

Pipeline::~Pipeline()
{
    m_terminate = true;
}

void Pipeline::initThreads()
{
    SetThreadAffinity(GetCurrentThread(), 0);

    for (size_t i = 1; i < ThreadNum; i++)
    {
        std::thread thread(&Pipeline::executeLoop, this, i);
        SetThreadAffinity(thread.native_handle(), i);

        thread.detach();
    }

    for (size_t k = 0; k < YTiles; k++)
        for (size_t i = 0; i < XTiles; i++)
        {
            size_t tid = k * XTiles + i;
            m_tileState[tid].tx = i;
            m_tileState[tid].ty = k;
        }
}

void Pipeline::bindFrameBuffer(ColorBuffer* surface, DepthBuffer* depth)
{
    m_colorBuffer = surface;
    m_depthBuffer = depth;

    uint32_t width, height;

    if (surface)
    {
        width = surface->width();
        height = surface->height();
    }
    else
    {
        width = depth->width();
        height = depth->height();
    }

    if (m_width == width && m_height == height) return;

    m_width = width;
    m_height = height;

    m_xblocks = m_width >> 2;
    m_yblocks = m_height >> 1;

    uint32_t tbx = (m_xblocks + XTiles - 1) / XTiles;
    uint32_t tby = (m_yblocks + YTiles - 1) / YTiles;

    float xfrac = float(tbx) / m_xblocks * 2.0f;
    float yfrac = float(tby) / m_yblocks * 2.0f;

    for (size_t k = 0; k < YTiles; k++)
        for (size_t i = 0; i < XTiles; i++)
        {
            size_t tid = k * XTiles + i;

            float left = 1.0f - xfrac * i;
            float right = xfrac * (i + 1) - 1.0f;

            float top = 1.0f - yfrac * (YTiles - k - 1) + 0.01f;
            float bottom = yfrac * (YTiles - k) - 1.0f;

            /*********************************
            * Full screen clip planes:
            { {0.0f, 0.0f, 1.0f, 0.0f},
              {0.0f, 0.0f, -1.0f, 1.0f},
              {1.0f, 0.0f, 0.0f, 1.0f},
              {-1.0f, 0.0f, 0.0f, 1.0f},
              {0.0f, 1.0f, 0.0f, 1.0f},
              {0.0f, -1.0f, 0.0f, 1.0f} };
            *********************************/

            m_tileState[tid].clipPlanes[0] = { 0.0f, 0.0f, 1.0f, 0.0f };
            m_tileState[tid].clipPlanes[1] = { 0.0f, 0.0f, -1.0f, 1.0f };

            m_tileState[tid].clipPlanes[2] = { 1.0f, 0.0f, 0.0f, left };
            m_tileState[tid].clipPlanes[3] = { -1.0f, 0.0f, 0.0f, right };

            m_tileState[tid].clipPlanes[4] = { 0.0f, 1.0f, 0.0f, top };
            m_tileState[tid].clipPlanes[5] = { 0.0f, -1.0f, 0.0f, bottom };

            m_tileState[tid].xmin = tbx * i;
            m_tileState[tid].xmax = std::min<int32_t>(tbx * (i + 1) - 1, m_xblocks - 1);

            m_tileState[tid].ymin = tby * k;
            m_tileState[tid].ymax = std::min<int32_t>(tby * (k + 1) - 1, m_yblocks - 1);
        }
}

bool Pipeline::ClipPolygon(const glm::vec4& plane,
                           float* in, float* out,
                           size_t& vnum,
                           uint32_t attribSets) noexcept
{
    size_t n = 0;

    size_t addribnum = (attribSets << 3);

    float dist[MaxClippedVerts];

    __m128 pvec = _mm_load_ps((float*)&plane);
    __m256 pvec2 = _mm256_set_m128(pvec, pvec);

    size_t back = 0;

    for (size_t v = 0; v < vnum / 2; v++)
    {
        size_t i = v * 2;
        size_t k = i + 1;

        float* addra = in + i * addribnum;
        float* addrb = in + k * addribnum;

        __m256 pos = _mm256_set_m128(_mm_load_ps(addrb), _mm_load_ps(addra));

        __m256 dp = _mm256_dp_ps(pvec2, pos, 0xFF);
        dist[i] = dp.m256_f32[0];
        dist[k] = dp.m256_f32[4];

        if (dist[i] < -eps) back++;
        if (dist[k] < -eps) back++;
    }

    if (vnum % 2 > 0)
    {
        size_t v = vnum - 1;
        float* addr = in + v * addribnum;

        __m128 pos = _mm_load_ps(addr);
        __m128 dp = _mm_dp_ps(pvec, pos, 0xFF);
        dist[v] = _mm_cvtss_f32(dp);

        if (dist[v] < -eps) back++;
    }

    if (back == 0) return false;

    if (back == vnum)
    {
        vnum = 0;
        return false;
    }

    for (size_t i = 0; i < vnum; i++)
    {
        size_t k = (i == vnum - 1) ? 0 : i + 1;

        float* addra = in + i * addribnum;
        float* addrb = in + k * addribnum;

        if ((dist[i] > eps && dist[k] < -eps) ||
            (dist[i] < -eps && dist[k] > eps))
        {
            float frac = dist[i] / (dist[i] - dist[k]);

            __m256 t = _mm256_set1_ps(frac);
            __m256 rt = _mm256_set1_ps(1.0f - frac);

            float* aptr = addra;
            float* bptr = addrb;
            float* outptr = out + n * addribnum;

            for (size_t k = 0; k < attribSets; k++)
            {
                __m256 a = _mm256_load_ps(aptr);
                __m256 b = _mm256_load_ps(bptr);
                __m256 res = _mm256_mul_ps(a, rt);
                res = _mm256_fmadd_ps(b, t, res);
                _mm256_store_ps(outptr, res);

                aptr += 8;
                bptr += 8;
                outptr += 8;
            }

            n++;
        }

        if (dist[k] > -eps)
        {
            float* bptr = addrb;
            float* outptr = out + n * addribnum;

            for (size_t k = 0; k < attribSets; k++)
            {
                __m256 b = _mm256_load_ps(bptr);
                _mm256_store_ps(outptr, b);

                bptr += 8;
                outptr += 8;
            }

            n++;
        }
    }

    vnum = n;

    return true;
}

void Pipeline::draw(size_t n, size_t num, size_t offset) noexcept
{
    size_t attribnum = (m_tileState[n].attribSets << 3);

    // Buffers for clipping in ping-pong manner
    size_t bsize = MaxClippedVerts * (m_tileState[n].attribSets << 3) * sizeof(float);

    float* vbuff1 = reinterpret_cast<float*>(_alloca(bsize));
    float* vbuff2 = reinterpret_cast<float*>(_alloca(bsize));

    glm::vec2 scverts[MaxClippedVerts];

    for (size_t i = offset; i + 2 < offset + num; i += 3)
    {
        size_t vnum = 3;

        float* vin = vbuff1;
        float* vout = vbuff2;

        const uint8_t* in1 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * i;
        const uint8_t* in2 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * (i + 1);
        const uint8_t* in3 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * (i + 2);

        m_tileState[n].vertexShader(m_tileState[n].shaderData, in1, vin);
        m_tileState[n].vertexShader(m_tileState[n].shaderData, in2, vin + attribnum);
        m_tileState[n].vertexShader(m_tileState[n].shaderData, in3, vin + attribnum * 2);

        for (size_t p = 0; p < 6; p++)
        {
            if (ClipPolygon(m_tileState[n].clipPlanes[p], vin, vout, vnum, m_tileState[n].attribSets))
                std::swap(vin, vout);

            if (vnum == 0) break;
        }

        if (vnum == 0) continue;

        for (size_t v = 0; v < vnum; v++)
        {
            size_t vptr = v * attribnum;

            __m128 vert = _mm_setr_ps(vin[vptr], vin[vptr + 1], vin[vptr + 2], 1.0f);
            __m128 rz = _mm_set1_ps(1.0f / vin[vptr + 3]);
            vert = _mm_mul_ps(vert, rz);
            _mm_store_ps(vin + vptr, vert);

            scverts[v].x = (vin[vptr] * 0.5f + 0.5f) * m_width;
            scverts[v].y = (0.5f - vin[vptr + 1] * 0.5f) * m_height;
        }

        if (m_tileState[n].polygonMode == PolygonMode::Fill)
        {
            for (size_t v = 0; v < vnum - 1; v++)
                drawTriangle(n, i / 3, vin, scverts, 0, v, v + 1, attribnum);
        }
        else if (m_tileState[n].polygonMode == PolygonMode::Line)
        {
            drawWirePolygon(scverts, vnum);
        }
        else if (m_tileState[n].polygonMode == PolygonMode::Point)
        {
            for (size_t v = 0; v < vnum; v++)
                m_colorBuffer->pixel(scverts[v].x, scverts[v].y) = 0xFFFFFFFF;
        }
    }
}

void Pipeline::drawIndexed(size_t n, size_t num, size_t offset, size_t vertexOffset) noexcept
{
    size_t attribnum = (m_tileState[n].attribSets << 3);

    // Buffers for clipping in ping-pong manner
    size_t bsize = MaxClippedVerts * (m_tileState[n].attribSets << 3) * sizeof(float);

    float* vbuff1 = reinterpret_cast<float*>(_alloca(bsize));
    float* vbuff2 = reinterpret_cast<float*>(_alloca(bsize));

    glm::vec2 scverts[MaxClippedVerts];

    for (size_t i = offset; i + 2 < offset + num; i += 3)
    {
        IndexType idx[3] = { m_tileState[n].indexBuffer[i],
                             m_tileState[n].indexBuffer[i + 1],
                             m_tileState[n].indexBuffer[i + 2] };

        size_t vnum = 3;

        float* vin = vbuff1;
        float* vout = vbuff2;

        const uint8_t* in1 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * idx[0];
        const uint8_t* in2 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * idx[1];
        const uint8_t* in3 = m_tileState[n].vertexBuffer.data + m_tileState[n].vertexBuffer.stride * idx[2];
        
        m_tileState[n].vertexShader(m_tileState[n].shaderData, in1, vin);
        m_tileState[n].vertexShader(m_tileState[n].shaderData, in2, vin + attribnum);
        m_tileState[n].vertexShader(m_tileState[n].shaderData, in3, vin + attribnum * 2);

        for (size_t p = 0; p < 6; p++)
        {
            if (ClipPolygon(m_tileState[n].clipPlanes[p], vin, vout, vnum, m_tileState[n].attribSets))
                std::swap(vin, vout);

            if (vnum == 0) break;
        }

        if (vnum == 0) continue;

        for (size_t v = 0; v < vnum; v++)
        {
            size_t vptr = v * attribnum;

            __m128 vert = _mm_setr_ps(vin[vptr], vin[vptr + 1], vin[vptr + 2], 1.0f);
            __m128 rz = _mm_set1_ps(1.0f / vin[vptr + 3]);
            vert = _mm_mul_ps(vert, rz);
            _mm_store_ps(vin + vptr, vert);

            scverts[v].x = (vin[vptr] * 0.5f + 0.5f) * m_width;
            scverts[v].y = (0.5f - vin[vptr + 1] * 0.5f) * m_height;
        }

        if (m_tileState[n].polygonMode == PolygonMode::Fill)
        {
            for (size_t v = 0; v < vnum - 1; v++)
                drawTriangle(n, i / 3, vin, scverts, 0, v, v + 1, attribnum);
        }
        else if (m_tileState[n].polygonMode == PolygonMode::Line)
        {
            drawWirePolygon(scverts, vnum);
        }
        else if (m_tileState[n].polygonMode == PolygonMode::Point)
        {
            for (size_t v = 0; v < vnum; v++)
            {
                if (scverts[v].x >= 0 && scverts[v].x < m_colorBuffer->width() &&
                    scverts[v].y >= 0 && scverts[v].y < m_colorBuffer->height())

                m_colorBuffer->pixel(scverts[v].x, scverts[v].y) = 0xFFFFFFFF;
            }
        }
    }
}

void Pipeline::clearColor(size_t n, uint8_t r, uint8_t g, uint8_t b) noexcept
{
    uint32_t width = m_colorBuffer->width() >> 2;
    uint32_t height = m_colorBuffer->height() >> 1;

    uint32_t color = 0xFF | (b << 8) | (g << 16) | (r << 24);
    __m256i cvec = _mm256_set1_epi32(color);
    
    uint32_t x = (m_tileState[n].xmax - m_tileState[n].xmin) >> 1;
    uint32_t y = (m_tileState[n].ymax - m_tileState[n].ymin + 1) << 1;
    uint32_t stride = m_width >> 3;

    __m256i* addr = reinterpret_cast<__m256i*>(m_colorBuffer->data()) + 
                    (m_tileState[n].ymin << 1) * stride +
                    (m_tileState[n].xmin >> 1);

    stride -= (x + 1);

    for (uint32_t k = 0; k < y; k++, addr += stride)
    {
        for (uint32_t i = 0; i <= x; i++, addr++)
            _mm256_storeu_si256(addr, cvec);
    }
}

void Pipeline::clearDepth(size_t n, float val) noexcept
{
    uint32_t width = m_width >> 2;
    uint32_t height = m_height >> 1;

    __m256 depth = _mm256_set1_ps(val);

    for (uint32_t y = m_tileState[n].ymin; y <= m_tileState[n].ymax; y++)
        for (uint32_t x = m_tileState[n].xmin; x <= m_tileState[n].xmax; x++)
        {
            float* addr = m_depthBuffer->data() + (y * m_xblocks + x) * 8;
            _mm256_store_ps(addr, depth);
        }
}

void Pipeline::unpackDepth() noexcept
{
    float* buffer = new float[m_xblocks * m_yblocks * 8];

    for (size_t k = 0; k < m_yblocks; k++)
        for (size_t i = 0; i < m_xblocks; i++)
        {
            float* addr = m_depthBuffer->data() + (k * m_xblocks + i) * 8;
            __m256 depth = _mm256_loadu_ps(addr);

            __m128 depth_lo = _mm256_extractf128_ps(depth, 0);
            __m128 depth_hi = _mm256_extractf128_ps(depth, 1);

            uint32_t x = i << 2;
            uint32_t y = k << 1;

            float* addr1 = buffer + y * m_width + x;
            float* addr2 = addr1 + m_width;
            _mm_storeu_ps(addr1, depth_lo);
            _mm_storeu_ps(addr2, depth_hi);
        }

    float* ptr1 = buffer;
    float* ptr2 = m_depthBuffer->data();
    size_t sz = m_xblocks * m_yblocks;

    for (size_t i = 0; i < sz; i++, ptr1 +=8, ptr2 += 8)
    {
        __m256 val = _mm256_loadu_ps(ptr1);
        _mm256_storeu_ps(ptr2, val);
    }

    delete buffer;
}

void Pipeline::linex(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;

    int32_t yinc = dy >= 0 ? 1 : -1;
    if (dy < 0) dy = -dy;

    int32_t y = y1;
    int32_t p = 2 * dy - dx;

    for (int32_t x = x1; x <= x2; x++)
    {
        if (x >= 0 && x < m_colorBuffer->width() &&
            y >= 0 && y < m_colorBuffer->height())
                m_colorBuffer->pixel(x, y) = 0xFFFFFFFF;

        if (p > 0)
        {
            y += yinc;
            p = p + 2 * (dy - dx);
        }
        else
            p = p + 2 * dy;
    }
}

void Pipeline::liney(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;

    int32_t xinc = dx >= 0 ? 1 : -1;
    if (dx < 0) dx = -dx;

    int32_t x = x1;
    int32_t p = 2 * dx - dy;

    for (int32_t y = y1; y <= y2; y++)
    {
        if (x >= 0 && x < m_colorBuffer->width() && 
            y >= 0 && y < m_colorBuffer->height())
                m_colorBuffer->pixel(x, y) = 0xFFFFFFFF;

        if (p > 0)
        {
            x += xinc;
            p = p + 2 * (dx - dy);
        }
        else
            p = p + 2 * dx;
    }
}

void Pipeline::line(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    if (abs(y2 - y1) < abs(x2 - x1))
    {
        if (x1 > x2)
            linex(x2, y2, x1, y1);
        else
            linex(x1, y1, x2, y2);
    }
    else
    {
        if (y1 > y2)
            liney(x2, y2, x1, y1);
        else
            liney(x1, y1, x2, y2);
    }
}

void Pipeline::drawWirePolygon(const glm::vec2* scpos, size_t num)
{
    for (size_t v1 = 0; v1 < num; v1++)
    {
        size_t v2 = (v1 == num - 1) ? 0 : v1 + 1;
        line(scpos[v1].x, scpos[v1].y, scpos[v2].x, scpos[v2].y);
    }
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

//-----------------------------------------------------------------------------
//  drawBlock
//-----------------------------------------------------------------------------
// float posx[4] = { -1.5f, -0.5f, 0.5f, 1.5f };
// float posy[2] = { -0.5f, 0.5f };
//
// float d1 = block.d1 - dstx[i] * triangle.e1.x - dsty[k] * triangle.e1.y;
// float d2 = block.d2 - dstx[i] * triangle.e2.x - dsty[k] * triangle.e2.y;
// 
// float u = d1;
// float v = d2;
// float w = 1.0f - u - v;
// 
// if (u < 0.0f) continue;
// if (v < 0.0f) continue;
// if (w < 0.0f) continue;
// 
// u *= triangle.weights[0];
// v *= triangle.weights[1];
// w *= triangle.weights[2];
// 
// if (if pixelShader) --> call pixelShader
// 
//---------------------------------------------------------------
void Pipeline::drawBlock(size_t n, const BlockData& block) noexcept
{
    const TriangleData& triangle = block.triangle;

    __m256 u = _mm256_add_ps(_mm256_set1_ps(block.d1), triangle.te1);
    __m256 v = _mm256_add_ps(_mm256_set1_ps(block.d2), triangle.te2);
    __m256 w = _mm256_set1_ps(1.0f);
    w = _mm256_sub_ps(w, u);
    w = _mm256_sub_ps(w, v);

    __m256 eps = _mm256_set1_ps(-0.001f);

    __m256 utest = _mm256_cmp_ps(eps, u, _CMP_LT_OS);
    __m256 vtest = _mm256_cmp_ps(eps, v, _CMP_LT_OS);
    __m256 wtest = _mm256_cmp_ps(eps, w, _CMP_LT_OS);

    __m256 test = _mm256_and_ps(utest, vtest);
    test = _mm256_and_ps(test, wtest);

    if (_mm256_testz_ps(test, test)) return;

    __m256 depth;
    float* depthptr;

    if (triangle.flags & DepthTest)
    {
        __m256 z[3];
        z[0] = _mm256_set1_ps(triangle.az);
        z[1] = _mm256_set1_ps(triangle.bz);
        z[2] = _mm256_set1_ps(triangle.cz);

        depth = _mm256_mul_ps(z[0], u);

        depth = _mm256_fmadd_ps(z[1], v, depth);
        depth = _mm256_fmadd_ps(z[2], w, depth);

        depthptr = m_depthBuffer->data() + (block.y * m_xblocks + block.x) * 8;
        __m256 bufdepth = _mm256_load_ps(depthptr);

        __m256 dtest;
        
        switch (m_tileState[n].depthFunc)
        {
        case Less: dtest = _mm256_cmp_ps(depth, bufdepth, _CMP_LT_OS); break;
        case LessOrEqual: dtest = _mm256_cmp_ps(depth, bufdepth, _CMP_LE_OS); break;
        case EqualOne: dtest = _mm256_cmp_ps(_mm256_set1_ps(1.0f), bufdepth, _CMP_EQ_OS); break;
        default:
            dtest = _mm256_setzero_ps();
        }

        test = _mm256_and_ps(test, dtest);

        if (_mm256_testz_ps(test, test)) return;
    }

    if (m_tileState[n].pixelShader)
    {
        __m256 w1 = _mm256_set1_ps(triangle.weights[0]);
        __m256 w2 = _mm256_set1_ps(triangle.weights[1]);
        __m256 w3 = _mm256_set1_ps(triangle.weights[2]);

        u = _mm256_mul_ps(u, w1);
        v = _mm256_mul_ps(v, w2);
        w = _mm256_mul_ps(w, w3);

        __m256 z = _mm256_add_ps(_mm256_add_ps(u, v), w);
        __m256 rz = _mm256_div_ps(_mm256_set1_ps(1.0f), z); // _mm256_rcp_ps isnt precise enought - visible banding artifacts

        __m256 r, g, b, a;

        m_tileState[n].pixelShader(m_tileState[n].shaderData,
                                   triangle.idx, block.x, block.y, 
                                   triangle.attrib_a, triangle.attrib_b, triangle.attrib_c, 
                                   u, v, w, rz,
                                   r, g, b, a);

        if (triangle.flags & AlphaTest)
        {
            __m256 eps = _mm256_set1_ps(m_tileState[n].alpha);
            __m256 atest = _mm256_cmp_ps(a, eps, _CMP_GT_OS);

            test = _mm256_and_ps(test, atest);

            if (_mm256_testz_ps(test, test)) return;
        }

        if (triangle.flags & Blend)
        {
            uint32_t* addr1 = &m_colorBuffer->pixel(block.x * 4, block.y * 2);
            uint32_t* addr2 = addr1 + m_colorBuffer->width();

            __m128i color_lo = _mm_load_si128((__m128i*)addr1);
            __m128i color_hi = _mm_load_si128((__m128i*)addr2);

            __m256i color = _mm256_setr_m128i(color_lo, color_hi);
            __m256i mask = _mm256_set1_epi32(0xFF);

            __m256 srcr = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(color, 24)));
            __m256 srcg = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(color, 16)));
            __m256 srcb = _mm256_cvtepi32_ps(_mm256_and_epi32(mask, _mm256_srli_epi32(color, 8)));

            __m256 lower_threshold = _mm256_set1_ps(1.5f);
            __m256 upper_threshold = _mm256_set1_ps(254.5f);
            
            __m256 lotest = _mm256_cmp_ps(a, lower_threshold, _CMP_GE_OS);
            __m256 hitest = _mm256_cmp_ps(a, upper_threshold, _CMP_GE_OS);

            if (_mm256_testz_ps(lotest, lotest)) return; // alpha is effectively 0.0

            if (_mm256_testz_ps(hitest, hitest)) // blend
            {
                __m256 alpha = _mm256_mul_ps(a, _mm256_set1_ps(1.0f / 255.0f));
                __m256 ralpha = _mm256_sub_ps(_mm256_set1_ps(1.0f), alpha);

                srcr = _mm256_mul_ps(srcr, ralpha);
                srcg = _mm256_mul_ps(srcg, ralpha);
                srcb = _mm256_mul_ps(srcb, ralpha);

                r = _mm256_fmadd_ps(r, alpha, srcr);
                g = _mm256_fmadd_ps(g, alpha, srcg);
                b = _mm256_fmadd_ps(b, alpha, srcb);
            }
        }

        if (triangle.flags & ColorWrite)
        {
            __m256i mask = _mm256_castps_si256(test);

            __m256i color = ToRGB8(r, g, b);

            __m128i color_lo = _mm256_extracti128_si256(color, 0);
            __m128i color_hi = _mm256_extracti128_si256(color, 1);

            __m128i mask_lo = _mm256_extracti128_si256(mask, 0);
            __m128i mask_hi = _mm256_extracti128_si256(mask, 1);

            uint32_t* addr1 = &m_colorBuffer->pixel(block.x * 4, block.y * 2);
            uint32_t* addr2 = addr1 + m_colorBuffer->width();
            _mm_maskstore_epi32((int32_t*)addr1, mask_lo, color_lo);
            _mm_maskstore_epi32((int32_t*)addr2, mask_hi, color_hi);
        }
    }

    if (triangle.flags & DepthWrite)
    {
        __m256i mask = _mm256_castps_si256(test);
        _mm256_maskstore_ps(depthptr, mask, depth);
    }
}

void Pipeline::drawTriangle(size_t n,
                            uint32_t idx, 
                            const float* attrib,
                            const glm::vec2* scpos,
                            size_t ai, size_t bi, size_t ci,
                            size_t attribnum) noexcept
{
    static constexpr float ystep = 2.0f;
    static constexpr float xstep = 4.0f;

    static constexpr float ygrid = 1.0f / ystep;
    static constexpr float xgrid = 1.0f / xstep;

    if (m_tileState[n].flags & CullFace)
    {
        glm::vec2 ab = scpos[bi] - scpos[ai];
        glm::vec2 ac = scpos[ci] - scpos[ai];
        float abc = ab.x * ac.y - ab.y * ac.x;
        float rabc = 1.0f / abc;

        if (abc < 0) return; // cull conter-clockwise
    }

    if (scpos[ai].y > scpos[bi].y) std::swap(ai, bi);
    if (scpos[bi].y > scpos[ci].y) std::swap(bi, ci);
    if (scpos[ai].y > scpos[bi].y) std::swap(ai, bi);

    const glm::vec2& a = scpos[ai];
    const glm::vec2& b = scpos[bi];
    const glm::vec2& c = scpos[ci];

    int32_t xmin = std::min(a.x, std::min(b.x, c.x)) * xgrid;
    int32_t xmax = std::max(a.x, std::max(b.x, c.x)) * xgrid;

    if (xmin > m_tileState[n].xmax) return;
    if (xmax < m_tileState[n].xmin) return;

    int32_t ay = a.y * ygrid;
    int32_t by = b.y * ygrid;
    int32_t cy = c.y * ygrid;

    if (ay > m_tileState[n].ymax) return;
    if (cy < m_tileState[n].ymin) return;

    glm::vec2 ab = scpos[bi] - scpos[ai];
    glm::vec2 ca = scpos[ai] - scpos[ci];
    glm::vec2 bc = scpos[ci] - scpos[bi];
    float area = ab.x * bc.y - ab.y * bc.x;
    float rarea = 1.0f / area;

    const glm::vec4& posa = *(const glm::vec4*)(attrib + ai * attribnum);
    const glm::vec4& posb = *(const glm::vec4*)(attrib + bi * attribnum);
    const glm::vec4& posc = *(const glm::vec4*)(attrib + ci * attribnum);

    TriangleData data = { .idx = idx,
                          .flags = m_tileState[n].flags,
                          .az = posa.z,
                          .bz = posb.z,
                          .cz = posc.z,
                          .attrib_a = (const float*)(attrib + ai * attribnum + 4),
                          .attrib_b = (const float*)(attrib + bi * attribnum + 4),
                          .attrib_c = (const float*)(attrib + ci * attribnum + 4),
                          .weights = { posa.w, posb.w, posc.w },
                          .e1 = glm::vec2(bc.y, -bc.x) * rarea, // edge functions directed outwards of triangle
                          .e2 = glm::vec2(ca.y, -ca.x) * rarea,
                        };

    __m256 posx = { -1.5f, -0.5f, 0.5f, 1.5f, -1.5f, -0.5f, 0.5f, 1.5f };
    __m256 posy = { -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f };

    __m256 e1 = _mm256_set1_ps(-data.e1.x);
    __m256 e2 = _mm256_set1_ps(-data.e2.x);

    __m256 te1 = _mm256_mul_ps(posx, e1);
    __m256 te2 = _mm256_mul_ps(posx, e2);

    e1 = _mm256_set1_ps(-data.e1.y);
    e2 = _mm256_set1_ps(-data.e2.y);

    data.te1 = _mm256_fmadd_ps(posy, e1, te1);
    data.te2 = _mm256_fmadd_ps(posy, e2, te2);

    int32_t yend = std::min(cy, m_tileState[n].ymax);

    int32_t iy = std::max(ay, m_tileState[n].ymin);
    float y = iy == ay ? a.y : iy * 2; //a.y;
    float dy = iy == ay ? (iy + 1) * 2 - a.y : ystep;

    float dx1 = iy <= by ? (a.x - b.x) / (a.y - b.y) : (b.x - c.x) / (b.y - c.y);
    float dx2 = (a.x - c.x) / (a.y - c.y);

    float fx1 = iy <= by ? 
                fabs(a.y - b.y) < eps ? b.x : a.x + dx1 * (y - a.y) : 
                fabs(b.y - c.y) < eps ? c.x : b.x + dx1 * (y - b.y);
    float fx2 = fabs(a.y - c.y) < eps ? c.x : a.x + dx2 * (y - a.y);

    int32_t x1 = fx1 * xgrid;
    int32_t x2 = fx2 * xgrid;

    if (x1 > x2) std::swap(x1, x2);

    // First block edge functions value
    glm::vec2 pt = glm::vec2((x1 + 0.5f) * xstep, (iy + 0.5f) * ystep);
    glm::vec2 pb = scpos[bi] - pt;
    glm::vec2 pc = scpos[ci] - pt;

    float ld1 = glm::dot(data.e1, pb);
    float ld2 = glm::dot(data.e2, pc);

    while(true)
    {
        if (iy == by)
        {
            dy = b.y - y;

            fx1 = b.x;
            fx2 += dx2 * dy;
            dx1 = (b.x - c.x) / (b.y - c.y);

            int32_t bx = b.x * xgrid;

            x1 = std::min(x1, bx);
            x2 = std::max(x2, bx);

            dy = (iy + 1) * 2 - b.y;
        }

        int32_t x3, x4;

        if (iy == cy)
        {
            int32_t cx = c.x * xgrid;

            x3 = cx;
            x4 = cx;
        }
        else
        {
            fx1 += dx1 * dy;
            fx2 += dx2 * dy;

            y += dy;

            x3 = fx1 * xgrid;
            x4 = fx2 * xgrid;
        }

        if (x3 > x4) std::swap(x3, x4);

        int32_t xstart = std::max(std::min(x1, x3), m_tileState[n].xmin);
        int32_t xend = std::min(std::max(x2, x4), m_tileState[n].xmax);

        float d1 = ld1 + (pt.x - (xstart + 0.5f) * xstep) * data.e1.x;
        float d2 = ld2 + (pt.x - (xstart + 0.5f) * xstep) * data.e2.x;

        for (int32_t x = xstart; x <= xend; x++)
        {
            BlockData block = { .triangle = data,
                                .x = uint32_t(x),
                                .y = uint32_t(iy),
                                .d1 = d1,
                                .d2 = d2
            };

            drawBlock(n, block);

            d1 -= xstep * data.e1.x;
            d2 -= xstep * data.e2.x;
        }

        if (iy >= yend) break;

        x1 = x3;
        x2 = x4;

        iy++;
        dy = ystep;

        ld1 -= ystep * data.e1.y;
        ld2 -= ystep * data.e2.y;
    }
}

void Pipeline::dispatch(size_t n, uint32_t x, uint32_t y)
{
    uint32_t tilex = x / XTiles;
    uint32_t tiley = y / YTiles;

    uint32_t xmin = m_tileState[n].tx * tilex;
    uint32_t xmax = xmin + tilex - 1;

    uint32_t ymin = m_tileState[n].ty * tiley;
    uint32_t ymax = ymin + tiley - 1;

    for (uint32_t k = ymin; k <= ymax; k++)
        for (uint32_t i = xmin; i <= xmax; i++)
        {
            m_tileState[n].computeShader(m_tileState[n].shaderData, i, k);
        }
}

void Pipeline::dispatch(size_t n, uint32_t num)
{
    uint32_t count = num / ThreadNum;

    uint32_t min = n * count;
    uint32_t max = min + count - 1;

    for (uint32_t i = min; i <= max; i++) m_tileState[n].computeShader(m_tileState[n].shaderData, i, 0);
}

void Pipeline::bitblt_rgb8(Image* image, int32_t x, int32_t y, int32_t width, int32_t height)
{
    int32_t x1 = std::max(x, 0);
    int32_t x2 = std::min(x + width - 1, (int32_t)m_colorBuffer->width() - 1);

    int32_t y1 = std::max(y, 0);
    int32_t y2 = std::min(y + height - 1, (int32_t)m_colorBuffer->height() - 1);

    float dx = image->width() / float(width);
    float dy = image->height() / float(height);

    uint32_t lod = std::clamp<int32_t>(std::max(std::ceil(log2(dx)), std::ceil(log2(dy))), 0, image->mipmaps() - 1);

    for (int32_t k = y1; k <= y2; k++)
    {
        for (int32_t i = x1; i <= x2; i++)
        {
            float u = (i - x) / float(width);
            float v = (y + height - 1 - k) / float(height);

            m_colorBuffer->pixel(i, k) = image->loadLod<RGBAPixel>(u, v, lod);
        }
    }
}

void Pipeline::bitblt_f32(Image* image, int32_t x, int32_t y, int32_t width, int32_t height)
{
    int32_t x1 = std::max(x, 0);
    int32_t x2 = std::min(x + width - 1, (int32_t)m_colorBuffer->width() - 1);

    int32_t y1 = std::max(y, 0);
    int32_t y2 = std::min(y + height - 1, (int32_t)m_colorBuffer->height() - 1);

    float dx = image->width() / float(width);
    float dy = image->height() / float(height);

    for (int32_t k = y1; k <= y2; k++)
    {
        for (int32_t i = x1; i <= x2; i++)
        {
            float u = (i - x) / float(width);
            float v = (y + height - 1 - k) / float(height);

            uint32_t val = std::min<uint32_t>(255, std::min(1.0f, image->loadLodNearest<float>(u, v, 0)) * 255.0f);

            m_colorBuffer->pixel(i, k) = 0xFF | (val << 8) | (val << 16) | (val << 24);
        }
    }
}

void Pipeline::bitblt(Image* image, int32_t x, int32_t y, int32_t width, int32_t height)
{
    switch (image->format())
    {
    case PixelFormat::RGBX8:
    case PixelFormat::RGBA8: bitblt_rgb8(image, x, y, width, height); break;
    case PixelFormat::F32: bitblt_f32(image, x, y, width, height); break;
    default:
        break;
    }
}

void Pipeline::executeLoop(uint32_t n)
{
    while (true)
    {
        m_executeBarrier.arrive_and_wait();
        executeLocal(m_commands, n);
        m_finishBarrier.arrive_and_wait();
    }
}

void Pipeline::executeLocal(const Command* commands, size_t n, bool reset)
{
    if (reset)
    {
        m_tileState[n].flags = 0;
        m_tileState[n].depthFunc = Less;
        m_tileState[n].polygonMode = PolygonMode::Fill;
    }

    while (true)
    {
        const Command& cmd = commands[m_tileState[n].cmdptr++];

        if (cmd.type == Command::Finish) return;

        switch (cmd.type)
        {
            case Command::Execute:
            {
                size_t cmdptr = m_tileState[n].cmdptr;
                m_tileState[n].cmdptr = 0;
                executeLocal(reinterpret_cast<const Command*>(cmd.address), n, false);
                m_tileState[n].cmdptr = cmdptr;
            }
            break;
            case Command::SetFlags: m_tileState[n].flags = cmd.intparam; break;
            case Command::SetDepthFunc: m_tileState[n].depthFunc = cmd.intparam; break;
            case Command::SetAlpha: m_tileState[n].alpha = cmd.intparam; break;
            case Command::SetPolygonMode: m_tileState[n].polygonMode = static_cast<PolygonMode>(cmd.intparam); break;
            case Command::SetConstantBuffer: m_tileState[n].shaderData = cmd.address; break;
            case Command::SetVertexBuffer: m_tileState[n].vertexBuffer.data = cmd.vbuffer; m_tileState[n].vertexBuffer.stride = cmd.stride; break;
            case Command::SetIndexBuffer: m_tileState[n].indexBuffer = reinterpret_cast<const IndexType*>(cmd.address); break;
            case Command::SetShader: 
                m_tileState[n].vertexShader = cmd.vertexShader;
                m_tileState[n].pixelShader = cmd.pixelShader;
                m_tileState[n].attribSets = cmd.attribSets;
            break;
            case Command::SetComputeShader: m_tileState[n].computeShader = reinterpret_cast<ComputeShaderPtr>(cmd.address); break;
            case Command::ClearColor: clearColor(n, cmd.r, cmd.g, cmd.b); break;
            case Command::ClearDepth: clearDepth(n, cmd.floatparam); break;
            case Command::Draw: draw(n, cmd.num, cmd.offset); break;
            case Command::DrawIndexed: drawIndexed(n, cmd.num, cmd.offset, cmd.voffset); break;
            case Command::Dispatch: 
                if (cmd.param2 != 0)
                    dispatch(n, cmd.param1, cmd.param2);
                else
                    dispatch(n, cmd.param1 == 0 ? ThreadNum : cmd.param1);
            break;
            default: break;
        }
    }
}

void Pipeline::execute(const CommandList& commandList)
{
    m_commands = *commandList;
    size_t cmdptr = 0;

    for (size_t cmdptr = 0; ;cmdptr++)
    {
        const Command& cmd = m_commands[cmdptr];

        if (cmd.type == Command::SetComputeShader)
        {
            for (size_t n = 0; n < ThreadNum; n++)
            {
                m_tileState[n].computeShader = reinterpret_cast<ComputeShaderPtr>(cmd.address);
                m_tileState[n].cmdptr = cmdptr;
            }

            m_executeBarrier.arrive_and_wait();
            executeLocal(m_commands, 0);
            m_finishBarrier.arrive_and_wait();

            return;
        }
        
        if (cmd.type == Command::SetFrameBuffer)
        {
            bindFrameBuffer(cmd.colorBuffer, cmd.depthBuffer);

            for (size_t n = 0; n < ThreadNum; n++) m_tileState[n].cmdptr = cmdptr;

            m_executeBarrier.arrive_and_wait();
            executeLocal(m_commands, 0);
            m_finishBarrier.arrive_and_wait();

            if (m_tileState[0].flags & DepthUnpack) unpackDepth();

            return;
        }

        if (cmd.type == Command::Finish) return;
    }
}

} // namespace Render