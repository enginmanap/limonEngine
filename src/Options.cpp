//
// Created by engin on 24.06.2018.
//

#include <memory>
#include <GamePlay/APISerializer.h>
#include "limonAPI/Options.h"

#include "Utils/GLMConverter.h"
#include "Utils/GLMUtils.h"
#include "Utils/HashUtil.h"
#include "Utils/StringUtils.hpp"

bool OptionsUtil::Options::loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3 &vector) {
    tinyxml2::XMLElement *vectorNode = optionsNode->FirstChildElement(name.c_str());
    if(vectorNode == nullptr) {
        return false;
    }
    //get the 3 elements of vector;
    tinyxml2::XMLElement *vectorElementNode = vectorNode->FirstChildElement("X");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.x = std::stof(vectorElementNode->GetText());
    } else {
        vector.x = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Y");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.y = std::stof(vectorElementNode->GetText());
    } else {
        vector.y = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Z");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.z = std::stof(vectorElementNode->GetText());
    } else {
        vector.z = 0;
    }
    return true;
}

bool OptionsUtil::Options::loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4 &vector) {
    tinyxml2::XMLElement *vectorNode = optionsNode->FirstChildElement(name.c_str());
    //get the 4 elements of vector;
    if(vectorNode == nullptr) {
        return false;
    }
    tinyxml2::XMLElement *vectorElementNode = vectorNode->FirstChildElement("X");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.x = std::stof(vectorElementNode->GetText());
    } else {
        vector.x = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Y");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.y = std::stof(vectorElementNode->GetText());
    } else {
        vector.y = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Z");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.z = std::stof(vectorElementNode->GetText());
    } else {
        vector.z = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("W");
    if(vectorElementNode != nullptr && vectorElementNode->GetText() != nullptr) {
        vector.w = std::stof(vectorElementNode->GetText());
    } else {
        vector.w = 0;
    }
    return true;
}

bool OptionsUtil::Options::loadDouble(tinyxml2::XMLNode *optionsNode, const std::string &name, double& value) {
    tinyxml2::XMLElement *doubleNode = optionsNode->FirstChildElement(name.c_str());
    if(doubleNode == nullptr || doubleNode->GetText() == nullptr) {
        return false;
    }
    value = std::stod(doubleNode->GetText());
    return true;
}

bool OptionsUtil::Options::loadLong(tinyxml2::XMLNode *optionsNode, const std::string &name, long& value) {
    tinyxml2::XMLElement *longNode = optionsNode->FirstChildElement(name.c_str());
    if(longNode == nullptr || longNode->GetText() == nullptr) {
        return false;
    }
    value = std::stol(longNode->GetText());
    return true;
}

bool OptionsUtil::Options::loadString(tinyxml2::XMLNode *optionsNode, const std::string &name, std::string& value) {
    tinyxml2::XMLElement *stringNode = optionsNode->FirstChildElement(name.c_str());
    if(stringNode == nullptr || stringNode->GetText() == nullptr) {
        return false;
    }
    value = stringNode->GetText();
    return true;
}

bool OptionsUtil::Options::loadBool(tinyxml2::XMLNode *optionsNode, const std::string &name, bool& value) {
    tinyxml2::XMLElement *boolNode = optionsNode->FirstChildElement(name.c_str());
    if(boolNode == nullptr || boolNode->GetText() == nullptr) {
        return false;
    }
    if(std::string("True").compare(boolNode->GetText()) == 0) {
        value = true;
    } else if(std::string("False").compare(boolNode->GetText()) == 0) {
        value = false;
    } else {
        return false;
    }
    return true;
}

bool OptionsUtil::Options::loadOptionsNew(const std::string &optionsFileName) {

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(optionsFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML " << optionsFileName << ": " << xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode *allOptionsNode = xmlDoc.FirstChild();
    if (allOptionsNode == nullptr) {
        std::cerr << "options xml is not a valid XML." << std::endl;
        return false;
    }
    LimonTypes::GenericParameter option;
    tinyxml2::XMLElement *optionNode = allOptionsNode->FirstChildElement("Parameter");
    uint32_t index;
    while(optionNode != nullptr) {
        std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(optionNode, index);
        if(request == nullptr) {
            return false;
        } else {
            options[hash(request->description)] = request;
            std::string value;
            switch (request->valueType){
            case LimonTypes::GenericParameter::STRING:
                {
                    Option<std::string> readOption = getOption<std::string>(hash(request->description));
                    value = readOption.get<std::string>();
                }
                break;
            case LimonTypes::GenericParameter::DOUBLE:
                {
                    Option<double> readOption = getOption<double>(hash(request->description));
                    value = std::to_string(readOption.get<double>());
                }
                break;
            case LimonTypes::GenericParameter::LONG:
                {
                    Option<long> readOption = getOption<long>(hash(request->description));
                    value = std::to_string(readOption.get<long>());
                }
                break;
            case LimonTypes::GenericParameter::LONG_ARRAY:
                {
                    std::stringstream ss;
                    Option<std::vector<long>> readOption = getOption<std::vector<long>>(hash(request->description));
                    std::vector<long> valueList = readOption.get<std::vector<long>>();
                    for (long i : valueList)
                    {
                        ss << std::to_string(i);
                        ss << ", ";
                    }
                    value = ss.str();
                }
                break;
            case LimonTypes::GenericParameter::BOOLEAN:
                {
                    Option<bool> readOption = getOption<bool>(hash(request->description));
                    if (readOption.get<bool>())
                    {
                        value = "true";
                    } else
                    {
                        value = "false";
                    }
                }
                break;
            case LimonTypes::GenericParameter::VEC4:
                {
                    Option<LimonTypes::Vec4> readOption = getOption<LimonTypes::Vec4>(hash(request->description));
                    value = GLMUtils::vectorToString(GLMConverter::LimonToGLM(readOption.get<LimonTypes::Vec4>()));
                }
                break;
            case LimonTypes::GenericParameter::MAT4:
                {
                    Option<LimonTypes::Mat4> readOption = getOption<LimonTypes::Mat4>(hash(request->description));
                    value = GLMUtils::matrixToString(GLMConverter::LimonToGLM(readOption.get<LimonTypes::Mat4>()));
                }
                break;
            default:
                value = "unknown"
                ;
            }

            std::cout << "Loaded option " << request->description << " with value " << value << std::endl;
        }
        optionNode = optionNode->NextSiblingElement("Parameter");
    }
    heightOption = getOption<long>(HASH("screenHeight"));
    widthOption = getOption<long>(HASH("screenWidth"));
    return true;
}