//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_MODEL_H
#define UBERGAME_MODEL_H


#include <vector>

//TODO maybe we should not have direct dependency to glm and gl
#include "glm/glm.hpp"
#include "Renderable.h"
#include "Texture.h"



class Model :public Renderable {
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;
    Texture* texture;

    std::vector<glm::vec4> colors;
public:
    Model(GLHelper* glHelper) : Model(glHelper,0) {};
    Model(GLHelper*, float mass);
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
