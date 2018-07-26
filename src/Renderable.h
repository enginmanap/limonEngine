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
    std::vector<bool> inLightFrustum;
    uint_fast32_t vao, ebo;
    GLHelper *glHelper;
    GLSLProgram *renderProgram = nullptr;
    bool isInCameraFrustum = true;
    bool dirtyForFrustum = true;//is this object require a frustum recalculate


    explicit Renderable(GLHelper *glHelper) :
            glHelper(glHelper) {
        this->inLightFrustum.reserve(4);//FIXME 4 is current light max, it will require update
    }

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
        return isInCameraFrustum;
    }

    void setIsInFrustum(bool isInFrustum) {
        Renderable::isInCameraFrustum = isInFrustum;
    }

    bool isInLightFrustum(uint32_t lightIndex) const {
        assert(lightIndex < 4);
        return Renderable::inLightFrustum[lightIndex];
    }

    void setIsInLightFrustum(uint32_t lightIndex, bool isInFrustum) {
        assert(lightIndex < 4);
        Renderable::inLightFrustum[lightIndex] = isInFrustum;
    }

    bool isDirtyForFrustum() {
        return this->dirtyForFrustum;
    }

    void setCleanForFrustum() {
        this->dirtyForFrustum = false;
    }

    Transformation* getTransformation() {
        return &transformation;
    }

};


#endif //LIMONENGINE_RENDERABLE_H
