//
// Created by Engin Manap on 3.03.2016.
//

#ifndef UBERGAME_RENDERABLE_H
#define UBERGAME_RENDERABLE_H


#include "GLHelper.h"
#include "GLSLProgram.h"

class Renderable {
protected:
    GLHelper* glHelper;
    GLuint vao, vbo, ebo;
    GLSLProgram* renderProgram;
    std::vector<std::string> uniforms;
    glm::mat4 worldTransform;



    Renderable(GLHelper* glHelper);
public:
    void setWorldTransform(const glm::mat4 &worldTransform) {
        this->worldTransform = worldTransform;
    }

    const glm::mat4 &getWorldTransform() const {
        return worldTransform;
    }

    virtual void render() = 0;

};


#endif //UBERGAME_RENDERABLE_H
