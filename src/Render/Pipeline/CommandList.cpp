#include "CommandList.h"

namespace Render
{

void CommandList::begin()
{
    m_commands.clear();
    m_flags = 0;
}

void CommandList::finish()
{
    m_commands.emplace_back(Command::Finish);
}

void CommandList::enable(uint32_t flags) 
{ 
    m_flags |= flags;
    m_commands.emplace_back(Command::SetFlags, m_flags);
}

void CommandList::disable(uint32_t flags) 
{ 
    m_flags &= ~flags;
    m_commands.emplace_back(Command::SetFlags, m_flags);
}

void CommandList::setDepthFunc(uint32_t compareOp) 
{ 
    m_commands.emplace_back(Command::SetDepthFunc, compareOp);
}

void CommandList::setAlpha(float value)
{
    m_commands.emplace_back(Command::SetAlpha, static_cast<uint32_t>(value * 255.0f));
}

void CommandList::setPolygonMode(PolygonMode mode)
{
    m_commands.emplace_back(Command::SetPolygonMode, static_cast<uint32_t>(mode));
}

void CommandList::bindConstantBuffer(uint8_t* data)
{
    m_commands.emplace_back(Command::SetConstantBuffer, data);
}

void CommandList::bindFrameBuffer(ColorBuffer* color, DepthBuffer* depth)
{
    m_commands.emplace_back(Command::SetFrameBuffer, color, depth);
}

void CommandList::bindVertexBuffer(const VertexBufferView& buffer)
{
    m_commands.emplace_back(Command::SetVertexBuffer, (uint8_t*)buffer.data, buffer.stride);
}

void CommandList::bindIndexBuffer(const IndexType* indexBuffer)
{
    m_commands.emplace_back(Command::SetIndexBuffer, (uint8_t*)indexBuffer);
}

void CommandList::bindShader(const Shader& shader)
{
    uint32_t attribSets = (shader.attribnum + 7) / 8;
    m_commands.emplace_back(Command::SetShader, shader.vertexShaderPtr, shader.pixelShaderPtr, attribSets);
}

void CommandList::bindComputeShader(ComputeShaderPtr shader)
{
    m_commands.emplace_back(Command::SetComputeShader, (uint8_t*)shader);
}

void CommandList::clearColor(float r, float g, float b)
{
    uint8_t ir = r * 255.0f;
    uint8_t ig = g * 255.0f;
    uint8_t ib = b * 255.0f;

    m_commands.emplace_back(Command::ClearColor, ir, ig, ib);
}

void CommandList::clearDepth(float val)
{
    m_commands.emplace_back(Command::ClearDepth, val);
}

void CommandList::draw(uint32_t num, uint32_t offset)
{
    m_commands.emplace_back(Command::Draw, num, offset, uint32_t(0));
}

void CommandList::drawIndexed(uint32_t num, uint32_t offset, uint32_t vertexOffset)
{
    m_commands.emplace_back(Command::DrawIndexed, num, offset, vertexOffset);
}

void CommandList::dispatch(uint32_t x, uint32_t y)
{
    m_commands.emplace_back(Command::Dispatch, x, y);
}

void CommandList::execute(const CommandList& commandList)
{
    m_commands.emplace_back(Command::Execute, (uint8_t*)*commandList);
}

} // namespace Render