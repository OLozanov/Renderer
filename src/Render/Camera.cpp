#include "Camera.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Render
{

Camera::Camera()
: m_pos()
, m_vrot(0)
, m_hrot(0)
{
}

Camera::Camera(float x, float y, float z)
: m_pos(x, y, z)
, m_vrot(0)
, m_hrot(0)
{
}

glm::vec3 Camera::direction() const
{
    float hCos = cos(m_hrot*glm::pi<float>()/180.0f);
    float hSin = sin(m_hrot*glm::pi<float>()/180.0f);

    float vCos = cos((m_vrot+90.0f)*glm::pi<float>()/180.0f);
    float vSin = sin((m_vrot+90.0f)*glm::pi<float>()/180.0f);

    return glm::vec3(-hSin*vSin, -vCos, hCos*vSin);
}

void Camera::rotate(float v, float h)
{
    m_vrot -= v;
    if(m_vrot > 90) m_vrot = 90;
    if(m_vrot < -90) m_vrot = -90;

    m_hrot -= h;
    if(m_hrot > 360) m_hrot -= 360;
    if(m_hrot < 0) m_hrot += 360;
}

void Camera::update()
{
    m_rotMat = glm::rotate(glm::mat4(1.0f), m_vrot / 180.0f * glm::pi<float>(), { 1.0f, 0.0f, 0.0f });
    m_rotMat = glm::rotate(m_rotMat, m_hrot / 180.0f * glm::pi<float>(), { 0.0f, 1.0f, 0.0f });
    m_viewMat = glm::translate(m_rotMat, {-m_pos.x, -m_pos.y, -m_pos.z});
}

} // namespace Render