//
// Created by Engin Manap on 28/10/2024.
//

#include "GraphicsProgramPreprocessor.h"
#include <fstream>
#include <streambuf>
#include <regex>

void GraphicsProgramPreprocessor::preprocess(GraphicsProgram *graphicsProgram, const std::string &headerString,
                                             const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> &variablesMap) {
    std::string shaderFiles[3];
    std::string shaderContents[3];
    shaderFiles[0] = graphicsProgram->getVertexShaderFile();
    shaderFiles[1] = graphicsProgram->getGeometryShaderFile();
    shaderFiles[2] = graphicsProgram->getFragmentShaderFile();

    for (int i = 0; i < 3; ++i) {
        if(shaderFiles[i].empty()) {
            shaderContents[i] = "";
            continue;
        }
        shaderContents[i] = readShaderCode(shaderFiles[i]);
        replaceDefinitions(shaderContents[i], variablesMap);
        replaceIncludes(shaderFiles[i], shaderContents[i]);
        addHeader(headerString, shaderContents[i]);
    }
    graphicsProgram->setVertexShaderContent(shaderContents[0]);
    graphicsProgram->setGeometryShaderContent(shaderContents[1]);
    graphicsProgram->setFragmentShaderContent(shaderContents[2]);
}

std::string GraphicsProgramPreprocessor::readShaderCode(const std::string& shaderFile){
    std::string shaderCode;
    std::ifstream shaderStream(shaderFile.c_str(), std::ios::in);

    if (shaderStream.is_open()) {
        shaderCode.assign((std::istreambuf_iterator<char>(shaderStream)),
                           std::istreambuf_iterator<char>());
        shaderStream.close();

        if(shaderCode.empty()){
            return "";
        }
        // Normalize line endings to \n
        shaderCode = std::regex_replace(shaderCode, std::regex("\\r\\n|\\r"), "\n");
    } else {
        std::cerr << shaderFile.c_str() << " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return "";
    }
    return "\n" + shaderCode;
}

void GraphicsProgramPreprocessor::replaceIncludes(const std::string &currentShaderFile, std::string &shaderCode) {
    std::regex relativeImportPattern("\\n#import \\\"([^\\\"]*)\\\"\\n");
    //there is a match, we should determine the current path because this pattern is relative
    size_t lastIndex = currentShaderFile.find_last_of('/');
    std::string currentPath;
    if(lastIndex != std::string::npos) {
        currentPath = currentShaderFile.substr(0, lastIndex+1);
    }
    replaceImportPattern(shaderCode, relativeImportPattern, currentPath);
    std::regex rootImportPattern("\\n#import \\<([^\\>]*)\\>\\n");
    replaceImportPattern(shaderCode, rootImportPattern, "");//root is not calculated in this case
}

void GraphicsProgramPreprocessor::replaceDefinitions(std::string &shaderCode, const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>& variablesMap) {
    std::regex definePattern("\\n#define_option (.*)\\n");

    std::smatch matches;
    while(std::regex_search(shaderCode, matches, definePattern)) {//while because changing the shader code invalidates the matches
        if (matches.size() > 1) {
            std::string optionName = matches[1].str();
            std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>::const_iterator optionIt = variablesMap.find(optionName);
            if(optionIt != variablesMap.cend()) {
                //now get the value of the option
                std::string optionStringValue = optionIt->second->to_string();
                if(optionStringValue.find('[') != optionStringValue.npos) {
                    //this contains [ and ], it needs to be removed to be used in shaders
                    optionStringValue = optionStringValue.substr(1, optionStringValue.length() - 2);
                }
                shaderCode.replace(matches[0].first, matches[0].second, "\n#define " + optionName + " " + optionStringValue + "\n");
            } else {
                //option not found, remove the whole line
                shaderCode.replace(matches[0].first, matches[0].second, "\n");
            }
        }
    }

}

void GraphicsProgramPreprocessor::replaceImportPattern(std::string &shaderCode, std::regex &importPattern, const std::string &currentPath) {
    std::smatch matches;
    while(std::regex_search(shaderCode, matches, importPattern)) {//while because changing the shader code invalidates the matches
        if (matches.size() > 1) {
            std::string importfile = matches[1].str();
            std::string importedShaderCode = readShaderCode(currentPath + importfile);
            if (!importedShaderCode.empty()) {
                importedShaderCode += "\n";//we add a new line incase the file ends without one
            }
            //now put the imported code in to the actual shader code
            shaderCode.replace(matches[0].first, matches[0].second, importedShaderCode);
        }
    }
}

void GraphicsProgramPreprocessor::addHeader(const std::string &headerString, std::string &shaderCode) {
    if (!headerString.empty() && shaderCode.rfind(headerString, 0) == 0) {
        // it already has the header string, don't add again.
        return;
    }

    // Since we normalized line endings to \n, we can safely add \n.
    shaderCode.insert(0, headerString + "\n");
}
