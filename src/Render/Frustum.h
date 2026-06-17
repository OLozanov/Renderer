#pragma once

#include <glm/glm.hpp>

#include "Render/Camera.h"
#include "BBox.h"

namespace Render
{

class Frustum
{
public:
    Frustum() = default;

    void update(const glm::mat4& mat);
    bool test(const glm::vec3& pos, const glm::vec3& bbox) const;
    bool test(const BBox& bbox) const;

private:
    glm::vec4 m_planes[6];
};

} // namespace Render
