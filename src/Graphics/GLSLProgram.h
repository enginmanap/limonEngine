//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GLSLPROGRAM_H
#define LIMONENGINE_GLSLPROGRAM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "OpenGLGraphics.h"


class GLSLProgram {
    OpenGLGraphics *glHelper;
    std::string programName;

    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;
    std::unordered_map<std::string, const OpenGLGraphics::Uniform *> uniformMap;
    std::unordered_map<std::string, OpenGLGraphics::VariableTypes>outputMap;
    bool materialRequired;
    GLuint programID;

    GLSLProgram(OpenGLGraphics *glHelper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed);
    GLSLProgram(OpenGLGraphics *glHelper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed);

public:

    ~GLSLProgram();

    friend std::shared_ptr<GLSLProgram> OpenGLGraphics::createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed);
    friend std::shared_ptr<GLSLProgram> OpenGLGraphics::createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed);

    GLuint getID() const { return programID; }

    const std::unordered_map<std::string, const OpenGLGraphics::Uniform *> &getUniformMap() const {
        return uniformMap;
    }

    const std::unordered_map<std::string, OpenGLGraphics::VariableTypes> &getOutputMap() const {
        return outputMap;
    }

    bool setUniform(const std::string &uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == OpenGLGraphics::FLOAT_MAT4) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == OpenGLGraphics::FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == OpenGLGraphics::FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vectorArray);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == OpenGLGraphics::FLOAT) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
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
                    (uniformMap[uniformName]->type == OpenGLGraphics::INT ||
                     uniformMap[uniformName]->type == OpenGLGraphics::CUBEMAP ||
                     uniformMap[uniformName]->type == OpenGLGraphics::CUBEMAP_ARRAY ||
                     uniformMap[uniformName]->type == OpenGLGraphics::TEXTURE_2D ||
                     uniformMap[uniformName]->type == OpenGLGraphics::TEXTURE_2D_ARRAY))  {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

    bool setUniformArray(const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        if (uniformMap.count(uniformArrayName) && uniformMap[uniformArrayName]->type == OpenGLGraphics::FLOAT_MAT4) {
            //FIXME this should have a control of some sort
            return glHelper->setUniformArray(programID, uniformMap[uniformArrayName]->location, matrix);
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
        tinyxml2::XMLElement *programNode = document.NewElement("GLSLProgram");
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

    static std::shared_ptr<GLSLProgram> deserialize(tinyxml2::XMLElement *programNode, OpenGLGraphics *glHelper) {
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
            return glHelper->createGLSLProgram(vertexShader, geometryShader, fragmentShader, materialRequired);
        } else {
            return glHelper->createGLSLProgram(vertexShader, fragmentShader, materialRequired);
        }
    }

};



#endif //LIMONENGINE_GLSLPROGRAM_H
