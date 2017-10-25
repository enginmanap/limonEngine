//
// Created by engin on 17.10.2017.
//

#ifndef UBERGAME_GLMUTILS_H
#define UBERGAME_GLMUTILS_H

#include <glm/glm.hpp>
#include <iostream>

class GLMUtils {
public:
    static void printMatrix(const glm::mat4 matrix) {
        std::cout << matrix[0][0] << ", " << matrix[1][0] << ", " << matrix[2][0] << ", " << matrix[3][0] << "\n"
                  << matrix[0][1] << ", " << matrix[1][1] << ", " << matrix[2][1] << ", " << matrix[3][1] << "\n"
                  << matrix[0][2] << ", " << matrix[1][2] << ", " << matrix[2][2] << ", " << matrix[3][2] << "\n"
                  << matrix[0][3] << ", " << matrix[1][3] << ", " << matrix[2][3] << ", " << matrix[3][3] << std::endl;
    }
};


#endif //UBERGAME_GLMUTILS_H
