//
// Created by Engin Manap on 3.03.2016.
//

#ifndef UBERGAME_RENDERABLE_H
#define UBERGAME_RENDERABLE_H


#include "GLHelper.h"
#include "GLSLProgram.h"
#include <btBulletDynamicsCommon.h>
#include "glm/gtx/matrix_decompose.hpp"

class Renderable {
protected:
    GLHelper* glHelper;
    GLuint vao, vbo, ebo;
    GLSLProgram* renderProgram;
    std::vector<std::string> uniforms;
    glm::mat4 worldTransform, oldWorldTransform;
    glm::vec3 scale, translate;
    glm::quat orientation;
    bool isDirty;

    void generateWorldTransform() ;


    Renderable(GLHelper* glHelper);
    btRigidBody* rigidBody;

public:
    void addScale(const glm::vec3& scale){
        this->scale *= scale;
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
        isDirty=true;
    }
    void addTranslate(const glm::vec3& translate){
        this->translate += translate;
        btTransform transform =  this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x,this->translate.y,this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        //this->rigidBody->setCenterOfMassTransform(transform);
        isDirty=true;
    }
    void addOrientation(const glm::quat& orientation){
        this->orientation *= orientation;
        this->orientation = glm::normalize(this->orientation);
        std::cerr << "not handled case for physics" << std::endl;
        isDirty=true;
    }

    const glm::mat4 &getWorldTransform() {
        if(isDirty) {
            generateWorldTransform();
        }
        return worldTransform;
    }

    virtual void render() = 0;

    //FIXME this should be removed
    void setWorldTransform(const glm::mat4& transformMatrix){
        this->oldWorldTransform = this->worldTransform;
        this->worldTransform = transformMatrix;
        //these 2 values are not used afterwards
        glm::vec3 tempSkew;
        glm::vec4 tempPerspective;
        glm::decompose(transformMatrix, scale, orientation, translate, tempSkew, tempPerspective);
        btQuaternion tempOrientation(this->orientation.x, this->orientation.y, this->orientation.z, this->orientation.w);
        btVector3 tempTranslate(this->translate.x, this->translate.y, this->translate.z);
        this->rigidBody->setCenterOfMassTransform(btTransform(tempOrientation, tempTranslate));
        this->isDirty = false;
    }

    btRigidBody* getRigidBody() { return rigidBody;};
    void updateTransformFromPhysics();
};


#endif //UBERGAME_RENDERABLE_H
