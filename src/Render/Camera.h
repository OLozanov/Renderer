#pragma once

#include <glm/glm.hpp>

namespace Render
{

class Camera
{
private:
    float m_vrot;
    float m_hrot;

    glm::vec3 m_pos;

    glm::mat4 m_rotMat;
    glm::mat4 m_viewMat;

public:
    Camera();
    Camera(float x, float y, float z);

    glm::vec3 direction() const;

    float verticalAngle() const { return m_vrot; }
    float horyzontalAngle() const { return m_hrot; }

    void setAngles(float v, float h)
    {
        m_vrot = v;
        m_hrot = h;
    }

    const glm::mat4 & rotation() const { return m_rotMat; }
    const glm::mat4 & transform() const { return m_viewMat; }

    const glm::vec3 & pos() const { return m_pos; }
    void setPos(const glm::vec3 & pos) { m_pos = pos; }
    void move(const glm::vec3 & dir) { m_pos += dir; }

    void rotate(float v, float h);

    void update();
};

} // namespace Render