//
// Created by Engin Manap on 9.03.2016.
//

#include "BulletDebugDrawer.h"


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
