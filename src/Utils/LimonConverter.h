//
// Created by Engin Manap on 16.11.2018.
//

#ifndef LIMONENGINE_LIMONCONVERTER_H
#define LIMONENGINE_LIMONCONVERTER_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../GamePlay/LimonAPI.h"

class LimonConverter {
public:

    static LimonAPI::Vec4 GLMToLimon(const glm::vec4 &vector) {
        return LimonAPI::Vec4(vector.x, vector.y, vector.z, vector.w);
    }

    static LimonAPI::Vec4 GLMToLimon(const glm::vec3 &vector) {
        return LimonAPI::Vec4(vector.x, vector.y, vector.z);
    }

    static LimonAPI::Vec2 GLMToLimon(const glm::vec2 &vector) {
        return LimonAPI::Vec2(vector.x, vector.y);
    }

    static glm::vec4 LimonToGLM(const LimonAPI::Vec4 &vector) {
        return glm::vec4(vector.x, vector.y, vector.z, vector.w);
    }

    static glm::vec2 LimonToGLM(const LimonAPI::Vec2 &vector) {
        return glm::vec2(vector.x, vector.y);
    }

    static LimonAPI::Mat4 GLMToLimon(const glm::mat4 &matrix) {
        return LimonAPI::Mat4(GLMToLimon(matrix[0]),
                              GLMToLimon(matrix[1]),
                              GLMToLimon(matrix[2]),
                              GLMToLimon(matrix[3]));
    }

    static glm::mat4 LimonToGLM (const LimonAPI::Mat4 &matrix) {
        return glm::mat4(LimonToGLM(matrix[0]),
                         LimonToGLM(matrix[1]),
                         LimonToGLM(matrix[2]),
                         LimonToGLM(matrix[3]));
    }
};


#endif //LIMONENGINE_LIMONCONVERTER_H
