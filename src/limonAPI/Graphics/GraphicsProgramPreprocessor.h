//
// Created by Engin Manap on 28/10/2024.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAMPREPROCESSOR_H
#define LIMONENGINE_GRAPHICSPROGRAMPREPROCESSOR_H


#include <regex>
#include "GraphicsProgram.h"

class GraphicsProgramPreprocessor {
public:
    /*
     * This preprocessor supports 3 functionality:
     * 1) replace #include "" to contents of the file in same directory
     * or replace #include <> to contents of the file in given path, relative to run directory
     * 2) replace #define_option X to #define X Y, if X is defined in options
     * 3) add header definition from loaded backend to the beginning of the file. This can be multiline
     */

    static void preprocess(GraphicsProgram *graphicsProgram, const std::string &headerString,
                           const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> &variablesMap);

private:
    static std::string readShaderCode(const std::string& shaderFile);

    static void replaceIncludes(const std::string &currentShaderFile, std::string &shaderCode);
    static void replaceDefinitions(std::string &shaderCode, const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>>& variablesMap);
    static void addHeader(const std::string &headerString, std::string &shaderCode);

    static void replaceImportPattern(std::string &shaderCode, std::basic_regex<char> &importPattern, const std::string &currentPath);
};


#endif //LIMONENGINE_GRAPHICSPROGRAMPREPROCESSOR_H
