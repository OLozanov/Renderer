#pragma once

#include "Render/Render.h"
#include "Geometry.h"
#include <vector>
#include <unordered_map>
#include <string>

struct SceneNode
{
    BBox bbox;
    uint32_t offset;
    uint32_t num;
};

class Scene
{
public:
    Scene();

    void setGeometryData(std::vector<Vertex>& vertices, std::vector<Render::IndexType>& indices, 
                         std::vector<uint32_t>& faceData);

    void setMaterials(std::vector<Material>& materials);

    void addNode(const SceneNode& node);

    const Render::VertexBufferView& vertexBuffer() const { return m_vertexBuffer; }
    const Render::IndexBufferView& indexBuffer() const { return m_indexBuffer; }

    const uint32_t* faces() const { return m_faceData.data(); }
    const std::vector<Material>& materials() const { return m_materials; }

    const std::vector<SceneNode>& nodes() const { return m_nodes; }

private:
    std::vector<Vertex> m_vertices;
    std::vector<Render::IndexType> m_indices;
    std::vector<uint32_t> m_faceData;

    std::vector<Material> m_materials;
    std::vector<SceneNode> m_nodes;

    Render::VertexBufferView m_vertexBuffer;
    Render::IndexBufferView m_indexBuffer;
};