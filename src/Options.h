//
// Created by engin on 5.11.2017.
//

#ifndef LIMONENGINE_OPTIONS_H
#define LIMONENGINE_OPTIONS_H

#include <glm/glm.hpp>
#include "Utils/Logger.h"

class Options {
public:
    enum MoveModes {WALK, RUN};
private:
    Logger *logger{};

    glm::vec3 walkSpeed = glm::vec3(5, 0, 5);
    glm::vec3 runSpeed = glm::vec3(8, 0, 8);
    glm::vec3 moveSpeed = walkSpeed;
    float jumpFactor = 7.0f;
    float lookAroundSpeed = -2.5f;
    unsigned int screenHeight = 480;
    unsigned int screenWidth = 640;

    uint32_t shadowWidth = 2048;
    uint32_t shadowHeight = 2048; //TODO these values should be parameters
    float lightOrthogonalProjectionNearPlane = 1.0f;
    float  lightOrthogonalProjectionFarPlane = 100.0f;
    glm::vec4 lightOrthogonalProjectionValues = glm::vec4(-100.0f, 100.0f, -100.0f, 100.0f);
    glm::vec3 lightPerspectiveProjectionValues = glm::vec3((float)shadowWidth/(float)shadowHeight, 1.0f, 100.0f);
    //aspect,near,far

    uint32_t debugDrawBufferSize = 1000;

public:
    uint32_t getDebugDrawBufferSize() const {
        return debugDrawBufferSize;
    }

    void setDebugDrawBufferSize(uint32_t debugDrawBufferSize) {
        //we must resize the buffer for debug draw lines
        //Options::debugDrawBufferSize = debugDrawBufferSize;
        std::cerr << "Setting debugDrawBufferSize(" << debugDrawBufferSize << ") is not implemented." << std::endl;
    }

    uint32_t getShadowWidth() const {
        return shadowWidth;
    }

    void setShadowWidth(uint32_t shadowWidth) {
        Options::shadowWidth = shadowWidth;
    }

    uint32_t getShadowHeight() const {
        return shadowHeight;
    }

    void setShadowHeight(uint32_t shadowHeight) {
        Options::shadowHeight = shadowHeight;
    }

    float getLightOrthogonalProjectionNearPlane() const {
        return lightOrthogonalProjectionNearPlane;
    }

    void setLightOrthogonalProjectionNearPlane(float lightOrthogonalProjectionNearPlane) {
        Options::lightOrthogonalProjectionNearPlane = lightOrthogonalProjectionNearPlane;
    }

    float getLightOrthogonalProjectionFarPlane() const {
        return lightOrthogonalProjectionFarPlane;
    }

    void setLightOrthogonalProjectionFarPlane(float lightOrthogonalProjectionFarPlane) {
        Options::lightOrthogonalProjectionFarPlane = lightOrthogonalProjectionFarPlane;
    }

    const glm::vec4 &getLightOrthogonalProjectionValues() const {
        return lightOrthogonalProjectionValues;
    }

    void setLightOrthogonalProjectionValues(const glm::vec4 &lightOrthogonalProjectionValues) {
        Options::lightOrthogonalProjectionValues = lightOrthogonalProjectionValues;
    }

    const glm::vec3 &getLightPerspectiveProjectionValues() const {
        return lightPerspectiveProjectionValues;
    }

    void setLightPerspectiveProjectionValues(const glm::vec3 &lightPerspectiveProjectionValues) {
        Options::lightPerspectiveProjectionValues = lightPerspectiveProjectionValues;
    }

    unsigned int getScreenHeight() const {
        return screenHeight;
    }

    void setScreenHeight(unsigned int height) {
        Options::screenHeight = height;
    }

    unsigned int getScreenWidth() const {
        return screenWidth;
    }

    void setScreenWidth(unsigned int width) {
        Options::screenWidth = width;
    }

    const glm::vec3 &getMoveSpeed() const {
        return moveSpeed;
    }

    float getJumpFactor() const {
        return jumpFactor;
    }

    float getLookAroundSpeed() const {
        return lookAroundSpeed;
    }

    void setMoveSpeed(const MoveModes moveMode) {
        switch(moveMode) {
            case RUN:
                moveSpeed = runSpeed;
                break;
            case WALK:
            default:
                moveSpeed = walkSpeed;

        }
    }

    void setJumpFactor(float jumpFactor) {
        Options::jumpFactor = jumpFactor;
    }

    void setLookAroundSpeed(float lookAroundSpeed) {
        this->lookAroundSpeed = lookAroundSpeed;
        logger->log(Logger::SETTINGS, Logger::DEBUG, "Look around speed set to " + std::to_string(lookAroundSpeed));
    }

    Options() {
        this->logger = new Logger();
    };

    Logger* getLogger() {
        return logger;
    }
};


#endif //LIMONENGINE_OPTIONS_H
