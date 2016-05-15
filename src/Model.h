//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_MODEL_H
#define UBERGAME_MODEL_H


#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Utils/AssimpUtils.h"

//TODO maybe we should not have direct dependency to glm and gl
#include "glm/glm.hpp"
#include "PhysicalRenderable.h"
#include "Texture.h"



class Model :public PhysicalRenderable {

    std::string modelFile;
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;
    Texture* texture;

    std::vector<glm::vec4> colors;
public:
    Model(GLHelper* glHelper, const std::string& modelFile) : Model(glHelper, 0, modelFile) {};
    Model(GLHelper*, const float mass, const std::string& modelFile);
    void render();


    //TODO we need to free the texture. Destructor needed.
    ~Model() {
        delete texture;

        delete rigidBody->getMotionState();
        delete rigidBody->getCollisionShape();
        delete rigidBody;

    }
};
#endif //UBERGAME_MODEL_H
