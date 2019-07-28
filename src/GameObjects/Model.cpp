//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"
#include "API/ActorInterface.h"
#include "../ImGuiHelper.h"
#include "GamePlay/APISerializer.h"
#include <random>

#ifdef CEREAL_SUPPORT
#include <cereal/archives/binary.hpp>
#endif
Model::Model(uint32_t objectID, AssetManager *assetManager, const float mass, const std::string &modelFile,
             bool disconnected = false) :
        PhysicalRenderable(assetManager->getGlHelper(), mass, disconnected), objectID(objectID), assetManager(assetManager),
        name(modelFile) {

    transformation.setUpdateCallback(std::bind(&Model::transformChangeCallback, this));

    //this is required because the shader has fixed size arrays
    boneTransforms.resize(128);
    modelAsset = assetManager->loadAsset<ModelAsset>({modelFile});
    //set up the rigid body
    this->triangleCount = 0;
    this->vao = 0;
    this->ebo = 0;//these are not per Model, but per Mesh, and comes from ModelAsset->MeshAsset, shared between instances
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
    std::vector<std::shared_ptr<MeshAsset>> assetMeshes = modelAsset->getMeshes();

    for (auto iter = assetMeshes.begin(); iter != assetMeshes.end(); ++iter) {
        meshMeta = new MeshMeta();
        meshMeta->mesh = (*iter);
        meshMetaData.push_back(meshMeta);
    }

    std::vector<std::shared_ptr<MeshAsset>> physicalMeshes = modelAsset->getPhysicsMeshes();

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
}

void Model::setupForTime(long time) {
    if(animated && !animationLastFramePlayed) {
        //check if we need to blend
        if(animationBlend) {
            //we need 2 animation times, and a factor
            animationTime = animationTime + (time - lastSetupTime) * animationTimeScale;

            animationTimeOld = animationTimeOld + (time - lastSetupTime) * animationTimeScale;

            float blendFactor = std::min(1.0f, (float)animationTime / (float)animationBlendTime);//don't blend after 1.0

            if(blendFactor == 1) {
                animationBlend = false; // no need to blend anymore.
            }
            animationLastFramePlayed = modelAsset->getTransformBlended(animationNameOld, animationTimeOld, animationLoopedOld,
                                                                       animationName, animationTime, animationLooped,
                                                                       blendFactor, boneTransforms);
            //std::cout << "blend " << animationNameOld << " with " << animationName << " for " << blendFactor << " factor" << std::endl;
        } else {
            animationTime = animationTime + (time - lastSetupTime) * animationTimeScale;
            animationLastFramePlayed = modelAsset->getTransform(animationTime, animationLooped, animationName, boneTransforms);
        }
        if(disconnected) {
            for (unsigned int i = 0; i < boneTransforms.size(); ++i) {
                if (boneIdCompoundChildMap.find(i) != boneIdCompoundChildMap.end()) {
                    boneTransforms[i] = centerOffsetMatrix * boneTransforms[i];
                }
            }
        } else {
            btVector3 scale;
            if(isScaled) {
                scale = this->getRigidBody()->getCollisionShape()->getLocalScaling();
                this->getRigidBody()->getCollisionShape()->setLocalScaling(btVector3(1, 1, 1));
            }
            for (unsigned int i = 0; i < boneTransforms.size(); ++i) {
                if (boneIdCompoundChildMap.find(i) != boneIdCompoundChildMap.end()) {
                    btTransform transform;
                    transform.setFromOpenGLMatrix(glm::value_ptr(boneTransforms[i]));
                    compoundShape->updateChildTransform(boneIdCompoundChildMap[i], transform, false);
                    boneTransforms[i] = centerOffsetMatrix * boneTransforms[i];
                }
            }
            if(isScaled) {
                this->getRigidBody()->getCollisionShape()->setLocalScaling(scale);
            }
            compoundShape->recalculateLocalAabb();
        }
    }
    lastSetupTime = time;
}

void Model::activateTexturesOnly(std::shared_ptr<const Material>material) {
    if(material->hasDiffuseMap()) {
        glHelper->attachTexture(material->getDiffuseTexture()->getID(), diffuseMapAttachPoint);
    }
    if(material->hasAmbientMap()) {
        glHelper->attachTexture(material->getAmbientTexture()->getID(), ambientMapAttachPoint);
    }

    if(material->hasSpecularMap()) {
        glHelper->attachTexture(material->getSpecularTexture()->getID(), specularMapAttachPoint);
    }

    if(material->hasOpacityMap()) {
        glHelper->attachTexture(material->getOpacityTexture()->getID(), opacityMapAttachPoint);
    }

    if(material->hasNormalMap()) {
        glHelper->attachTexture(material->getNormalTexture()->getID(), normalMapAttachPoint);
    }
}
/*
bool Model::setupRenderVariables(MeshMeta *meshMetaData) {
    std::shared_ptr<GLSLProgram> program  = meshMetaData->program;

    if (meshMetaData->mesh != nullptr && meshMetaData->mesh->getMaterial() != nullptr) {
        glHelper->attachMaterialUBO(program->getID(), meshMetaData->mesh->getMaterial()->getMaterialIndex());
    } else {
        std::cerr << "No material setup, passing rendering. " << std::endl;
        return false;
    }

    if (animated) {
        //set all of the bones to unitTransform for testing

        for (auto boneIterator = exposedBoneTransforms.begin();
             boneIterator != exposedBoneTransforms.end(); ++boneIterator) {
                glm::vec3 temp1;//these are not used
                glm::vec4 temp2;
                glm::vec3 translate, scale;
                glm::quat orientation;

                glm::decompose(this->transformation.getWorldTransform() * boneTransforms[boneIterator->first], scale, orientation, translate, temp1, temp2);

                exposedBoneTransforms[boneIterator->first]->setTranslate(translate);
                exposedBoneTransforms[boneIterator->first]->setScale(scale);
                exposedBoneTransforms[boneIterator->first]->setOrientation(orientation);
        }

        program->setUniformArray("boneTransformArray[0]", boneTransforms);
    }
    return true;
}
*/
void Model::render() {
    std::cerr << "Model render method is a place holder, renderWithProgram should be used!" << std::endl;
}


void Model::renderWithProgram(GLSLProgram &program) {
    glHelper->attachModelUBO(program.getID());
    for (auto iter = meshMetaData.begin(); iter != meshMetaData.end(); ++iter) {

        if (animated) {
            //set all of the bones to unitTransform for testing
            program.setUniformArray("boneTransformArray[0]", boneTransforms);
            program.setUniform("isAnimated", true);
        } else {
            program.setUniform("isAnimated", false);
        }
        if(program.IsMaterialRequired()) {
            glHelper->attachMaterialUBO(program.getID(), (*iter)->mesh->getMaterial()->getMaterialIndex());
        }
        glHelper->render(program.getID(), (*iter)->mesh->getVao(), (*iter)->mesh->getEbo(), (*iter)->mesh->getTriangleCount() * 3);
    }
}

void Model::renderWithProgramInstanced(std::vector<uint32_t> &modelIndices, GLSLProgram &program) {
    glHelper->setModelIndexesUBO(modelIndices);

    glHelper->attachModelUBO(program.getID());
    glHelper->attachModelIndicesUBO(program.getID());

    for (auto boneIterator = exposedBoneTransforms.begin();
         boneIterator != exposedBoneTransforms.end(); ++boneIterator) {
        glm::vec3 temp1;//these are not used
        glm::vec4 temp2;
        glm::vec3 translate, scale;
        glm::quat orientation;

        glm::decompose(this->transformation.getWorldTransform() * boneTransforms[boneIterator->first], scale, orientation, translate, temp1, temp2);

        exposedBoneTransforms[boneIterator->first]->setTranslate(translate);
        exposedBoneTransforms[boneIterator->first]->setScale(scale);
        exposedBoneTransforms[boneIterator->first]->setOrientation(orientation);
    }

    for (auto iter = meshMetaData.begin(); iter != meshMetaData.end(); ++iter) {
        if (animated) {
            //set all of the bones to unitTransform for testing
            program.setUniformArray("boneTransformArray[0]", boneTransforms);
            program.setUniform("isAnimated", true);
        } else {
            program.setUniform("isAnimated", false);
        }
        if(program.IsMaterialRequired()) {
            glHelper->attachMaterialUBO(program.getID(), (*iter)->mesh->getMaterial()->getMaterialIndex());
            this->activateTexturesOnly((*iter)->mesh->getMaterial());

        }
        glHelper->renderInstanced(program.getID(), (*iter)->mesh->getVao(), (*iter)->mesh->getEbo(), (*iter)->mesh->getTriangleCount() * 3, modelIndices.size());
    }
}

bool Model::fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const {
    if(this->temporary) {
        return false;//don't save objects if they are temporary
    }
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
    currentElement = document.NewElement("Disconnected");
    if(disconnected) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    objectElement->InsertEndChild(currentElement);
    if(AIActor != nullptr) {
        APISerializer::serializeActorInterface(*AIActor,document, objectElement);
    }

    currentElement = document.NewElement("Mass");
    currentElement->SetText(mass);
    objectElement->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(objectID);
    objectElement->InsertEndChild(currentElement);

    if(this->parentObject != nullptr) {
        GameObject* parent = dynamic_cast<GameObject*>(this->parentObject);
        if(parent != nullptr) {
            currentElement = document.NewElement("ParentID");
            currentElement->SetText(std::to_string(parent->getWorldObjectID()).c_str());
            objectElement->InsertEndChild(currentElement);
        }
        if(parentBoneID != -1) {
            currentElement = document.NewElement("ParentBoneID");
            currentElement->SetText(std::to_string(parentBoneID).c_str());
            objectElement->InsertEndChild(currentElement);
        }
    }

    if(stepOnSound) {
        currentElement = document.NewElement("StepOnSound");
        currentElement->SetText(stepOnSound->getName().c_str());
        objectElement->InsertEndChild(currentElement);
    }
    if(!customAnimation) {
        transformation.serialize(document, objectElement);
    } else {
        //if part of custom animation, it means the original position is at the parent. Serialize that
        const Transformation* parent = transformation.getParentTransform();
        parent->serialize(document, objectElement);
    }

    //now handle children
    if(this->children.size() > 0) {
        tinyxml2::XMLElement *childrenNode = document.NewElement("Children");
        tinyxml2::XMLElement *childrenCountNode = document.NewElement("Count");
        childrenCountNode->SetText(std::to_string(children.size()).c_str());
        childrenNode->InsertEndChild(childrenCountNode);
        objectElement->InsertEndChild(childrenNode);
       for (size_t i = 0; i < children.size(); ++i) {
           tinyxml2::XMLElement *childNode = document.NewElement("Child");
           childNode->SetAttribute("Index", (uint32_t)i);
           PhysicalRenderable* child = children[i];
           if(child->fillObjects(document, childNode)) {
               childrenNode->InsertEndChild(childNode);
           }

        }
    }

    modelAsset->serializeCustomizations();
    return true;
}

uint32_t Model::getAIID() {
    if(AIActor == nullptr) {
        return 0;
    }
    return this->AIActor->getWorldID();
}

GameObject::ImGuiResult Model::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    //Allow transformation editing.
    if(transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix)) {
        //true means transformation changed, activate rigid body
        rigidBody->activate();
        result.updated = true;
    }

        ImGui::NewLine();
    if (isAnimated()) {
        if (ImGui::CollapsingHeader("Model animation properties")) {
            if (ImGui::BeginCombo("Animation Name", animationName.c_str())) {
                for (auto it = modelAsset->getAnimations().begin(); it != modelAsset->getAnimations().end(); it++) {
                    bool isThisAnimationCurrent = this->getAnimationName() == it->first;
                    if (ImGui::Selectable(it->first.c_str(), isThisAnimationCurrent)) {
                        setAnimation(it->first, true);
                    }
                    if (isThisAnimationCurrent) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SliderFloat("Animation time scale", &(this->animationTimeScale), 0.01f, 2.0f);

            ImGui::Text("Separate selected animation by time");
            static char newAnimationName[256] = {0};
            static float times[2] = {0};
            ImGui::InputText("New animation Name", newAnimationName, sizeof(newAnimationName) - 1 );
            ImGui::InputFloat2("Animation start and end times", times);
            if(ImGui::Button("CreateSection")){
                this->modelAsset->addAnimationAsSubSequence(this->animationName, std::string(newAnimationName), times[0], times[1]);
            }

        }
    }
    if (isAnimated()) { //in animated objects can't have AI, can they?
        if (ImGui::CollapsingHeader("AI properties")) {
            if(isAIParametersDirty) {
                if(this->AIActor != nullptr) {
                    this->aiParameters = this->AIActor->getParameters();
                } else {
                    this->aiParameters.clear();
                }
                isAIParametersDirty = false;
            }
            result = putAIonGUI(this->AIActor, this->aiParameters, request, lastSelectedAIName);//ATTENTION is somehow user manages to update transform and AI at the same frame, this will override transform.
            if(result.removeAI || result.addAI) {
                isAIParametersDirty = true;
            }
        }
    }
    if (ImGui::CollapsingHeader("Sound properties")) {
        ImGui::Indent(16.0f);
        static const AssetManager::AvailableAssetsNode *selectedSoundAsset = nullptr;
        static char stepOnSoundFilter[32] = {0};
        ImGui::InputText("Filter Assets ##StepOnSoundAssetTreeFilter", stepOnSoundFilter, sizeof(stepOnSoundFilter),
                         ImGuiInputTextFlags_CharsNoBlank);
        std::string stepOnSoundFilterStr = stepOnSoundFilter;
        std::transform(stepOnSoundFilterStr.begin(), stepOnSoundFilterStr.end(), stepOnSoundFilterStr.begin(),
                       ::tolower);
        const AssetManager::AvailableAssetsNode *filteredAssets = assetManager->getAvailableAssetsTreeFiltered(
                AssetManager::Asset_type_SOUND, stepOnSoundFilterStr);
        ImGuiHelper::buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_SOUND,
                                          "StepOnSound",
                                          &selectedSoundAsset);

        if (this->stepOnSound != nullptr) {
            ImGui::Text("step On Sound: %s", this->stepOnSound->getName().c_str());
        } else {
            ImGui::Text("No step on sound set.");
        }

        if (selectedSoundAsset != nullptr) {
            if (ImGui::Button("Set Step On Sound")) {
                if (this->stepOnSound != nullptr) {
                    this->stepOnSound->stop();
                }
                this->stepOnSound = std::make_shared<Sound>(0, assetManager, selectedSoundAsset->fullPath);
                this->stepOnSound->changeGain(0.125f);
                this->stepOnSound->setLoop(true);
            }
        } else {
            ImGui::Button("Set Step On Sound");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No sound asset selected");
        }
    }
    if(animated) {
        if (ImGui::CollapsingHeader("Expose Bone for attachment")) {
            int32_t newSelectedBoneID = this->modelAsset->buildEditorBoneTree(selectedBoneID);
            if (newSelectedBoneID != -1 && newSelectedBoneID != selectedBoneID) {
                selectedBoneID = newSelectedBoneID;
                std::cout << "selected bone is " << selectedBoneID << std::endl;
            }
        } else {
            selectedBoneID = -1;
        }
    }
    return result;
}

Model::~Model() {
    if(this->parentObject != nullptr) {
        this->parentObject->removeChild(this);
    }
    delete rigidBody->getMotionState();
    delete rigidBody;
    delete compoundShape;
    delete AIActor;

    for (size_t i = 0; i < meshMetaData.size(); ++i) {
        delete meshMetaData[i];
    }

    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->setParentObject(nullptr);
    }

    assetManager->freeAsset({name});
}

Model::Model(const Model &otherModel, uint32_t objectID) :
        Model(objectID, otherModel.assetManager, otherModel.mass, otherModel.name, otherModel.disconnected) {
    //we have constructed the object, now set the properties that might have been changed
    this->transformation.setTransformationsNotPropagate(
            otherModel.transformation.getTranslate(),
            otherModel.transformation.getOrientation(),
            otherModel.transformation.getScale()
            );
    this->updateAABB();
    transformation.setUpdateCallback(std::bind(&Model::transformChangeCallback, this));

    this->animationName = otherModel.animationName;
    this->animationTimeScale = otherModel.animationTimeScale;
    this->animationTime = otherModel.animationTime;
}

GameObject::ImGuiResult Model::putAIonGUI(ActorInterface *actorInterface,
                                          std::vector<LimonAPI::ParameterRequest> &parameters,
                                          const ImGuiRequest &request, std::string &lastSelectedAIName) {
    GameObject::ImGuiResult result;
    std::string currentAIName;
    if (actorInterface == nullptr && lastSelectedAIName == "") {
        currentAIName = "Not selected";
    } else {
        if(lastSelectedAIName == "") {
            currentAIName = actorInterface->getName();
        } else {
            currentAIName = lastSelectedAIName;
        }
    }
    //let user select what kind of Actor required
    std::vector<std::string> actorNames = ActorInterface::getActorNames();

    if (ImGui::BeginCombo("Actor type##AI", currentAIName.c_str())) {
        for (auto it = actorNames.begin(); it != actorNames.end(); it++) {
            bool isThisActorSelected = (lastSelectedAIName == *it);
            if (ImGui::Selectable(it->c_str(), isThisActorSelected)) {
                if (!isThisActorSelected) {//if this is not the previously selected Actor type
                    lastSelectedAIName = *it;
                }
            }
            if(isThisActorSelected) {
                ImGui::SetItemDefaultFocus();
            }

        }
        ImGui::EndCombo();
    }
    if (actorInterface != nullptr) {
        if(actorInterface->getName() != lastSelectedAIName) {
            if(ImGui::Button("Change Actor type##AI")) {
                result.addAI = true;
                result.removeAI = true;
                result.actorTypeName = lastSelectedAIName;
            }

        } else {//if actor is set, and not modified
            bool isSet = request.limonAPI->generateEditorElementsForParameters(parameters, 0);
            if(isSet) {
                if(ImGui::Button("Apply changes##AI")) {
                    actorInterface->setParameters(parameters);

                }
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Remove AI##AI")) {
            result.removeAI = true;
        }
    } else {//if no actor is set
        if(lastSelectedAIName != "") {
            if(ImGui::Button("Add AI##AI")) {
                result.addAI = true;
                result.actorTypeName = lastSelectedAIName;
            }
        }
    }

    return result;
}

void Model::attachAI(ActorInterface *AIActor) {
    //after this, clearing the AI is job of the model.
    this->AIActor = AIActor;
    lastSelectedAIName = AIActor->getName();
}

void Model::convertAssetToLimon(std::set<std::vector<std::string>> &convertedModels [[gnu::unused]]) {
#ifdef CEREAL_SUPPORT
    std::vector<std::string> nameVector;
    nameVector.push_back(name);
    std::string newName = name.substr(0, name.find_last_of(".")) + ".limonmodel";
    if(convertedModels.find(nameVector) == convertedModels.end()) {
        std::ofstream os(newName, std::ios::binary);
        cereal::BinaryOutputArchive archive( os );

        archive(*modelAsset);
        convertedModels.insert(nameVector);
    }
    this->name = newName;//change name of self so next time converted file would be used.

    for (auto childIt = children.begin(); childIt != children.end(); ++childIt) {
        Model* modelChild = dynamic_cast<Model*>(*childIt);
        if(modelChild != nullptr) {
            modelChild->convertAssetToLimon(convertedModels);
        }
    }
#else
    std::cerr << "Cereal support disabled" << std::endl;
#endif
}