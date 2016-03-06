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
    glm::mat4 worldTransform, oldWorldTransform;
    glm::vec3 scale, translate;
    glm::quat orientation;
    bool isDirty;

    void generateWorldTransform() ;


    Renderable(GLHelper* glHelper);
public:
    void addScale(const glm::vec3& scale){
        this->scale *= scale;
        isDirty=true;
    }
    void addTranslate(const glm::vec3& translate){
        this->translate += translate;
        isDirty=true;
    }
    void addOrientation(const glm::quat& orientation){
        this->orientation *= orientation;
        this->orientation = glm::normalize(this->orientation);
        isDirty=true;
    }

    const glm::mat4 &getWorldTransform() {
        if(isDirty) {
            generateWorldTransform();
        }
        return worldTransform;
    }

    virtual void render() = 0;

};


#endif //UBERGAME_RENDERABLE_H
