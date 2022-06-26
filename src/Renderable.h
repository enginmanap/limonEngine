//
// Created by Engin Manap on 3.03.2016.
//

#ifndef LIMONENGINE_RENDERABLE_H
#define LIMONENGINE_RENDERABLE_H


#include "GameObjects/GameObject.h"
#include "API/Graphics/GraphicsInterface.h"
#include "API/Graphics/GraphicsProgram.h"
#include "Transformation.h"
#include <btBulletDynamicsCommon.h>
#include <glm/gtx/matrix_decompose.hpp>

class Renderable {
protected:
    Transformation transformation;
    std::vector<uint32_t> bufferObjects;
    std::vector<bool> inLightFrustum;
    uint32_t vao = 0, ebo = 0;
    GraphicsInterface* graphicsWrapper;
    bool isInCameraFrustum = true;
    bool dirtyForFrustum = true;//is this object require a frustum recalculate
    bool customAnimation = false;

    explicit Renderable(GraphicsInterface* graphicsWrapper) :
            graphicsWrapper(graphicsWrapper) {
        this->inLightFrustum.resize(NR_TOTAL_LIGHTS);
    }

public:

    virtual void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel) = 0;

    virtual void setupForTime(long time) = 0;

    virtual ~Renderable() {
        for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
            graphicsWrapper->freeBuffer(bufferObjects[i]);
        }
        graphicsWrapper->freeBuffer(ebo);
        graphicsWrapper->freeVAO(vao);

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

    bool getCustomAnimation() const {
        return customAnimation;
    }
};


#endif //LIMONENGINE_RENDERABLE_H
