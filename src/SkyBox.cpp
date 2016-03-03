//
// Created by Engin Manap on 1.03.2016.
//

#include "SkyBox.h"

SkyBox::SkyBox(GLHelper* glHelper, std::string right, std::string left, std::string top, std::string down, std::string back,
               std::string front):
    Renderable(glHelper)
    {
    cubeMap = new CubeMap(glHelper, ".\\Data\\Textures\\Skyboxes\\ThickCloudsWater",
                          "right.jpg", "left.jpg",
                          "top.jpg", "bottom.jpg",
                          "back.jpg", "front.jpg");

    static const GLfloat vertex_positions[] = {
            -1.0f, -1.0f, -1.0f, 1.0f,
             1.0f, -1.0f, -1.0f, 1.0f,
             1.0f,  1.0f, -1.0f, 1.0f,
            -1.0f,  1.0f, -1.0f, 1.0f,

            -1.0f, -1.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 1.0f, 1.0f,
    };

    static const GLfloat vertex_texture_coords[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,

            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,

    };

    static const GLuint vertex_indices[] = {
            //front
            0, 1, 2,
            0, 2, 3,

            //Back
            4, 7, 6,
            4, 6, 5,
            //right
            4, 0, 3,
            4, 3, 7,

            //left
            5, 6, 2,
            5, 2, 1,

            //down
            4, 1, 0,
            4, 5, 1,

            //up
            3, 6, 7,
            3, 2, 6,
    };


    glHelper->bufferVertexData(vertex_positions, sizeof(vertex_positions),
                               vertex_indices, sizeof(vertex_indices),
                               vao, vbo, 2, ebo);
//    glHelper->bufferVertexTextureCoordinates(vertex_texture_coords, sizeof(vertex_texture_coords),
//                                vao, vbo, 2, ebo);
    uniforms.push_back("cameraTransformMatrix");
    renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/SkyCube/vertex.shader","./Data/Shaders/SkyCube/fragment.shader",uniforms);
}

void SkyBox::render(){
    glHelper->attachCubeMap(cubeMap->getID());
    glm::mat4 viewMatrix = glHelper->getProjectionMatrix() * glm::mat4(glm::mat3(glHelper->getCameraMatrix()));
    GLuint location;
    if(renderProgram->getUniformLocation("cameraTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, viewMatrix);
        glHelper->render(renderProgram->getID(), vao, ebo, 36);
    }
}
