#pragma once

#include <glm/glm.hpp>

template<class T>
T square(const T& val) { return val * val; }

struct BBox
{
    glm::vec3 min;
    glm::vec3 max;

    bool intersectsSphere(glm::vec3 center, float r) const
    {
        float r2 = r * r;
        float dmin = 0;
        
        for (size_t i = 0; i < 3; i++) 
        {
            if (center[i] < min[i]) dmin += square(center[i] - min[i]);
            else if (center[i] > max[i]) dmin += square(center[i] - max[i]);
        }

        return dmin <= r2;
    }
};