#pragma once

#include "Render/Pipeline/Pipeline.h"
#include <cstdint>
#include <vector>

namespace Render
{

struct Command
{
    enum Type : uint32_t
    {
        Execute = 0,
        SetFlags = 1,
        SetDepthFunc = 2,
        SetAlpha = 3,
        SetParameter = 4,
        SetConstantBuffer = 5,
        SetFrameBuffer = 6,
        SetVertexBuffer = 7,
        SetIndexBuffer = 8,
        SetShader = 9,
        SetComputeShader = 10,
        SetPolygonMode = 11,
        ClearColor = 12,
        ClearDepth = 13,
        Draw = 14,
        DrawIndexed = 15,
        Dispatch = 16,
        Finish = 17
    };

    uint32_t type;

    union
    {
        uint8_t* address;
        uint32_t intparam;
        float floatparam;

        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };

        struct // draw call params
        {
            uint32_t num;
            uint32_t offset;
            uint32_t voffset;
        };

        struct // vertex buffer view
        {
            uint8_t* vbuffer;
            uint32_t stride;
        };

        struct // framebuffer
        {
            ColorBuffer* colorBuffer;
            DepthBuffer* depthBuffer;
        };

        struct // shader
        {
            VertexShaderPtr vertexShader;
            PixelShaderPtr pixelShader;
            uint32_t attribSets;
        };

        struct
        {
            uint32_t param1;
            uint32_t param2;
        };
    };

    Command(uint32_t type) : type(type) {}
    Command(uint32_t type, uint8_t* addr) : type(type), address(addr) {}
    Command(uint32_t type, uint32_t param) : type(type), intparam(param) {}
    Command(uint32_t type, float param) : type(type), floatparam(param) {}
    Command(uint32_t type, uint8_t r, uint8_t g, uint8_t b) : type(type), r(r), g(g), b(b) {}
    Command(uint32_t type, ColorBuffer* color, DepthBuffer* depth) : type(type), colorBuffer(color), depthBuffer(depth) {}
    Command(uint32_t type, VertexShaderPtr vertexShader, PixelShaderPtr pixelShader, uint32_t attribSets) 
    : type(type)
    , vertexShader(vertexShader)
    , pixelShader(pixelShader)
    , attribSets(attribSets) 
    {
    }
    Command(uint32_t type, uint8_t* buffer, uint32_t stride) : type(type), vbuffer(buffer), stride(stride) {}
    Command(uint32_t type, uint32_t param1, uint32_t param2) : type(type), param1(param1), param2(param2) {}
    Command(uint32_t type, uint32_t num, uint32_t offset, uint32_t voffset) 
    : type(type)
    , num(num)
    , offset(offset)
    , voffset(voffset) 
    {
    }
};

class CommandList
{
public:
    CommandList() = default;

    void begin();
    void finish();

    void enable(uint32_t flags);
    void disable(uint32_t flags);

    void setDepthFunc(uint32_t compareOp);
    void setAlpha(float value);
    void setPolygonMode(PolygonMode mode);

    void bindConstantBuffer(uint8_t* data);
    void bindFrameBuffer(ColorBuffer* color, DepthBuffer* depth = nullptr);
    void bindVertexBuffer(const VertexBufferView& vertexBuffer);
    void bindIndexBuffer(const IndexType* indexBuffer);
    void bindShader(const Shader& shader);
    void bindComputeShader(ComputeShaderPtr shader);

    void clearColor(float r, float g, float b);
    void clearDepth(float val = 1.0f);

    void draw(uint32_t num, uint32_t offset = 0);
    void drawIndexed(uint32_t num, uint32_t offset = 0, uint32_t vertexOffset = 0);

    void dispatch(uint32_t x, uint32_t y);

    void execute(const CommandList& commandList);

    Command* operator*() { return m_commands.data(); }
    const Command* operator*() const { return m_commands.data(); }

private:
    std::vector<Command> m_commands;

    uint32_t m_flags;
};

} // namespace Render