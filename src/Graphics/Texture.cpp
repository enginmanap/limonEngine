//
// Created by engin on 22.04.2019.
//

#include "Texture.h"

bool Texture::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options [[gnu::unused]]) {
    tinyxml2::XMLElement *textureNode = document.NewElement("Texture");
    parentNode->InsertEndChild(textureNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("TextureType");
    switch (textureType) {
        case OpenGLGraphics::TextureTypes::T2D: currentElement->SetText("T2D"); break;
        case OpenGLGraphics::TextureTypes::T2D_ARRAY: currentElement->SetText("T2D_ARRAY"); break;
        case OpenGLGraphics::TextureTypes::TCUBE_MAP: currentElement->SetText("TCUBE_MAP"); break;
        case OpenGLGraphics::TextureTypes::TCUBE_MAP_ARRAY: currentElement->SetText("TCUBE_MAP_ARRAY"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("InternalFormat");
    switch (internalFormat) {
        case OpenGLGraphics::InternalFormatTypes::RED: currentElement->SetText("RED"); break;
        case OpenGLGraphics::InternalFormatTypes::RGB: currentElement->SetText("RGB"); break;
        case OpenGLGraphics::InternalFormatTypes::RGBA: currentElement->SetText("RGBA"); break;
        case OpenGLGraphics::InternalFormatTypes::RGB16F: currentElement->SetText("RGB16F"); break;
        case OpenGLGraphics::InternalFormatTypes::RGB32F: currentElement->SetText("RGB32F"); break;
        case OpenGLGraphics::InternalFormatTypes::DEPTH: currentElement->SetText("DEPTH"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Format");
    switch (format) {
        case OpenGLGraphics::FormatTypes::RED: currentElement->SetText("RED"); break;
        case OpenGLGraphics::FormatTypes::RGB: currentElement->SetText("RGB"); break;
        case OpenGLGraphics::FormatTypes::RGBA: currentElement->SetText("RGBA"); break;
        case OpenGLGraphics::FormatTypes::DEPTH: currentElement->SetText("DEPTH"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DataType");
    switch (dataType) {
        case OpenGLGraphics::DataTypes::UNSIGNED_BYTE: currentElement->SetText("UNSIGNED_BYTE"); break;
        case OpenGLGraphics::DataTypes::FLOAT: currentElement->SetText("FLOAT"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Height");
    currentElement->SetText(std::to_string(this->height).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Width");
    currentElement->SetText(std::to_string(this->width).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Depth");
    currentElement->SetText(std::to_string(this->depth).c_str());
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("borderColor");

    tinyxml2::XMLElement *borderElement = document.NewElement("R");
    borderElement->SetText(borderColor[0]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("G");
    borderElement->SetText(borderColor[1]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("B");
    borderElement->SetText(borderColor[2]);
    currentElement->InsertEndChild(borderElement);

    borderElement = document.NewElement("A");
    borderElement->SetText(borderColor[3]);
    currentElement->InsertEndChild(borderElement);
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("BorderColorSet");
    if(borderColorSet) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FilterMode");
    switch (filterMode) {
        case OpenGLGraphics::FilterModes::NEAREST: currentElement->SetText("NEAREST"); break;
        case OpenGLGraphics::FilterModes::LINEAR: currentElement->SetText("LINEAR"); break;
        case OpenGLGraphics::FilterModes::TRILINEAR: currentElement->SetText("TRILINEAR"); break;
    }
    textureNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("WrapModes");
    tinyxml2::XMLElement *wrapModeNode= document.NewElement("S");
    switch (wrapModeS) {
        case OpenGLGraphics::TextureWrapModes::NONE: currentElement->SetText("NONE"); break;
        case OpenGLGraphics::TextureWrapModes::BORDER: currentElement->SetText("BORDER"); break;
        case OpenGLGraphics::TextureWrapModes::EDGE: currentElement->SetText("EDGE"); break;
        case OpenGLGraphics::TextureWrapModes::REPEAT: currentElement->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);

    wrapModeNode= document.NewElement("T");
    switch (wrapModeT) {
        case OpenGLGraphics::TextureWrapModes::NONE: currentElement->SetText("NONE"); break;
        case OpenGLGraphics::TextureWrapModes::BORDER: currentElement->SetText("BORDER"); break;
        case OpenGLGraphics::TextureWrapModes::EDGE: currentElement->SetText("EDGE"); break;
        case OpenGLGraphics::TextureWrapModes::REPEAT: currentElement->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);

    wrapModeNode= document.NewElement("R");
    switch (wrapModeR) {
        case OpenGLGraphics::TextureWrapModes::NONE: currentElement->SetText("NONE"); break;
        case OpenGLGraphics::TextureWrapModes::BORDER: currentElement->SetText("BORDER"); break;
        case OpenGLGraphics::TextureWrapModes::EDGE: currentElement->SetText("EDGE"); break;
        case OpenGLGraphics::TextureWrapModes::REPEAT: currentElement->SetText("REPEAT"); break;
    }
    currentElement->InsertEndChild(wrapModeNode);
    textureNode->InsertEndChild(currentElement);

    return true;
}

Texture *Texture::deserialize(tinyxml2::XMLElement *TextureNode, OpenGLGraphics *glHelper, Options *options [[gnu::unused]]) {
    tinyxml2::XMLElement* textureNodeAttribute = nullptr;

    OpenGLGraphics::TextureTypes textureType;
    OpenGLGraphics::InternalFormatTypes internalFormat;
    OpenGLGraphics::FormatTypes format;
    OpenGLGraphics::DataTypes dataType;
    uint32_t height, width;
    uint32_t depth;

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
        textureType = OpenGLGraphics::TextureTypes::T2D;
    } else if(textureTypeString == "T2D_ARRAY") {
        textureType = OpenGLGraphics::TextureTypes::T2D_ARRAY;
    } else if(textureTypeString == "TCUBE_MAP") {
        textureType = OpenGLGraphics::TextureTypes::TCUBE_MAP;
    } else if(textureTypeString == "TCUBE_MAP_ARRAY") {
        textureType = OpenGLGraphics::TextureTypes::TCUBE_MAP_ARRAY;
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
        internalFormat = OpenGLGraphics::InternalFormatTypes ::RED;
    } else if(internalFormatString == "RGB") {
        internalFormat = OpenGLGraphics::InternalFormatTypes ::RGB;
    } else if(internalFormatString == "RGBA") {
        internalFormat = OpenGLGraphics::InternalFormatTypes ::RGBA;
    } else if(internalFormatString == "RGB16F") {
        internalFormat = OpenGLGraphics::InternalFormatTypes ::RGB16F;
    } else if(internalFormatString == "RGB32F") {
        internalFormat = OpenGLGraphics::InternalFormatTypes ::RGB32F;
    } else if(internalFormatString == "DEPTH") {
        internalFormat = OpenGLGraphics::InternalFormatTypes ::DEPTH;
    } else {
        std::cerr << "Texture internal format is unknown, skipping! " << std::endl;
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
        format = OpenGLGraphics::FormatTypes::RED;
    } else if(formatString == "RGB") {
        format = OpenGLGraphics::FormatTypes::RGB;
    } else if(formatString == "RGBA") {
        format = OpenGLGraphics::FormatTypes::RGBA;
    } else if(formatString == "DEPTH") {
        format = OpenGLGraphics::FormatTypes::DEPTH;
    } else {
        std::cerr << "Texture format is unknown, skipping! " << std::endl;
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
        dataType = OpenGLGraphics::DataTypes::UNSIGNED_BYTE;
    } else if(dataTypeString == "FLOAT") {
        dataType = OpenGLGraphics::DataTypes::FLOAT;
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
    height = std::stoi(heightString);

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
    width = std::stoi(widthString);

    textureNodeAttribute = TextureNode->FirstChildElement("Depth");
    if (textureNodeAttribute == nullptr) {
        std::cerr << "Texture must have depth. Skipping" << std::endl;
        return nullptr;
    }
    if(textureNodeAttribute->GetText() == nullptr) {
        std::cerr << "Texture depth has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string depthString = textureNodeAttribute->GetText();
    depth = std::stoi(depthString);



    Texture* texture = new Texture(glHelper, textureType, internalFormat, format, dataType, width, height, depth);

    textureNodeAttribute = TextureNode->FirstChildElement("FilterMode");
    if (textureNodeAttribute != nullptr) {
        if (textureNodeAttribute->GetText() == nullptr) {
            std::cerr << "Texture filter mode has no text, using default value." << std::endl;
        } else {
            OpenGLGraphics::FilterModes filterMode;
            bool fail = false;
            std::string filterModeString = textureNodeAttribute->GetText();
            if (filterModeString == "NEAREST") {
                filterMode = OpenGLGraphics::FilterModes::NEAREST;
            } else if (filterModeString == "LINEAR") {
                filterMode = OpenGLGraphics::FilterModes::LINEAR;
            } else if (filterModeString == "TRILINEAR") {
                filterMode = OpenGLGraphics::FilterModes::TRILINEAR;
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
        OpenGLGraphics::TextureWrapModes wrapModeS, wrapModeT, wrapModeR;
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
                    wrapModeS = OpenGLGraphics::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeS = OpenGLGraphics::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeS = OpenGLGraphics::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeS = OpenGLGraphics::TextureWrapModes::REPEAT;
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
                    wrapModeT = OpenGLGraphics::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeT = OpenGLGraphics::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeT = OpenGLGraphics::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeT = OpenGLGraphics::TextureWrapModes::REPEAT;
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
                    wrapModeR = OpenGLGraphics::TextureWrapModes::NONE;
                } else if (wrapModeString == "BORDER") {
                    wrapModeR = OpenGLGraphics::TextureWrapModes::BORDER;
                } else if (wrapModeString == "EDGE") {
                    wrapModeR = OpenGLGraphics::TextureWrapModes::EDGE;
                } else if (wrapModeString == "REPEAT") {
                    wrapModeR = OpenGLGraphics::TextureWrapModes::REPEAT;
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
