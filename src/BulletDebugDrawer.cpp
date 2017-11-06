//
// Created by Engin Manap on 9.03.2016.
//

#include "BulletDebugDrawer.h"


void BulletDebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor,
                                 const btVector3 &toColor) {
    lineBuffer.push_back(Line(GLMConverter::BltToGLM(from),
                              GLMConverter::BltToGLM(to),
                              GLMConverter::BltToGLM(fromColor),
                              GLMConverter::BltToGLM(toColor)));
    if(lineBuffer.size() == options->getDebugDrawBufferSize()) {
      flushDraws();
    }

}

void BulletDebugDrawer::flushDraws() {
    if(lineBuffer.size() > 0) {
        glHelper->drawLines(*renderProgram, vao, vbo, lineBuffer);
        lineBuffer.clear();
    }
}
