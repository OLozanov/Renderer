#include "Scene.h"

Scene::Scene()
{
}

void Scene::setGeometryData(std::vector<Vertex>& vertices, std::vector<Render::IndexType>& indices,
                            std::vector<uint32_t>& faceData)
{
    m_vertices = std::move(vertices);
    m_indices = std::move(indices);
    m_faceData = std::move(faceData);

    m_vertexBuffer.data = reinterpret_cast<const uint8_t*>(m_vertices.data());
    m_vertexBuffer.stride = sizeof(Vertex);
    m_indexBuffer = m_indices.data();
}

void Scene::setMaterials(std::vector<Material>& materials)
{
    m_materials = std::move(materials);
}

void Scene::addNode(const SceneNode& node)
{
    m_nodes.push_back(node);
}