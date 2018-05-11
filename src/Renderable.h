//
// Created by Engin Manap on 3.03.2016.
//

#ifndef LIMONENGINE_RENDERABLE_H
#define LIMONENGINE_RENDERABLE_H


#include "GameObjects/GameObject.h"
#include "GLHelper.h"
#include "GLSLProgram.h"
#include "Transformation.h"
#include <btBulletDynamicsCommon.h>
#include <glm/gtx/matrix_decompose.hpp>

class Renderable {
protected:
    Transformation transformation;
    std::vector<uint_fast32_t > bufferObjects;
    uint_fast32_t vao, ebo;
    GLHelper *glHelper;
    GLSLProgram *renderProgram;
    bool isInFrustum = true;

    explicit Renderable(GLHelper *glHelper) :
            glHelper(glHelper), renderProgram(nullptr) {}
public:

    virtual void render() = 0;

    virtual void setupForTime(long time) = 0;

    virtual ~Renderable() {
        for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
            glHelper->freeBuffer(bufferObjects[i]);
        }
        glHelper->freeBuffer(ebo);
        glHelper->freeVAO(vao);

        //model renderable creates its own
        delete renderProgram;

    }

    bool isIsInFrustum() const {
        return isInFrustum;
    }

    void setIsInFrustum(bool isInFrustum) {
        Renderable::isInFrustum = isInFrustum;
    }

    Transformation* getTransformation() {
        return &transformation;
    }

};


#endif //LIMONENGINE_RENDERABLE_H
