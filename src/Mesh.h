#pragma once

#include "Geometry.h"

template<class T>
class Mesh
{
public:
    Mesh() = default;

    void reset(std::vector<T>& vertices, std::vector<Render::IndexType>& indices)
    {
        m_vertices = std::move(vertices);
        m_indices = std::move(indices);

        m_vertexBuffer.data = reinterpret_cast<const uint8_t*>(m_vertices.data());
        m_vertexBuffer.stride = sizeof(T);
        m_indexBuffer = m_indices.data();
    }

    uint32_t size() const { return m_indices.size(); }

    const Render::VertexBufferView& vertexBuffer() const { return m_vertexBuffer; }
    const Render::IndexBufferView& indexBuffer() const { return m_indexBuffer; }

private:
    std::vector<T> m_vertices;
    std::vector<Render::IndexType> m_indices;

    Render::VertexBufferView m_vertexBuffer;
    Render::IndexBufferView m_indexBuffer;
};