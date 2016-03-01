//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_MODEL_H
#define UBERGAME_MODEL_H

//TODO maybe we should not have direct dependency to glm and gl
#include "glm/glm.hpp"
#include "GLHelper.h"

#include <GL/gl.h>

class Model {
    GLHelper* glHelper;
    GLuint vao, vbo, ebo;
    glm::mat4 worldTransform;


public:
    const glm::mat4 &getWorldTransform() const {
        return worldTransform;
    }

    void setWorldTransform(const glm::mat4 &worldTransform) {
        this->worldTransform = worldTransform;
    }

    Model(GLHelper*);
    void render(GLHelper*);

};
#endif //UBERGAME_MODEL_H
