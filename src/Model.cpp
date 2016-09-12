//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"


Model::Model(AssetManager *assetManager, const float mass, const std::string &modelFile) :
        PhysicalRenderable(assetManager->getGlHelper()) {

    modelAsset = assetManager->loadAsset<ModelAsset>({modelFile});
    //set up the rigid body

    this->triangleCount = modelAsset->getTriangleCount();
    this->centerOffset = modelAsset->getCenterOffset();

    bulletConvexShape = modelAsset->getCollisionMesh();

    btDefaultMotionState *boxMotionState = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), GLMConverter::GLMToBlt(centerOffset)));
    btVector3 fallInertia(0, 0, 0);
    bulletConvexShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo *rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(
            mass, boxMotionState, bulletConvexShape,
            fallInertia);

    this->vao = modelAsset->getVao();
    this->ebo = modelAsset->getEbo();
    this->materialMap = modelAsset->getMaterialMap();

    rigidBody = new btRigidBody(*rigidBodyConstructionInfo);
    //TODO check if this values are too low.
    rigidBody->setSleepingThresholds(0.1, 0.1);

    //glHelper->bufferVertexColor(colors,vao,vbo,3);
    worldTransform = glm::mat4(1.0f);

    //set up the program to render object
    renderProgram = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertex.glsl",
                                    "./Data/Shaders/Model/fragment.glsl");


}

void Model::activateMaterial(const Material *material) {
    GLuint location;
    if (!renderProgram->setUniform("material.ambient", material->getAmbientColor())) {
        std::cerr << "Uniform \"material.ambient\" could not be set." << std::endl;
    }

    if (!renderProgram->setUniform("material.shininess", material->getSpecularExponent())) {
        std::cerr << "Uniform \"material.shininess\" could not be set" << std::endl;
    }

    glHelper->attachTexture(material->getDiffuseTexture()->getID(), diffuseMapAttachPoint);
    //TODO we should support multi texture on one pass
}

bool Model::setupRenderVariables() {
    if (renderProgram->setUniform("worldTransformMatrix", getWorldTransform())) {
        if (renderProgram->setUniform("cameraPosition", glHelper->getCameraPosition())) {
            if (this->materialMap.begin() != this->materialMap.end()) {
                this->activateMaterial(materialMap.begin()->second);
            } else {
                std::cerr << "No material setup, passing rendering. " << std::endl;
            }
            if (!renderProgram->setUniform("diffuseSampler",
                                           diffuseMapAttachPoint)) { //even if diffuse map cannot attach, we still render
                std::cerr << "Uniform \"diffuseSampler\" could not be set" << std::endl;
            }
            if (!renderProgram->setUniform("shadowSampler",
                                           glHelper->getMaxTextureImageUnits() -
                                           1)) { //even if shadow map cannot attach, we still render
                std::cerr << "Uniform \"shadowSampler\" could not be set" << std::endl;
            }
            return true;
        } else {
            std::cerr << "Uniform \"cameraPosition\" could not be set, passing rendering." << std::endl;
        }
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
    return false;

}

void Model::render() {
    if (setupRenderVariables()) {
        glHelper->render(renderProgram->getID(), vao, ebo, triangleCount * 3);
    }
}

void Model::renderWithProgram(GLSLProgram &program) {
    if (program.setUniform("worldTransformMatrix", getWorldTransform())) {
        glHelper->render(program.getID(), vao, ebo, triangleCount * 3);
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
}