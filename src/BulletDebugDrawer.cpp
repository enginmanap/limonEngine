//
// Created by Engin Manap on 9.03.2016.
//

#include "BulletDebugDrawer.h"
#include "Assets/AssetManager.h"

BulletDebugDrawer::BulletDebugDrawer(std::shared_ptr<AssetManager> assetManager, Options* options) : assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), vao(0), vbo(0), ebo(0), options(options) {
        renderProgram = std::make_shared<GraphicsProgram>(assetManager.get(), "./Engine/Shaders/Lines/vertex.glsl",
                                                          "./Engine/Shaders/Lines/fragment.glsl", false);
        std::cout << "Render program is ready with id " << renderProgram->getID() << std::endl;
        graphicsWrapper->createDebugVAOVBO(vao, vbo, options->getDebugDrawBufferSize());
}

void BulletDebugDrawer::drawLine(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &fromColor,
                                 const glm::vec3 &toColor, bool needsCameraTransform) {
    lineBuffer.push_back(Line(from,
                              to,
                              fromColor,
                              toColor, needsCameraTransform));
    if(lineBuffer.size() == options->getDebugDrawBufferSize()) {
      flushDraws();
    }

}

void BulletDebugDrawer::flushDraws() {
    if(!lineBuffer.empty()) {
        graphicsWrapper->drawLines(*renderProgram, vao, vbo, lineBuffer);
        lineBuffer.clear();
    }
}
