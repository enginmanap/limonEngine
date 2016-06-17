//
// Created by Engin Manap on 9.03.2016.
//

#ifndef UBERGAME_BULLETDEBUGDRAWER_H
#define UBERGAME_BULLETDEBUGDRAWER_H


#include <btBulletDynamicsCommon.h>
#include "GLHelper.h"
#include "GLSLProgram.h"
#include "Utils/GLMConverter.h"

class BulletDebugDrawer : public btIDebugDraw {
    DebugDrawModes currentMode;
    GLHelper *glHelper;
    std::vector<std::string> uniforms;
    GLSLProgram *renderProgram;
    GLuint vao, vbo, ebo;
public:
    BulletDebugDrawer(GLHelper *glHelper) : glHelper(glHelper), vao(0), vbo(0), ebo(0) {
        uniforms.push_back("cameraTransformMatrix");
        renderProgram = new GLSLProgram(glHelper, "./Data/Shaders/Line/vertex.shader",
                                        "./Data/Shaders/Line/fragment.shader", uniforms);
    }

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor);

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        drawLine(from, to, color, color);
    }

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
                          const btVector3 &fromColor, const btVector3 &toColor) {
        std::cerr << "Draw contact point requested, but not implemented." << std::endl;
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
    };

    void setDebugMode(int debugMode) { currentMode = DebugDrawModes(debugMode); };

    int getDebugMode() const { return currentMode; }
};


#endif //UBERGAME_BULLETDEBUGDRAWER_H
