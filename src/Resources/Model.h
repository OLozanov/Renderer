#pragma once

#include <glm/glm.hpp>
#include "Scene.h"

#include <string>
#include <vector>

class ObjLoader
{
public:
    explicit ObjLoader(Scene& scene, float scale = 1.0f) 
    : m_scene(scene)
    , m_scale(scale)
    {
    }

    void load(const std::string& filename);

private:
    struct TangentSpace
    {
        glm::vec3 normal = { 0.0f, 0.0f, 0.0f };
        glm::vec3 tangent = { 0.0f, 0.0f, 0.0f };
        glm::vec3 binormal = { 0.0f, 0.0f, 0.0f };
        uint32_t shared = 0;
    };

    using VertexKey = std::tuple<uint32_t, uint32_t, uint32_t>; // position id / texcoord id / normal id

private:
    uint32_t materialId(const std::string& matname);
    void loadMaterials(const std::string& matlib);


    static void CalculateTangentSpace(const glm::vec3& a,
                                      const glm::vec3& b,
                                      const glm::vec3& c,
                                      const glm::vec2& ta,
                                      const glm::vec2& tb,
                                      const glm::vec2& tc,
                                      TangentSpace& tspace);

    void smoothTSpace(uint32_t smgroup);
    void mergeFlatTSpace();
    void mergeVertices();

    BBox calculateBBox(uint32_t offset, uint32_t vnum);

private:
    Scene& m_scene;
    float m_scale;

    std::vector<Vertex> m_vertices;
    std::vector<Render::IndexType> m_indices;
    std::vector<uint32_t> m_faces;

    std::vector<glm::vec3> m_pos;
    std::vector<glm::vec2> m_tcoord;
    std::vector<glm::vec3> m_norms;
    std::vector<TangentSpace> m_tspace;
    std::vector<TangentSpace> m_facetang;

    std::vector<VertexKey> m_vertidx;
    std::vector<uint32_t> m_facesm;

    std::vector<Material> m_materials;
    std::unordered_map<std::string, uint32_t> m_materialMap;
};