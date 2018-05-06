//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"
#include "../AI/Actor.h"

Model::Model(uint32_t objectID, AssetManager *assetManager, const float mass, const std::string &modelFile) :
        PhysicalRenderable(assetManager->getGlHelper(), mass), objectID(objectID), assetManager(assetManager),
        name(modelFile) {
    //this is required because the shader has fixed size arrays
    boneTransforms.resize(128);
    modelAsset = assetManager->loadAsset<ModelAsset>({modelFile});
    //set up the rigid body
    this->triangleCount = 0;
    this->vao = 0;
    this->ebo = 0;//these are not per Model, but per Mesh
    this->centerOffset = modelAsset->getCenterOffset();
    this->centerOffsetMatrix = glm::translate(glm::mat4(1.0f), centerOffset);

    compoundShape = new btCompoundShape();
    btTransform baseTransform;
    baseTransform.setIdentity();
    baseTransform.setOrigin(GLMConverter::GLMToBlt(-1.0f * centerOffset));
    this->animated = modelAsset->isAnimated();
    std::map<uint_fast32_t, btConvexHullShape *> hullMap;

    std::map<uint_fast32_t, btTransform> btTransformMap;

    MeshMeta *meshMeta;
    std::vector<MeshAsset *> assetMeshes = modelAsset->getMeshes();

    for (std::vector<MeshAsset *>::iterator iter = assetMeshes.begin(); iter != assetMeshes.end(); ++iter) {
        meshMeta = new MeshMeta();
        meshMeta->mesh = (*iter);

        if (this->animated) {//this was hasBones, but it turns out, there are models with bones, but no animation.
            //set up the program to render object
            meshMeta->program = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertexAnimated.glsl",
                                                "./Data/Shaders/Model/fragment.glsl", true);
            //Now we should find out about bone tree

        } else {
            //set up the program to render object without bones
            meshMeta->program = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertex.glsl",
                                                "./Data/Shaders/Model/fragment.glsl", true);
        }
        meshMetaData.push_back(meshMeta);
    }

    std::vector<MeshAsset *> physicalMeshes = modelAsset->getPhysicsMeshes();

    for(auto iter = physicalMeshes.begin(); iter != physicalMeshes.end(); ++iter) {

        btTriangleMesh *rawCollisionMesh = (*iter)->getBulletMesh(&hullMap, &btTransformMap);
        if (rawCollisionMesh != nullptr) {
            btCollisionShape *meshCollisionShape;
            btConvexTriangleMeshShape *convexTriangleMeshShape;
            if (mass == 0 && !animated ) {
                meshCollisionShape = new btBvhTriangleMeshShape(rawCollisionMesh, true);
            } else {
                convexTriangleMeshShape = new btConvexTriangleMeshShape(rawCollisionMesh);
                meshCollisionShape = convexTriangleMeshShape;
                if (rawCollisionMesh->getNumTriangles() > 24) {
                    btShapeHull *hull = new btShapeHull(convexTriangleMeshShape);
                    btScalar margin = convexTriangleMeshShape->getMargin();
                    hull->buildHull(margin);
                    delete convexTriangleMeshShape;
                    convexTriangleMeshShape = nullptr; //this is not needed, but I am leaving here in case I try to use it at a later revision.

                    meshCollisionShape = new btConvexHullShape((const btScalar *) hull->getVertexPointer(),
                                                               hull->numVertices());
                    delete hull;
                }
            }
            //since there is no animation, we don't have to put the elements in order.
            compoundShape->addChildShape(baseTransform, meshCollisionShape);//this add the mesh to collision shape
        }
    }

    if (animated) {
        std::map<uint_fast32_t, btConvexHullShape *>::iterator it;
        for (unsigned int i = 0;i < 128; i++) {//FIXME 128 is the number of bones supported. It should be an option or an constant
            if (btTransformMap.find(i) != btTransformMap.end() && hullMap.find(i) != hullMap.end()) {
                boneIdCompoundChildMap[i] = compoundShape->getNumChildShapes();//get numchild actually increase with each new child add below
                compoundShape->addChildShape(btTransformMap[i], hullMap[i]);//this add the mesh to collision shape, in order
            }
        }
    }

    btDefaultMotionState *initialMotionState = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), GLMConverter::GLMToBlt(centerOffset)));

    btVector3 fallInertia(0, 0, 0);
    compoundShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo *rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(
            mass, initialMotionState, compoundShape, fallInertia);
    rigidBody = new btRigidBody(*rigidBodyConstructionInfo);
    delete rigidBodyConstructionInfo;

    this->materialMap = modelAsset->getMaterialMap();

    rigidBody->setSleepingThresholds(0.1, 0.1);
    rigidBody->setUserPointer(static_cast<GameObject *>(this));

    if(animated) {
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);
        //for animated bodies, setup the first frame
        this->setupForTime(0);
    }

    isDirty = true;
}

void Model::setupForTime(long time) {
    if(animated) {
        animationTime = animationTime + (time - lastSetupTime) * animationTimeScale;
        modelAsset->getTransform(animationTime, animationName, boneTransforms);
        btVector3 scale = this->getRigidBody()->getCollisionShape()->getLocalScaling();
        this->getRigidBody()->getCollisionShape()->setLocalScaling(btVector3(1, 1, 1));
        for (unsigned int i = 0; i < boneTransforms.size(); ++i) {
            if (boneIdCompoundChildMap.find(i) != boneIdCompoundChildMap.end()) {
                btTransform transform;
                transform.setFromOpenGLMatrix(glm::value_ptr(boneTransforms[i]));
                compoundShape->updateChildTransform( boneIdCompoundChildMap[i], transform, false);
                boneTransforms[i] = centerOffsetMatrix * boneTransforms[i];
            }
        }
        this->getRigidBody()->getCollisionShape()->setLocalScaling(scale);
        compoundShape->recalculateLocalAabb();
    }
    lastSetupTime = time;
}

void Model::activateMaterial(const Material *material, GLSLProgram *program) {
    if(material == nullptr ) {
        return;
    }
    if (!program->setUniform("material.ambient", material->getAmbientColor() + glm::vec3(0.15f,0.15f,0.15f))) {
        std::cerr << "Uniform \"material.ambient\" could not be set for program " << program->getProgramName()  << std::endl;
    }

    if (!program->setUniform("material.diffuse", material->getDiffuseColor())) {
        std::cerr << "Uniform \"material.diffuse\" could not be set for program "  << program->getProgramName() << std::endl;
    }

    if (!program->setUniform("material.shininess", material->getSpecularExponent())) {
        std::cerr << "Uniform \"material.shininess\" could not be set for program "  << program->getProgramName() << std::endl;
    }

    if(material->hasDiffuseMap()) {
        glHelper->attachTexture(material->getDiffuseTexture()->getID(), diffuseMapAttachPoint);
        if (!program->setUniform("diffuseSampler",
                                 diffuseMapAttachPoint)) { //even if diffuse map cannot attach, we still render
            std::cerr << "Uniform \"diffuseSampler\" could not be set" << std::endl;
        }
    } else {
        //FIXME why is this here?
    }

    if(material->hasAmbientMap()) {
        glHelper->attachTexture(material->getAmbientTexture()->getID(), ambientMapAttachPoint);
        if (!program->setUniform("ambientSampler",
                                 ambientMapAttachPoint)) { //even if ambient map cannot attach, we still render
            std::cerr << "Uniform \"ambientSampler\" could not be set" << std::endl;
        }
    }

    if(material->hasSpecularMap()) {
        glHelper->attachTexture(material->getSpecularTexture()->getID(), specularMapAttachPoint);
        if (!program->setUniform("specularSampler",
                                 specularMapAttachPoint)) { //even if specular map cannot attach, we still render
            std::cerr << "Uniform \"specularSampler\" could not be set" << std::endl;
        }
    }

    if(material->hasOpacityMap()) {
        glHelper->attachTexture(material->getOpacityTexture()->getID(), opacityMapAttachPoint);
        if (!program->setUniform("opacitySampler",
                                 opacityMapAttachPoint)) { //even if opacity map cannot attach, we still render
            std::cerr << "Uniform \"opacitySampler\" could not be set" << std::endl;
        }
    }

    int maps = 0;
    if(material->hasAmbientMap()) {
        maps +=8;
    }
    if(material->hasDiffuseMap()) {
        maps +=4;
    }
    if(material->hasSpecularMap()) {
        maps +=2;
    }
    if(material->hasOpacityMap()) {
        maps +=1;
    }

    if (!program->setUniform("material.isMap", maps)) {
        std::cerr << "Uniform \"material.isMap\" could not be set for program "  << program->getProgramName() << std::endl;
    }

    //TODO we should support multi texture on one pass
}

bool Model::setupRenderVariables(MeshMeta *meshMetaData) {
    GLSLProgram* program  = meshMetaData->program;
    if (program->setUniform("worldTransformMatrix", getWorldTransform())) {
            if (meshMetaData->mesh != nullptr && meshMetaData->mesh->getMaterial() != nullptr) {
                this->activateMaterial(meshMetaData->mesh->getMaterial(), program);
            } else {
                std::cerr << "No material setup, passing rendering. " << std::endl;
            }
            if (!program->setUniform("shadowSamplerDirectional",
                                           glHelper->getMaxTextureImageUnits() -
                                           1)) { //even if shadow map cannot attach, we still render
                std::cerr << "Uniform \"shadowSamplerDirectional\" could not be set" << std::endl;
            }
            if (!program->setUniform("shadowSamplerPoint",
                                     glHelper->getMaxTextureImageUnits() -
                                     2)) { //even if shadow map cannot attach, we still render
                std::cerr << "Uniform \"shadowSamplerPoint\" could not be set" << std::endl;
            }
            if (!program->setUniform("farPlanePoint",100.0f)) { //even if far plane cannot attach, we still render
                std::cerr << "Uniform \"farPlanePoint\" could not be set" << std::endl;
            }

            if (animated) {
                //set all of the bones to unitTransform for testing
                program->setUniformArray("boneTransformArray[0]", boneTransforms);
            }
            return true;
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
    return false;
}

void Model::render() {
    for (std::vector<MeshMeta *>::iterator iter = meshMetaData.begin(); iter != meshMetaData.end(); ++iter) {
        if (setupRenderVariables((*iter))) {
            glHelper->render((*iter)->program->getID(), (*iter)->mesh->getVao(), (*iter)->mesh->getEbo(),
                             (*iter)->mesh->getTriangleCount() * 3);
        }
    }
}

void Model::renderWithProgram(GLSLProgram &program) {
    if (program.setUniform("worldTransformMatrix", getWorldTransform())) {
        std::vector<MeshAsset *> meshes = modelAsset->getMeshes();
        for (std::vector<MeshAsset *>::iterator iter = meshes.begin(); iter != meshes.end(); ++iter) {//FIXME why this uses meshes, while normal render doesn't?
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

void Model::fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode) const {
    tinyxml2::XMLElement *objectElement = document.NewElement("Object");
    objectsNode->InsertEndChild(objectElement);

    tinyxml2::XMLElement *currentElement = document.NewElement("File");
    currentElement->SetText(name.c_str());
    objectElement->InsertEndChild(currentElement);

    if(animated) {
        currentElement = document.NewElement("Animation");
        currentElement->SetText(animationName.c_str());
        objectElement->InsertEndChild(currentElement);
    }

    if(AIActor != nullptr) {
        currentElement = document.NewElement("AI");
        currentElement->SetText("True");
        objectElement->InsertEndChild(currentElement);
    }

    currentElement = document.NewElement("Mass");
    currentElement->SetText(mass);
    objectElement->InsertEndChild(currentElement);

    tinyxml2::XMLElement *parent = document.NewElement("Scale");
    currentElement = document.NewElement("X");
    currentElement->SetText(scale.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(scale.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(scale.z);
    parent->InsertEndChild(currentElement);
    objectElement->InsertEndChild(parent);

    parent = document.NewElement("Translate");
    currentElement = document.NewElement("X");
    currentElement->SetText(translate.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(translate.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(translate.z);
    parent->InsertEndChild(currentElement);
    objectElement->InsertEndChild(parent);

    parent = document.NewElement("Rotate");
    currentElement = document.NewElement("X");
    currentElement->SetText(orientation.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(orientation.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(orientation.z);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("W");
    currentElement->SetText(orientation.w);
    parent->InsertEndChild(currentElement);
    objectElement->InsertEndChild(parent);
}

uint32_t Model::getAIID() {
    if(AIActor == nullptr) {
        return 0;
    }
    return this->AIActor->getWorldID();
}

GameObject::GizmoRequest Model::addImGuiEditorElements() {
    static GizmoRequest request;
    static glm::vec3 preciseTranslatePoint = this->translate;

    bool updated = false;
    bool crudeUpdated = false;

    if (ImGui::IsKeyPressed(83)) {
        request.useSnap = !request.useSnap;
    }

/*
 * at first we decide whether we are in rotation, scale or translate mode.
 */

    if (ImGui::RadioButton("Translate", request.mode == EditorModes::TRANSLATE_MODE)) {
        request.mode = EditorModes::TRANSLATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", request.mode == EditorModes::ROTATE_MODE)) {
        request.mode = EditorModes::ROTATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", request.mode == EditorModes::SCALE_MODE)) {
        request.mode = EditorModes::SCALE_MODE;
    }

    switch (request.mode) {
        case TRANSLATE_MODE:
            updated =
                    ImGui::DragFloat("Precise Position X", &(this->translate.x), 0.01f, preciseTranslatePoint.x - 5.0f,
                                     preciseTranslatePoint.x + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Y", &(this->translate.y), 0.01f, preciseTranslatePoint.y - 5.0f,
                                     preciseTranslatePoint.y + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Z", &(this->translate.z), 0.01f, preciseTranslatePoint.z - 5.0f,
                                     preciseTranslatePoint.z + 5.0f) || updated;
            ImGui::NewLine();
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position X", &(this->translate.x), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Y", &(this->translate.y), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Z", &(this->translate.z), -100.0f, 100.0f) || crudeUpdated;
            if (updated || crudeUpdated) {
                this->setTranslate(translate);
                this->rigidBody->activate();
            }
            if (crudeUpdated) {
                preciseTranslatePoint = this->translate;
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", &(request.snap[0]));
            break;
        case ROTATE_MODE:
            updated = ImGui::SliderFloat("Rotate X", &(this->orientation.x), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Y", &(this->orientation.y), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Z", &(this->orientation.z), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate W", &(this->orientation.w), -1.0f, 1.0f) || updated;
            if (updated || crudeUpdated) {
                this->setOrientation(orientation);
                this->rigidBody->activate();
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Angle Snap", &(request.snap[0]));
            break;
        case SCALE_MODE:
            glm::vec3 tempScale = scale;
            updated = ImGui::DragFloat("Scale X", &(tempScale.x), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Y", &(tempScale.y), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Z", &(tempScale.z), 0.01, 0.01f, 10.0f) || updated;
            ImGui::NewLine();
            updated = ImGui::SliderFloat("Massive Scale X", &(tempScale.x), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Y", &(tempScale.y), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Z", &(tempScale.z), 0.01f, 100.0f) || updated;
            if ((updated || crudeUpdated) && (tempScale.x != 0.0f && tempScale.y != 0.0f && tempScale.z != 0.0f)) {
//it is possible to enter any scale now. If user enters 0, don't update
                this->setScale(tempScale);
                this->rigidBody->activate();
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Scale Snap", &(request.snap[0]));
            break;
    }
    ImGui::NewLine();
    if (isAnimated()) {
        if (ImGui::CollapsingHeader("Animation properties")) {
            if (ImGui::BeginCombo("Animation Name", animationName.c_str())) {
//ImGui::Combo();
                for (auto it = modelAsset->getAnimations().begin(); it != modelAsset->getAnimations().end(); it++) {
                    if (ImGui::Selectable(it->first.c_str())) {
                        setAnimation(it->first);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SliderFloat("Animation time scale", &(this->animationTimeScale), 0.01f, 2.0f);
        }
    }
    request.isEdited = updated || crudeUpdated;
    request.isRequested = true;
    return request;
}

//TODO we need to free the texture. Destructor needed.
Model::~Model() {
    delete rigidBody->getMotionState();
    delete rigidBody;
    delete compoundShape;
    delete AIActor;

    for (unsigned int i = 0; i < meshMetaData.size(); ++i) {
        delete meshMetaData[i];
    }
    assetManager->freeAsset({name});
}