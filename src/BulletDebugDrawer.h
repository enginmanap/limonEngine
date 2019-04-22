//
// Created by Engin Manap on 9.03.2016.
//

#ifndef LIMONENGINE_BULLETDEBUGDRAWER_H
#define LIMONENGINE_BULLETDEBUGDRAWER_H


#include <btBulletDynamicsCommon.h>
#include <vector>

#include "Graphics/GLHelper.h"
#include "Graphics/GLSLProgram.h"
#include "Utils/GLMConverter.h"
#include "Options.h"

class BulletDebugDrawer : public btIDebugDraw {
    DebugDrawModes currentMode;
    GLHelper *glHelper;
    std::shared_ptr<GLSLProgram> renderProgram;
    GLuint vao, vbo, ebo;
    std::vector<Line> lineBuffer;
    Options* options;

public:
    BulletDebugDrawer(GLHelper *glHelper, Options* options) : glHelper(glHelper), vao(0), vbo(0), ebo(0), options(options) {
        renderProgram = glHelper->createGLSLProgram("./Engine/Shaders/Lines/vertex.glsl",
                                        "./Engine/Shaders/Lines/fragment.glsl", false);
        //std::cout << "Render program is ready with id " << renderProgram->getID() << std::endl;
        glHelper->createDebugVAOVBO(vao, vbo, options->getDebugDrawBufferSize());
    }

    void drawLine(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &fromColor, const glm::vec3 &toColor, bool needsCameraTransform);

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor, bool needsCameraTransform) {
        drawLine(GLMConverter::BltToGLM(from),
                 GLMConverter::BltToGLM(to),
                 GLMConverter::BltToGLM(fromColor),
                 GLMConverter::BltToGLM(toColor),
                 needsCameraTransform);
    };

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor) {
        drawLine(from, to, fromColor, toColor, true);
    };

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        drawLine(from, to, color, color, true);
    }

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
                          const btVector3 &fromColor, const btVector3 &toColor) {
        std::cerr << "Draw contact point requested, but not implemented." << std::endl;
        std::cerr << "called with parameters: " << PointOnB << ", " << normalOnB << ", " << distance << ", " << lifeTime << ", " << fromColor << ", " << toColor << std::endl;
    };

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
                          const btVector3 &color) {
        drawContactPoint(PointOnB, normalOnB, distance, lifeTime, color, color);
    };

    void reportErrorWarning(const char *warningString) {
        std::cerr << "Bullet warning: " << warningString << std::endl;
    };

    void draw3dText(const btVector3 &location, const char *textString) {
        std::cerr << "Draw 3D text requested, but not implemented." << std::endl;
        std::cout << "called with parameters: " << location << ", " << textString << std::endl;
    };

    void setDebugMode(int debugMode) { currentMode = DebugDrawModes(debugMode); };

    int getDebugMode() const { return currentMode; }

    void flushDraws();
};


#endif //LIMONENGINE_BULLETDEBUGDRAWER_H
