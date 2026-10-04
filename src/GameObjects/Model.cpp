//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"

#include "Camera/Camera.h"
#include "limonAPI/ActorInterface.h"
#include "../ImGuiHelper.h"
#include "GamePlay/APISerializer.h"
#include "Utils/HardCodedTags.h"
#include <random>
#include <Editor/Editor.h>
#include <Occlusion/RenderList.h>

#ifdef CEREAL_SUPPORT
#include <cereal/archives/binary.hpp>
#endif
Model::Model(uint32_t objectID,  std::shared_ptr<AssetManager> assetManager, const float mass, const std::string &modelFile,
             bool disconnected, const std::string &flipAxes) :
        PhysicalRenderable(assetManager->getGraphicsWrapper(), mass, disconnected), objectID(objectID), assetManager(assetManager),
        name(modelFile), flipAxes(flipAxes) {

    //this is required because the shader has fixed size arrays
    boneTransforms.resize(128);
    std::string assetKey = flipAxes.empty() ? modelFile : modelFile + "?flip" + flipAxes;
    modelAsset = assetManager->loadAsset<ModelAsset>({assetKey});
    //set up the rigid body
    this->triangleCount = 0;
    this->vao = 0;
    this->ebo = 0;//these are not per Model, but per Mesh, and comes from ModelAsset->MeshAsset, shared between instances
    this->centerOffset = modelAsset->getCenterOffset();
    this->centerOffsetMatrix = glm::translate(glm::mat4(1.0f), centerOffset);

    btTransform baseTransform;
    baseTransform.setIdentity();
    baseTransform.setOrigin(GLMConverter::GLMToBlt(-1.0f * centerOffset));
    this->animated = modelAsset->isAnimated();
    std::map<uint32_t, btConvexHullShape *> hullMap;

    std::map<uint32_t, btTransform> btTransformMap;

    MeshMeta *meshMeta;
    std::vector<std::shared_ptr<MeshAsset>> assetMeshes = modelAsset->getMeshes();

    bool hasAmbientMapInAnyMesh = false;
    for (auto iter = assetMeshes.begin(); iter != assetMeshes.end(); ++iter) {
        meshMeta = new MeshMeta();
        meshMeta->mesh = (*iter);
        meshMeta->material = modelAsset->getMeshMaterial((*iter));
        if (meshMeta->material->hasAmbientMap()) {
            hasAmbientMapInAnyMesh = true;
        }
        meshMetaData.push_back(meshMeta);
    }

    compoundShape = this->modelAsset->getCompoundShapeForMass(this->mass, this->boneIdCompoundChildMap, childrenPhysicsShapes);
    motionState = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), GLMConverter::GLMToBlt(centerOffset)));

    btVector3 fallInertia(0, 0, 0);
    compoundShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo *rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(
            mass, motionState, compoundShape, fallInertia);
    rigidBody = new btRigidBody(*rigidBodyConstructionInfo);
    delete rigidBodyConstructionInfo;

    rigidBody->setSleepingThresholds(0.1, 0.1);
    rigidBody->setUserPointer(static_cast<GameObject *>(this));

    if(animated) {
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);
        //for animated bodies, setup the first frame
        this->setupForTime(0);
    }

    addDefaultTags();
}

//FIXME temporarily set the tags as hard coded
void Model::addDefaultTags() {
    bool hasAmbientMapInAnyMesh = false;
    for (const MeshMeta* meshMeta:meshMetaData) {
        if (meshMeta->material->hasAmbientMap()) {
            hasAmbientMapInAnyMesh = true;
            break;
        }
    }
    if(animated && !hasAmbientMapInAnyMesh) {
        this->addTag(HardCodedTags::OBJECT_MODEL_ANIMATED);
    }
    if(this->isTransparent()) {
        this->addTag(HardCodedTags::OBJECT_MODEL_TRANSPARENT);
    }
    if(!animated && !this->isTransparent() && !hasAmbientMapInAnyMesh) {
        this->addTag(HardCodedTags::OBJECT_MODEL_BASIC);
    }
    if(hasAmbientMapInAnyMesh) {
        this->addTag(HardCodedTags::OBJECT_MODEL_AMBIENT);
    }
    if(this->mass > 0) {
        this->addTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
    } else {
        this->addTag(HardCodedTags::OBJECT_MODEL_STATIC);
    }
}

void Model::setTags(const std::vector<std::string> &tagList) {
    std::list<HashUtil::HashedString> previousTags = this->getTags();//first get a copy of old tags
    //If we don't add before clean up, the automatic fall back will add defaults
    for (const std::string& tagToSet:tagList) {
        this->addTag(tagToSet);
    }
    for (const HashUtil::HashedString& previousTag:previousTags) {
        if (previousTag.text == HardCodedTags::PICKED_OBJECT) {
            continue;//editor selection state, nobody sets it through a tag list
        }
        if (std::find(tagList.begin(), tagList.end(), previousTag.text) == tagList.end()) {
            this->removeTag(previousTag.text);
        }
    }
    this->dirtyForFrustum = true;// incase we did not remove any tags
}

void Model::setupForTime(uint32_t time) {
    advanceAnimationClock(time);
    evaluatePose();
}

void Model::advanceAnimationClock(uint32_t time) {
    if(animated && !animationLastFramePlayed) {
        animationTime = animationTime + (time - lastSetupTime) * animationTimeScale;
        if(animationBlend) {
            animationTimeOld = animationTimeOld + (time - lastSetupTime) * animationTimeScale;
            if((float)animationTime >= (float)animationBlendTime) {
                animationBlend = false; // no need to blend anymore, the pose is fully the new animation.
            }
            //getTransformBlended reports a missing name as not finished
            animationLastFramePlayed = !animationNameOld.empty() && !animationName.empty() &&
                                       modelAsset->isAnimationFinished(animationNameOld, animationTimeOld, animationLoopedOld) &&
                                       modelAsset->isAnimationFinished(animationName, animationTime, animationLooped);
        } else {
            animationLastFramePlayed = modelAsset->isAnimationFinished(animationName, animationTime, animationLooped);
        }
        posePending = true;
    }
    lastSetupTime = time;
}

void Model::evaluatePose() {
    // we might need to evaluate a pose, if an attachment to a bone exists. In that case, parent pose needs to be updated for child to actually follow
    Model* boneParentModel = parentBoneID != -1 ? dynamic_cast<Model*>(parentObject) : nullptr;
    if(boneParentModel != nullptr) {
        boneParentModel->evaluatePose();
    }
    if(posePending) {
        if(animationBlend) {
            float blendFactor = std::min(1.0f, (float)animationTime / (float)animationBlendTime);
            modelAsset->getTransformBlended(animationNameOld, animationTimeOld, animationLoopedOld,
                                            animationName, animationTime, animationLooped,
                                            blendFactor, boneTransforms);
        } else {
            modelAsset->getTransform(animationTime, animationLooped, animationName, boneTransforms);
        }
        //written even while disconnected, reconnecting would otherwise bring back the bind pose shape
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
        updateAABB();
        posePending = false;
    }

    for (auto boneIterator = exposedBoneTransforms.begin();
         boneIterator != exposedBoneTransforms.end(); ++boneIterator) {
        glm::vec3 temp1;//these are not used
        glm::vec4 temp2;
        glm::vec3 translate, scale;
        glm::quat orientation;

        glm::decompose(this->transformation.getWorldTransform() * boneTransforms[boneIterator->first], scale, orientation, translate, temp1, temp2);
        exposedBoneTransforms[boneIterator->first]->setTransformations(translate,scale, orientation);
    }
}

void Model::renderWithProgram(std::shared_ptr<GraphicsProgram> program, uint32_t lodLevel) {
    for (auto iter = meshMetaData.begin(); iter != meshMetaData.end(); ++iter) {

        if (animated) {
            //set all of the bones to unitTransform for testing
            program->setUniformArray("boneTransformArray[0]", boneTransforms);
            program->setUniform("isAnimated", true);
        } else {
            program->setUniform("isAnimated", false);
        }
        if(program->isMaterialRequired()) {
            (*iter)->material->activateTextures(graphicsWrapper);
        }
        graphicsWrapper->render(program->getID(), (*iter)->mesh->getVao(), (*iter)->mesh->getEbo(), (*iter)->mesh->getTriangleCount()[lodLevel] * 3);
    }
}

RenderList Model::convertToRenderList(uint32_t lodLevel, float depth, int32_t rigIdOverride) const {
    RenderList renderList;

    for (auto iter = meshMetaData.begin(); iter != meshMetaData.end(); ++iter) {
        renderList.addMeshMaterial((*iter)->material, (*iter)->mesh, this, lodLevel, depth, rigIdOverride);
    }
    return renderList;
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

    if (!flipAxes.empty()) {
        currentElement = document.NewElement("Flip");
        currentElement->SetText(flipAxes.c_str());
        objectElement->InsertEndChild(currentElement);
    }

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
    // We can save world transform, or transform relative to parent. But the decision is not as simple as
    // If parent -> local because we have 2 other concerns,
    // 1) Model groups don't actually have a parent, they are located by averaging all objects in group
    // 2) Custom animation is not a parent, but it changes the transform by acting as if
    const GameObject* parentGameObject = dynamic_cast<const GameObject*>(this->parentObject);
    const bool saveLocal = parentGameObject != nullptr && parentGameObject->getTypeID() != GameObject::ObjectTypes::MODEL_GROUP;
    //if part of custom animation, the original position is at the transform parent. Serialize that
    const Transformation* transformationToSave = customAnimation ? transformation.getParentTransform() : &transformation;
    if(saveLocal) {
        transformationToSave->serializeLocal(document, objectElement);
    } else {
        transformationToSave->serialize(document, objectElement);
    }

    //Material customizations
    const std::vector<std::pair<std::string, std::shared_ptr<const Material>>> meshMaterialMap = getNewMeshMaterials();
    if (!meshMaterialMap.empty()) {
        tinyxml2::XMLElement *childrenNode = document.NewElement("MeshMaterialList");
        tinyxml2::XMLElement *childrenCountNode = document.NewElement("Count");
        childrenCountNode->SetText(std::to_string(meshMaterialMap.size()).c_str());
        childrenNode->InsertEndChild(childrenCountNode);
        objectElement->InsertEndChild(childrenNode);
        for (auto& meshMaterial:meshMaterialMap) {
            tinyxml2::XMLElement *childNode = document.NewElement("MeshMaterial");
            childNode->SetAttribute("MeshName", meshMaterial.first.c_str());
            meshMaterial.second->serialize(document, childNode);
            childrenNode->InsertEndChild(childNode);
        }
    }

    std::vector<std::string> tagsToSave;
    for (const HashUtil::HashedString& currentTag:getTags()) {
        if (currentTag.text != HardCodedTags::PICKED_OBJECT) {//editor selection, it is not part of the object
            tagsToSave.emplace_back(currentTag.text);
        }
    }
    if (!tagsToSave.empty()) {
        tinyxml2::XMLElement *tagsNode = document.NewElement("Tags");
        tinyxml2::XMLElement *tagsCountNode = document.NewElement("Count");
        tagsCountNode->SetText(std::to_string(tagsToSave.size()).c_str());
        tagsNode->InsertEndChild(tagsCountNode);
        objectElement->InsertEndChild(tagsNode);
        for (const std::string& tagToSave:tagsToSave) {
            tinyxml2::XMLElement *tagNode = document.NewElement("Tag");
            tagNode->SetText(tagToSave.c_str());
            tagsNode->InsertEndChild(tagNode);
        }
    }
    return true;
}

uint32_t Model::getAIID() {
    if(AIActor == nullptr) {
        return 0;
    }
    return this->AIActor->getWorldID();
}



//the same scale the visibility pass multiplies level distances with, so a distance printed here is the one it
//switches at
float Model::getLodObjectScale() const {
    glm::vec3 scale = this->transformation.getScale();
    return std::max(std::abs(scale.x), std::max(std::abs(scale.y), std::abs(scale.z)));
}

static const char *lodSkipReasonText(LodSkipReason reason) {
    switch (reason) {
        case LodSkipReason::NO_GAIN: return "not built: saves too few triangles over the step before it";
        case LodSkipReason::NOTHING_FITS: return "not built: even the smallest simplification breaks a limit at this distance";
        case LodSkipReason::EMPTY: return "not built: simplifies away to nothing";
        default: return "not built";
    }
}

void Model::putLodPanelInGui(ImGuiResult &result, const ImGuiRequest &request, bool animated, bool isLimonModel) {
    const LodLadder &ladder = modelAsset->getLodLadder();
    const std::vector<LodStep> &steps = ladder.getSteps();
    if (animated) {
        ImGui::TextWrapped("Animated model. A deforming mesh has no pose to render, so its steps keep the project's triangle targets and are never measured.");
    }
    uint32_t originalTriangleCount = ladder.getOriginalTriangleCount();
    ImGui::Text("Original: %u triangles", originalTriangleCount);
    if (steps.empty()) {
        ImGui::Text("No LOD step was configured, everything renders at the original.");
    }

    const float objectScale = getLodObjectScale();
    //the steps as configured, not the levels that happened to be built: a step that could not be built is worth
    //seeing and worth retargeting, and hiding it is how it became unreachable
    for (size_t step = 0; step < steps.size(); ++step) {
        const LodStep &entry = steps[step];
        const LodStep::Outcome &outcome = entry.structure;
        ImGui::PushID((int) step);
        ImGui::Text("%d: used from %.0f m", (int) (step + 1), entry.distance * objectScale);
        if (!outcome.built) {
            ImGui::TextDisabled("   %s", lodSkipReasonText(outcome.skipReason));
        } else {
            ImGui::Text("   %u tris (%.0f%%, target %.0f%%)", outcome.triangleCount, outcome.achievedRatio * 100.0f,
                        entry.triangleTarget * 100.0f);
            //what the level shows at that distance against what it may show, all in pixels of the reference screen
            ImGui::Text("   surface %.1f/%.0f  outline %.1f/%.0f  holes %.1f/%.0f  texture %.1f/%.0f  normal %.1f/%.0f px",
                        outcome.measured.surface, entry.limits.surface, outcome.measured.outline, entry.limits.outline,
                        outcome.measured.holes, entry.limits.holes, outcome.measured.texture, entry.limits.texture,
                        outcome.measured.normal, entry.limits.normal);
            if (outcome.clipped) {
                ImGui::TextWrapped("   Checked below its real screen size, LOD_calibrationMaxResolution capped the render.");
            }
        }
        if (entry.welded.built) {
            ImGui::Text("   welded twin for shadows: %u tris", entry.welded.triangleCount);
        }
        ImGui::PopID();
    }

    //the panel keeps its own copy, so a half typed number never reaches the asset
    static uint32_t editedObjectID = 0xFFFFFFFF;
    //what is in the triangle boxes while they are being typed into, so a half finished number never commits
    static std::vector<float> editedTrianglePercents;
    static int32_t previewLevel = 0;
    if (editedObjectID != this->getWorldObjectID()) {
        editedObjectID = this->getWorldObjectID();
        editedTrianglePercents.clear();
        previewLevel = 0;
    }

    if (!animated) {
        //the live map switches levels by distance, this is how one level is judged on its own
        int32_t maximumLevel = (int32_t) ladder.getMeshLodCount() - 1;
        ImGui::SliderInt("Preview level", &previewLevel, 0, maximumLevel < 0 ? 0 : maximumLevel);
        uint32_t previewWidth = (uint32_t) std::max(ImGui::GetContentRegionAvail().x, 128.0f);
        drawModelPreview(request, result, previewLevel, previewWidth, (previewWidth * 3) / 4);

        //the level next to the original at a size the developer picks, magnified so the difference is visible
        //without resampling it away
        static int32_t comparisonSize = 48;
        if (request.renderLodComparison && previewLevel > 0) {
            ImGui::SliderInt("Compare at px", &comparisonSize, 8, 256);
            LodComparisonImages comparison = request.renderLodComparison(this, previewLevel, (uint32_t) comparisonSize);
            float magnifiedSize = std::min(ImGui::GetContentRegionAvail().x * 0.45f, 220.0f);
            ImVec2 imageSize(magnifiedSize, magnifiedSize);
            ImGui::Text("Level %d and the original at %d px", previewLevel, comparisonSize);
            if (comparison.levelImage != nullptr && comparison.levelImage->texture != nullptr) {
                ImGui::Image((ImTextureID)(intptr_t)comparison.levelImage, imageSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            }
            ImGui::SameLine();
            if (comparison.originalImage != nullptr && comparison.originalImage->texture != nullptr) {
                ImGui::Image((ImTextureID)(intptr_t)comparison.originalImage, imageSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            }
        }
    }

    //edits stay in memory until saved here or by a world save. A converted model rewrites its limonmodel
    if (modelAsset->hasUnsavedChanges()) {
        ImGui::TextWrapped("Unsaved changes, lost on reload unless saved here or with the world.");
    }
    //keyed by object, not reset with editedObjectID: that one is also cleared to re-read after a triangle commit
    static uint32_t savePendingObjectID = 0xFFFFFFFF;
    if (savePendingObjectID != this->getWorldObjectID()) {
        if (ImGui::Button("Save changes to disk")) {
            if (isLimonModel) {
                savePendingObjectID = this->getWorldObjectID();
            } else {
                result.lodPanel.saveChanges = true;
            }
        }
    } else {
        ImGui::TextWrapped("Overwrites the model in the game data. There is no undo.");
        if (ImGui::Button("Overwrite the file")) {
            result.lodPanel.saveChanges = true;
            savePendingObjectID = 0xFFFFFFFF;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            savePendingObjectID = 0xFFFFFFFF;
        }
    }

    if (animated) {
        return;//kept at the project triangle targets, never measured, so there is nothing to retarget
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Triangle counts for this model")) {
        ImGui::TextWrapped("Set how much of the original each step keeps, overriding the step's limits for this model. The engine finds the simplification that lands there and records what it costs, so the step comes back the same on every load.");
        //each step has to stay between its neighbours, or the ladder would go backwards. The gain heuristic does
        //not apply to a number that was typed here, so the bounds are just the neighbours
        for (size_t step = 0; step < steps.size(); ++step) {
            ImGui::PushID((int) step);
            float currentPercent = originalTriangleCount == 0 ? 0.0f
                                                              : 100.0f * (float) steps[step].structure.triangleCount / (float) originalTriangleCount;
            float finerPercent = 100.0f;
            if (step > 0 && originalTriangleCount != 0 && steps[step - 1].structure.built) {
                finerPercent = 100.0f * (float) steps[step - 1].structure.triangleCount / (float) originalTriangleCount;
            }
            float coarserPercent = 0.1f;
            if (step + 1 < steps.size() && originalTriangleCount != 0 && steps[step + 1].structure.built) {
                coarserPercent = 100.0f * (float) steps[step + 1].structure.triangleCount / (float) originalTriangleCount;
            }

            char label[48];
            snprintf(label, sizeof(label), "Step %d triangles %%", (int) (step + 1));
            if (editedTrianglePercents.size() <= step) {
                editedTrianglePercents.resize(step + 1, currentPercent);
            }
            ImGui::InputFloat(label, &editedTrianglePercents[step]);
            //only once the box is finished with. Committing on every keystroke rebuilds the model for the 3 of a
            //32, and a rebuild measures the whole model from fourteen directions
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                //clamped rather than refused, so a number typed past a neighbour still does something sensible
                float requested = std::min(std::max(editedTrianglePercents[step], coarserPercent), finerPercent);
                editedTrianglePercents[step] = requested;
                result.lodPanel.triangleTargetLevel = (int32_t) step;
                result.lodPanel.triangleTargetRatio = requested / 100.0f;
            } else if (!ImGui::IsItemActive()) {
                editedTrianglePercents[step] = currentPercent;//follow the asset while nobody is typing
            }
            ImGui::Text("   allowed %.1f%% to %.1f%%", coarserPercent, finerPercent);
            if (steps[step].requestedRatio > 0.0f &&
                steps[step].structure.achievedRatio > steps[step].requestedRatio * 1.05f) {
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Asked for less than the simplifier will give. Hard edges, protected UV seams and separate parts put a floor under how far a mesh can go.");
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Back to project defaults")) {
            result.lodPanel.clearOverrides = true;
            editedObjectID = 0xFFFFFFFF;//so the next frame re-reads what the asset ended up with
        }
        ImGui::TreePop();
    }
    if (ladder.hasOverrides()) {
        ImGui::Text("Using triangle counts set on this model, not the project defaults.");
    }
    if (ImGui::Button("Recalibrate")) {
        result.lodPanel.recalibrate = true;
    }
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Rebuilds the steps and rewrites the cached result beside the asset. The map picks them up on the next frame.");
}

void Model::drawModelPreview(const ImGuiRequest &request, ImGuiResult &result, int32_t forcedLodLevel, uint32_t previewWidth, uint32_t previewHeight) {
    if (!request.renderModelPreview) {
        return;
    }
    ImGuiImageWrapper* previewWrapper = request.renderModelPreview(this, forcedLodLevel, previewWidth, previewHeight);
    if (previewWrapper == nullptr || previewWrapper->texture == nullptr) {
        return;
    }
    ImVec2 size(static_cast<float>(previewWrapper->texture->getWidth()), static_cast<float>(previewWrapper->texture->getHeight()));
    //flipped via uv0/uv1, not a negated size, that would invert the item rect and IsItemClicked
    //would never fire
    //Child + oversized dummy claims the wheel from the panel, same trick as FlameGraph::HandleInput's canvas.
    //SetScrollY(0) every frame cancels the scroll that trick would otherwise leave on the image.
    //No border/padding: a child adds both by default, and this should sit flush like before the wrapper.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("ModelPreviewImage", size, false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetScrollY(0.0f);
    ImGui::Image((ImTextureID)(intptr_t)previewWrapper, size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    if (ImGui::IsItemClicked()) {
        //raw local pixel only, Editor owns bone screen positions and does the hit-test
        ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 mouse = ImGui::GetMousePos();
        result.modelPreview.clicked = true;
        result.modelPreview.clickPixelX = mouse.x - rectMin.x;
        result.modelPreview.clickPixelY = mouse.y - rectMin.y;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        //orbits the preview camera, raw delta only, PreviewRenderer applies it
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        result.modelPreview.orbitDragging = true;
        result.modelPreview.orbitDeltaX = mouseDelta.x;
        result.modelPreview.orbitDeltaY = mouseDelta.y;
    }
    if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        result.modelPreview.zoomDelta = ImGui::GetIO().MouseWheel;
    }
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

ImGuiResult Model::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    //Allow transformation editing.
    if(transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix, false, parentObject != nullptr)) {
        //true means transformation changed, activate rigid body
        rigidBody->activate();
        result.updated = true;
    }
    static bool usePutOnTop = false;
    result.putOnTop = usePutOnTop;
    ImGui::Checkbox("On Top##PuthOnTopCheckBox", &usePutOnTop);
    if(ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("This will automatically put the object on top of what is found under:");
        ImGui::Text("shortcut T");
        ImGui::EndTooltip();
    }
    ImGui::NewLine();
    static char tagsBuffer[512] = {0};
    std::vector<std::string> editableTags;
    for (const HashUtil::HashedString& currentTag : this->getTags()) {
        if (currentTag.text != HardCodedTags::PICKED_OBJECT) {//selection state, the next click sets it again anyway
            editableTags.emplace_back(currentTag.text);
        }
    }
    std::string joinedTags = StringUtils::join(editableTags, ",");
    strncpy(tagsBuffer, joinedTags.c_str(), std::min(joinedTags.length(), sizeof(tagsBuffer) - 1));
    tagsBuffer[std::min(joinedTags.length(), sizeof(tagsBuffer) - 1)] = 0;

    ImGui::InputText("Tags##ForModelObject", tagsBuffer, sizeof(tagsBuffer), ImGuiInputTextFlags_CharsNoBlank);
    if(ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Render stages pick objects by these tags. The engine set ones can be removed,");
        ImGui::Text("but clearing all of them puts them back, an object with no tags renders nowhere.");
        ImGui::EndTooltip();
    }
    joinedTags = tagsBuffer;
    this->setTags(StringUtils::split(joinedTags, ","));

    if (isAnimated()) {
        ImGui::TextDisabled("Mass: animated objects are kinematic");
    } else {
        float editMass = this->mass;
        if (ImGui::DragFloat("Mass##ForModelObject", &editMass, 0.1f, 0.0f, 1000.0f)) {
            if (editMass < 0.0f) {
                editMass = 0.0f;
            }
            //store the value live (single home for mass, cheap); the actual physics reload is applied on release
            //below, since switching between 0 and >0 removes/re-adds the body and shouldn't run every drag frame.
            this->setMassValue(editMass);
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            result.massChanged = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0 = static (triangle mesh), >0 = dynamic (convex hull). Applied on release.");
        }
    }

    if (isAnimated()) {
        if (ImGui::CollapsingHeader("Model animation properties")) {
            if (ImGui::BeginCombo("Animation Name", animationName.c_str())) {
                for (auto it = modelAsset->getAnimations().begin(); it != modelAsset->getAnimations().end(); it++) {
                    bool isThisAnimationCurrent = this->getAnimationName() == it->first;
                    if (ImGui::Selectable((it->first + "##AnimationName").c_str(), isThisAnimationCurrent)) {
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
        if (ImGui::CollapsingHeader("Expose Bone for attachment")) {
            //Editor bakes the model and skeleton overlay into one texture, see PreviewRenderer::renderModelPreview.
            //Just a display here, no camera/joint logic
            drawModelPreview(request, result, -1, 640, 480);

            ImGui::BeginChild("BoneTreeScrollRegion", ImVec2(0.0f, 200.0f), true);
            int32_t newSelectedBoneID = this->modelAsset->buildEditorBoneTree(selectedBoneID, boneTreeShouldFollowSelection);
            boneTreeShouldFollowSelection = false;//consumed for this frame regardless of whether it found a target
            if (newSelectedBoneID != -1 && newSelectedBoneID != selectedBoneID) {
                selectedBoneID = newSelectedBoneID;
                std::cout << "selected bone is " << selectedBoneID << std::endl;
            }
            ImGui::EndChild();
        } else {
            selectedBoneID = -1;
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
        if(request.imgGuiHelper != nullptr) {
            request.imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_SOUND,
                                                      "StepOnSound",
                                                      &selectedSoundAsset);
        }

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
    bool isLimonModel = name.substr(name.find_last_of(".") + 1) == "limonmodel";
    if (!animated && !isLimonModel) {//a limonmodel carries its flip in the vertices, there is nothing left to toggle
        if (ImGui::CollapsingHeader("Flip axes")) {
            bool flipX = flipAxes.find('X') != std::string::npos;
            bool flipY = flipAxes.find('Y') != std::string::npos;
            bool flipZ = flipAxes.find('Z') != std::string::npos;
            bool flipChanged = false;
            flipChanged |= ImGui::Checkbox("Flip X##ModelFlip", &flipX);
            ImGui::SameLine();
            flipChanged |= ImGui::Checkbox("Flip Y##ModelFlip", &flipY);
            ImGui::SameLine();
            flipChanged |= ImGui::Checkbox("Flip Z##ModelFlip", &flipZ);
            if (flipChanged) {
                std::string newFlipAxes;
                if (flipX) newFlipAxes += 'X';
                if (flipY) newFlipAxes += 'Y';
                if (flipZ) newFlipAxes += 'Z';
                result.flipChanged = true;
                result.newFlipAxes = newFlipAxes;
            }
        }
    }
    if (ImGui::CollapsingHeader("LOD levels")) {
        putLodPanelInGui(result, request, animated, isLimonModel);
    }
    static int32_t selectedIndex = -1;
    static uint32_t selectedModel = 0;
    if (this->getWorldObjectID() != selectedModel) {
        selectedIndex = -1;
        selectedModel = this->getWorldObjectID();
    }
    if (ImGui::CollapsingHeader("Material properties")) {
        //add material listing
        bool isSelected = false;
        if(ImGui::BeginListBox("Meshes##ModelObject")) {
            for (size_t i = 0; i < meshMetaData.size(); ++i) {
                isSelected = selectedIndex == static_cast<int32_t>(i);
                //assimp keeps the source name on both halves of a split mesh, so only the index keeps the ids apart
                if (ImGui::Selectable((meshMetaData[i]->mesh->getName() + " -> " + meshMetaData[i]->material->getName() + "##meshMaterial" + std::to_string(i)).c_str(), isSelected)) {
                    if (selectedIndex != static_cast<int32_t>(i)) { //means selection changed, trigger material change on main window
                        result.selectedMeshMaterial = meshMetaData[i]->material;
                    }
                    selectedIndex = static_cast<int32_t>(i);
                }
            }
            ImGui::EndListBox();
            if (selectedIndex == -1) {
                ImGui::BeginDisabled();
            }
            if (request.materialSelectedInList == nullptr) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Switch material")) {
                //this model now holds a reference of its own, released when the mesh is reseated again or the model dies
                this->setMeshMaterial(selectedIndex, request.materialSelectedInList);
                result.materialChanged = true;
                this->dirtyForFrustum = true;
            }
            if (request.materialSelectedInList == nullptr) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();

            ImGuiHelper::ShowHelpMarker("This will switch the material to the one selected in world editor");
            ImGui::SameLine();
            //altering an alteration would chain copies, and Revert would then land on the previous copy
            //rather than on the material the mesh started with
            if (request.alteredMaterial != nullptr) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Alter material for this model")) {
                //the copy and its edit window belong to the Editor, it outlives this panel and this object
                result.alterMaterialForMeshIndex = selectedIndex;
            }
            if (request.alteredMaterial != nullptr) {
                ImGui::EndDisabled();
            }
            if (request.alteredMaterial != nullptr) {
                if (request.alteredMaterial->addImGuiEditorElements(request).materialDirty) {
                    refreshTransparencyTags();
                }
                if (ImGui::Button("Revert##materialAlterInModel")) {
                    result.revertAlteredMaterial = true;
                }
            }
            if (selectedIndex == -1) {
                ImGui::EndDisabled();
            }
        }
    }
    return result;
}

Model::~Model() {
    if(this->transformation.getParentTransform() != nullptr) {
        this->transformation.removeParentTransform();
    }
    if(this->parentObject != nullptr) {
        this->parentObject->removeChild(this);
    }

    delete motionState;
    delete rigidBody;
    delete compoundShape;
    for (btCollisionShape* shape:childrenPhysicsShapes) {
        delete shape;
    }
    delete AIActor;

    for (size_t i = 0; i < meshMetaData.size(); ++i) {
        //a material this model overrode to belongs to this model, so it goes away with it
        releaseOwnedMeshMaterial(i);
        delete meshMetaData[i];
    }

    // Null out children's parent pointer so they don't dangle; their own
    // destructors will handle transform cleanup.
    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->setParentObject(nullptr);
    }
    assetManager->freeAsset({modelAsset->getAssetName()});
}

Model::Model(const Model &otherModel, uint32_t objectID) :
        Model(objectID, otherModel.assetManager, otherModel.mass, otherModel.name, otherModel.disconnected, otherModel.flipAxes) {
    //we have constructed the object, now set the properties that might have been changed
    this->transformation.setTransformationsNotPropagate(
            otherModel.transformation.getTranslate(),
            otherModel.transformation.getOrientation(),
            otherModel.transformation.getScale()
            );
    this->updateAABB();

    this->animationName = otherModel.animationName;
    this->animationTime = otherModel.animationTime;
    this->animationLooped = otherModel.animationLooped;

    this->animationNameOld = otherModel.animationNameOld;
    this->animationTimeOld = otherModel.animationTimeOld;
    this->animationLoopedOld = otherModel.animationLoopedOld;

    this->animationBlend = otherModel.animationBlend;
    this->animationBlendTime = otherModel.animationBlendTime;

    this->animationLastFramePlayed = otherModel.animationLastFramePlayed;
    this->animationTimeScale = otherModel.animationTimeScale;
    this->lastSetupTime = otherModel.lastSetupTime;

    this->temporary = otherModel.temporary;

    //own Sound instance for the same file, so playback state (soundHandleID) isn't shared with the original
    if (otherModel.stepOnSound != nullptr) {
        this->setPlayerStepOnSound(std::make_shared<Sound>(0, assetManager, otherModel.stepOnSound->getName()));
    }

    std::vector<std::string> otherTags;
    for (const HashUtil::HashedString& otherTag : otherModel.getTags()) {
        if (otherTag.text != HardCodedTags::PICKED_OBJECT) {//the copy is not the selected object, the original is
            otherTags.emplace_back(otherTag.text);
        }
    }
    this->setTags(otherTags);//setTags, so a default tag the original had removed stays removed on the copy

    for (const auto& materialOverride : otherModel.getNewMeshMaterials()) {
        for (size_t meshIndex = 0; meshIndex < this->meshMetaData.size(); ++meshIndex) {
            if (this->meshMetaData[meshIndex]->mesh->getName() == materialOverride.first) {
                //the copy holds its own reference to the override, it does not share the original's
                this->setMeshMaterial(meshIndex, materialOverride.second);
                break;
            }
        }
    }

    //AI and parent/child attachment are intentionally not copied here: AI needs a world-registered
    //ID and children need their own copies. See Editor::copyAttachableRecursive, which builds on
    //this constructor via clone().
}

ImGuiResult Model::putAIonGUI(ActorInterface *actorInterface,
                                          std::vector<LimonTypes::GenericParameter> &parameters,
                                          const ImGuiRequest &request, std::string &lastSelectedAIName) {
    ImGuiResult result;
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
            bool isSet = request.generateEditorElementsForParameters(parameters, 0);
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

std::vector<std::pair<std::string, std::shared_ptr<const Material>>> Model::getNewMeshMaterials() const{
    std::vector<std::pair<std::string, std::shared_ptr<const Material>>> meshMaterialList;

    for (const auto& thisMeshMaterial:meshMetaData) {
        std::shared_ptr<Material> material = this->modelAsset->getMeshMaterial(thisMeshMaterial->mesh);
        if (material == nullptr) {
            std::cerr << "Model has a mesh that is not part of asset. This should not happen. " << std::endl;
            continue;
        }
        if (material != thisMeshMaterial->material) {//difference in materials, we should save this.
            //global overrides are already saved once as a <Materials> entry and come back through their
            //forwarding rule, don't repeat them for every mesh showing them
            /*
             * A world override ends up written here as well as under <Materials>. That is redundant, not
             * wrong: on load the per mesh copy and the world one dedup by content into a single material.
             * Filtering on originalHash instead would give that field a second meaning and collide with the
             * "is this world owned" test in Editor::ensureWorldOwnedMaterial.
             */
            meshMaterialList.emplace_back(thisMeshMaterial->mesh->getName(), thisMeshMaterial->material);
        }
    }
    return meshMaterialList;
}

void Model::releaseOwnedMeshMaterial(size_t meshIndex) {
    MeshMeta* meshMeta = meshMetaData[meshIndex];
    if (meshMeta->material == nullptr) {
        return;
    }
    if (meshMeta->material == modelAsset->getMeshMaterial(meshMeta->mesh)) {
        return;//borrowed from the asset, which releases it itself in ~ModelAsset
    }
    assetManager->getMaterialRegistry().unregisterMaterial(meshMeta->material);
    meshMeta->material = nullptr;
}

std::shared_ptr<const Material> Model::setMeshMaterial(size_t meshIndex, std::shared_ptr<const Material> material) {
    if (meshIndex >= meshMetaData.size()) {
        std::cerr << "Mesh index " << meshIndex << " is out of range for model " << this->name << std::endl;
        return nullptr;
    }
    MeshMeta* meshMeta = meshMetaData[meshIndex];
    if (meshMeta->material == material) {
        return material;
    }
    releaseOwnedMeshMaterial(meshIndex);

    std::shared_ptr<const Material> materialToUse = material;
    if (material != nullptr && material != modelAsset->getMeshMaterial(meshMeta->mesh)) {
        //const_pointer_cast because registering assigns materialIndex. Install what comes back, not what
        //we asked for, dedup can hand us a different instance
        materialToUse = assetManager->getMaterialRegistry().registerMaterial(std::const_pointer_cast<Material>(material));
    }
    meshMeta->material = materialToUse;
    refreshTransparencyTags();
    return materialToUse;
}

void Model::loadOverriddenMeshMaterial(std::vector<std::pair<std::string, std::shared_ptr<Material>>> &customisedMeshMaterialList) {
    for (const auto& thisMeshMaterial:customisedMeshMaterialList) {
        //find the mesh
        bool found = false;
        for (size_t meshIndex = 0; meshIndex < this->meshMetaData.size(); ++meshIndex) {
            if (this->meshMetaData[meshIndex]->mesh->getName() == thisMeshMaterial.first) {
                std::shared_ptr<Material> newMaterial = thisMeshMaterial.second;
                newMaterial->loadGPUSide(assetManager.get());
                //setMeshMaterial registers, and can hand back a deduplicated instance instead
                std::shared_ptr<const Material> installedMaterial = setMeshMaterial(meshIndex, newMaterial);
                if (installedMaterial != nullptr) {
                    graphicsWrapper->setMaterial(*installedMaterial);
                }
                found = true;
            }
        }
        if (!found) {
            std::cerr << "Model " << this->name << " doesn't have a mesh with name " << thisMeshMaterial.first << " This should never happen" << std::endl;
        }
    }
}

void Model::attachAI(ActorInterface *AIActor) {
    //after this, clearing the AI is job of the model.
    this->AIActor = AIActor;
    lastSelectedAIName = AIActor->getName();
}

void Model::reloadWithFlip(const std::string &newFlipAxes) {
    if (animated) {
        std::cerr << "WARNING: flip change requested for animated model " << name << " — flip is not supported for animated meshes, ignoring." << std::endl;
        return;
    }

    //release before modelAsset is swapped below, after that we can't tell which of these we registered
    for (size_t i = 0; i < meshMetaData.size(); ++i) {
        releaseOwnedMeshMaterial(i);
    }

    std::string oldKey = modelAsset->getAssetName();
    std::string newKey = newFlipAxes.empty() ? name : name + "?flip" + newFlipAxes;
    flipAxes = newFlipAxes;

    std::shared_ptr<ModelAsset> newModelAsset = assetManager->loadAsset<ModelAsset>({newKey});
    assetManager->freeAsset({oldKey});
    modelAsset = newModelAsset;

    this->centerOffset = modelAsset->getCenterOffset();
    this->centerOffsetMatrix = glm::translate(glm::mat4(1.0f), centerOffset);
    this->transformation.markDirty();

    reloadPhysicsShape();

    for (size_t i = 0; i < meshMetaData.size(); ++i) { delete meshMetaData[i]; }
    meshMetaData.clear();
    for (auto &mesh : modelAsset->getMeshes()) {
        MeshMeta *meshMeta = new MeshMeta();
        meshMeta->mesh = mesh;
        meshMeta->material = modelAsset->getMeshMaterial(mesh);
        meshMetaData.push_back(meshMeta);
    }

    this->dirtyForFrustum = true;
    markTransformChanged();
}

void Model::reloadPhysicsShape() {
    delete compoundShape;
    for (btCollisionShape *shape : childrenPhysicsShapes) { delete shape; }
    childrenPhysicsShapes.clear();
    boneIdCompoundChildMap.clear();

    compoundShape = modelAsset->getCompoundShapeForMass(this->mass, this->boneIdCompoundChildMap, childrenPhysicsShapes);
    rigidBody->setCollisionShape(compoundShape);
    glm::vec3 currentScale = transformation.getScale();
    rigidBody->getCollisionShape()->setLocalScaling(btVector3(currentScale.x, currentScale.y, currentScale.z));

    btVector3 fallInertia(0, 0, 0);
    if (this->mass > 0) {
        compoundShape->calculateLocalInertia(this->mass, fallInertia);
        //became (or stayed) dynamic: clear the static flag and make sure the body is awake
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
    } else {
        //became (or stayed) static: Bullet decides static-ness by this flag, the group selection on re-add reads it
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    }
    rigidBody->setMassProps(this->mass, fallInertia);
    rigidBody->updateInertiaTensor();
    rigidBody->activate();

    // We want to keep the physics tags match the physics state, but if user removed them, we don't object
    const bool hasPhysicalTag = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_PHYSICAL));
    const bool hasStaticTag = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_STATIC));
    if (this->mass > 0 && hasStaticTag) {
        //add before remove, removing the last tag would refill the list with the defaults
        this->addTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
        this->removeTag(HardCodedTags::OBJECT_MODEL_STATIC);
    } else if (this->mass <= 0 && hasPhysicalTag) {
        this->addTag(HardCodedTags::OBJECT_MODEL_STATIC);
        this->removeTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
    }
}

bool Model::isTransparent() const {
    if (animated) {
        return false;//no stage renders animated transparent models
    }
    for (const MeshMeta* meshMeta : meshMetaData) {
        const std::shared_ptr<const Material> &material = meshMeta->material;
        if (material == nullptr) {
            continue;
        }
        if (material->hasOpacityMap()) {
            return true;
        }
        if (material->hasDiffuseMap() && material->getDiffuseTexture()->hasTransparentTexels()) {
            return true;
        }
    }
    return false;
}

void Model::refreshTransparencyTags() {
    const bool transparent = this->isTransparent();
    const bool hasBasicTag = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_BASIC));
    const bool hasTransparentTag = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_TRANSPARENT));
    const bool hasAmbientTag = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_AMBIENT));
    const bool underPlayer = this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_PLAYER_BASIC)) ||
                             this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_PLAYER_ANIMATED)) ||
                             this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_PLAYER_TRANSPARENT));
    //add before remove, removing the last tag would refill the list with the defaults
    if (transparent && !hasTransparentTag && (hasBasicTag || hasAmbientTag)) {
        this->addTag(HardCodedTags::OBJECT_MODEL_TRANSPARENT);
        if (underPlayer) {
            this->addTag(HardCodedTags::OBJECT_PLAYER_TRANSPARENT);
        }
        if (hasBasicTag) {
            this->removeTag(HardCodedTags::OBJECT_MODEL_BASIC);
            if (this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_PLAYER_BASIC))) {
                this->removeTag(HardCodedTags::OBJECT_PLAYER_BASIC);
            }
        }
    } else if (!transparent && hasTransparentTag) {
        if (!hasAmbientTag) {//an ambient model renders in the ambient stage, basic too would draw it twice
            this->addTag(HardCodedTags::OBJECT_MODEL_BASIC);
            if (underPlayer) {
                this->addTag(HardCodedTags::OBJECT_PLAYER_BASIC);
            }
        }
        this->removeTag(HardCodedTags::OBJECT_MODEL_TRANSPARENT);
        if (this->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_PLAYER_TRANSPARENT))) {
            this->removeTag(HardCodedTags::OBJECT_PLAYER_TRANSPARENT);
        }
    }
}

void Model::convertAssetToLimon(std::set<std::vector<std::string>> &convertedModels [[gnu::unused]]) {
#ifdef CEREAL_SUPPORT
    std::vector<std::string> nameVector;
    nameVector.push_back(name);
    nameVector.push_back(flipAxes);//each flip is different geometry, so it gets its own file
    std::string newName = name.substr(0, name.find_last_of("."));
    if (!flipAxes.empty()) {
        newName += "_flip" + flipAxes;//windows filenames can not hold the ?flip the asset key uses
    }
    newName += ".limonmodel";
    bool written = convertedModels.find(nameVector) != convertedModels.end();
    if (!written && modelAsset->writeBinary(newName)) {
        convertedModels.insert(nameVector);
        written = true;
    }
    if (written) {//renaming to a file that was not written would make the next world load exit
        this->name = newName;
        this->flipAxes.clear();//the mirror is in the exported vertices now, flipping again would undo it
    }

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
