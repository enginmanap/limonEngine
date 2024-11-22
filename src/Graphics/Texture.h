//
// Created by engin on 22.04.2019.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include "API/Graphics/GraphicsInterface.h"

class AssetManager;

class Texture {
public:
    struct TextureInfo {
        GraphicsInterface::TextureTypes  textureType = GraphicsInterface::TextureTypes::T2D;
        GraphicsInterface::InternalFormatTypes internalFormatType = GraphicsInterface::InternalFormatTypes::RED;
        GraphicsInterface::FormatTypes formatType = GraphicsInterface::FormatTypes::RGB;
        GraphicsInterface::DataTypes dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
        GraphicsInterface::TextureWrapModes  textureWrapModeS = GraphicsInterface::TextureWrapModes::NONE;
        GraphicsInterface::TextureWrapModes  textureWrapModeT = GraphicsInterface::TextureWrapModes::NONE;
        GraphicsInterface::TextureWrapModes  textureWrapModeR = GraphicsInterface::TextureWrapModes::NONE;
        GraphicsInterface::FilterModes  filterMode = GraphicsInterface::FilterModes::LINEAR;
        std::string name;
        std::string heightOption;//size can be set using an option parameter. This variable keeps thee name of that parameter.
        std::string widthOption;//same
        std::string depthOption;//same
        int defaultSize[2] = {1920, 1080};
        int depth = 0;//3D textures, or texture arrays have this as element count
        float borderColor[4] = {0.0, 0.0, 0.0, 0.0};
        uint32_t serializeID;
        bool borderColorSet = false;

    };
private:
    GraphicsInterface* graphicsWrapper;
    TextureInfo textureInfo;
    uint32_t textureID;
    std::string source;
    uint32_t height, width;//These are current values, as it might be either a default, or coming from an option.

public:
    Texture(GraphicsInterface* graphicsWrapper, const TextureInfo &textureInfo)
            : graphicsWrapper(graphicsWrapper), textureInfo(textureInfo) {
        if(textureInfo.heightOption.empty()) {
            height = textureInfo.defaultSize[1];
        } else {
            OptionsUtil::Options::Option<long> heightOption = graphicsWrapper->getOptions()->getOption<long>(hash(textureInfo.heightOption));
            height = heightOption.getOrDefault(textureInfo.defaultSize[1]);
        }
        if(textureInfo.widthOption.empty()) {
            width = textureInfo.defaultSize[0];
        } else {
            OptionsUtil::Options::Option<long> widthOption = graphicsWrapper->getOptions()->getOption<long>(hash(textureInfo.widthOption));
            width = widthOption.getOrDefault(textureInfo.defaultSize[0]);
        }
        this->textureID = graphicsWrapper->createTexture(height, width, textureInfo.textureType, textureInfo.internalFormatType, textureInfo.formatType, textureInfo.dataType, textureInfo.depth);
        //there are things that are not auto set. Lets set them
        setFilterMode(textureInfo.filterMode);
        setWrapModes(textureInfo.textureWrapModeS, textureInfo.textureWrapModeT, textureInfo.textureWrapModeR);
        if(textureInfo.borderColorSet) {
            setBorderColor(textureInfo.borderColor[0], textureInfo.borderColor[1], textureInfo.borderColor[2], textureInfo.borderColor[3]);
        }
    }

    Texture(GraphicsInterface* graphicsWrapper, GraphicsInterface::TextureTypes textureType, GraphicsInterface::InternalFormatTypes internalFormat, GraphicsInterface::FormatTypes format, GraphicsInterface::DataTypes dataType, uint32_t width, uint32_t height, uint32_t depth = 0)
            : graphicsWrapper(graphicsWrapper) {
        textureInfo.textureType = textureType;
        textureInfo.internalFormatType = internalFormat;
        textureInfo.formatType = format;
        textureInfo.dataType = dataType;
        textureInfo.defaultSize[0] = width;
        textureInfo.defaultSize[1] = height;
        textureInfo.depth = depth;
        this->textureID = graphicsWrapper->createTexture(textureInfo.defaultSize[1], textureInfo.defaultSize[0], textureInfo.textureType, textureInfo.internalFormatType, textureInfo.formatType, textureInfo.dataType, textureInfo.depth);
    }

    void loadData(void *data, void *data2 = nullptr, void *data3 = nullptr, void *data4 = nullptr, void *data5 = nullptr, void *data6 = nullptr) {
        graphicsWrapper->loadTextureData(this->textureID, textureInfo.defaultSize[1], textureInfo.defaultSize[0], textureInfo.textureType, textureInfo.internalFormatType, textureInfo.formatType, textureInfo.dataType, textureInfo.depth, data, data2, data3, data4, data5, data6);
    }

    ~Texture() {
        graphicsWrapper->deleteTexture(textureID);
    }

    void setBorderColor(float red, float green, float blue, float alpha) {
        textureInfo.borderColor[0] = red;
        textureInfo.borderColor[1] = green;
        textureInfo.borderColor[2] = blue;
        textureInfo.borderColor[3] = alpha;
        textureInfo.borderColorSet = true;
        graphicsWrapper->setTextureBorder(textureID, textureInfo.textureType, textureInfo.borderColorSet,
                                          getBorderColor());
    }

    void setWrapModes(GraphicsInterface::TextureWrapModes wrapModeS, GraphicsInterface::TextureWrapModes wrapModeT, GraphicsInterface::TextureWrapModes wrapModeR = GraphicsInterface::TextureWrapModes::NONE) {
        graphicsWrapper->setWrapMode(this->getTextureID(), this->getType(), wrapModeS, wrapModeT, wrapModeR);
        textureInfo.textureWrapModeS = wrapModeS;
        textureInfo.textureWrapModeT = wrapModeT;
        textureInfo.textureWrapModeR = wrapModeR;
    }

    void setFilterMode(GraphicsInterface::FilterModes filterMode) {
        graphicsWrapper->setFilterMode(textureID, textureInfo.textureType, filterMode);
        textureInfo.filterMode = filterMode;
    }

    void removeBorderColor() {
        textureInfo.borderColorSet = false;
        graphicsWrapper->setTextureBorder(textureID, textureInfo.textureType, textureInfo.borderColorSet,
                                          getBorderColor());
    }

    bool isBorderColorSet() {
        return textureInfo.borderColorSet;
    }
    std::vector<float> getBorderColor() {
        return std::vector<float>(textureInfo.borderColor, textureInfo.borderColor + (sizeof(textureInfo.borderColor)/sizeof(float)));
    }

    GraphicsInterface::TextureTypes getType(){
        return textureInfo.textureType;
    }

    uint32_t getTextureID() {
        return textureID;
    }

    GraphicsInterface::FormatTypes getFormat() {
        return textureInfo.formatType;
    }

    uint32_t getHeight() const {
        return textureInfo.defaultSize[1];
    }

    uint32_t getWidth() const {
        return textureInfo.defaultSize[0];
    }

    uint32_t getSerializeID() const {
        return textureInfo.serializeID;
    }

    void setSerializeID(uint32_t serializeID) {
        this->textureInfo.serializeID = serializeID;
    }

    const std::string &getSource() const {
        return source;
    }

    void setSource(const std::string &source) {
        Texture::source = source;
    }

    const std::string &getName() const {
        return textureInfo.name;
    }

    void setName(const std::string &name) {
        textureInfo.name = name;
    }

    const TextureInfo &getTextureInfo() const {
        return textureInfo;
    }

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, OptionsUtil::Options *options);

    static std::shared_ptr<Texture>
    deserialize(tinyxml2::XMLElement *TextureNode, GraphicsInterface *graphicsWrapper, std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options *options);

};


#endif //LIMONENGINE_TEXTURE_H
