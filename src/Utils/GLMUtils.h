//
// Created by engin on 17.10.2017.
//

#ifndef LIMONENGINE_GLMUTILS_H
#define LIMONENGINE_GLMUTILS_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

class GLMUtils {
public:

    static float maxMatrixDifference(const glm::mat4& matrix1, const glm::mat4& matrix2) {
        float max = 0;

        max = std::max(std::fabs(matrix1[0][0] - matrix2[0][0]), max); max = std::max(std::fabs(matrix1[1][0] - matrix2[1][0]), max);    max = std::max(std::fabs(matrix1[2][0] - matrix2[2][0]), max); max = std::max(std::fabs(matrix1[3][0] - matrix2[3][0]), max);
        max = std::max(std::fabs(matrix1[0][1] - matrix2[0][1]), max); max = std::max(std::fabs(matrix1[1][1] - matrix2[1][1]), max);    max = std::max(std::fabs(matrix1[2][1] - matrix2[2][1]), max); max = std::max(std::fabs(matrix1[3][1] - matrix2[3][1]), max);
        max = std::max(std::fabs(matrix1[0][2] - matrix2[0][2]), max); max = std::max(std::fabs(matrix1[1][2] - matrix2[1][2]), max);    max = std::max(std::fabs(matrix1[2][2] - matrix2[2][2]), max); max = std::max(std::fabs(matrix1[3][2] - matrix2[3][2]), max);
        max = std::max(std::fabs(matrix1[0][3] - matrix2[0][3]), max); max = std::max(std::fabs(matrix1[1][3] - matrix2[1][3]), max);    max = std::max(std::fabs(matrix1[2][3] - matrix2[2][3]), max); max = std::max(std::fabs(matrix1[3][3] - matrix2[3][3]), max);
        return max;
    }

    static void printMatrix(const glm::mat4& matrix) {
        std::cout << matrix[0][0] << ", " << matrix[1][0] << ", " << matrix[2][0] << ", " << matrix[3][0] << "\n"
                  << matrix[0][1] << ", " << matrix[1][1] << ", " << matrix[2][1] << ", " << matrix[3][1] << "\n"
                  << matrix[0][2] << ", " << matrix[1][2] << ", " << matrix[2][2] << ", " << matrix[3][2] << "\n"
                  << matrix[0][3] << ", " << matrix[1][3] << ", " << matrix[2][3] << ", " << matrix[3][3] << std::endl;
    }

    static void printVector(const glm::vec3 &vector) {
        std::cout << " (" << vector[0] << ", " << vector[1] << ", " << vector[2] << ") " << std::endl;
    }

    static std::string vectorToString(const glm::vec3 &vector) {
        return " (" + std::to_string(vector[0]) + ", " + std::to_string(vector[1]) + ", " + std::to_string(vector[2]) + ") ";
    }

    static std::string vectorToString(const glm::vec4 &vector) {
        return " (" + std::to_string(vector[0]) + ", " + std::to_string(vector[1]) + ", " + std::to_string(vector[2]) +", " + std::to_string(vector[3]) + ") ";
    }

    static std::string vectorToString(const glm::uvec4 &vector) {
        return " (" + std::to_string(vector[0]) + ", " + std::to_string(vector[1]) + ", " + std::to_string(vector[2]) +", " + std::to_string(vector[3]) + ") ";
    }

    static std::string vectorToString(const glm::quat &vector) {
        return " (" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ", " + std::to_string(vector.z) + ", " + std::to_string(vector.w) + ") ";
    }

    static std::string matrixToString(const glm::mat4& matrix) {
        return " ("  + std::to_string(matrix[0][0]) + ", " + std::to_string(matrix[1][0]) + ", " + std::to_string(matrix[2][0]) + ", " + std::to_string(matrix[3][0]) + "\n"
                     + std::to_string(matrix[0][1]) + ", " + std::to_string(matrix[1][1]) + ", " + std::to_string(matrix[2][1]) + ", " + std::to_string(matrix[3][1]) + "\n"
                     + std::to_string(matrix[0][2]) + ", " + std::to_string(matrix[1][2]) + ", " + std::to_string(matrix[2][2]) + ", " + std::to_string(matrix[3][2]) + "\n"
                     + std::to_string(matrix[0][3]) + ", " + std::to_string(matrix[1][3]) + ", " + std::to_string(matrix[2][3]) + ", " + std::to_string(matrix[3][3]) + ") ";
    }

    static std::string vectorOfVectorToString(const std::vector<glm::uvec4> &vector){
        std::string result = "\"";
        for (size_t i = 0; i < vector.size(); ++i) {
            result += vectorToString(vector[i]);
        }
        result += "\"";
        return result;

    }
};


#endif //LIMONENGINE_GLMUTILS_H
