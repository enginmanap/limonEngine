//
// Created by engin on 22.04.2019.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include "API/GraphicsInterface.h"

class Texture {
    GraphicsInterface* graphicsWrapper;
    uint32_t textureID;
    GraphicsInterface::TextureTypes textureType;
    GraphicsInterface::InternalFormatTypes internalFormat;
    GraphicsInterface::FormatTypes format;
    GraphicsInterface::DataTypes dataType;
    GraphicsInterface::FilterModes filterMode = GraphicsInterface::FilterModes::LINEAR;
    GraphicsInterface::TextureWrapModes wrapModeS = GraphicsInterface::TextureWrapModes::NONE;
    GraphicsInterface::TextureWrapModes wrapModeT = GraphicsInterface::TextureWrapModes::NONE;
    GraphicsInterface::TextureWrapModes wrapModeR = GraphicsInterface::TextureWrapModes::NONE;
    uint32_t height, width;
    uint32_t depth;//3D textures, or texture arrays have this as element count

    float borderColor[4] = {0};
    bool borderColorSet = false;
public:
    Texture(GraphicsInterface* graphicsWrapper, GraphicsInterface::TextureTypes textureType, GraphicsInterface::InternalFormatTypes internalFormat, GraphicsInterface::FormatTypes format, GraphicsInterface::DataTypes dataType, uint32_t width, uint32_t height, uint32_t depth = 0)
            : graphicsWrapper(graphicsWrapper), textureType(textureType), internalFormat(internalFormat), format(format), dataType(dataType), height(height), width(width), depth(depth) {
        this->textureID = graphicsWrapper->createTexture(height, width, textureType, internalFormat, format, dataType, depth);
    }

    void loadData(void *data, void *data2 = nullptr, void *data3 = nullptr, void *data4 = nullptr, void *data5 = nullptr, void *data6 = nullptr) {
        graphicsWrapper->loadTextureData(this->textureID, height, width, textureType, internalFormat, format, dataType, depth, data, data2, data3, data4, data5, data6);
    }

    ~Texture() {
        graphicsWrapper->deleteTexture(textureID);
    }

    void setBorderColor(float red, float green, float blue, float alpha) {
        borderColor[0] = red;
        borderColor[1] = green;
        borderColor[2] = blue;
        borderColor[3] = alpha;
        borderColorSet = true;
        graphicsWrapper->setTextureBorder(*this);
    }

    void setWrapModes(GraphicsInterface::TextureWrapModes wrapModeS, GraphicsInterface::TextureWrapModes wrapModeT, GraphicsInterface::TextureWrapModes wrapModeR = GraphicsInterface::TextureWrapModes::NONE) {
        if(this->wrapModeS != wrapModeS || this->wrapModeT != wrapModeT || this->wrapModeR != wrapModeR ) {
            graphicsWrapper->setWrapMode(*this, wrapModeS, wrapModeT, wrapModeR);
            this->wrapModeS = wrapModeS;
            this->wrapModeT = wrapModeT;
            this->wrapModeR = wrapModeR;
        }
    }

    void setFilterMode(GraphicsInterface::FilterModes filterMode) {
        graphicsWrapper->setFilterMode(*this, filterMode);
        this->filterMode = filterMode;
    }

    void removeBorderColor() {
        borderColorSet = false;
        graphicsWrapper->setTextureBorder(*this);
    }

    bool isBorderColorSet() {
        return borderColorSet;
    }
    std::vector<float> getBorderColor() {
        return std::vector<float>(borderColor, borderColor + (sizeof(borderColor)/sizeof(float)));
    }

    GraphicsInterface::TextureTypes getType(){
        return textureType;
    }

    uint32_t getTextureID() {
        return textureID;
    }

    GraphicsInterface::FormatTypes getFormat() {
        return format;
    }

    uint32_t getHeight() const {
        return height;
    }

    uint32_t getWidth() const {
        return width;
    }

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static Texture *deserialize(tinyxml2::XMLElement *TextureNode, GraphicsInterface* graphicsWrapper, Options *options);

};


#endif //LIMONENGINE_TEXTURE_H
