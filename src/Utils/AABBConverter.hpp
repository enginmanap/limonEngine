//
// Created by engin on 04/03/2026.
//

#ifndef LIMONENGINE_AABBSCREENSPACECONVERTER_HPP
#define LIMONENGINE_AABBSCREENSPACECONVERTER_HPP
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>

struct AABBConverter {
    struct AABB {
        glm::vec3 min, max;
        bool isValid = false; // Helps handle cases where everything is behind the camera
    };

    static AABB GetScreenSpaceAABB(const glm::vec3& min, const glm::vec3& max,
                                 const glm::mat4& viewProj) {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };

        glm::vec3 finalMin(std::numeric_limits<float>::max());
        glm::vec3 finalMax(std::numeric_limits<float>::lowest());
        bool anyVisible = false;

        for (int i = 0; i < 8; ++i) {
            glm::vec4 clipPos = viewProj * glm::vec4(corners[i], 1.0f);

            // Simple clipping handling:
            // If w <= 0, the point is behind the camera.
            // In a full implementation, you would perform line-plane intersection.
            // For standard AABB usage, we clamp w to a small epsilon to prevent projection flips.
            if (clipPos.w <= 0.0001f) {
                clipPos.w = 0.0001f;
            }

            glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

            finalMin = glm::min(finalMin, ndc);
            finalMax = glm::max(finalMax, ndc);
            anyVisible = true;
        }
        finalMin = glm::max(finalMin, glm::vec3(-1.0, -1.0, -1.0));
        finalMax = glm::min(finalMax, glm::vec3(1.0, 1.0, 1.0));

        return {finalMin, finalMax, anyVisible};
    }
};


#endif //LIMONENGINE_AABBSCREENSPACECONVERTER_HPP