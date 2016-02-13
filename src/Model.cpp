//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"


Model::Model(GLHelper* glHelper):
        glHelper(glHelper)
{

    // A single triangle
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

    // Color for each vertex
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

    // Indices for the triangle strips
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

    glHelper->bufferVertexData(vertex_positions, vertex_colors, sizeof(vertex_positions),
                     vertex_indices, sizeof(vertex_indices),
                     vao, vbo, ebo);
    worldTransform = glm::translate(glm::mat4(1.0f),glm::vec3(0.5f, 0.3f, -3.0f));
}

void Model::render() {

    glHelper->render(vao, ebo, worldTransform);
}