//
// Created by Engin Manap on 28.09.2019.
//

#include "GraphicsInterface.h"
#include "GraphicsProgram.h"

std::shared_ptr<GraphicsProgram> GraphicsInterface::createGraphicsProgramInternal(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed, std::function<void(GraphicsProgram*)> deleterMethod) {
    return std::shared_ptr<GraphicsProgram>(new GraphicsProgram(this, vertexShader, geometryShader, fragmentShader, isMaterialUsed), deleterMethod);
}
std::shared_ptr<GraphicsProgram> GraphicsInterface::createGraphicsProgramInternal(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed, std::function<void(GraphicsProgram*)> deleterMethod) {
    return std::shared_ptr<GraphicsProgram>(new GraphicsProgram(this, vertexShader, fragmentShader, isMaterialUsed), deleterMethod);
}
