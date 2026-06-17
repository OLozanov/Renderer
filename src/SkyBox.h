#pragma once

#include "Render/Render.h"
#include "Resources/Resources.h"
#include "Geometry.h"
#include <string>
#include <memory>

class SkyBox
{
public:
    SkyBox(const std::string& name);
    ~SkyBox();

    const Render::VertexBufferView& vertexBuffer() const { return m_vertexBuffer; }
    Render::IndexBufferView indexBuffer() const { return m_indices; }

    const Render::Image* face(size_t n) const { return m_faces[n]; }

    static constexpr uint32_t VertexNum = 36;

private:
    SimpleVertex m_vertices[24];
    Render::IndexType m_indices[36];

    Render::VertexBufferView m_vertexBuffer;

    Render::Image* m_faces[6];
};
