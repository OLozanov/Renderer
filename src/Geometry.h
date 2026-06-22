#pragma once

#include "Render/Pipeline/Image.h"
#include "Render/BBox.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct SimpleVertex
{
	glm::vec3 position;
	glm::vec2 uv;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec2 uv;

    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 binormal;
};

struct OutVertex
{
    glm::vec4 position;
    glm::vec2 uv;

    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 binormal;

    glm::vec3 realpos;
};

struct Material
{
    Render::Image* diffuseMap = nullptr;
    Render::Image* normalMap = nullptr;
    float sfactor = 0;
    float sexponent = 0;
};