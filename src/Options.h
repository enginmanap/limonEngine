//
// Created by engin on 5.11.2017.
//

#ifndef LIMONENGINE_OPTIONS_H
#define LIMONENGINE_OPTIONS_H

#include <glm/glm.hpp>
#include <iostream>
#include <tinyxml2.h>
#include <unordered_map>
#include <memory>

#include "Utils/Logger.h"
#include "API/LimonTypes.h"

class Options {
public:

    static constexpr float PI = 3.14159265358979f;
    static constexpr float PI_DOUBLE = 3.141592653589793238463;
    enum class MoveModes {WALK, RUN};
    enum class TextureFilteringModes { NEAREST, BILINEAR, TRILINEAR };

    bool getOption(const std::string& optionName, long &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::LONG) {
            return false;
        }
        value = it->second->value.longValue;
        return true;
    }

    bool getOptionOrDefault(const std::string& optionName, long &value, long defaultValue) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::LONG) {
            value = defaultValue;
            return false;
        }
        value = it->second->value.longValue;
        return true;
    }

    void setOption(const std::string& optionName, long value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter;
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::LONG;
        it->second->value.longValue = value;
    }
    bool getOption(const std::string& optionName, double &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::DOUBLE) {
            return false;
        }
        value = it->second->value.doubleValue;
        return true;
    }

    bool getOptionOrDefault(const std::string& optionName, double &value, double defaultValue) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::DOUBLE) {
            value = defaultValue;
            return false;
        }
        value = it->second->value.doubleValue;
        return true;
    }

    void setOption(const std::string& optionName, double value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter;
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::DOUBLE;
        it->second->value.doubleValue = value;
    }

    bool getOption(const std::string& optionName, LimonTypes::Vec4 &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::VEC4) {
            return false;
        }
        value = it->second->value.vectorValue;
        return true;
    }

    void setOption(const std::string& optionName, LimonTypes::Vec4 value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter;
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::VEC4;
        it->second->value.vectorValue = value;
    }

private:
    Logger *logger{};
    std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> options;

    glm::vec3 walkSpeed = glm::vec3(8, 0, 8);
    glm::vec3 runSpeed = glm::vec3(12, 0, 12);
    glm::vec3 moveSpeed = walkSpeed;
    glm::vec3 freeMovementSpeed = glm::vec3(0.1f,0.1f,0.1f);
    float jumpFactor = 7.0f;
    float lookAroundSpeed = -6.5f;
/*    uint32_t screenHeight = 1080;
    uint32_t screenWidth = 1920;*/

    uint32_t shadowMapDirectionalWidth = 2048;
    uint32_t shadowMapDirectionalHeight = 2048;
    uint32_t shadowMapPointWidth = 512;
    uint32_t shadowMapPointHeight = 512;
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

    uint32_t ssaoSampleCount = 9;
    bool ssaoEnabled = false;
    bool renderInformations = true;

    bool loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3&);
    bool loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4&);
    bool loadDouble(tinyxml2::XMLNode *optionsNode, const std::string &name, double&);
    bool loadLong(tinyxml2::XMLNode *optionsNode, const std::string &name, long&);
    bool loadString(tinyxml2::XMLNode *optionsNode, const std::string &name, std::string&);
    bool loadBool(tinyxml2::XMLNode *optionsNode, const std::string &name, bool&);
public:

    bool loadOptions(const std::string& optionsFileName);
    bool loadOptionsNew(const std::string& optionsFileName);

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

    uint32_t getScreenHeight() const {
        long height;
        getOption("screenHeight", height);
        return height;
    }

    void setScreenHeight(unsigned int height) {
        setOption("screenHeight", (long)height);
    }

    uint32_t getScreenWidth() const {
        long width;
        getOption("screenWidth", width);
        return width;
    }

    void setScreenWidth(unsigned int width) {
        setOption("screenWidth", (long)width);
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
            case MoveModes::RUN:
                moveSpeed = runSpeed;
                break;
            case MoveModes::WALK:
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

    uint32_t getSSAOSampleCount() const {
        return ssaoSampleCount;
    }

    void setSSAOSampleCount(uint32_t sampleCount) {
        ssaoSampleCount = sampleCount;
    }

    bool isSsaoEnabled() const {
        return ssaoEnabled;
    }

    void setSsaoEnabled(bool ssaoEnabled) {
        this->ssaoEnabled = ssaoEnabled;
    }

    bool getRenderInformations() {
        return renderInformations;
    }

    void setRenderInformations(bool renderInformations) {
        this->renderInformations = renderInformations;
    }
};


#endif //LIMONENGINE_OPTIONS_H
