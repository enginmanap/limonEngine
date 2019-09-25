//
// Created by Engin Manap on 3.03.2016.
//

#ifndef LIMONENGINE_RENDERABLE_H
#define LIMONENGINE_RENDERABLE_H


#include "GameObjects/GameObject.h"
#include "Graphics/GraphicsInterface.h"
#include "Graphics/GLSLProgram.h"
#include "Transformation.h"
#include <btBulletDynamicsCommon.h>
#include <glm/gtx/matrix_decompose.hpp>

class Renderable {
protected:
    Transformation transformation;
    std::vector<uint_fast32_t > bufferObjects;
    std::vector<bool> inLightFrustum;
    uint_fast32_t vao, ebo;
    GraphicsInterface *glHelper;
    bool isInCameraFrustum = true;
    bool dirtyForFrustum = true;//is this object require a frustum recalculate
    bool customAnimation = false;

    explicit Renderable(GraphicsInterface *glHelper) :
            glHelper(glHelper) {
        this->inLightFrustum.resize(NR_TOTAL_LIGHTS);
    }

public:

    virtual void renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram) = 0;

    virtual void setupForTime(long time) = 0;

    virtual ~Renderable() {
        for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
            glHelper->freeBuffer(bufferObjects[i]);
        }
        glHelper->freeBuffer(ebo);
        glHelper->freeVAO(vao);

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

    void setCustomAnimation(bool customAnimation) {
        this->customAnimation = customAnimation;
    }

    bool getCustomAnimation() {
        return customAnimation;
    }
};


#endif //LIMONENGINE_RENDERABLE_H
