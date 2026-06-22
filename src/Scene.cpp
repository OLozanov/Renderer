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

    uint32_t vsize = m_vertices.size();
    if (vsize % 2) vsize++;

    uint32_t batch = (vsize + 11) / 12;

    m_outVertices.resize(vsize);

    for (size_t i = 0; i < m_vertices.size(); i++)
    {
        m_outVertices[i].position = glm::vec4(m_vertices[i].position, 1.0f);
        m_outVertices[i].uv = m_vertices[i].uv;
        m_outVertices[i].normal = m_vertices[i].normal;
        m_outVertices[i].tangent = m_vertices[i].tangent;
        m_outVertices[i].binormal = m_vertices[i].binormal;
        m_outVertices[i].realpos = m_vertices[i].position;
    }

    m_transformData.num = vsize;
    m_transformData.batch = batch;

    m_transformData.in = m_vertices.data();
    m_transformData.out = m_outVertices.data();

    m_vertexBuffer.data = reinterpret_cast<const uint8_t*>(m_outVertices.data());
    m_vertexBuffer.stride = sizeof(OutVertex);
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