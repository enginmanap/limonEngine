//
// Created by engin-evam on 9.03.2016.
//

#ifndef UBERGAME_BULLETDEBUGDRAWER_H
#define UBERGAME_BULLETDEBUGDRAWER_H


#include <btBulletDynamicsCommon.h>
#include "GLHelper.h"
#include "GLSLProgram.h"
#include "Utils/BulletGLMConverter.h"

class BulletDebugDrawer : public btIDebugDraw {
    GLHelper* glHelper;
    std::vector<std::string> uniforms;
    GLSLProgram* renderProgram;
    GLuint vao,vbo, ebo;
public:
    BulletDebugDrawer(GLHelper* glHelper):glHelper(glHelper), vao(0),vbo(0), ebo(0) {
        uniforms.push_back("cameraTransformMatrix");
        renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/Line/vertex.shader","./Data/Shaders/Line/fragment.shader",uniforms);
    }

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
    void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& fromColor, const btVector3& toColor){};
    void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color) {};
    void reportErrorWarning(const char* warningString){};
    void draw3dText(const btVector3& location,const char* textString){};
    void setDebugMode(int debugMode){};
    int getDebugMode() const {return 1;}
};


#endif //UBERGAME_BULLETDEBUGDRAWER_H
