#pragma once

#include "Render/Pipeline/Surface.h"
#include "Render/Pipeline/Image.h"
#include <glm/glm.hpp>
#include <thread>
#include <barrier>

#include <immintrin.h>

namespace Render
{

using IndexType = uint32_t;

enum class PolygonMode
{
    Fill,
    Line,
    Point
};

struct VertexBufferView
{
    size_t stride = 0;
    const uint8_t* data = nullptr;
};

using IndexBufferView = const IndexType*;

using VertexShaderPtr = void (*)(const uint8_t* data, const uint8_t* in, float* out) noexcept;
using PixelShaderPtr = void (* vectorcall)(const uint8_t* data,
                                           uint32_t pid, uint32_t tx, uint32_t ty,
                                           const float* attrib_a, const float* attrib_b, const float* attrib_c, 
                                           __m256 u, __m256 v, __m256 w, __m256 rz,
                                           __m256& r, __m256& g, __m256& b, __m256& a) noexcept;

using ComputeShaderPtr = void(*)(const uint8_t* data, uint32_t x, uint32_t y) noexcept;

struct Shader
{
    VertexShaderPtr vertexShaderPtr;
    PixelShaderPtr pixelShaderPtr;
    uint32_t attribnum;

    Shader(VertexShaderPtr vertexPtr, PixelShaderPtr pixelPtr = nullptr, uint32_t attribnum = 4)
    : vertexShaderPtr(vertexPtr)
    , pixelShaderPtr(pixelPtr)
    , attribnum(attribnum)
    {
    }
};

struct Command;
class CommandList;

class Pipeline
{
public:
    enum Flags : uint32_t
    {
        CullFace = 1,
        DepthTest = 2,
        DepthWrite = 4,
        DepthUnpack = 8,
        ColorWrite = 16,
        AlphaTest = 32,
        Blend = 64
    };

    enum DepthFunc : uint32_t
    {
        Less = 0,
        LessOrEqual = 1,
        EqualOne = 2        // if depth buffer value is 1.0, for background drawing
    };

public:
    Pipeline();
    ~Pipeline();

    void line(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    void bitblt(Image* image, int32_t x, int32_t y, int32_t width, int32_t height);

    void execute(const CommandList& commandList);

private:
    struct TriangleData
    {
        uint32_t idx;   // primitive id
        uint32_t flags;

        float az;
        float bz;
        float cz;

        const float* attrib_a;
        const float* attrib_b;
        const float* attrib_c;

        float weights[3];

        // edge functions
        glm::vec2 e1;
        glm::vec2 e2;

        // edge function in-block fragments offsets
        __m256 te1;
        __m256 te2;
    };

    struct BlockData // 4x2 pixel blocks
    {
        TriangleData& triangle;

        uint32_t x;
        uint32_t y;

        float d1;
        float d2;
    };

    struct TileState
    {
        glm::vec4 clipPlanes[6];
        int32_t xmin, xmax;
        int32_t ymin, ymax;

        uint32_t tx, ty;

        size_t cmdptr;

        uint32_t flags;
        uint32_t depthFunc;
        uint32_t alpha;

        PolygonMode polygonMode;

        VertexBufferView vertexBuffer;
        IndexBufferView indexBuffer;

        VertexShaderPtr vertexShader;
        PixelShaderPtr pixelShader;
        ComputeShaderPtr computeShader;

        uint32_t attribSets = 1;

        uint8_t* shaderData;
    };

    void initThreads();

    static bool ClipPolygon(const glm::vec4& plane, 
                            float* in, float* out,
                            size_t& vnum,
                            uint32_t attribSets) noexcept;

    void linex(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    void liney(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

    void bitblt_rgb8(Image* image, int32_t x, int32_t y, int32_t width, int32_t height);
    void bitblt_f32(Image* image, int32_t x, int32_t y, int32_t width, int32_t height);

    void bindFrameBuffer(ColorBuffer* surface, DepthBuffer* depth = nullptr);

    void clearColor(size_t n, uint8_t r, uint8_t g, uint8_t b) noexcept;
    void clearDepth(size_t n, float val = 1.0f) noexcept;

    void unpackDepth() noexcept;

    void draw(size_t n, size_t num, size_t offset = 0) noexcept;
    void drawIndexed(size_t n, size_t num, size_t offset = 0, size_t vertexOffset = 0) noexcept;

    void drawWirePolygon(const glm::vec2* scpos, size_t num);

    void drawBlock(size_t n, const BlockData& tile) noexcept;

    void drawTriangle(size_t n,
                      uint32_t idx,
                      const float* attrib,
                      const glm::vec2* scpos,
                      size_t ai, size_t bi, size_t ci,
                      size_t attribnum) noexcept;

    void dispatch(size_t n, uint32_t x, uint32_t y);
    void dispatch(size_t n, uint32_t num);

    void executeLocal(const Command* commands, size_t n, bool reset = true);

    void executeLoop(uint32_t n);

private:
    static constexpr float eps = 0.0001f;

    static constexpr uint32_t XTiles = 4;
    static constexpr uint32_t YTiles = 3;
    static constexpr size_t ThreadNum = XTiles * YTiles;
    
    // Clip planes with guard band
    // But somehow individual tile clip planes still faster
    /*static constexpr glm::vec4 ClipPlanes[6] = { {0.0f, 0.0f, 1.0f, 0.0f},
                                                 {0.0f, 0.0f, -1.0f, 1.0f},
                                                 {1.0f, 0.0f, 0.0f, 6.0f},
                                                 {-1.0f, 0.0f, 0.0f, 6.0f},
                                                 {0.0f, 1.0f, 0.0f, 6.0f},
                                                 {0.0f, -1.0f, 0.0f, 6.0f} };*/

    static constexpr size_t MaxClippedVerts = 9; // 3 original verts + 6 potential new ones from 6 clip planes

    static constexpr size_t VertexCacheSize = 1024;

    ColorBuffer* m_colorBuffer;
    DepthBuffer* m_depthBuffer;

    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_xblocks;
    uint32_t m_yblocks;

    const Command* m_commands;

    TileState m_tileState[ThreadNum];

    std::barrier<> m_executeBarrier;
    std::barrier<> m_finishBarrier;

    bool m_terminate = false;
};

} // namespace Render
