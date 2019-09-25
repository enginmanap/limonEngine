//
// Created by Engin Manap on 2.03.2016.
//

#include "GraphicsProgram.h"

GraphicsProgram::GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed) :
        graphicsWrapper(graphicsWrapper), vertexShader(vertexShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader +"|"+ fragmentShader;
    //FIXME is passing empty string acceptable?
    programID = graphicsWrapper->initializeProgram(vertexShader, "", fragmentShader, uniformMap, outputMap);
}

GraphicsProgram::GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed) :
        graphicsWrapper(graphicsWrapper), vertexShader(vertexShader), geometryShader(geometryShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader +"|"+ geometryShader +"|"+ fragmentShader;
    programID = graphicsWrapper->initializeProgram(vertexShader, geometryShader, fragmentShader, uniformMap, outputMap);
}

GraphicsProgram::~GraphicsProgram() {
    graphicsWrapper->destroyProgram(programID);
}