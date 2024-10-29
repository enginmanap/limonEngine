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
        std::string Line;

        while (getline(shaderStream, Line))
            shaderCode += "\n" + Line;

        shaderStream.close();
    } else {
        std::cerr << shaderFile.c_str() << " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return "";
    }
    return shaderCode;
}

void GraphicsProgramPreprocessor::replaceIncludes(const std::string &currentShaderFile, std::string &shaderCode) {
    std::regex relativeImportPattern("\\n#import \\\".*\\\"\\n");
    //there is a match, we should determine the current path because this pattern is relative
    size_t lastIndex = currentShaderFile.find_last_of('/');
    std::string currentPath;
    if(lastIndex != std::string::npos) {
        currentPath = currentShaderFile.substr(0, lastIndex+1);
    }
    replaceImportPattern(shaderCode, relativeImportPattern, currentPath);
    std::regex rootImportPattern("\\n#import \\<.*\\>\\n");
    replaceImportPattern(shaderCode, rootImportPattern, "");//root is not calculated in this case
}

void GraphicsProgramPreprocessor::replaceDefinitions(std::string &shaderCode, const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>& variablesMap) {
    std::regex definePattern("\\n#define_limon .*\\n");

    std::smatch matches;
    while(std::regex_search(shaderCode, matches, definePattern)) {//while because changing the shader code invalidates the matches
        for(auto match: matches) {
            std::string optionName = match.str().substr(15, match.str().length() - (1 + 15));
            std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>::const_iterator optionIt = variablesMap.find(optionName);
            if(optionIt != variablesMap.cend()) {
                //now get the value of the option
                std::string optionStringValue = optionIt->second->to_string();
                if(optionStringValue.find('[') != optionStringValue.npos) {
                    //this contains [ and ], it needs to be removed to be used in shaders
                    optionStringValue = optionStringValue.substr(1, optionStringValue.length() - 2);
                }
                shaderCode.replace(match.first, match.second, "\n#define " + optionName + " " + optionStringValue + "\n");
                break;
            }
        }
    }

}

void GraphicsProgramPreprocessor::replaceImportPattern(std::string &shaderCode, std::regex &importPattern, const std::string &currentPath) {
    std::smatch matches;
    while(std::regex_search(shaderCode, matches, importPattern)) {//while because changing the shader code invalidates the matches
        for(auto match: matches) {
            std::string importfile = match.str().substr(10, match.str().length()-(2+10));
            std::string importedShaderCode = readShaderCode(currentPath + importfile) + "\n";//we add a new line incase the file ends without one
            //now put the imported code in to the actual shader code
            shaderCode.replace(match.first, match.second, importedShaderCode);
            break;
        }
    }
}

void GraphicsProgramPreprocessor::addHeader(const std::string &headerString, std::string &shaderCode) {
    std::regex headerPositionFinder("\\n*" + headerString);
    std::smatch matches;

    if(std::regex_search(shaderCode, matches, headerPositionFinder)) {
        //it already has the header string, don't add again.
        return;
    }
    //First check if the header is multi line
    if(headerString.find("\n") != headerString.npos) {
        std::regex newLineReplacer("\\n");
        std::regex_replace(headerString, newLineReplacer, "\\\\n");
    }
    //now we have the header, add it to the shader
    shaderCode = headerString + "\n" + shaderCode;
}
