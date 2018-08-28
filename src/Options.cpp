//
// Created by engin on 24.06.2018.
//

#include "Options.h"

void Options::loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3 &vector) {
    tinyxml2::XMLElement *vectorNode = optionsNode->FirstChildElement(name.c_str());
    //get the 3 elements of vector;
    tinyxml2::XMLElement *vectorElementNode = vectorNode->FirstChildElement("X");
    if(vectorElementNode != nullptr) {
        vector.x = std::stof(vectorElementNode->GetText());
    } else {
        vector.x = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Y");
    if(vectorElementNode != nullptr) {
        vector.y = std::stof(vectorElementNode->GetText());
    } else {
        vector.y = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Z");
    if(vectorElementNode != nullptr) {
        vector.z = std::stof(vectorElementNode->GetText());
    } else {
        vector.z = 0;
    }
}

void Options::loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4 &vector) {
    tinyxml2::XMLElement *vectorNode = optionsNode->FirstChildElement(name.c_str());
    //get the 3 elements of vector;
    tinyxml2::XMLElement *vectorElementNode = vectorNode->FirstChildElement("X");
    if(vectorElementNode != nullptr) {
        vector.x = std::stof(vectorElementNode->GetText());
    } else {
        vector.x = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Y");
    if(vectorElementNode != nullptr) {
        vector.y = std::stof(vectorElementNode->GetText());
    } else {
        vector.y = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Z");
    if(vectorElementNode != nullptr) {
        vector.z = std::stof(vectorElementNode->GetText());
    } else {
        vector.z = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("W");
    if(vectorElementNode != nullptr) {
        vector.w = std::stof(vectorElementNode->GetText());
    } else {
        vector.w = 0;
    }
}

bool Options::loadOptions(const std::string &optionsFileName) {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(optionsFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML " << optionsFileName << ": " << xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode *optionsNode = xmlDoc.FirstChild();
    if (optionsNode == nullptr) {
        std::cerr << "options xml is not a valid XML." << std::endl;
        return false;
    }

    tinyxml2::XMLElement *screenHeightNode = optionsNode->FirstChildElement("screenHeight");
    if (screenHeightNode != nullptr) {
        screenHeight = std::stoi(screenHeightNode->GetText());
    }

    tinyxml2::XMLElement *screenWidthNode = optionsNode->FirstChildElement("screenWidth");
    if (screenWidthNode != nullptr) {
        screenWidth = std::stoul(screenWidthNode->GetText());
    }

    tinyxml2::XMLElement *shadowMapDirectionalWidthNode = optionsNode->FirstChildElement(
            "shadowMapDirectionalWidth");
    if (shadowMapDirectionalWidthNode != nullptr) {
        shadowMapDirectionalWidth = std::stoul(shadowMapDirectionalWidthNode->GetText());
    }

    tinyxml2::XMLElement *shadowMapDirectionalHeightNode = optionsNode->FirstChildElement(
            "shadowMapDirectionalHeight");
    if (shadowMapDirectionalHeightNode != nullptr) {
        shadowMapDirectionalHeight = std::stoul(shadowMapDirectionalHeightNode->GetText());
    }

    tinyxml2::XMLElement *shadowMapPointWidthNode = optionsNode->FirstChildElement("shadowMapPointWidth");
    if (shadowMapPointWidthNode != nullptr) {
        shadowMapPointWidth = std::stoul(shadowMapPointWidthNode->GetText());
    }

    tinyxml2::XMLElement *shadowMapPointHeightNode = optionsNode->FirstChildElement("shadowMapPointHeight");
    if (shadowMapPointHeightNode != nullptr) {
        shadowMapPointHeight = std::stoul(shadowMapPointHeightNode->GetText());
    }

    tinyxml2::XMLElement *debugDrawBufferSizeNode = optionsNode->FirstChildElement("debugDrawBufferSize");
    if (debugDrawBufferSizeNode != nullptr) {
        debugDrawBufferSize = std::stoul(debugDrawBufferSizeNode->GetText());
    }

    tinyxml2::XMLElement *jumpFactorNode = optionsNode->FirstChildElement("jumpFactor");
    if (jumpFactorNode != nullptr) {
        jumpFactor = std::stof(jumpFactorNode->GetText());
    }

    tinyxml2::XMLElement *lookAroundSpeedNode = optionsNode->FirstChildElement("lookAroundSpeed");
    if (lookAroundSpeedNode != nullptr) {
        lookAroundSpeed = std::stof(lookAroundSpeedNode->GetText());
    }

    tinyxml2::XMLElement *lightOrthogonalProjectionNearPlaneNode = optionsNode->FirstChildElement(
            "lightOrthogonalProjectionNearPlane");
    if (lightOrthogonalProjectionNearPlaneNode != nullptr) {
        lightOrthogonalProjectionNearPlane = std::stof(lightOrthogonalProjectionNearPlaneNode->GetText());
    }

    tinyxml2::XMLElement *lightOrthogonalProjectionFarPlaneNode = optionsNode->FirstChildElement(
            "lightOrthogonalProjectionFarPlane");
    if (lightOrthogonalProjectionFarPlaneNode != nullptr) {
        lightOrthogonalProjectionFarPlane = std::stof(lightOrthogonalProjectionFarPlaneNode->GetText());
    }

    tinyxml2::XMLElement *lightPerspectiveProjectionNearPlaneNode = optionsNode->FirstChildElement(
            "lightPerspectiveProjectionNearPlane");
    if (lightPerspectiveProjectionNearPlaneNode != nullptr) {
        lightPerspectiveProjectionNearPlane = std::stof(lightPerspectiveProjectionNearPlaneNode->GetText());
    }

    tinyxml2::XMLElement *lightPerspectiveProjectionFarPlaneNode = optionsNode->FirstChildElement(
            "lightPerspectiveProjectionFarPlane");
    if (lightPerspectiveProjectionFarPlaneNode != nullptr) {
        lightPerspectiveProjectionFarPlane = std::stof(lightPerspectiveProjectionFarPlaneNode->GetText());
    }

    tinyxml2::XMLElement *textureFilteringNode = optionsNode->FirstChildElement(
            "TextureFiltering");
    if (textureFilteringNode != nullptr) {
        if(textureFilteringNode->GetText() != nullptr) {
            std::string lightMode = textureFilteringNode->GetText();
            if( lightMode == "Nearest") {
                currentTextureFilteringMode = TextureFilteringModes::NEAREST;
            } else if(lightMode == "Bilinear") {
                currentTextureFilteringMode = TextureFilteringModes::BILINEAR;
            } else if(lightMode  == "Trilinear") {
                currentTextureFilteringMode = TextureFilteringModes::TRILINEAR;
            } else {
              std::cerr << "Texture filtering mode can't be read, assuming Trilinear" << std::endl;
            }
        }
    }

    loadVec3(optionsNode, "walkSpeed", walkSpeed);
    loadVec3(optionsNode, "runSpeed", runSpeed);
    loadVec3(optionsNode, "freeMovementSpeed", freeMovementSpeed);

    loadVec4(optionsNode, "lightOrthogonalProjectionValues", lightOrthogonalProjectionValues);

    return false;
}