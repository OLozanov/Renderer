#define GLM_FORCE_SWIZZLE GLM_SWIZZLE_XYZW
#include "Frustum.h"

namespace Render
{

void Frustum::update(const glm::mat4& mat)
{
	// left
	m_planes[0].x = mat[0].w + mat[0].x;
	m_planes[0].y = mat[1].w + mat[1].x;
	m_planes[0].z = mat[2].w + mat[2].x;
	m_planes[0].w = mat[3].w + mat[3].x;

	// right
	m_planes[1].x = mat[0].w - mat[0].x;
	m_planes[1].y = mat[1].w - mat[1].x;
	m_planes[1].z = mat[2].w - mat[2].x;
	m_planes[1].w = mat[3].w - mat[3].x;

	// top
	m_planes[2].x = mat[0].w - mat[0].y;
	m_planes[2].y = mat[1].w - mat[1].y;
	m_planes[2].z = mat[2].w - mat[2].y;
	m_planes[2].w = mat[3].w - mat[3].y;
	
	// bottom
	m_planes[3].x = mat[0].w + mat[0].y;
	m_planes[3].y = mat[1].w + mat[1].y;
	m_planes[3].z = mat[2].w + mat[2].y;
	m_planes[3].w = mat[3].w + mat[3].y;
	
	// near
	m_planes[4].x = mat[0].z;
	m_planes[4].y = mat[1].z;
	m_planes[4].z = mat[2].z;
	m_planes[4].w = mat[3].z;
	
	// far
	m_planes[5].x = mat[0].w - mat[0].z;
	m_planes[5].y = mat[1].w - mat[1].z;
	m_planes[5].z = mat[2].w - mat[2].z;
	m_planes[5].w = mat[3].w - mat[3].z;

	for (size_t i = 0; i < 6; i++)
	{
		float rlen = 1.0f / sqrt(glm::dot(m_planes[i].xyz(), m_planes[i].xyz()));
		m_planes[i] *= rlen;
	}
}

bool Frustum::test(const glm::vec3& pos, const glm::vec3& bbox) const
{
    for (size_t i = 0; i < 4; i++)
    {
		float r = fabs(m_planes[i].x) * bbox.x + fabs(m_planes[i].y) * bbox.y + fabs(m_planes[i].z) * bbox.z;
		float dist = glm::dot(m_planes[i].xyz(), pos) + m_planes[i].w;
    
        if (dist < -r) return false;
    }

    return true;
}

bool Frustum::test(const BBox& bbox) const
{
	glm::vec3 pos = (bbox.min + bbox.max) * 0.5f;
	glm::vec3 extent = bbox.max - pos;

	return test(pos, extent);
}

} // namespace Render