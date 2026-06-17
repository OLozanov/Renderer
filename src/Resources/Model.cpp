#include "Resources/Resources.h"

#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <tuple>
#include <map>
#include <set>
#include <stdint.h>

struct TSpaceKey
{
    uint32_t posid;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 binormal;

    bool operator==(const TSpaceKey& key) const
    {
        static constexpr float threshold = 1.0f - 0.0001f;

        return key.posid == posid &&
               glm::dot(key.normal, normal) > threshold &&
               glm::dot(key.tangent, tangent) > threshold &&
               glm::dot(key.binormal, binormal) > threshold;
    }
};

namespace std {
    template <>
    struct hash<TSpaceKey> 
    {
        std::size_t operator()(const TSpaceKey& key) const noexcept 
        {
            return std::hash<uint32_t>{}(key.posid);
        }
    };
}

Render::Image* LoadImage(const std::string& filename)
{
    size_t extpos = filename.find_last_of('.');
    std::string extension = extpos != filename.npos ? filename.substr(++extpos) : "";

    Render::Image* image = nullptr;

    if (extension == "bmp") image = LoadBmp(filename.c_str());
    if (extension == "tga") image = LoadTga(filename.c_str());

    return image;
}

void ObjLoader::CalculateTangentSpace(const glm::vec3& a,
                                      const glm::vec3& b,
                                      const glm::vec3& c,
                                      const glm::vec2& ta,
                                      const glm::vec2& tb,
                                      const glm::vec2& tc,
                                      TangentSpace& tspace)
{
    static constexpr float eps = 0.0001f;

    glm::vec3 l1 = b - a;
    glm::vec3 l2 = c - a;

    glm::vec2 tl1 = tb - ta;
    glm::vec2 tl2 = tc - ta;

    float det = tl1.x * tl2.y - tl1.y * tl2.x;

    glm::vec3 s, t;

    if (fabs(det) < eps)
    {
        s = l1;
        t = l2;
    }
    else
    {
        float sx = (tl2.y * l1.x - tl1.y * l2.x) / det;
        float sy = (tl2.y * l1.y - tl1.y * l2.y) / det;
        float sz = (tl2.y * l1.z - tl1.y * l2.z) / det;

        float tx = (-tl2.x * l1.x + tl1.x * l2.x) / det;
        float ty = (-tl2.x * l1.y + tl1.x * l2.y) / det;
        float tz = (-tl2.x * l1.z + tl1.x * l2.z) / det;

        s = { sx, sy, sz };
        t = { tx, ty, tz };
    }

    tspace.tangent = glm::normalize(s);
    tspace.binormal = glm::normalize(-t);

    tspace.normal = glm::normalize(glm::cross(l1, l2));
}

void ObjLoader::smoothTSpace(uint32_t smgroup)
{
    std::unordered_map<uint32_t, uint32_t> tspaceMap;

    for (uint32_t i = 0; i < m_vertidx.size(); i++)
    {
        uint32_t p = std::get<0>(m_vertidx[i]);
        uint32_t face = i / 3;

        if (m_facesm[face] != smgroup) continue;

        auto it = tspaceMap.find(p);

        if (it == tspaceMap.end())
        {
            tspaceMap[p] = m_tspace.size();
            m_tspace.emplace_back();
        }

        uint32_t tsid = tspaceMap[p];
        std::get<2>(m_vertidx[i]) = tsid;

        m_tspace[tsid].normal += m_facetang[face].normal;
        m_tspace[tsid].tangent += m_facetang[face].tangent;
        m_tspace[tsid].binormal += m_facetang[face].binormal;
        m_tspace[tsid].shared++;
    }

    for (auto tsindex : tspaceMap)
    {
        uint32_t tsid = tsindex.second;

        float frac = m_tspace[tsid].shared;

        m_tspace[tsid].normal *= frac;
        m_tspace[tsid].tangent *= frac;
        m_tspace[tsid].binormal *= frac;
    }
}

void ObjLoader::mergeFlatTSpace()
{
    std::unordered_map<TSpaceKey, uint32_t> tspaceMap;

    for (uint32_t i = 0; i < m_vertidx.size(); i++)
    {
        uint32_t p = std::get<0>(m_vertidx[i]);
        uint32_t face = i / 3;

        if (m_facesm[face] != 0) continue;

        TSpaceKey key = { p, m_facetang[face].normal, m_facetang[face].tangent, m_facetang[face].binormal };
        auto it = tspaceMap.find(key);

        uint32_t tsid;

        if (it == tspaceMap.end())
        {
            tsid = m_tspace.size();

            tspaceMap[key] = tsid;
            m_tspace.emplace_back();

            m_tspace[tsid].normal = key.normal;
            m_tspace[tsid].tangent = key.tangent;
            m_tspace[tsid].binormal = key.binormal;
        }
        else
            tsid = it->second;

        std::get<2>(m_vertidx[i]) = tsid;
    }
}

void ObjLoader::mergeVertices()
{
    std::map<VertexKey, Render::IndexType> vertexmap;

    auto getvertex = [&](const VertexKey& vkey) -> Render::IndexType
    {
        auto it = vertexmap.find(vkey);

        if (it != vertexmap.end()) return it->second;

        Render::IndexType newidx = m_vertices.size();

        auto [vid, tid, nid] = vkey;
        m_vertices.push_back({ m_pos[vid], m_tcoord[tid], m_tspace[nid].normal, m_tspace[nid].tangent, m_tspace[nid].binormal});

        vertexmap[vkey] = newidx;

        return newidx;
    };

    for (size_t i = 0; i < m_vertidx.size(); i++)
    {
        Render::IndexType idx = getvertex(m_vertidx[i]);
        m_indices.push_back(idx);
    }
}

BBox ObjLoader::calculateBBox(uint32_t offset, uint32_t vnum)
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::infinity());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::infinity());

    for (size_t i = offset; i < offset + vnum; i++)
    {
        uint32_t vid = std::get<0>(m_vertidx[i]);
        const glm::vec3& vert = m_pos[vid];

        min.x = std::min(min.x, vert.x);
        min.y = std::min(min.y, vert.y);
        min.z = std::min(min.z, vert.z);

        max.x = std::max(max.x, vert.x);
        max.y = std::max(max.y, vert.y);
        max.z = std::max(max.z, vert.z);
    }

    return { min, max };
}

uint32_t ObjLoader::materialId(const std::string& matname)
{
    auto it = m_materialMap.find(matname);

    if (it != m_materialMap.end()) return it->second;

    uint32_t newmat = m_materials.size();
    m_materials.push_back({});

    m_materialMap[matname] = newmat;

    return newmat;
}

void ObjLoader::loadMaterials(const std::string& matlib)
{
    std::ifstream file(matlib);

    if (!file.is_open())
    {
        std::cout << "Can't open file " << matlib << std::endl;
        return;
    }

    std::cout << "Loading materials " << matlib << " ..." << std::endl;

    uint32_t matid = 0;

    while (!file.eof())
    {
        std::string str;
        file >> str;

        if (str[0] == '#')
        {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (str == "newmtl")
        {
            std::string matname;
            file >> matname;

            matid = materialId(matname);

            continue;
        }

        if (str == "map_Kd")
        {
            std::string mapname;
            file >> mapname;

            Material& mat = m_materials[matid];
            mat.diffuseMap = LoadImage(mapname);

            continue;
        }

        if (str == "map_Disp")
        {
            std::string mapname;
            file >> mapname;

            Material& mat = m_materials[matid];
            mat.normalMap = LoadImage(mapname.c_str());

            continue;
        }

        if (str == "Ns")
        {
            float specularExp;
            file >> specularExp;

            Material& mat = m_materials[matid];
            mat.sexponent = specularExp;

            continue;
        }

        if (str == "Ks")
        {
            float sr, sg, sb;
            file >> sr;
            file >> sg;
            file >> sb;

            Material& mat = m_materials[matid];
            mat.sfactor = sr * 255.0f;

            continue;
        }

        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    file.close();
}

void ObjLoader::load(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cout << "Can't open file " << filename << std::endl;
        return;
    }

    std::cout << "Loading scene " << filename << " ..." << std::endl;

    std::string mtlib;

    std::set<uint32_t> smoothGroups;

    uint32_t smgroup = 0;
    uint32_t material = 0;

    uint32_t offset = 0;

    auto addmesh = [&]()
    {
        if (m_vertidx.empty()) return;

        uint32_t vnum = m_vertidx.size() - offset;
        BBox bbox = calculateBBox(offset, vnum);

        m_scene.addNode({ bbox, offset, vnum });

        offset = m_vertidx.size();
    };

    while (!file.eof())
    {
        std::string str;
        file >> str;

        if (str[0] == '#')
        {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (str == "mtllib")
        {
            file >> mtlib;
            continue;
        }

        if (str == "o")
        {
            std::string objname;
            file >> objname;

            addmesh();

            continue;
        }

        if (str == "v")
        {
            float x, y, z;

            file >> x;
            file >> y;
            file >> z;

            m_pos.push_back(glm::vec3(x, y, z) * m_scale);

            continue;
        }

        if (str == "vt")
        {
            float u, v;

            file >> u;
            file >> v;

            m_tcoord.push_back({ u, v });

            continue;
        }

        if (str == "vn")
        {
            float x, y, z;

            file >> x;
            file >> y;
            file >> z;

            m_norms.push_back({ x, y, z });

            continue;
        }

        if (str == "s")
        {
            file >> str;

            if (str == "off") smgroup = 0;
            else smgroup = std::stoi(str);

            smoothGroups.insert(smgroup);

            continue;
        }

        if (str == "usemtl")
        {
            std::string matname;
            file >> matname;

            material = materialId(matname);

            continue;
        }

        if (str == "f")
        {
            for (size_t i = 0; i < 3; i++)
            {
                uint32_t v, t, n;
                uint8_t delim;

                file >> v;
                file >> delim;
                file >> t;
                file >> delim;
                file >> n;

                m_vertidx.push_back({ v - 1, t - 1, n - 1 });
            }

            m_faces.push_back(material);
            m_facesm.push_back(smgroup);

            continue;
        }
    }

    addmesh();

    m_facetang.resize(m_faces.size());

    for (uint32_t i = 0; i < m_faces.size(); i++)
    {
        auto [p1, t1, n1] = m_vertidx[i * 3];
        auto [p2, t2, n2] = m_vertidx[i * 3 + 1];
        auto [p3, t3, n3] = m_vertidx[i * 3 + 2];

        CalculateTangentSpace(m_pos[p1], m_pos[p2], m_pos[p3],
                              m_tcoord[t1], m_tcoord[t2], m_tcoord[t3],
                              m_facetang[i]);
    }

    for (uint32_t smgroup : smoothGroups)
    {
        if (smgroup == 0)
            mergeFlatTSpace();
        else
            smoothTSpace(smgroup);
    }

    mergeVertices();

    m_scene.setGeometryData(m_vertices, m_indices, m_faces);

    file.close();

    if (!mtlib.empty())
    {
        std::string path("models/");
        loadMaterials(path + mtlib);
    }

    m_scene.setMaterials(m_materials);
}