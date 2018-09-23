//
// Created by engin on 5.11.2017.
//

#ifndef LIMONENGINE_OPTIONS_H
#define LIMONENGINE_OPTIONS_H

#include <glm/glm.hpp>
#include <iostream>
#include "Utils/Logger.h"
#include <tinyxml2.h>
class Options {
public:

    static constexpr float PI = 3.14159265358979f;
    static constexpr float PI_DOUBLE = 3.141592653589793238463;
    enum MoveModes {WALK, RUN};
    enum class TextureFilteringModes { NEAREST, BILINEAR, TRILINEAR };
private:
    Logger *logger{};

    glm::vec3 walkSpeed = glm::vec3(8, 0, 8);
    glm::vec3 runSpeed = glm::vec3(12, 0, 12);
    glm::vec3& moveSpeed = walkSpeed;
    glm::vec3 freeMovementSpeed = glm::vec3(0.1f,0.1f,0.1f);
    float jumpFactor = 7.0f;
    float lookAroundSpeed = -6.5f;
    uint32_t screenHeight = 1080;
    uint32_t screenWidth = 1920;

    uint32_t shadowMapDirectionalWidth = 2048;
    uint32_t shadowMapDirectionalHeight = 2048; //TODO these values should be parameters
    uint32_t shadowMapPointWidth = 512;
    uint32_t shadowMapPointHeight = 512; //TODO these values should be parameters
    float lightOrthogonalProjectionNearPlane = 1.0f;
    float  lightOrthogonalProjectionFarPlane = 100.0f;
    glm::vec4 lightOrthogonalProjectionValues = glm::vec4(-100.0f, 100.0f, -100.0f, 100.0f);
    float lightPerspectiveProjectionNearPlane = 1.0f;
    float  lightPerspectiveProjectionFarPlane = 100.0f;
    glm::vec3 lightPerspectiveProjectionValues = glm::vec3((float)shadowMapPointWidth/(float)shadowMapPointHeight, lightPerspectiveProjectionNearPlane, lightPerspectiveProjectionFarPlane);
    //aspect,near,far

    uint32_t debugDrawBufferSize = 1000;

    /*SDL properties that should be available */
    void* imeWindowHandle;
    int drawableWidth, drawableHeight;
    int windowWidth, windowHeight;
    bool isWindowInFocus;
    TextureFilteringModes currentTextureFilteringMode = TextureFilteringModes::TRILINEAR;

    bool fullScreen = false;

    void loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3&);
    void loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4&);
public:

    bool loadOptions(const std::string& optionsFileName);

    void *getImeWindowHandle() const {
        return imeWindowHandle;
    }

    void setImeWindowHandle(void *imeWindowHandle) {
        Options::imeWindowHandle = imeWindowHandle;
    }

    int getDrawableWidth() const {
        return drawableWidth;
    }

    void setDrawableWidth(int drawableWidth) {
        Options::drawableWidth = drawableWidth;
    }

    int getDrawableHeight() const {
        return drawableHeight;
    }

    void setDrawableHeight(int drawableHeight) {
        Options::drawableHeight = drawableHeight;
    }

    int getWindowWidth() const {
        return windowWidth;
    }

    void setWindowWidth(int windowWidth) {
        Options::windowWidth = windowWidth;
    }

    int getWindowHeight() const {
        return windowHeight;
    }

    void setWindowHeight(int windowHeight) {
        Options::windowHeight = windowHeight;
    }

    bool isIsWindowInFocus() const {
        return isWindowInFocus;
    }

    void setIsWindowInFocus(bool isWindowInFocus) {
        Options::isWindowInFocus = isWindowInFocus;
    }

    uint32_t getDebugDrawBufferSize() const {
        return debugDrawBufferSize;
    }

    void setDebugDrawBufferSize(uint32_t debugDrawBufferSize) {
        //we must resize the buffer for debug draw lines
        //Options::debugDrawBufferSize = debugDrawBufferSize;
        std::cerr << "Setting debugDrawBufferSize(" << debugDrawBufferSize << ") is not implemented." << std::endl;
    }

    uint32_t getShadowMapDirectionalWidth() const {
        return shadowMapDirectionalWidth;
    }

    uint32_t getShadowMapDirectionalHeight() const {
        return shadowMapDirectionalHeight;
    }

    uint32_t getShadowMapPointWidth() const {
        return shadowMapPointWidth;
    }

    uint32_t getShadowMapPointHeight() const {
        return shadowMapPointHeight;
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

    const uint32_t& getScreenHeight() const {
        return screenHeight;
    }

    void setScreenHeight(unsigned int height) {
        Options::screenHeight = height;
    }

    const uint32_t& getScreenWidth() const {
        return screenWidth;
    }

    void setScreenWidth(unsigned int width) {
        Options::screenWidth = width;
    }

    const glm::vec3 &getMoveSpeed() const {
        return moveSpeed;
    }

    const glm::vec3 &getFreeMovementSpeed() const {
        return freeMovementSpeed;
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
        logger->log(Logger::log_Subsystem_SETTINGS, Logger::log_level_DEBUG, "Look around speed set to " + std::to_string(lookAroundSpeed));
    }

    Options() {
        this->logger = new Logger();
    };

    Logger* getLogger() {
        return logger;
    }

    TextureFilteringModes getTextureFiltering() {
        return currentTextureFilteringMode;
    }

    bool isFullScreen() const {
        return fullScreen;
    }

    void setFullScreen(bool isFullScreen) {
        this->fullScreen = isFullScreen;
    }
};


#endif //LIMONENGINE_OPTIONS_H
