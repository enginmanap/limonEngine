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
#include <functional>
#include "Utils/HashUtil.h"
#include "Utils/Logger.h"
#include "API/LimonTypes.h"

class Options {
public:

    static constexpr float PI = 3.14159265358979f;
    static constexpr float PI_DOUBLE = 3.141592653589793238463;
    enum class MoveModes {WALK, RUN};
    enum class TextureFilteringModes { NEAREST, BILINEAR, TRILINEAR };


    bool getOption(const std::string& optionName, uint32_t &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::LONG) {
            return false;
        }
        value = it->second->value.longValue;
        return true;
    }

    bool getOption(const std::string& optionName, long &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::LONG) {
            return false;
        }
        value = it->second->value.longValue;
        return true;
    }

    bool getOption(const std::string& optionName, bool &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::BOOLEAN) {
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

    bool getOptionOrDefault(const std::string& optionName, std::string &value, const std::string &defaultValue) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::STRING) {
            value = defaultValue;
            return false;
        }
        value = it->second->value.stringValue;
        return true;
    }

    bool getOption(const std::string& optionName, std::string &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::STRING) {
            return false;
        }
        value = it->second->value.stringValue;
        return true;
    }

    bool getOption(const std::string& optionName, double &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::DOUBLE) {
            return false;
        }
        value = it->second->value.doubleValue;
        return true;
    }

    bool getOption(const std::string& optionName, float &value) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::DOUBLE) {
            return false;
        }
        value = (float)it->second->value.doubleValue;
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

    bool getOptionOrDefault(const std::string& optionName, float &value, float defaultValue) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::DOUBLE) {
            value = defaultValue;
            return false;
        }
        value = (float)it->second->value.doubleValue;
        return true;
    }

    bool getOptionOrDefault(const std::string& optionName, bool &value, bool defaultValue) const {
        auto it = this->options.find(optionName);
        if(it == this->options.end() || it->second->valueType != LimonTypes::GenericParameter::BOOLEAN) {
            value = defaultValue;
            return false;
        }
        value = it->second->value.boolValue;
        return true;
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
            std::shared_ptr<LimonTypes::GenericParameter> parameter = std::make_shared<LimonTypes::GenericParameter>();
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::VEC4;
        it->second->value.vectorValue = value;
    }

    void setOption(const std::string& optionName, double value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter = std::make_shared<LimonTypes::GenericParameter>();
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::DOUBLE;
        it->second->value.doubleValue = value;
    }

    void setOption(const std::string& optionName, long value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter = std::make_shared<LimonTypes::GenericParameter>();
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::LONG;
        it->second->value.longValue = value;
    }

    void setOption(const std::string& optionName, bool value){
        auto it = this->options.find(optionName);
        if(it == this->options.end()) {
            std::shared_ptr<LimonTypes::GenericParameter> parameter = std::make_shared<LimonTypes::GenericParameter>();
            parameter->description = optionName;
            this->options[optionName] = parameter;
            it = this->options.find(optionName);
        }
        it->second->valueType = LimonTypes::GenericParameter::BOOLEAN;
        it->second->value.boolValue = value;
    }

private:
    Logger *logger{};
    std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> options;
    std::unordered_map<uint64_t, std::function<void(uint64_t)>> changeRegisters;


    /*SDL properties that should be available */
    void* imeWindowHandle;
    int drawableWidth, drawableHeight;
    int windowWidth, windowHeight;
    bool isWindowInFocus;

    bool loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3&);
    bool loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4&);
    bool loadDouble(tinyxml2::XMLNode *optionsNode, const std::string &name, double&);
    bool loadLong(tinyxml2::XMLNode *optionsNode, const std::string &name, long&);
    bool loadString(tinyxml2::XMLNode *optionsNode, const std::string &name, std::string&);
    bool loadBool(tinyxml2::XMLNode *optionsNode, const std::string &name, bool&);
public:

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

    uint32_t getScreenHeight() const {
        uint32_t height = 1080;//default
        getOption("screenHeight", height);
        return height;
    }

    void setScreenHeight(unsigned int height) {
        setOption("screenHeight", (long)height);
    }

    uint32_t getScreenWidth() const {
        uint32_t width = 1920;//default
        getOption("screenWidth", width);
        return width;
    }

    void setScreenWidth(unsigned int width) {
        setOption("screenWidth", (long)width);
    }

    Options() {
        this->logger = new Logger();
    };

    Logger* getLogger() {
        return logger;
    }

    const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>& getAllOptions() const {
        return this->options;
    }

};


#endif //LIMONENGINE_OPTIONS_H
