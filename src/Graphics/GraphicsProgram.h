//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAM_H
#define LIMONENGINE_GRAPHICSPROGRAM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "API/GraphicsInterface.h"
#include "OpenGLGraphics.h"


class GraphicsProgram {
    GraphicsInterface* graphicsWrapper;
    std::string programName;

    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;
    std::unordered_map<std::string, const GraphicsInterface::Uniform *> uniformMap;
    std::unordered_map<std::string, GraphicsInterface::VariableTypes>outputMap;
    bool materialRequired;
    uint32_t programID;

    GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed);
    GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed);

public:

    ~GraphicsProgram();

    friend std::shared_ptr<GraphicsProgram> GraphicsInterface::createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed);
    friend std::shared_ptr<GraphicsProgram> GraphicsInterface::createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed);


    friend std::shared_ptr<GraphicsProgram> OpenGLGraphics::createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed);
    friend std::shared_ptr<GraphicsProgram> OpenGLGraphics::createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed);


    uint32_t getID() const { return programID; }

    const std::unordered_map<std::string, const GraphicsInterface::Uniform *> &getUniformMap() const {
        return uniformMap;
    }

    const std::unordered_map<std::string, GraphicsInterface::VariableTypes> &getOutputMap() const {
        return outputMap;
    }

    bool setUniform(const std::string &uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GraphicsInterface::FLOAT_MAT4) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GraphicsInterface::FLOAT_VEC3) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GraphicsInterface::FLOAT_VEC3) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, vectorArray);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GraphicsInterface::FLOAT) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }
/**
 * This method is used to set samplers, so it can alter int uniforms, and sampler uniforms.
 * @param uniformName
 * @param value
 * @return
 */
    bool setUniform(const std::string &uniformName, const int value) {
        if (uniformMap.count(uniformName) &&
                    (uniformMap[uniformName]->type == GraphicsInterface::INT ||
                     uniformMap[uniformName]->type == GraphicsInterface::CUBEMAP ||
                     uniformMap[uniformName]->type == GraphicsInterface::CUBEMAP_ARRAY ||
                     uniformMap[uniformName]->type == GraphicsInterface::TEXTURE_2D ||
                     uniformMap[uniformName]->type == GraphicsInterface::TEXTURE_2D_ARRAY))  {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

    bool setUniformArray(const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        if (uniformMap.count(uniformArrayName) && uniformMap[uniformArrayName]->type == GraphicsInterface::FLOAT_MAT4) {
            //FIXME this should have a control of some sort
            return graphicsWrapper->setUniformArray(programID, uniformMap[uniformArrayName]->location, matrix);
        }
        return false;
    }

    const std::string &getProgramName() const {
        return programName;
    }

    bool IsMaterialRequired() const {
        return materialRequired;
    }

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) {
        tinyxml2::XMLElement *programNode = document.NewElement("GraphicsProgram");
        parentNode->InsertEndChild(programNode);
        tinyxml2::XMLElement *currentElement = nullptr;

        currentElement = document.NewElement("VertexShader");
        currentElement->SetText(this->vertexShader.c_str());
        programNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("GeometryShader");
        currentElement->SetText(this->geometryShader.c_str());
        programNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("FragmentShader");
        currentElement->SetText(this->fragmentShader.c_str());
        programNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("MaterialRequired");
        if(materialRequired) {
            currentElement->SetText("True");
        } else {
            currentElement->SetText("False");
        }
        return true;
    }

    static std::shared_ptr<GraphicsProgram> deserialize(tinyxml2::XMLElement *programNode, GraphicsInterface* graphicsWrapper) {
        std::string vertexShader;
        std::string geometryShader;
        std::string fragmentShader;

        tinyxml2::XMLElement* programNodeAttribute = programNode->FirstChildElement("VertexShader");
        if (programNodeAttribute != nullptr) {
            if(programNodeAttribute->GetText() == nullptr) {
                std::cerr << "GLSL Program vertex shader has no text, this case is not handled!" << std::endl;
                return nullptr;
            } else {
                vertexShader = programNodeAttribute->GetText();
            }
        }

        programNodeAttribute = programNode->FirstChildElement("GeometryShader");
        if (programNodeAttribute != nullptr) {
            if(programNodeAttribute->GetText() == nullptr) {
                std::cout << "GLSL Program geometry shader has no text." << std::endl;
            } else {
                geometryShader = programNodeAttribute->GetText();
            }
        }

        programNodeAttribute = programNode->FirstChildElement("VertexShader");
        if (programNodeAttribute != nullptr) {
            if(programNodeAttribute->GetText() == nullptr) {
                std::cerr << "GLSL Program vertex shader has no text, this case is not handled!" << std::endl;
                return nullptr;
            } else {
                fragmentShader = programNodeAttribute->GetText();
            }
        }

        bool materialRequired = false;
        programNodeAttribute = programNode->FirstChildElement("MaterialRequired");
        if (programNodeAttribute != nullptr) {
            if(programNodeAttribute->GetText() == nullptr) {
                std::cerr << "GLSL Program material required flag couldn't be read, assuming no!" << std::endl;
            } else {
                std::string materialRequiredString = programNodeAttribute->GetText();
                if(materialRequiredString == "True") {
                    materialRequired = true;
                } else if(materialRequiredString == "False") {
                    materialRequired = false;
                } else {
                    std::cerr << "GLSL Program material required flag is unknown, assuming no!" << std::endl;
                }
            }
        } else {
            std::cerr << "GLSL Program material required flag not found, assuming no!" << std::endl;
        }

        if(geometryShader.length() > 0 ) {
            return graphicsWrapper->createGLSLProgram(vertexShader, geometryShader, fragmentShader, materialRequired);
        } else {
            return graphicsWrapper->createGLSLProgram(vertexShader, fragmentShader, materialRequired);
        }
    }

};



#endif //LIMONENGINE_GRAPHICSPROGRAM_H
