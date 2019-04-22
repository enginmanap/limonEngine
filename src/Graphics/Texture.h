//
// Created by engin on 22.04.2019.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include "GLHelper.h"

class Texture {
    GLHelper* glHelper;
    uint32_t textureID;
    GLHelper::TextureTypes textureType;
    GLHelper::InternalFormatTypes internalFormat;
    GLHelper::FormatTypes format;
    GLHelper::DataTypes dataType;
    GLHelper::FilterModes filterMode = GLHelper::FilterModes::LINEAR;
    GLHelper::TextureWrapModes wrapModeS = GLHelper::TextureWrapModes::NONE;
    GLHelper::TextureWrapModes wrapModeT = GLHelper::TextureWrapModes::NONE;
    GLHelper::TextureWrapModes wrapModeR = GLHelper::TextureWrapModes::NONE;
    uint32_t height, width;
    uint32_t depth;//3D textures, or texture arrays have this as element count

    float borderColor[4] = {0};
    bool borderColorSet = false;
public:
    Texture(GLHelper* glHelper, GLHelper::TextureTypes textureType, GLHelper::InternalFormatTypes internalFormat, GLHelper::FormatTypes format, GLHelper::DataTypes dataType,uint32_t width, uint32_t height, uint32_t depth = 0)
            : glHelper(glHelper), textureType(textureType), internalFormat(internalFormat), format(format), dataType(dataType), height(height), width(width), depth(depth) {
        this->textureID = glHelper->createTexture(height, width, textureType, internalFormat, format, dataType, depth);
    }

    void loadData(void *data, void *data2 = nullptr, void *data3 = nullptr, void *data4 = nullptr, void *data5 = nullptr, void *data6 = nullptr) {
        glHelper->loadTextureData(this->textureID, height, width, textureType, internalFormat, format, dataType, depth, data, data2, data3, data4, data5, data6);
    }

    ~Texture() {
        glHelper->deleteTexture(textureID);
    }

    void setBorderColor(float red, float green, float blue, float alpha) {
        borderColor[0] = red;
        borderColor[1] = green;
        borderColor[2] = blue;
        borderColor[3] = alpha;
        borderColorSet = true;
        glHelper->setTextureBorder(*this);
    }

    void setWrapModes(GLHelper::TextureWrapModes wrapModeS, GLHelper::TextureWrapModes wrapModeT, GLHelper::TextureWrapModes wrapModeR = GLHelper::TextureWrapModes::NONE) {
        if(this->wrapModeS != wrapModeS || this->wrapModeT != wrapModeT || this->wrapModeR != wrapModeR ) {
            glHelper->setWrapMode(*this, wrapModeS, wrapModeT, wrapModeR);
            this->wrapModeS = wrapModeS;
            this->wrapModeT = wrapModeT;
            this->wrapModeR = wrapModeR;
        }
    }

    void setFilterMode(GLHelper::FilterModes filterMode) {
        glHelper->setFilterMode(*this, filterMode);
        this->filterMode = filterMode;
    }

    void removeBorderColor() {
        borderColorSet = false;
        glHelper->setTextureBorder(*this);
    }

    bool isBorderColorSet() {
        return borderColorSet;
    }
    std::vector<float> getBorderColor() {
        return std::vector<float>(borderColor, borderColor + (sizeof(borderColor)/sizeof(float)));
    }

    GLHelper::TextureTypes getType(){
        return textureType;
    }

    uint32_t getTextureID() {
        return textureID;
    }

    GLHelper::FormatTypes getFormat() {
        return format;
    }

    uint32_t getHeight() const {
        return height;
    }

    uint32_t getWidth() const {
        return width;
    }

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static Texture *deserialize(tinyxml2::XMLElement *TextureNode, GLHelper *glHelper, Options *options);

};


#endif //LIMONENGINE_TEXTURE_H
