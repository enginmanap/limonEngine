//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"


Model::Model(GLHelper* glHelper):
    glHelper(glHelper){
    static const GLfloat vertex_positions[] = {
            +0.5f, +0.5f,  0.5f, 1.0f,
            -0.5f, -0.5f,  0.5f, 1.0f,
            -0.5f, +0.5f, -0.5f, 1.0f,
            +0.5f, -0.5f, -0.5f, 1.0f,

            -0.5f, -0.5f, -0.5f, 1.0f,
            +0.5f, +0.5f, -0.5f, 1.0f,
            +0.5f, -0.5f,  0.5f, 1.0f,
            -0.5f, +0.5f,  0.5f, 1.0f,
    };

    static const GLfloat vertex_colors[] = {
            0.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,

            0.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,
    };

    static const GLuint vertex_indices[] = {
            1, 0, 2,
            0, 1, 3,
            3, 2, 0,
            2, 3, 1,

            4, 5, 6,
            5, 4, 7,
            6, 7, 4,
            7, 6, 5,
    };

    glHelper->bufferVertexData(vertex_positions, sizeof(vertex_positions), vertex_indices, sizeof(vertex_indices),
    vao,vbo,0,ebo);

    glHelper->bufferVertexColor(vertex_colors, sizeof(vertex_colors),vao,vbo,1);
    worldTransform = glm::mat4(1.0f);
}

void Model::render(GLHelper* glHelper) {
    glHelper->render(vao, ebo, worldTransform);
}