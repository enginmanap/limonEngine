//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"


Model::Model(AssetManager *assetManager, const float mass, const std::string &modelFile) :
        PhysicalRenderable(assetManager->getGlHelper()) {

    //this is required because the shader has fixed size arrays
    boneTransforms.resize(128);
    modelAsset = assetManager->loadAsset<ModelAsset>({modelFile});
    //set up the rigid body
    this->triangleCount = 0;
    this->vao = 0;
    this->ebo = 0;//these are not per Model, but per Mesh
    this->centerOffset = modelAsset->getCenterOffset();

    btCompoundShape *compoundShape = new btCompoundShape();
    btTransform emptyTransform(btQuaternion(0, 0, 0, 1));

    this->animated = modelAsset->isAnimated();

    MeshMeta *meshMeta;
    std::vector<MeshAsset *> assetMeshes = modelAsset->getMeshes();
    for (std::vector<MeshAsset *>::iterator iter = assetMeshes.begin(); iter != assetMeshes.end(); ++iter) {

        meshMeta = new MeshMeta();
        meshMeta->mesh = (*iter);

        if ((*iter)->hasBones()) {
            meshMeta->skeleton = (*iter)->getSkeletonCopy();
            //set up the program to render object
            meshMeta->program = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertexAnimated.glsl",
                                                "./Data/Shaders/Model/fragmentAnimated.glsl", true);
            //Now we should find out about bone tree

        } else {
            //set up the program to render object without bones
            meshMeta->program = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertex.glsl",
                                                "./Data/Shaders/Model/fragment.glsl", true);
        }
        meshes.push_back(meshMeta);
        compoundShape->addChildShape(emptyTransform, (*iter)->getCollisionShape());//this add the mesh to collition shape
    }

    btDefaultMotionState *boxMotionState = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), GLMConverter::GLMToBlt(centerOffset)));
    btVector3 fallInertia(0, 0, 0);
    compoundShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo *rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(
            mass, boxMotionState, compoundShape,
            fallInertia);


    this->materialMap = modelAsset->getMaterialMap();

    rigidBody = new btRigidBody(*rigidBodyConstructionInfo);
    //TODO check if this values are too low.
    rigidBody->setSleepingThresholds(0.1, 0.1);

    //glHelper->bufferVertexColor(colors,vao,vbo,3);
    worldTransform = glm::mat4(1.0f);

}

void Model::setupForTime(long time) {
    if(animated) {
        modelAsset->getTransform(time, boneTransforms);
    }
}

void Model::activateMaterial(const Material *material, GLSLProgram *program) {
    if(material == NULL ) {
        return;
    }
    GLuint location;
    if (!program->setUniform("material.ambient", material->getAmbientColor())) {
        std::cerr << "Uniform \"material.ambient\" could not be set for program " << program->getProgramName()  << std::endl;
    }

    if (!program->setUniform("material.shininess", material->getSpecularExponent())) {
        std::cerr << "Uniform \"material.shininess\" could not be set for program "  << program->getProgramName() << std::endl;
    }
    TextureAsset* diffuse = material->getDiffuseTexture();
    if(diffuse!= NULL) {
        glHelper->attachTexture(diffuse->getID(), diffuseMapAttachPoint);
    } else {
        //DEBUG log no diffuse found
    }
    //TODO we should support multi texture on one pass
}

bool Model::setupRenderVariables(GLSLProgram *program) {
    if (program->setUniform("worldTransformMatrix", getWorldTransform())) {
            if (this->materialMap.begin() != this->materialMap.end()) {
                this->activateMaterial(materialMap.begin()->second, program);
            } else {
                std::cerr << "No material setup, passing rendering. " << std::endl;
            }
            if (!program->setUniform("diffuseSampler",
                                           diffuseMapAttachPoint)) { //even if diffuse map cannot attach, we still render
                std::cerr << "Uniform \"diffuseSampler\" could not be set" << std::endl;
            }
            if (!program->setUniform("shadowSamplerDirectional",
                                           glHelper->getMaxTextureImageUnits() -
                                           1)) { //even if shadow map cannot attach, we still render
                std::cerr << "Uniform \"shadowSamplerDirectional\" could not be set" << std::endl;
            }
            if (!program->setUniform("shadowSamplerPoint",
                                     glHelper->getMaxTextureImageUnits() -
                                     2)) { //even if shadow map cannot attach, we still render
                std::cerr << "Uniform \"shadowSamplerDirectional\" could not be set" << std::endl;
            }
            if (!program->setUniform("farPlanePoint",100.0f)) { //even if far plane cannot attach, we still render
                std::cerr << "Uniform \"farPlanePoint\" could not be set" << std::endl;
            }


            if (animated) {
                //set all of the bones to unitTransform for testing
                program->setUniformArray("boneTransformArray[0]", boneTransforms);
            }
            /********* This is before animation loading, to fill the bone data ***********/

            return true;
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
    return false;

}

void Model::render() {
    for (std::vector<MeshMeta *>::iterator iter = meshes.begin(); iter != meshes.end(); ++iter) {
        if (setupRenderVariables((*iter)->program)) {
            this->activateMaterial((*iter)->mesh->getMaterial(), (*iter)->program);
            glHelper->render((*iter)->program->getID(), (*iter)->mesh->getVao(), (*iter)->mesh->getEbo(),
                             (*iter)->mesh->getTriangleCount() * 3);
        }
    }
}

void Model::renderWithProgram(GLSLProgram &program) {
    if (program.setUniform("worldTransformMatrix", getWorldTransform())) {
        std::vector<MeshAsset *> meshes = modelAsset->getMeshes();
        for (std::vector<MeshAsset *>::iterator iter = meshes.begin(); iter != meshes.end(); ++iter) {
            if (animated) {
                //set all of the bones to unitTransform for testing
                program.setUniformArray("boneTransformArray[0]", boneTransforms);
                program.setUniform("isAnimated", true);
            } else {
                program.setUniform("isAnimated", false);
            }
            if(program.IsMaterialRequired()) {
                this->activateMaterial((*iter)->getMaterial(), &program);
            }
            glHelper->render(program.getID(), (*iter)->getVao(), (*iter)->getEbo(), (*iter)->getTriangleCount() * 3);
        }
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
}
