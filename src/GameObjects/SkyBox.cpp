//
// Created by Engin Manap on 1.03.2016.
//

#include "SkyBox.h"


SkyBox::SkyBox(uint32_t objectID, AssetManager *assetManager, std::string path, std::string right, std::string left,
               std::string top, std::string down, std::string back, std::string front) :
        Renderable(assetManager->getGlHelper()),
        objectID(objectID),
        assetManager(assetManager),
        path(path), right(right), left(left), top(top), down(down), back(back), front(front){
    cubeMap = assetManager->loadAsset<CubeMapAsset>({path,
                                                     right, left,
                                                     top, down,
                                                     back, front});

    vertices.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3(1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3(1.0f, 1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f, 1.0f, -1.0f));

    vertices.push_back(glm::vec3(-1.0f, -1.0f, 1.0f));
    vertices.push_back(glm::vec3(1.0f, -1.0f, 1.0f));
    vertices.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    vertices.push_back(glm::vec3(-1.0f, 1.0f, 1.0f));

    //front
    faces.push_back(glm::mediump_uvec3(0, 1, 2));
    faces.push_back(glm::mediump_uvec3(0, 2, 3));
    //Back
    faces.push_back(glm::mediump_uvec3(4, 7, 6));
    faces.push_back(glm::mediump_uvec3(4, 6, 5));
    //right
    faces.push_back(glm::mediump_uvec3(4, 0, 3));
    faces.push_back(glm::mediump_uvec3(4, 3, 7));
    //left
    faces.push_back(glm::mediump_uvec3(5, 6, 2));
    faces.push_back(glm::mediump_uvec3(5, 2, 1));
    //down
    faces.push_back(glm::mediump_uvec3(4, 1, 0));
    faces.push_back(glm::mediump_uvec3(4, 5, 1));
    //up
    faces.push_back(glm::mediump_uvec3(3, 6, 7));
    faces.push_back(glm::mediump_uvec3(3, 2, 6));


    uint_fast32_t vbo;
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    renderProgram = glHelper->createGLSLProgram("./Engine/Shaders/SkyCube/vertex.glsl",
                                    "./Engine/Shaders/SkyCube/fragment.glsl", false);
}

void SkyBox::render() {
    int texturePoint = 1;

    glHelper->attachCubeMap(cubeMap->getID(), texturePoint);
    //this is because we want to remove translate component from cameraMatrix.
    glm::mat4 viewMatrix = glHelper->getProjectionMatrix() * glm::mat4(glm::mat3(glHelper->getCameraMatrix()));
    if (renderProgram->setUniform("cubeSampler", texturePoint)) {
        if (renderProgram->setUniform("cameraTransformMatrix", viewMatrix)) {
            glHelper->render(renderProgram->getID(), vao, ebo, faces.size() * 3);
        } else {
            std::cerr << "Uniform \"cameraTransformMatrix\" could not be set, passing rendering." << std::endl;
        }
    } else {
        std::cerr << "Uniform \"cubeSampler\" could not be set, passing rendering." << std::endl;
    }
}

const std::string &SkyBox::getPath() const {
    return path;
}

const std::string &SkyBox::getRight() const {
    return right;
}

const std::string &SkyBox::getLeft() const {
    return left;
}

const std::string &SkyBox::getTop() const {
    return top;
}

const std::string &SkyBox::getDown() const {
    return down;
}

const std::string &SkyBox::getBack() const {
    return back;
}

const std::string &SkyBox::getFront() const {
    return front;
}
