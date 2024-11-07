//
// Created by engin on 22.04.2019.
//

#include <Assets/AssetManager.h>
#include <Assets/TextureAsset.h>
#include "Texture.h"

bool Texture::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options [[gnu::unused]]) {
    tinyxml2::XMLElement *textureNode = document.NewElement("Texture");
    parentNode->InsertEndChild(textureNode);

    tinyxml2::XMLElement *currentElement;

    if (this->source.length() > 0) {
        currentElement = document.NewElement("Source");
        currentElement->SetText(this->source.c_str());
        textureNode->InsertEndChild(currentElement);
    }

    if (!textureInfo.name.empty()) {
        currentElement = document.NewElement("Name");
        currentElement->SetText(textureInfo.name.c_str());
        textureNode->InsertEndChild(currentElement);
    }

    currentElement = document.NewElement("TextureType");
    switch (textureInfo.textureType) {
        case GraphicsInterface::TextureTypes::T2D: currentElement->SetText("T2D"); break;
        case GraphicsInterface::TextureTypes::T2D_ARRAY: currentElement->SetText("T2D_ARRAY"); break;
        case GraphicsInterface::TextureTypes::TCUBE_MAP: currentElement->SetText("TCUBE_MAP"); break;
        case GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY: currentElement->SetText("TCUBE_MAP_ARRAY"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("InternalFormat");
    switch (textureInfo.internalFormatType) {
        case GraphicsInterface::InternalFormatTypes::RED: currentElement->SetText("RED"); break;
        case GraphicsInterface::InternalFormatTypes::R32F: currentElement->SetText("R32F"); break;
        case GraphicsInterface::InternalFormatTypes::RGB: currentElement->SetText("RGB"); break;
        case GraphicsInterface::InternalFormatTypes::RGBA: currentElement->SetText("RGBA"); break;
        case GraphicsInterface::InternalFormatTypes::RGB16F: currentElement->SetText("RGB16F"); break;
        case GraphicsInterface::InternalFormatTypes::RGB32F: currentElement->SetText("RGB32F"); break;
        case GraphicsInterface::InternalFormatTypes::RGBA32F: currentElement->SetText("RGB32AF"); break;
        case GraphicsInterface::InternalFormatTypes::DEPTH: currentElement->SetText("DEPTH"); break;
        case GraphicsInterface::InternalFormatTypes::COMPRESSED_RGB: currentElement->SetText("COMPRESSED_RGB"); break;
        case GraphicsInterface::InternalFormatTypes::COMPRESSED_RGBA: currentElement->SetText("COMPRESSED_RGBA"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Format");
    switch (textureInfo.formatType) {
        case GraphicsInterface::FormatTypes::RED: currentElement->SetText("RED"); break;
        case GraphicsInterface::FormatTypes::RGB: currentElement->SetText("RGB"); break;
        case GraphicsInterface::FormatTypes::RGBA: currentElement->SetText("RGBA"); break;
        case GraphicsInterface::FormatTypes::DEPTH: currentElement->SetText("DEPTH"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DataType");
    switch (textureInfo.dataType) {
        case GraphicsInterface::DataTypes::UNSIGNED_BYTE: currentElement->SetText("UNSIGNED_BYTE"); break;
        case GraphicsInterface::DataTypes::UNSIGNED_SHORT: currentElement->SetText("UNSIGNED_SHORT"); break;
        case GraphicsInterface::DataTypes::UNSIGNED_INT: currentElement->SetText("UNSIGNED_INT"); break;
        case GraphicsInterface::DataTypes::FLOAT: currentElement->SetText("FLOAT"); break;
        case GraphicsInterface::DataTypes::HALF_FLOAT: currentElement->SetText("HALF_FLOAT"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Height");
    currentElement->SetText(std::to_string(textureInfo.defaultSize[1]).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Width");
    currentElement->SetText(std::to_string(textureInfo.defaultSize[0]).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("HeightOption");
    if (!textureInfo.heightOption.empty()) {
        currentElement->SetText(textureInfo.heightOption.c_str());
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("WidthOption");
    if (!textureInfo.widthOption.empty()) {
        currentElement->SetText(textureInfo.widthOption.c_str());
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Depth");
    currentElement->SetText(std::to_string(textureInfo.depth).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DepthOption");
    currentElement->SetText(textureInfo.depthOption.c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("borderColor");

    tinyxml2::XMLElement *borderElement = document.NewElement("R");
    borderElement->SetText(textureInfo.borderColor[0]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("G");
    borderElement->SetText(textureInfo.borderColor[1]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("B");
    borderElement->SetText(textureInfo.borderColor[2]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("A");
    borderElement->SetText(textureInfo.borderColor[3]);
    currentElement->InsertEndChild(borderElement);
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("BorderColorSet");
    if(textureInfo.borderColorSet) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("TextureSerializeID");
    currentElement->SetText(this->textureInfo.serializeID);
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FilterMode");
    switch (textureInfo.filterMode) {
        case GraphicsInterface::FilterModes::NEAREST: currentElement->SetText("NEAREST"); break;
        case GraphicsInterface::FilterModes::LINEAR: currentElement->SetText("LINEAR"); break;
        case GraphicsInterface::FilterModes::TRILINEAR: currentElement->SetText("TRILINEAR"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("WrapModes");
    tinyxml2::XMLElement *wrapModeNode= document.NewElement("S");
    switch (textureInfo.textureWrapModeS) {
        case GraphicsInterface::TextureWrapModes::NONE: wrapModeNode->SetText("NONE"); break;
        case GraphicsInterface::TextureWrapModes::BORDER: wrapModeNode->SetText("BORDER"); break;
        case GraphicsInterface::TextureWrapModes::EDGE: wrapModeNode->SetText("EDGE"); break;
        case GraphicsInterface::TextureWrapModes::REPEAT: wrapModeNode->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);

    wrapModeNode= document.NewElement("T");
    switch (textureInfo.textureWrapModeT) {
        case GraphicsInterface::TextureWrapModes::NONE: wrapModeNode->SetText("NONE"); break;
        case GraphicsInterface::TextureWrapModes::BORDER: wrapModeNode->SetText("BORDER"); break;
        case GraphicsInterface::TextureWrapModes::EDGE: wrapModeNode->SetText("EDGE"); break;
        case GraphicsInterface::TextureWrapModes::REPEAT: wrapModeNode->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);

    wrapModeNode= document.NewElement("R");
    switch (textureInfo.textureWrapModeR) {
        case GraphicsInterface::TextureWrapModes::NONE: wrapModeNode->SetText("NONE"); break;
        case GraphicsInterface::TextureWrapModes::BORDER: wrapModeNode->SetText("BORDER"); break;
        case GraphicsInterface::TextureWrapModes::EDGE: wrapModeNode->SetText("EDGE"); break;
        case GraphicsInterface::TextureWrapModes::REPEAT: wrapModeNode->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);
    textureNode->InsertEndChild(currentElement);

    return true;
}

std::shared_ptr<Texture> Texture::deserialize(tinyxml2::XMLElement *TextureNode, GraphicsInterface* graphicsWrapper, std::shared_ptr<AssetManager> assetManager, Options *options [[gnu::unused]]) {
    tinyxml2::XMLElement* textureNodeAttribute = nullptr;

    TextureInfo textureInfo;

    //if no serializeId is set, then we will not be able to assign textures to where they should, so it is the first thing.
    textureNodeAttribute = TextureNode->FirstChildElement("TextureSerializeID");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture doesn't have serializeID." << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture serializeID doesn't have text, this is must likely a bug." << std::endl;
        return nullptr;
    }
    std::string serializeIDString = textureNodeAttribute->GetText();
    textureInfo.serializeID = std::stoi(serializeIDString);

    //there is a possibility, that this texture is intended as a custom texture with data within, check for the file tags
    textureNodeAttribute = TextureNode->FirstChildElement("Source");
    if (textureNodeAttribute != nullptr) {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture source color setting has no text, skipping! " << std::endl;
        } else {
            std::string textureSource = textureNodeAttribute->GetText();
            std::shared_ptr<TextureAsset> asset = assetManager->loadAsset<TextureAsset>({ textureSource });
            std::shared_ptr<Texture> texture = asset->getTexture();
            texture->setSource(textureSource);
            texture->setSerializeID(textureInfo.serializeID);
            return texture;
        }
    }

    textureNodeAttribute = TextureNode->FirstChildElement("TextureType");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have type. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture type has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string textureTypeString = textureNodeAttribute->GetText();
    if(textureTypeString == "T2D") {
        textureInfo.textureType = GraphicsInterface::TextureTypes::T2D;
    } else if(textureTypeString == "T2D_ARRAY") {
        textureInfo.textureType = GraphicsInterface::TextureTypes::T2D_ARRAY;
    } else if(textureTypeString == "TCUBE_MAP") {
        textureInfo.textureType = GraphicsInterface::TextureTypes::TCUBE_MAP;
    } else if(textureTypeString == "TCUBE_MAP_ARRAY") {
        textureInfo.textureType = GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY;
    } else {
        std::cerr << "Texture type is unknown, skipping! " << std::endl;
        return nullptr;
    }

    textureNodeAttribute = TextureNode->FirstChildElement("InternalFormat");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have internal format. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture internal format has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string internalFormatString = textureNodeAttribute->GetText();
    if(internalFormatString == "RED") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::RED;
    } else if(internalFormatString == "R32F") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::R32F;
    } else if(internalFormatString == "RGB") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::RGB;
    } else if(internalFormatString == "RGBA") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::RGBA;
    } else if(internalFormatString == "RGB16F") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::RGB16F;
    } else if(internalFormatString == "RGB32F") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::RGB32F;
    } else if(internalFormatString == "DEPTH") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::DEPTH;
    } else if(internalFormatString == "COMPRESSED_RGB") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::COMPRESSED_RGB;
    } else if(internalFormatString == "COMPRESSED_RGBA") {
        textureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes ::COMPRESSED_RGBA;
    } else {
        std::cerr << "Texture internal format is (" << internalFormatString << ") unknown for serialize id " << textureInfo.serializeID <<", skipping! " << std::endl;
        return nullptr;
    }

    textureNodeAttribute = TextureNode->FirstChildElement("Format");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have format. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture format has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string formatString = textureNodeAttribute->GetText();
    if(formatString == "RED") {
        textureInfo.formatType = GraphicsInterface::FormatTypes::RED;
    } else if(formatString == "RGB") {
        textureInfo.formatType = GraphicsInterface::FormatTypes::RGB;
    } else if(formatString == "RGBA") {
        textureInfo.formatType = GraphicsInterface::FormatTypes::RGBA;
    } else if(formatString == "DEPTH") {
        textureInfo.formatType = GraphicsInterface::FormatTypes::DEPTH;
    } else {
        std::cerr << "Texture format (" << formatString <<")is unknown for serialize id "<< serializeIDString <<", skipping! " << std::endl;
        return nullptr;
    }

    textureNodeAttribute = TextureNode->FirstChildElement("DataType");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have Data Type. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture Data Type has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string dataTypeString = textureNodeAttribute->GetText();
    if(dataTypeString == "UNSIGNED_BYTE") {
        textureInfo.dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
    } else if(dataTypeString == "UNSIGNED_SHORT") {
        textureInfo.dataType = GraphicsInterface::DataTypes::UNSIGNED_SHORT;
    } else if(dataTypeString == "UNSIGNED_INT") {
        textureInfo.dataType = GraphicsInterface::DataTypes::UNSIGNED_INT;
    } else if(dataTypeString == "FLOAT") {
        textureInfo.dataType = GraphicsInterface::DataTypes::FLOAT;
    } else if(dataTypeString == "HALF_FLOAT") {
        textureInfo.dataType = GraphicsInterface::DataTypes::HALF_FLOAT;
    } else {
        std::cerr << "Texture data type is unknown, skipping! " << std::endl;
        return nullptr;
    }

    textureNodeAttribute = TextureNode->FirstChildElement("Height");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have Height. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture Height has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string heightString = textureNodeAttribute->GetText();
    textureInfo.defaultSize[1] = std::stoi(heightString);

    textureNodeAttribute = TextureNode->FirstChildElement("Width");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have width. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture width has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string widthString = textureNodeAttribute->GetText();
    textureInfo.defaultSize[0] = std::stoi(widthString);

    textureNodeAttribute = TextureNode->FirstChildElement("HeightOption");
    if (textureNodeAttribute != nullptr) {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cout << "Texture HeightOption has no text, skipping! " << std::endl;
        } else {
            textureInfo.heightOption = textureNodeAttribute->GetText();
        }
    }

    textureNodeAttribute = TextureNode->FirstChildElement("WidthOption");
    if (textureNodeAttribute != nullptr) {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cout << "Texture WidthOption has no text, skipping! " << std::endl;
        } else {
            textureInfo.widthOption = textureNodeAttribute->GetText();
        }
    }

    bool isTextureDepthSet = false;
    textureNodeAttribute = TextureNode->FirstChildElement("DepthOption");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture depth option not found. falling back to depth setting" << std::endl;
    } else {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture depth option has no text,  falling back to depth setting" << std::endl;
        } else {
            textureInfo.depthOption = textureNodeAttribute->GetText();
            long depthOption;
            if(options->getOption(textureInfo.depthOption, depthOption)) {
                textureInfo.depth = depthOption;
                isTextureDepthSet = true;
            }
        }
    }

    if(!isTextureDepthSet) {
        textureNodeAttribute = TextureNode->FirstChildElement("Depth");
        if (textureNodeAttribute == nullptr) {
            std::cerr << "Texture must have depth. Skipping" << std::endl;
            return nullptr;
        }
        if (textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture depth has no text, skipping! " << std::endl;
            return nullptr;
        }
        std::string depthString = textureNodeAttribute->GetText();
        textureInfo.depth = std::stoi(depthString);
    }

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(graphicsWrapper, textureInfo);

    textureNodeAttribute = TextureNode->FirstChildElement("Name");
    if (textureNodeAttribute != nullptr) {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture name has no text, skipping! " << std::endl;
        } else {
            std::string textureName = textureNodeAttribute->GetText();
            texture->setName(textureName);
        }
    }

    texture->setSerializeID(textureInfo.serializeID);

    textureNodeAttribute = TextureNode->FirstChildElement("FilterMode");
    if (textureNodeAttribute != nullptr) {
        if (textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture filter mode has no text, using default value." << std::endl;
        } else {
            GraphicsInterface::FilterModes filterMode;
            bool fail = false;
            std::string filterModeString = textureNodeAttribute->GetText();
            if (filterModeString == "NEAREST") {
                filterMode = GraphicsInterface::FilterModes::NEAREST;
            } else if (filterModeString == "LINEAR") {
                filterMode = GraphicsInterface::FilterModes::LINEAR;
            } else if (filterModeString == "TRILINEAR") {
                filterMode = GraphicsInterface::FilterModes::TRILINEAR;
            } else {
                std::cerr << "Texture filter mode is unknown, using default value." << std::endl;
                fail = true;
            }
            if(!fail) {
                texture->setFilterMode(filterMode);
            }
        }
    }

    textureNodeAttribute = TextureNode->FirstChildElement("WrapModes");
    {
        GraphicsInterface::TextureWrapModes wrapModeS, wrapModeT, wrapModeR;
        bool fail = false;
        tinyxml2::XMLElement *wrapNodeAttribute = nullptr;
        wrapNodeAttribute = textureNodeAttribute->FirstChildElement("S");
        if (wrapNodeAttribute != nullptr) {
            if (wrapNodeAttribute->GetText() == nullptr) {
                std::cerr << "Texture wrap mode S has no text, using default value." << std::endl;
                fail = true;
            } else {
                std::string wrapModeString = wrapNodeAttribute->GetText();
                if (wrapModeString == "NONE") {
                    wrapModeS = GraphicsInterface::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeS = GraphicsInterface::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeS = GraphicsInterface::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeS = GraphicsInterface::TextureWrapModes::REPEAT;
                } else {
                    std::cerr << "Texture wrap mode S is unknown, using default value." << std::endl;
                    fail = true;
                }
            }
        } else {
            fail = true;
        }

        wrapNodeAttribute = textureNodeAttribute->FirstChildElement("T");
        if (wrapNodeAttribute != nullptr) {
            if (wrapNodeAttribute->GetText() == nullptr) {
                std::cerr << "Texture wrap mode T has no text, using default value." << std::endl;
                fail = true;
            } else {
                std::string wrapModeString = wrapNodeAttribute->GetText();
                if (wrapModeString == "NONE") {
                    wrapModeT = GraphicsInterface::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeT = GraphicsInterface::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeT = GraphicsInterface::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeT = GraphicsInterface::TextureWrapModes::REPEAT;
                } else {
                    std::cerr << "Texture wrap mode T is unknown, using default value." << std::endl;
                    fail = true;
                }
            }
        } else {
            fail = true;
        }

        wrapNodeAttribute = textureNodeAttribute->FirstChildElement("R");
        if (wrapNodeAttribute != nullptr) {
            if (wrapNodeAttribute->GetText() == nullptr) {
                std::cerr << "Texture wrap mode R has no text, using default value." << std::endl;
                fail = true;
            } else {
                std::string wrapModeString = wrapNodeAttribute->GetText();
                if (wrapModeString == "NONE") {
                    wrapModeR = GraphicsInterface::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeR = GraphicsInterface::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeR = GraphicsInterface::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeR = GraphicsInterface::TextureWrapModes::REPEAT;
                } else {
                    std::cerr << "Texture wrap mode R is unknown, using default value." << std::endl;
                    fail = true;
                }
            }
        } else {
            fail = true;
        }

        if (!fail) {
            texture->setWrapModes(wrapModeS, wrapModeT, wrapModeR);
        }
    }

    bool borderColorSet = false;
    textureNodeAttribute = TextureNode->FirstChildElement("BorderColorSet");
    if (textureNodeAttribute != nullptr) {
        if(textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture border color setting has no text, skipping! " << std::endl;
        } else {
            std::string borderColorSetString = textureNodeAttribute->GetText();
            if(borderColorSetString == "True") {
                borderColorSet = true;
            } else if(borderColorSetString == "False") {
                borderColorSet = false;
            } else {
                std::cerr << "Texture border color setting is unknown, border color will not be set" << std::endl;
            }
        }
    }

    if(borderColorSet) {
        textureNodeAttribute = TextureNode->FirstChildElement("borderColor");
        {
            float borderColors[4] = {0};
            bool fail = false;
            tinyxml2::XMLElement *borderColorNode = nullptr;
            borderColorNode = textureNodeAttribute->FirstChildElement("R");
            if (borderColorNode != nullptr) {
                if (borderColorNode->GetText() == nullptr) {
                    std::cerr << "Border color R has no text, using default value." << std::endl;
                    fail = true;
                } else {
                    std::string borderColorString = borderColorNode->GetText();
                    borderColors[0] = std::stof(borderColorString);
                }
            } else {
                fail = true;
            }
            borderColorNode = textureNodeAttribute->FirstChildElement("G");
            if (borderColorNode != nullptr) {
                if (borderColorNode->GetText() == nullptr) {
                    std::cerr << "Border color G has no text, using default value." << std::endl;
                    fail = true;
                } else {
                    std::string borderColorString = borderColorNode->GetText();
                    borderColors[1] = std::stof(borderColorString);
                }
            } else {
                fail = true;
            }
            borderColorNode = textureNodeAttribute->FirstChildElement("B");
            if (borderColorNode != nullptr) {
                if (borderColorNode->GetText() == nullptr) {
                    std::cerr << "Border color B has no text, using default value." << std::endl;
                    fail = true;
                } else {
                    std::string borderColorString = borderColorNode->GetText();
                    borderColors[2] = std::stof(borderColorString);
                }
            } else {
                fail = true;
            }
            borderColorNode = textureNodeAttribute->FirstChildElement("A");
            if (borderColorNode != nullptr) {
                if (borderColorNode->GetText() == nullptr) {
                    std::cerr << "Border color A has no text, using default value." << std::endl;
                    fail = true;
                } else {
                    std::string borderColorString = borderColorNode->GetText();
                    borderColors[3] = std::stof(borderColorString);
                }
            } else {
                fail = true;
            }

            if(!fail) {
                texture->setBorderColor(borderColors[0], borderColors[1], borderColors[2], borderColors[3]);
            }
        }
    }

    return texture;
}
