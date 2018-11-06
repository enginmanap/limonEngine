//
// Created by Engin Manap on 8.03.2016.
//

#ifndef LIMONENGINE_GLMCONVERTER_H
#define LIMONENGINE_GLMCONVERTER_H

#include <btBulletDynamicsCommon.h>
#include "../glm/gtx/quaternion.hpp"
#include "../glm/glm.hpp"
#include "../GamePlay/LimonAPI.h"


#include <assimp/vector3.h>
#include <assimp/types.h>
#include <assimp/quaternion.h>


class GLMConverter {
public:

    static LimonAPI::Vec4 GLMToLimon(const glm::vec4 &vector) {
        return LimonAPI::Vec4(vector.x, vector.y, vector.z, vector.w);
    }

    static LimonAPI::Vec4 GLMToLimon(const glm::vec3 &vector) {
        return LimonAPI::Vec4(vector.x, vector.y, vector.z);
    }

    static glm::vec4 LimonToGLM(const LimonAPI::Vec4 &vector) {
        return glm::vec4(vector.x, vector.y, vector.z, vector.w);
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

    static glm::vec3 BltToGLM(const btVector3 &vector) {
        return glm::vec3(vector.getX(), vector.getY(), vector.getZ());
    }

    static btVector3 GLMToBlt(const glm::vec3 &vector) {
        return btVector3(vector.x, vector.y, vector.z);
    }

    static glm::quat BltToGLM(const btQuaternion &quaternion) {
        return glm::quat(quaternion.getW(), quaternion.getX(), quaternion.getY(), quaternion.getZ());
    }

    static btQuaternion GLMToBlt(const glm::quat &quaternion) {
        return btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
    }

    static glm::vec3 AssimpToGLM(const aiVector3D &vector) {
        return glm::vec3(vector.x, vector.y, vector.z);
    }

    static glm::quat AssimpToGLM(const aiQuaternion &quaternion) {
        return glm::quat(quaternion.w, quaternion.x, quaternion.y, quaternion.z);

    }

    static glm::vec3 AssimpToGLM(const aiColor3D &color) {
        return glm::vec3(color.r, color.g, color.b);
    }

    static glm::mat4 AssimpToGLM(const aiMatrix4x4 &matrix) {
        //the documentation says Assimp is row major while GLM and OpenGL is colomn major,
        //but access to elements are the same, so the following is correct.
        return glm::mat4(
                matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
                matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]
        );
    }


    static aiVector3D GLMToAssimp(const glm::vec3 &vector) {
        return aiVector3D(vector.x, vector.y, vector.z);
    }

    static btVector3 AssimpToBullet(const aiVector3D &vector) {
        return btVector3(vector.x, vector.y, vector.z);
    }
};


#endif //LIMONENGINE_GLMCONVERTER_H
