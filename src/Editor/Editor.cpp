//
// Created by engin on 28/09/2021.
//

#include <Assets/Animations/AnimationLoader.h>
#include "Editor.h"
#include "World.h"
#include "ImGuiHelper.h"
#include "Camera/PerspectiveCamera.h"
#include "GameObjects/Model.h"
#include "GameObjects/ModelGroup.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Players/MenuPlayer.h"
#include "GameObjects/Players/FreeCursorPlayer.h"
#include "GameObjects/Players/FreeMovingPlayer.h"
#include "GameObjects/TriggerObject.h"
#include "GUI/GUICursor.h"
#include "GUI/GUILayer.h"
#include "Assets/Animations/AnimationCustom.h"
#include "AnimationSequencer.h"
#include "WorldSaver.h"
#include "AI/AIMovementGrid.h"
#include "../Utils/ClosestNotMeConvexResultCallback.h"
#include "Graphics/Particles/Emitter.h"
#include "nodeGraph/src/NodeGraph.h"
#include "NodeEditorExtensions/PipelineExtension.h"
#include "NodeEditorExtensions/IterationExtension.h"
#include "NodeEditorExtensions/PipelineStageExtension.h"
#include "GameObjects/GUIText.h"
#include "GameObjects/GUIImage.h"
#include "GameObjects/GUIButton.h"
#include "GameObjects/GUIAnimation.h"

std::shared_ptr<const Material> EditorNS::selectedMeshesMaterial = nullptr;
std::shared_ptr<Material> EditorNS::selectedFromListMaterial = nullptr;

Editor::Editor(World *world) : world(world){
    backgroundRenderStage = std::make_unique<GraphicsPipelineStage>(world->graphicsWrapper, 640,480,"","",true,true,true,false,false);
    colorTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 640, 480);
    colorTexture->setName("EditorColorTexture");
    colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    depthTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::FLOAT, 640, 480);
    depthTexture->setName("EditorDepthTexture");
    colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    backgroundRenderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::COLOR0, colorTexture, true);
    backgroundRenderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, depthTexture, true);
    wrapper = new ImGuiImageWrapper();
    imgGuiHelper = new ImGuiHelper(world->assetManager, world->options);

}

Editor::~Editor() {
    delete wrapper;
    delete nodeGraph;
    delete pipelineExtension;
    delete iterationExtension;
    delete imgGuiHelper;
    delete request;
}

//This method is used only for ImGui loaded animations list generation
bool getNameOfLoadedAnimation(void* data, int index, const char** outText) {
    auto& animations = *static_cast<std::vector<AnimationCustom> *>(data);
    if(index < 0 || (uint32_t)index >= animations.size()) {
        return false;
    }
    *outText = animations.at(index).getName().c_str();
    return true;

}

Model* Editor::getModelAndMoveToEnd(const std::string& modelFilePath) {
    for(auto iter = modelQueue.begin(); iter != modelQueue.end(); ++iter) {
        Model* model = *iter;
        if (model->getName() == modelFilePath + "_" + std::to_string(model->getWorldObjectID())) {
            modelQueue.erase(iter);
            modelQueue.emplace_back(model);
            return model;
        }
    }
    return nullptr;
}
Model * Editor::createRenderAndAddModelToLRU(const std::string &modelFileName, const glm::vec3 &newObjectPosition, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    uint32_t newWorldObjectId;
    if(modelQueue.size() >= MAX_PRELOAD_MODEL_COUNT_EDITOR) {
        newWorldObjectId = modelQueue[0]->getWorldObjectID();
        delete modelQueue[0];
        modelQueue.erase(modelQueue.begin());
    } else {
        newWorldObjectId = (*modelIdSet.begin());
        modelIdSet.erase(modelIdSet.begin());
    }

    Model* model = new Model(newWorldObjectId, world->assetManager, modelFileName);// FIXME this will cause gaps, we should reserve and reuse
    modelQueue.push_back(model);
    setTransformToModel(model, newObjectPosition);
    renderSelectedObject(model, graphicsProgram);
    return model;
}

std::unique_ptr<ClosestNotMeConvexResultCallback> Editor::convexSweepTestDown(Model * selectedObject) const {
    std::unique_ptr<ClosestNotMeConvexResultCallback> resultCallback = std::make_unique<ClosestNotMeConvexResultCallback>(selectedObject->getRigidBody());
    btCompoundShape *compoundShape = selectedObject->getCompoundShapeForSweepTest();//Creates a new shape, that is convex hull of the compound shape or the shape itself if it is convex.
    btTransform originalTransform = selectedObject->getRigidBody()->getWorldTransform();
    originalTransform.setOrigin(originalTransform.getOrigin()  + btVector3(0, 1.0f, 0));
    for (int i = 0; i < compoundShape->getNumChildShapes(); ++i) {
        btCompoundShapeChild child = compoundShape->getChildList()[i];
        btTransform childBaseTransform = child.m_transform;
        btConvexShape *childConvexShape = dynamic_cast<btConvexShape *>(compoundShape->getChildShape(i));
        btVector3 scaling;
        scaling.setX(childBaseTransform.getBasis().getColumn(0).getX());
        scaling.setY(childBaseTransform.getBasis().getColumn(1).getY());
        scaling.setZ(childBaseTransform.getBasis().getColumn(2).getZ());
        childConvexShape->setLocalScaling(scaling * childConvexShape->getLocalScaling());
        btTransform fromTransform;
        fromTransform.setBasis(childBaseTransform.getBasis() * originalTransform.getBasis());
        fromTransform.setOrigin(childBaseTransform.getOrigin() + originalTransform.getOrigin());
        btTransform toTransform = fromTransform;
        toTransform.setOrigin(toTransform.getOrigin() + btVector3(0, -10, 0));
        world->dynamicsWorld->convexSweepTest(childConvexShape, fromTransform, toTransform, *resultCallback);
    }
    return resultCallback;
}

void Editor::renderEditor(std::shared_ptr<GraphicsProgram> graphicsProgram) {

    imgGuiHelper->NewFrame(graphicsProgram);
    if(this->showNodeGraph) {
        this->drawNodeEditor();
        imgGuiHelper->RenderDrawLists(graphicsProgram);
        return;
    }
    /* window definitions */
    {
        ImGui::Begin("Editor");
        ImGuiIO& io = ImGui::GetIO();
        if(world->guiPickMode == false) {
            if (ImGui::Button("Switch to GUI selection mode")) {
                world->guiPickMode = true;
            }
        } else {
            if (ImGui::Button("Switch to World selection mode")) {
                world->guiPickMode = false;
            }
        }

        //list available elements
        static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;
        glm::vec3 newObjectPosition = world->playerCamera->getPosition() + 10.0f * world->playerCamera->getCenter();


        if (ImGui::CollapsingHeader("Add New Object")) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            static char modelAssetFilter[32] = {0};
            ImGui::InputText("Filter Assets ##ModelsAssetTreeFilter", modelAssetFilter, sizeof(modelAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
            std::string modelAssetFilterStr = modelAssetFilter;
            std::transform(modelAssetFilterStr.begin(), modelAssetFilterStr.end(), modelAssetFilterStr.begin(), ::tolower);
            const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_MODEL, modelAssetFilterStr);
            imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_MODEL,
                                              "Model",
                                              &selectedAsset);
            io.ConfigFlags |= !ImGuiConfigFlags_NavEnableKeyboard;
            static float newObjectWeight;
            ImGui::NewLine();
            wrapper->layer = 0;
            wrapper->texture = colorTexture;
            ImVec2 size;
            size.x = wrapper->texture->getWidth();
            size.y = wrapper->texture->getHeight();
            ImGui::Dummy(ImVec2(0.0f, size.y));
            size.y = -1 * size.y;//This is because ImGui assumes y up. Since this code is shared with fonts, and fixing font generation is hard, I am using this hack for upside down fix.

            //ImGui::Image(this->colorTexture->getTextureID(), ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImColor(255,255,255,255), ImColor(255,255,255,128));
            ImGui::Image((ImTextureID)(intptr_t)wrapper, size);
            ImGui::NewLine();
            ImGui::SliderFloat("Weight", &newObjectWeight, 0.0f, 100.0f);
            ImGui::NewLine();
            if(selectedAsset == nullptr) {
                ImGui::Button("Add Object");
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("No Asset Selected!");
            } else {
                if(this->modelIdSet.empty() && modelQueue.empty()) {
                    for(size_t i =0; i < MAX_PRELOAD_MODEL_COUNT_EDITOR; ++i) {
                        this->modelIdSet.insert(world->getNextObjectID());
                    }
                }
                if((modelAssetsWaitingCPULoad.find(selectedAsset->fullPath) == modelAssetsWaitingCPULoad.end()
                        && world->assetManager->isLoaded({selectedAsset->fullPath})) || modelAssetsPreloaded.find(selectedAsset->fullPath) != modelAssetsPreloaded.end()) {
                    // Preloaded case
                    Model* model = getModelAndMoveToEnd(selectedAsset->fullPath);
                    if(model == nullptr) {
                        this->createRenderAndAddModelToLRU(selectedAsset->fullPath, newObjectPosition, graphicsProgram);
                        modelAssetsPreloaded.erase(selectedAsset->fullPath);
                        world->assetManager->freeAsset({selectedAsset->fullPath});
                    } else {
                        setTransformToModel(model, newObjectPosition);
                        renderSelectedObject(model, graphicsProgram);
                    }
                } else if(modelAssetsWaitingCPULoad.find(selectedAsset->fullPath) != modelAssetsWaitingCPULoad.end()) {
                    if(modelAssetsWaitingCPULoad[selectedAsset->fullPath]->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
                        world->assetManager->partialLoadGPUSide(modelAssetsWaitingCPULoad[selectedAsset->fullPath]);
                        modelAssetsPreloaded[selectedAsset->fullPath] = modelAssetsWaitingCPULoad[selectedAsset->fullPath];
                        modelAssetsWaitingCPULoad.erase(selectedAsset->fullPath);
                    } else {
                        ImGui::Text("Loading...");
                    }
                } else {
                    //Requesting Load case
                    modelAssetsWaitingCPULoad[selectedAsset->fullPath] = world->assetManager->partialLoadAssetAsync<ModelAsset>({selectedAsset->fullPath});
                }
                if(ImGui::Button("Add Object")) {
                    Model* newModel = new Model(world->getNextObjectID(), world->assetManager, newObjectWeight,
                                                selectedAsset->fullPath, false);
                    newModel->getTransformation()->setTranslate(newObjectPosition);
                    world->addModelToWorld(newModel);
                    newModel->getRigidBody()->activate();
                    if(this->pickedObject != nullptr ) {
                        this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                    }
                    this->pickedObject = static_cast<GameObject*>(newModel);
                    this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                }
            }
        }
        if(this->pickedObject != nullptr && this->pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
            ImGui::Separator();
            static float copyOffsets[3] { 0.25f, 0.25f, 0.25f};
            ImGui::DragFloat3("Copy position offsets", copyOffsets, 0.1f);
            if (ImGui::Button("Copy Selected object")) {
                if(this->pickedObject != nullptr ) {
                    this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                }
                Model* pickedModel = dynamic_cast<Model*>(this->pickedObject);
                this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                Model* newModel = new Model(*pickedModel, world->getNextObjectID());
                newModel->getTransformation()->addTranslate(glm::vec3(copyOffsets[0], copyOffsets[1], copyOffsets[2]));
                world->addModelToWorld(newModel);
                //now we should apply the animations

                if(world->onLoadAnimations.find(pickedModel) != world->onLoadAnimations.end() &&
                        world->activeAnimations.find(pickedModel) != world->activeAnimations.end()) {
                    world->addAnimationToObject(newModel->getWorldObjectID(), world->activeAnimations[pickedModel]->animationIndex,
                                         true, true);
                }
                if(this->pickedObject != nullptr ) {
                    this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                }
                this->pickedObject = static_cast<GameObject*>(newModel);
                this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
            }

            if(ImGui::Button("Attach this object to another")) {

                this->objectToAttach = dynamic_cast<Model*>(this->pickedObject);
            }
            if(this->objectToAttach != nullptr) {
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + this->objectToAttach->getName());
            }
            if(this->objectToAttach!= nullptr && this->objectToAttach->getWorldObjectID() != this->pickedObject->getWorldObjectID()) {
                std::string savedObjectName = this->objectToAttach->getName();
                if (ImGui::Button("Attach saved object to current")) {
                    Model *pickedModel = dynamic_cast<Model *>(this->pickedObject);
                    int32_t attachedBoneID;
                    Transformation* pickedModelTransformation = pickedModel->getAttachmentTransform(attachedBoneID);

                    glm::vec3 translate, scale;
                    glm::quat orientation;
                    pickedModelTransformation->getDifferenceStacked(*this->objectToAttach->getTransformation(), translate,
                                                                     scale, orientation);
                    this->objectToAttach->getTransformation()->setTranslate(translate);
                    this->objectToAttach->getTransformation()->setScale(scale);
                    this->objectToAttach->getTransformation()->setOrientation(orientation);
                    this->objectToAttach->getTransformation()->setParentTransform(pickedModelTransformation);
                    this->objectToAttach->setParentObject(pickedModel, attachedBoneID);
                    pickedModel->addChild(this->objectToAttach);
                    this->objectToAttach = nullptr;
                }
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + savedObjectName);
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Current Object: " + this->pickedObject->getName());
            }

            if(this->pickedObject != nullptr && this->pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
                Model *pickedModel = dynamic_cast<Model *>(this->pickedObject);
                if(pickedModel->getParentObject() != nullptr) {
                    if (ImGui::Button("Detach object from parent")) {
                        pickedModel->getTransformation()->removeParentTransform();
                        pickedModel->removeParentObject();
                    }
                }
            }
        }
        if(this->pickedObject != nullptr && this->pickedObject->getTypeID() == GameObject::ObjectTypes::PLAYER) {
            if(this->objectToAttach!= nullptr && this->objectToAttach->getWorldObjectID() != this->pickedObject->getWorldObjectID()) {
                if (ImGui::Button("Attach saved object to Player")) {
                    world->physicalPlayer->setAttachedModel(this->objectToAttach);
                    world->clearWorldRefsBeforeAttachment(this->objectToAttach, true);
                    world->startingPlayer.attachedModel = this->objectToAttach;
                    this->objectToAttach = nullptr;
                }
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Model Groups")) {
            static uint32_t selectedModelGroup = 0;

            if (ImGui::BeginCombo("Model Group##combobox", (selectedModelGroup == 0? "No Group Selected." : world->modelGroups[selectedModelGroup]->getName().c_str()))) {
                for (auto iterator = world->modelGroups.begin();
                     iterator != world->modelGroups.end(); ++iterator) {
                    bool isThisTypeSelected = iterator->first == selectedModelGroup;
                    if (ImGui::Selectable(iterator->second->getName().c_str(), isThisTypeSelected)) {
                        selectedModelGroup = iterator->first;
                    }
                    if (isThisTypeSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            PhysicalRenderable* pickedPhysicalRenderable = dynamic_cast<PhysicalRenderable*>(this->pickedObject);
            if(pickedPhysicalRenderable != nullptr && selectedModelGroup != 0 && this->pickedObject->getWorldObjectID() != selectedModelGroup) {
                //now prevent adding to self

                if(ImGui::Button("Add model to group")) {
                    world->modelGroups[selectedModelGroup]->addChild(pickedPhysicalRenderable);
                }
            } else {
                ImGui::Button("Add model to group");
                if(this->pickedObject == nullptr) {
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No object Selected");
                }else {
                    if (this->pickedObject->getWorldObjectID() == selectedModelGroup) {
                        ImGui::SameLine();
                        ImGuiHelper::ShowHelpMarker("Group can't be added to self");
                    }
                    if(pickedPhysicalRenderable == nullptr) {
                        ImGui::SameLine();
                        ImGuiHelper::ShowHelpMarker("Selected object is not a model or model group");
                    }
                }
                if(selectedModelGroup == 0) {
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No model group set to add.");
                }
            }
            ImGui::Separator();
            static char modelGroupNameBuffer[32] = {0};
            ImGui::InputText("Name of the Model Group: ", modelGroupNameBuffer, sizeof(modelGroupNameBuffer), ImGuiInputTextFlags_CharsNoBlank);
            if(modelGroupNameBuffer[0] != 0 ) {
                if(ImGui::Button("Create Group")) {
                    ModelGroup* modelGroup = new ModelGroup(world->graphicsWrapper, world->getNextObjectID(), std::string(modelGroupNameBuffer));
                    world->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;

                }
            } else {
                ImGui::Button("Create Group");
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Name is mandatory!");

            }
        }
        if(ImGui::Button("Add Trigger Volume")) {

            TriggerObject* to = new TriggerObject(world->getNextObjectID(), world->apiInstance);
            to->getTransformation()->setTranslate(newObjectPosition);
            world->dynamicsWorld->addCollisionObject(to->getGhostObject(), World::CollisionTypes::COLLIDE_TRIGGER_VOLUME | World::CollisionTypes::COLLIDE_EVERYTHING,
                                                    World::CollisionTypes::COLLIDE_PLAYER | World::CollisionTypes::COLLIDE_EVERYTHING);
            world->triggers[to->getWorldObjectID()] = to;
            if(this->pickedObject != nullptr ) {
                this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
            }
            this->pickedObject = static_cast<GameObject*>(to);
            this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
        }

        if (ImGui::CollapsingHeader("Add New Light")) {

            if(ImGui::Button("Add Point Light")) {
                Light* newLight = new Light(world->graphicsWrapper, world->getNextObjectID(), Light::LightTypes::POINT, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                world->addLight(newLight);
                this->pickedObject = newLight;
            }
            if(world->directionalLightIndex == -1) {//Allow single directional light
                if(ImGui::Button("Add Directional Light")) {
                    Light* newLight = new Light(world->graphicsWrapper, world->getNextObjectID(), Light::LightTypes::DIRECTIONAL, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                    world->addLight(newLight);
                }
            }

        }

        if (ImGui::CollapsingHeader("Add GUI Elements##The header")) {
            ImGui::Indent( 16.0f );
            if (ImGui::CollapsingHeader("Add GUI Layer##The header")) {
                this->addGUILayerControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Text##The header")) {
                this->addGUITextControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Image##The header")) {
                this->addGUIImageControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Button##The header")) {
                this->addGUIButtonControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Animation##The header")) {
                this->addGUIAnimationControls();
            }
            ImGui::Unindent( 16.0f );
        }
        if (ImGui::CollapsingHeader("Add Particle Emitter ##The header")) {
            this->addParticleEmitterEditor();
        }

        if (ImGui::CollapsingHeader("Custom Animations")) {
            //list loaded animations
            int listbox_item_current = -1;//not static because I don't want user to select a item.
            ImGui::ListBox("Loaded animations", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&world->loadedAnimations), world->loadedAnimations.size(), 10);
            ImGui::Separator();


            ImGui::NewLine();
            static char loadAnimationNameBuffer[32];
            ImGui::Text("Load animation from file:");
            //double # because I don't want to show it
            ImGui::InputText("##LoadAnimationNameField", loadAnimationNameBuffer, sizeof(loadAnimationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);
            if (ImGui::Button("load animation")) {
                AnimationCustom *animation = AnimationLoader::loadAnimation("./Data/Animations/" + std::string(loadAnimationNameBuffer) + ".xml");
                if (animation == nullptr) {
                    world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation load failed");
                } else {
                    world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation loaded");
                    world->loadedAnimations.push_back(*animation);
                }
            }
        }
        ImGui::Separator();
        if(ImGui::CollapsingHeader("Player properties")) {
            if (ImGui::BeginCombo("Starting Type", world->startingPlayer.typeToString().c_str())) {
                for (auto iterator = World::PlayerInfo::typeNames.begin();
                     iterator != World::PlayerInfo::typeNames.end(); ++iterator) {
                    bool isThisTypeSelected = iterator->second == world->startingPlayer.typeToString();
                    if (ImGui::Selectable(iterator->second.c_str(), isThisTypeSelected)) {
                        world->startingPlayer.setType(iterator->second);
                    }
                    if (isThisTypeSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            static bool showError = false;
            if(ImGui::InputText("Custom Extension name", this->extensionNameBuffer, 31, ImGuiInputTextFlags_CharsNoBlank)) {
                showError = false;
            }

            if(world->startingPlayer.attachedModel != nullptr) {
                if(ImGui::Button("Disconnect Attachment##player attachment")) {
                    Model* attachedModel = world->startingPlayer.attachedModel;
                    world->startingPlayer.attachedModel = nullptr;
                    world->physicalPlayer->setAttachedModel(nullptr);
                    world->addModelToWorld(attachedModel);
                }
            }

            if(ImGui::Button("Apply##PlayerExtensionUpdate")) {
                std::string tempName = this->extensionNameBuffer;
                //find the starting player, and apply this change to it:
                Player* playerToUpdate = nullptr;
                switch (world->startingPlayer.type) {
                    case World::PlayerInfo::Types::DEBUG_PLAYER: playerToUpdate = world->debugPlayer; break;
                    case World::PlayerInfo::Types::EDITOR_PLAYER: playerToUpdate = world->editorPlayer; break;
                    case World::PlayerInfo::Types::PHYSICAL_PLAYER: playerToUpdate = world->physicalPlayer; break;
                    case World::PlayerInfo::Types::MENU_PLAYER: playerToUpdate = world->menuPlayer; break;
                }
                if(playerToUpdate != nullptr && tempName != "") {
                    PlayerExtensionInterface* extension = PlayerExtensionInterface::createExtension(tempName, world->apiInstance);
                    if(extension != nullptr) {
                        world->startingPlayer.extensionName = tempName;
                        playerToUpdate->setPlayerExtension(extension);
                    } else {
                        showError = true;
                    }
                }
            }
            if(showError) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("The name didn't match an extension. The info won't be saved!");
                ImGui::PopStyleColor();
            }
        }
        ImGui::Separator();
        if(ImGui::CollapsingHeader("World properties")) {
            ImGui::Indent( 16.0f );

            if (ImGui::CollapsingHeader("Add on load trigger")) {
                static size_t onLoadTriggerIndex = 0;//maximum size
                std::string selectedID = std::to_string(onLoadTriggerIndex);
                if (ImGui::BeginCombo("Current Triggers", selectedID.c_str())) {
                    for (size_t i = 0; i < world->onLoadActions.size(); i++) {
                        bool isTriggerSelected = selectedID == std::to_string(i);

                        if (ImGui::Selectable(std::to_string(i).c_str(), isTriggerSelected)) {
                            onLoadTriggerIndex = i;
                        }
                        if (isTriggerSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                //currently any trigger object can have 3 elements, so this should be >2 to avoid collision on imgui tags. I am assigning 100 just to be safe
                TriggerObject::PutTriggerInGui(world->apiInstance, world->onLoadActions[onLoadTriggerIndex]->action,
                                               world->onLoadActions[onLoadTriggerIndex]->parameters,
                                               world->onLoadActions[onLoadTriggerIndex]->enabled, 100 + onLoadTriggerIndex);
                if (world->onLoadActions[world->onLoadActions.size() - 1]->enabled) {
                    //when user presses the enable button, add another and select it
                    onLoadTriggerIndex = world->onLoadActions.size();
                    world->onLoadActions.push_back(new World::ActionForOnload());
                }
            }

            if(ImGui::CollapsingHeader("Music")) {
                ImGui::Indent(16.0f);
                static const AssetManager::AvailableAssetsNode *selectedSoundAsset = nullptr;
                static char musicAssetFilter[32] = {0};
                ImGui::InputText("Filter Assets ##MusicAssetTreeFilter", musicAssetFilter, sizeof(musicAssetFilter),
                                 ImGuiInputTextFlags_CharsNoBlank);
                std::string musicAssetFilterStr = musicAssetFilter;
                std::transform(musicAssetFilterStr.begin(), musicAssetFilterStr.end(), musicAssetFilterStr.begin(),
                               ::tolower);
                const AssetManager::AvailableAssetsNode *filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(
                        AssetManager::Asset_type_SOUND, musicAssetFilterStr);
                imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_SOUND,
                                                  "Music",
                                                  &selectedSoundAsset);

                if (world->music != nullptr) {
                    ImGui::Text("Current Music: %s", world->music->getName().c_str());
                } else {
                    ImGui::Text("No music set for level. ");
                }

                if (selectedSoundAsset != nullptr) {
                    if (ImGui::Button("Change Music")) {
                        if (world->music != nullptr) {
                            world->music->stop();
                            delete world->music;
                        }
                        world->music = new Sound(world->getNextObjectID(), world->assetManager, selectedSoundAsset->fullPath);
                        world->music->setLoop(true);
                        world->music->setWorldPosition(glm::vec3(0, 0, 0), true);
                        world->music->play();
                    }
                } else {
                    ImGui::Button("Change Music");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No sound asset selected");
                }
            }
            if(ImGui::CollapsingHeader("SkyBox")) {
                ImGui::Indent(16.0f);
                this->addSkyBoxControls();
            }
            if(ImGui::CollapsingHeader("LoadingImage")) {
                ImGui::Indent(16.0f);
                static const AssetManager::AvailableAssetsNode* selectedLoadingImageAsset = nullptr;

                static char loadingImageAssetFilter[32] = {0};
                ImGui::InputText("Filter Assets ##TextureAssetTreeFilter", loadingImageAssetFilter, sizeof(loadingImageAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
                std::string loadingImageAssetFilterStr = loadingImageAssetFilter;
                std::transform(loadingImageAssetFilterStr.begin(), loadingImageAssetFilterStr.end(), loadingImageAssetFilterStr.begin(), ::tolower);
                const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, loadingImageAssetFilterStr);
                imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE, "LoadingImage", &selectedLoadingImageAsset);

                if(selectedLoadingImageAsset == nullptr) {
                    ImGui::Button("Set Loading Image");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No Asset Selected!");
                } else {
                    if (ImGui::Button("Set Loading Image")) {
                        world->loadingImage = selectedLoadingImageAsset->fullPath;
                    }
                }
                if(world->loadingImage != "" ) {
                    ImGui::Text("Current Loading Image: %s", world->loadingImage.c_str() );
                } else {
                    ImGui::Text("No loading image set");
                }
            }


            if(ImGui::CollapsingHeader("ESC handling")) {
                ImGui::Indent(16.0f);
                ImGui::Text("By default, esc quits the game");

                if (ImGui::RadioButton("Quit Game", world->currentQuitResponse == World::QuitResponse::QUIT_GAME)) {
                    world->currentQuitResponse = World::QuitResponse::QUIT_GAME;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Return Previous", world->currentQuitResponse == World::QuitResponse::RETURN_PREVIOUS)) {
                    world->currentQuitResponse = World::QuitResponse::RETURN_PREVIOUS;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Load World", world->currentQuitResponse == World::QuitResponse::LOAD_WORLD)) {
                    world->currentQuitResponse = World::QuitResponse::LOAD_WORLD;
                }
                if (world->currentQuitResponse == World::QuitResponse::LOAD_WORLD) {
                    ImGui::InputText("Custom World file ", this->quitWorldNameBuffer, sizeof(this->quitWorldNameBuffer));
                    if (ImGui::Button("Apply##custom world file setting")) {
                        world->quitWorldName = this->quitWorldNameBuffer;
                    }
                }
            }

            ImGui::Unindent( 16.0f );

        }

        ImGui::Separator();

        ImGui::InputText("##save world name", this->worldSaveNameBuffer, sizeof(this->worldSaveNameBuffer));
        ImGui::SameLine();
        if(ImGui::Button("Save World")) {
            for(auto animIt = world->loadedAnimations.begin(); animIt != world->loadedAnimations.end(); animIt++) {
                if(animIt->serializeAnimation("./Data/Animations/")) {
                    world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation saved");
                } else {
                    world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation save failed");
                }
            }
            //before saving, set the connection state
            for (auto objectIt = world->disconnectedModels.begin(); objectIt != world->disconnectedModels.end(); ++objectIt) {
                world->disconnectObjectFromPhysics(*objectIt);
            }

            if(WorldSaver::saveWorld(this->worldSaveNameBuffer, world)) {
                world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "World save successful");
            } else {
                world->options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "World save Failed");
            }
            //after save, set the states back
            for (auto objectIt = world->disconnectedModels.begin(); objectIt != world->disconnectedModels.end(); ++objectIt) {
                world->reconnectObjectToPhysics(*objectIt);
            }

        }
        if(ImGui::Button("Save AI walk Grid")) {
            if(world->grid != nullptr) {
                std::string AIWalkName = world->name.substr(0, world->name.find_last_of(".")) + ".aiwalk";
                world->grid->serializeXML(AIWalkName);
            }
        }
#ifdef CEREAL_SUPPORT
        if(ImGui::Button("Save AI walk Grid Binary")) {
            if(world->grid != nullptr) {
                std::string AIWalkName = world->name.substr(0, world->name.find_last_of(".")) + ".aiwalkb";
                std::ofstream os(AIWalkName, std::ios::binary);
                cereal::BinaryOutputArchive archive( os );

                archive(*(world->grid));
            }
        }
#endif
        if(ImGui::Button("Convert models to binary")) {
            std::set<std::vector<std::string>> convertedAssets;
            for (auto objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt) {

                Model* model = dynamic_cast<Model*>(objectIt->second);
                if(model!= nullptr) {
                    model->convertAssetToLimon(convertedAssets);
                }
            }
            if(world->startingPlayer.attachedModel != nullptr) {
                world->startingPlayer.attachedModel->convertAssetToLimon(convertedAssets);
            }
        }

        if(ImGui::Button("Change Render Pipeline")) {
            this->showNodeGraph = true;
        }
        if (ImGui::CollapsingHeader("Render Debugging")) {
            static int listbox_item_current = -1;//not static because I don't want user to select a item.
            static ImGuiImageWrapper wrapper;//keeps selected texture and layer;
            std::vector<std::shared_ptr<Texture>> allTextures = world->renderPipeline->getTextures();
            allTextures.emplace_back(colorTexture);
            allTextures.emplace_back(depthTexture);
            if(ImGui::ListBox("Current Textures##Render Debugging", &listbox_item_current, World::getNameOfTexture,
                              static_cast<void *>(&allTextures), allTextures.size(), 10)) {
                wrapper.layer = 0;
            }
            if(listbox_item_current != -1) {
                wrapper.texture = allTextures[listbox_item_current];
                float aspect = (float) wrapper.texture->getHeight() / (float)wrapper.texture->getWidth();
                ImVec2 size;
                size.x = 640;
                size.y = std::floor(size.x * aspect);
                ImGui::Text("%s", ("Texture id selected is: " + std::to_string(wrapper.texture->getTextureID())).c_str());

                if(wrapper.texture->getType() == GraphicsInterface::TextureTypes::T2D_ARRAY ||
                   wrapper.texture->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY) {
                    ImGui::InputInt("Layer##CurrentTextures", &wrapper.layer);

                }
                ImGui::Dummy(ImVec2(0.0f, size.y));
                size.y = -1 * size.y;//This is because ImGui assumes y up. Since this code is shared with fonts, and fixing font generation is hard, I am using this hack for upside down fix.
                ImGui::Image((ImTextureID)(intptr_t)&wrapper, size);
            }


        }
        if(ImGui::CollapsingHeader("List materials")) {
            //listing
            static size_t selectedHash = 0;
            if (EditorNS::selectedMeshesMaterial != nullptr) {
                selectedHash = EditorNS::selectedMeshesMaterial->getHash();
                EditorNS::selectedMeshesMaterial = nullptr;
            }
            bool isSelected = false;
            auto allMaterials = world->assetManager->getMaterials();
            ImGui::Text("Total material count is %lu", (unsigned long) allMaterials.size());
            if (ImGui::BeginListBox("Materials")) {
                for (auto it = allMaterials.begin(); it != allMaterials.end(); ++it) {
                    isSelected = selectedHash == it->first;
                    if (ImGui::Selectable((it->second.first->getName() + " -> " + std::to_string(it->first)).c_str(), isSelected)) {
                        selectedHash = it->first;
                    }
                }
                ImGui::EndListBox();
            }
            auto selectedMaterialIt = allMaterials.find(selectedHash);
            if(selectedMaterialIt != allMaterials.end()) {
                EditorNS::selectedFromListMaterial = selectedMaterialIt->second.first;
                selectedMaterialIt->second.first->addImGuiEditorElements(*this->request);
            }
        }
        ImGui::End();

        //ImGui::SetNextWindowSize(ImVec2(0,0), false);//true means set it only once

        ImGui::Begin("Selected Object Properties");
        std::string selectedName;
        if(this->pickedObject == nullptr) {
            selectedName = "No object selected";
        } else {
            selectedName = this->pickedObject->getName().c_str();
        }

        buildTreeFromAllGameObjects();

        if(this->pickedObject != nullptr) {
            //search for the selected element in the rendered elements
            ImGuiResult objectEditorResult = this->pickedObject->addImGuiEditorElements(*this->request);
            if (objectEditorResult.materialChanged) {
                for (auto& cameraVisibility: world->cullingResults) {
                    for (auto& tagVisibility:(*cameraVisibility.second)) {
                        tagVisibility.second.removeModelFromAll(this->pickedObjectID);
                    }
                }
            }
            switch(this->pickedObject->getTypeID()) {
                case GameObject::ObjectTypes::MODEL: {
                    if (objectEditorResult.updated) {
                        Model *selectedObject = static_cast<Model *>(this->pickedObject);
                        if (objectEditorResult.putOnTop) {
                            bool wasDisconnected = selectedObject->isDisconnected();
                            selectedObject->disconnectFromPhysicsWorld(world->dynamicsWorld);
                            GameObject *gameObjectUnderSelected = nullptr;
                            std::unique_ptr<ClosestNotMeConvexResultCallback> resultCallback = convexSweepTestDown(selectedObject);
                            if (resultCallback->hasHit() && resultCallback->m_hitCollisionObject->getUserPointer()) {
                                gameObjectUnderSelected = static_cast<GameObject *>(resultCallback->m_hitCollisionObject->getUserPointer());
                            }
                            if (gameObjectUnderSelected != nullptr) {
                                selectedObject->getTransformation()->addTranslate(glm::vec3(0.0f, resultCallback->m_hitPointWorld.y() - selectedObject->getAabbMin().y, 0.0f));
                                selectedObject->updateAABB();

                            }
                            if (!wasDisconnected) {
                                world->reconnectObjectToPhysics(selectedObject->getWorldObjectID());//multiple redirection but should be fine as this is single object in editor mode
                                world->dynamicsWorld->updateSingleAabb(selectedObject->getRigidBody());
                            }
                            world->updatedModels.push_back(selectedObject);
                        } else {
                            if(!selectedObject->isDisconnected()) {
                                world->dynamicsWorld->updateSingleAabb(selectedObject->getRigidBody());
                            }
                            world->updatedModels.push_back(selectedObject);
                        }
                    }
                    uint32_t removedActorID = 0;
                    if (objectEditorResult.removeAI) {
                        //remove AI requested
                        if (dynamic_cast<Model *>(this->pickedObject)->getAIID() != 0) {
                            removedActorID = dynamic_cast<Model *>(this->pickedObject)->getAIID();
                            world->actors.erase(dynamic_cast<Model *>(this->pickedObject)->getAIID());
                            dynamic_cast<Model *>(this->pickedObject)->detachAI();
                        }
                    }

                    if (objectEditorResult.addAI) {
                        std::cout << "adding AI to model " << std::endl;
                        if (removedActorID == 0) {
                            removedActorID = world->getNextObjectID();
                        }
                        //if remove and add is called in same frame, it means the type is changed, reuse the ID
                        ActorInterface *newEnemy = ActorInterface::createActor(objectEditorResult.actorTypeName, removedActorID, world->apiInstance);
                        Model *model = dynamic_cast<Model *>(this->pickedObject);
                        if (model != nullptr) {
                            newEnemy->setModel(model->getWorldObjectID());
                            model->attachAI(newEnemy);
                        } else {
                            std::cerr << "ActorInterface Model setting failed, because picked object is not a model." << std::endl;
                        }
                        world->addActor(newEnemy);
                    } else {
                        if (removedActorID != 0) {
                            world->unusedIDs.push(removedActorID);
                        }
                    }
                }
                    /* fall through */
/************** ATTENTION, NO BREAK ******************/
                case GameObject::ObjectTypes::GUI_TEXT:
                case GameObject::ObjectTypes::GUI_IMAGE:
                case GameObject::ObjectTypes::GUI_BUTTON:
                case GameObject::ObjectTypes::GUI_ANIMATION:
                {
                    Renderable* selectedObject = dynamic_cast<Renderable*>(this->pickedObject);
                    if(selectedObject != nullptr) {
                        //Now we are looking for animations
                        if (world->activeAnimations.find(selectedObject) != world->activeAnimations.end()) {
                            if (objectEditorResult.updated) {
                                world->activeAnimations[selectedObject]->originChange = true;
                            }

                            if (ImGui::Button(("Remove custom animation: " +
                                    world->loadedAnimations[world->activeAnimations[selectedObject]->animationIndex].getName()).c_str())) {
                                ImGui::OpenPopup("How To Remove Custom Animation");
                            }
                            if (ImGui::BeginPopupModal("How To Remove Custom Animation")) {
                                AnimationCustom &animationToRemove = world->loadedAnimations[world->activeAnimations[selectedObject]->animationIndex];
                                World::AnimationStatus *animationStatusToRemove = world->activeAnimations[selectedObject];
                                if (ImGui::Button("Set to Start##CustomAnimationRemoval")) {
                                    world->removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, 0);
                                    ImGui::CloseCurrentPopup();
                                }

                                if (ImGui::Button("Set to End##CustomAnimationRemoval")) {
                                    world->removeActiveCustomAnimation(animationToRemove, animationStatusToRemove,
                                                                animationToRemove.getDuration());
                                    ImGui::CloseCurrentPopup();
                                }
                                static float customTime = 0.0f;
                                ImGui::DragFloat("##CustomAnimationRemovalCustomTime", &customTime, 0.1, 0.0,
                                                 animationToRemove.getDuration());
                                ImGui::SameLine();
                                if (ImGui::Button("Set to custom time##CustomAnimationRemoval")) {
                                    world->removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, customTime);
                                    ImGui::CloseCurrentPopup();
                                }
                                if (ImGui::Button("Cancel##CustomAnimationRemoval")) {
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndPopup();

                            }
                        } else {
                            addAnimationDefinitionToEditor();
                        }
                    } else {
                        std::cerr << "Editor Animation section has non renderable object selected, this shouldn't have happened!" << std::endl;
                    }
                }
                    break;
                default:
                    // Animation creation works on only model and gui elements, so rest is passed
                    break;

            }

            // after this, remove and physics disconnect
            ImGui::NewLine();
            switch (this->pickedObject->getTypeID()) {
                case GameObject::ObjectTypes::MODEL: {
                    if (world->disconnectedModels.find(this->pickedObject->getWorldObjectID()) != world->disconnectedModels.end()) {
                        if (ImGui::Button("reconnect to physics")) {
                            world->reconnectObjectToPhysicsRequest(static_cast<Model *>(this->pickedObject)->getWorldObjectID());//Request because that action will not be carried out on editor mode
                        }
                    } else {
                        if (ImGui::Button("Disconnect from physics")) {
                            world->disconnectObjectFromPhysicsRequest(static_cast<Model *>(this->pickedObject)->getWorldObjectID());
                        }
                        ImGui::Text(
                                "If object is placed in trigger volume, \ndisconnecting drastically improve performance.");
                    }

                    if (ImGui::Button("Remove This Object")) {
                        world->removeObject(this->pickedObject->getWorldObjectID());
                        this->pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::ObjectTypes::TRIGGER: {
                    if (ImGui::Button("Remove This Trigger")) {
                        world->removeTriggerObject(this->pickedObject->getWorldObjectID());
                        this->pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::ObjectTypes::GUI_TEXT:
                case GameObject::ObjectTypes::GUI_IMAGE:
                case GameObject::ObjectTypes::GUI_BUTTON:
                case GameObject::ObjectTypes::GUI_ANIMATION: {
                    if(objectEditorResult.remove) {
                        world->guiElements.erase(this->pickedObject->getWorldObjectID());
                        world->unusedIDs.push(this->pickedObject->getWorldObjectID());
                        delete this->pickedObject;
                        this->pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::ObjectTypes::LIGHT: {
                    if(objectEditorResult.remove) {
                        for (auto iterator = world->lights.begin(); iterator != world->lights.end(); ++iterator) {
                            if((*iterator)->getWorldObjectID() == this->pickedObject->getWorldObjectID()) {
                                world->unusedIDs.push(this->pickedObject->getWorldObjectID());
                                const std::vector<Camera*>& cameras = (*iterator)->getCameras();
                                for (auto camera:cameras) {
                                    for (auto entry:world->visibilityThreadPool) {
                                        if (entry.first->camera == camera) {
                                            entry.first->running = false;
                                            VisibilityRequest::waitMainThreadCondition.signalWaiting();
                                            SDL_WaitThread(entry.second, nullptr);
                                            auto visRequest = entry.first;
                                            world->visibilityThreadPool.erase(visRequest);
                                            delete visRequest;
                                            break;
                                        }
                                    }
                                    world->cullingResults.erase(camera);
                                }
                                world->lights.erase(iterator);
                                break;
                            }
                        }
                        if(static_cast<Light*>(this->pickedObject)->getLightType() == Light::LightTypes::DIRECTIONAL) {
                            world->directionalLightIndex = -1;
                        }
                        delete this->pickedObject;
                        world->updateActiveLights(true);
                        this->pickedObject = nullptr;
                    }
                }
                    break;
                default: {
                    //there is nothing for now
                }

            }
        }

        ImGui::End();
    }

    /* window definitions */
    imgGuiHelper->RenderDrawLists(graphicsProgram);
}

void Editor::setTransformToModel(Model *model, const glm::vec3 &newObjectPosition) {
    //First reset the model transform
    model->getTransformation()->setTransformations(glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    //now calculate
    float expectedSize = 9.5f;
    const glm::mat4 reversalTransformation = glm::inverse(glm::lookAt(world->playerCamera->getPosition(),
                                                                      newObjectPosition, glm::vec3(0, 1, 0)));
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat rotationDe;
    glm::decompose(reversalTransformation, scale, rotationDe, translation, skew, perspective);
    model->getTransformation()->setOrientation(rotationDe * glm::quat(0.970f, 0.175f, -0.175f, 0.0f));
    glm::vec3 min = model->getAabbMin(), max = model->getAabbMax();
    glm::vec3 size = max - min;
    float maxDim = std::max(size.x, size.y);
    float scaleF = expectedSize /maxDim ;
    model->getTransformation()->setScale(glm::vec3(scaleF, scaleF, scaleF));
    glm::vec3 centerOffset = (min + max) * 0.5f * scaleF;
    translation = newObjectPosition - centerOffset;
    model->getTransformation()->setTranslate(translation);

}


void Editor::addAnimationDefinitionToEditor() {
    if (ImGui::CollapsingHeader("Custom animation properties")) {
        //If there is no animation setup ongoing, or there is one, but not for this model,
        //put start animation button.
        //else put time input, add and finalize buttons.
        if (world->animationInProgress == nullptr || world->animationInProgress->getAnimatingObject() != dynamic_cast<Renderable *>(this->pickedObject)) {

            static int listbox_item_current = 0;
            ImGui::Text("Loaded animations list");
            ImGui::ListBox("##Loaded animations listbox", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&world->loadedAnimations), world->loadedAnimations.size(), 10);

            if (ImGui::Button("Apply selected")) {
                world->addAnimationToObject(this->pickedObject->getWorldObjectID(), listbox_item_current,
                                     true, true);
            }

            ImGui::SameLine();
            if (ImGui::Button("Create new")) {
                if (world->animationInProgress == nullptr) {
                    world->animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(this->pickedObject));
                } else {
                    //ask for removal of the old work
                    delete world->animationInProgress;
                    world->animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(this->pickedObject));
                }
                // At this point we should know the animationInProgress is for current object
            }
        } else {
            ImGui::Text("Please use animation definition window.");
            bool finished, cancelled;
            world->animationInProgress->addAnimationSequencerToEditor(finished, cancelled);
            if (finished) {
                world->loadedAnimations.push_back(AnimationCustom(*world->animationInProgress->buildAnimationFromCurrentItems()));

                world->addAnimationToObject(this->pickedObject->getWorldObjectID(),
                                     world->loadedAnimations.size() - 1, true, true);
                delete world->animationInProgress;
                world->animationInProgress = nullptr;
            }
            if (cancelled) {
                delete world->animationInProgress;
                world->animationInProgress = nullptr;
            }
        }
    }
}

void Editor::buildTreeFromAllGameObjects() {

    std::vector<uint32_t> parentageList;
    if(this->pickedObject != nullptr) {
        if(this->pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL || this->pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL_GROUP) {
            PhysicalRenderable *physicalRenderable = dynamic_cast<PhysicalRenderable *>(this->pickedObject);
            if(physicalRenderable != nullptr) {
                if (ImGui::Button("Find selected") || this->pickedObjectID != this->pickedObject->getWorldObjectID()) {//trigger find if selected object changes
                    while (physicalRenderable != nullptr) {
                        GameObject* gameObject = dynamic_cast<GameObject*>(physicalRenderable);
                        if(gameObject != nullptr) {
                            parentageList.push_back(gameObject->getWorldObjectID());
                        } else {
                            std::cerr << "Find non game object parentage while searching. This shouldn't have happened." << std::endl;
                            parentageList.clear();
                            break;
                        }
                        physicalRenderable = physicalRenderable->getParentObject();
                    }

                    std::reverse(std::begin(parentageList), std::end(parentageList));
                }
            }
        }
        this->pickedObjectID = this->pickedObject->getWorldObjectID();
    }

    ImGui::BeginChild("Game Object Selector##treeMode", ImVec2(400, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    ImGuiTreeNodeFlags leafFlags = nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;//no recursion after this point

    if(!parentageList.empty()) {
        ImGui::SetNextItemOpen(true);
    }
    //objects
    if (ImGui::TreeNode("Objects##ObjectsTreeRoot")) {
        //ModelGroups
        for (auto iterator = world->modelGroups.begin(); iterator != world->modelGroups.end(); ++iterator) {
            if(iterator->second->getParentObject() != nullptr) {
                continue; //the parent will show this group
            }
            createObjectTreeRecursive(iterator->second, this->pickedObjectID, nodeFlags, leafFlags, parentageList);
        }
        //ModelGroups end

        //Objects recursive
        for (auto iterator = world->objects.begin(); iterator != world->objects.end(); ++iterator) {
            if(iterator->second->getParentObject() != nullptr) {
                continue; //the parent will show this group
            }
            if(iterator->second->hasChildren()) {
                createObjectTreeRecursive(iterator->second, this->pickedObjectID, nodeFlags, leafFlags, parentageList);
            } else {
                GameObject* currentObject = dynamic_cast<GameObject*>(iterator->second);
                if(currentObject != nullptr) {
                    bool isSelected = currentObject->getWorldObjectID() == this->pickedObjectID;
                    ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | (isSelected ? ImGuiTreeNodeFlags_Selected : 0));
                    if(isSelected && !parentageList.empty()) {
                        ImGui::SetScrollHereY();
                    }

                    if (ImGui::IsItemClicked()) {
                        if(this->pickedObject != nullptr ) {
                            this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                        }
                        this->pickedObject = currentObject;
                        this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                    }
                }
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextItemOpen(false);
    }
    //GUI elements
    if (ImGui::TreeNode("GUI Elements##guiElementsTreeRoot")) {
        for (auto iterator = world->guiLayers.begin(); iterator != world->guiLayers.end(); ++iterator) {
            if (ImGui::TreeNode((std::to_string((*iterator)->getLevel()) + "##guiLayerLevelTreeNode").c_str())) {
                std::vector<GameObject*> thisLayersElements = (*iterator)->getGuiElements();
                for (auto guiElement = thisLayersElements.begin(); guiElement != thisLayersElements.end(); ++guiElement) {
                    ImGui::TreeNodeEx((*guiElement)->getName().c_str(), leafFlags | (((*guiElement)->getWorldObjectID() == this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                    if (ImGui::IsItemClicked()) {
                        if(this->pickedObject != nullptr ) {
                            this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                        }
                        this->pickedObject = *guiElement;
                        this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                    }

                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextItemOpen(false);
    }
    //Lights
    if (ImGui::TreeNode("Lights##LightsTreeRoot")) {
        for (auto iterator = world->lights.begin(); iterator != world->lights.end(); ++iterator) {
            GameObject* currentObject = dynamic_cast<GameObject*>(*iterator);
            if(currentObject != nullptr) {
                ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | ((currentObject->getWorldObjectID() == this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked()) {
                    if(this->pickedObject != nullptr ) {
                        this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                    }
                    this->pickedObject = currentObject;
                    this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                }
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextItemOpen(false);
    }
    //Triggers
    if (ImGui::TreeNode("Trigger Volumes##TriggersTreeRoot")) {
        for (auto iterator = world->triggers.begin(); iterator != world->triggers.end(); ++iterator) {
            GameObject* currentObject = dynamic_cast<GameObject*>(iterator->second);
            if(currentObject != nullptr) {
                ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | ((currentObject->getWorldObjectID() == this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked()) {
                    this->pickedObject = currentObject;
                }
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextItemOpen(false);
    }
    //Particles
    if (ImGui::TreeNode("Particle Emitters##ParticleEmittersTreeRoot")) {
        for (auto iterator = world->emitters.begin(); iterator != world->emitters.end(); ++iterator) {
            std::shared_ptr<GameObject> currentObject = std::dynamic_pointer_cast<GameObject>(iterator->second);
            if(currentObject != nullptr) {
                ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | ((currentObject->getWorldObjectID() == this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked()) {
                    this->pickedObject = currentObject.get();//FIXME this is an unsafe use
                    this->pickedObjectID = this->pickedObject->getWorldObjectID();
                }
            }
        }
        ImGui::TreePop();
    }

    //player
    if(world->physicalPlayer == nullptr) {
        world->physicalPlayer = new PhysicalPlayer(1, world->options, world->cursor, world->startingPlayer.position, world->startingPlayer.orientation, world->startingPlayer.attachedModel);// 1 is reserved for physical player
    }
    bool isOpen = false;
    if(world->startingPlayer.attachedModel == nullptr) {
        ImGui::TreeNodeEx(world->physicalPlayer->getName().c_str(), leafFlags |
                                                                   ((world->physicalPlayer->getWorldObjectID() ==
                                                                     this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected
                                                                                     : 0));
    } else {
        isOpen = ImGui::TreeNodeEx(world->physicalPlayer->getName().c_str(), nodeFlags |
                                                                   ((world->physicalPlayer->getWorldObjectID() ==
                                                                     this->pickedObjectID) ? ImGuiTreeNodeFlags_Selected
                                                                                     : 0));
    }
    if (ImGui::IsItemClicked()) {
        this->pickedObject = world->physicalPlayer;
    }
    if(isOpen) {
        createObjectTreeRecursive(world->startingPlayer.attachedModel, this->pickedObjectID, nodeFlags, leafFlags, parentageList);
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void Editor::createObjectTreeRecursive(PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID,
                                      ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                      std::vector<uint32_t> parentage) {
    GameObject* gameObjectOfSame = dynamic_cast<GameObject*>(physicalRenderable);
    if(physicalRenderable == nullptr || gameObjectOfSame == nullptr) {
        return;
    }
    if(!parentage.empty()) {
        if(gameObjectOfSame->getWorldObjectID() == parentage[0]) {
            ImGui::SetNextItemOpen(true);
        } else {
            ImGui::SetNextItemOpen(false);
        }

    }
    bool isSelected = gameObjectOfSame->getWorldObjectID() == pickedObjectID;
    bool isNodeOpen = ImGui::TreeNodeEx((gameObjectOfSame->getName() + "##ModelGroupsTreeElement" + std::to_string(gameObjectOfSame->getWorldObjectID())).c_str(),
           nodeFlags | ( isSelected ? ImGuiTreeNodeFlags_Selected: 0));
    if(isSelected && !parentage.empty()) {
        ImGui::SetScrollHereY();
    }
    if (ImGui::IsItemClicked()) {
        if(this->pickedObject != nullptr ) {
            this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
        }
        this->pickedObject = gameObjectOfSame;
        this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
    }
    if(isNodeOpen){
       for (auto iterator = physicalRenderable->getChildren().begin(); iterator != physicalRenderable->getChildren().end(); ++iterator) {
           GameObject* currentObject = dynamic_cast<GameObject*>(*iterator);
           if(currentObject != nullptr) {
               if((*iterator)->hasChildren()) {
                   //if we came here, it means the first element of the parentage was this, remove that element and pass
                   if(!parentage.empty()) {
                       parentage.erase(parentage.begin());
                   }
                   createObjectTreeRecursive(static_cast<ModelGroup *>(currentObject), pickedObjectID, nodeFlags,
                                             leafFlags, parentage);
               } else {
                   isSelected = currentObject->getWorldObjectID() == pickedObjectID;
                   ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags |
                                                                       (isSelected ? ImGuiTreeNodeFlags_Selected
                                                                                         : 0));
                   if(isSelected && !parentage.empty()) {
                       ImGui::SetScrollHereY();
                   }
                   if (ImGui::IsItemClicked()) {
                       if(this->pickedObject != nullptr ) {
                           this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                       }
                       this->pickedObject = currentObject;
                       this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
                   }
               }
           }
       }
       ImGui::TreePop();
   }
}

void Editor::renderSelectedObject(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram) const {
    backgroundRenderStage->activate(true);
    graphicsProgram->setUniform("renderModelIMGUI", 1);
    model->convertToRenderList(0, 0).render(world->graphicsWrapper, graphicsProgram, true);
    graphicsProgram->setUniform("renderModelIMGUI", 0);
    world->renderPipeline->reActivateLastStage();
}

void Editor::addGUITextControls() {
    /**
     * we need these set:
     * 1) font
     * 2) font size
     * 3) name
     *
     */

    static int fontSize = 32;

    std::set<std::pair<std::string, uint32_t>> loadedFonts = world->fontManager.getLoadedFonts();
    auto it = loadedFonts.begin();
    static std::string selectedFontName = it->first;
    if (ImGui::BeginCombo("Font to use", it->first.c_str())) {
        for (; it != loadedFonts.end(); it++) {//first element already set

            bool isThisFontSelected = it->first == selectedFontName;
            if (ImGui::Selectable(it->first.c_str(), isThisFontSelected)) {
                selectedFontName = it->first;
            }
            if (isThisFontSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }


    ImGui::DragInt("Font size", &fontSize, 1, 1, 128);
    static char GUITextName[32];
    ImGui::InputText("GUIText Name", GUITextName, sizeof(GUITextName), ImGuiInputTextFlags_CharsNoBlank);

    static size_t selectedLayerIndex = 0;
    if (world->guiLayers.size() == 0) {
        world->guiLayers.push_back(new GUILayer(world->graphicsWrapper, world->debugDrawer, 10));
    }
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < world->guiLayers.size(); ++i) {
            bool isThisLayerSelected = selectedLayerIndex == i;
            if (ImGui::Selectable(std::to_string(i).c_str(), isThisLayerSelected)) {
                selectedLayerIndex = i;
            }
            if (isThisLayerSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Add GUI Text")) {
        GUIText *guiText = new GUIText(world->graphicsWrapper, world->getNextObjectID(), GUITextName,
                                       world->fontManager.getFont(selectedFontName, fontSize), "New Text", glm::vec3(0, 0, 0));
        guiText->set2dWorldTransform(
                glm::vec2(world->options->getScreenWidth() / 2.0f, world->options->getScreenHeight() / 2.0f), 0.0f);
        world->guiElements[guiText->getWorldObjectID()] = guiText;
        world->guiLayers[selectedLayerIndex]->addGuiElement(guiText);
        if(this->pickedObject != nullptr ) {
            this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
        }
        this->pickedObject = guiText;
        this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
    }
}

void Editor::addGUIImageControls() {
    /**
     * For a new GUI Image we need only name and filename
     */
    static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;

    static char textureAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureAssetTreeFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
    imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE,
                                      "GUIImage",
                                      &selectedAsset);

    static size_t selectedLayerIndex = 0;
    if (world->guiLayers.size() == 0) {
        world->guiLayers.push_back(new GUILayer(world->graphicsWrapper, world->debugDrawer, 10));
    }
    static char GUIImageName[32];
    ImGui::InputText("GUI Image Name", GUIImageName, sizeof(GUIImageName), ImGuiInputTextFlags_CharsNoBlank);
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < world->guiLayers.size(); ++i) {
            bool isThisLayerSelected = selectedLayerIndex == i;
            if (ImGui::Selectable(std::to_string(i).c_str(), isThisLayerSelected)) {
                selectedLayerIndex = i;
            }
            if (isThisLayerSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if(selectedAsset == nullptr) {
        ImGui::Button("Add GUI Image");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No Asset Selected!");
    } else {
        if (ImGui::Button("Add GUI Image")) {
            GUIImage *guiImage = new GUIImage(world->getNextObjectID(), world->options, world->assetManager, std::string(GUIImageName),
                                              selectedAsset->fullPath);
            guiImage->set2dWorldTransform(
                    glm::vec2(world->options->getScreenWidth() / 2.0f, world->options->getScreenHeight() / 2.0f), 0.0f);
            world->guiElements[guiImage->getWorldObjectID()] = guiImage;
            world->guiLayers[selectedLayerIndex]->addGuiElement(guiImage);
            if(this->pickedObject != nullptr ) {
                this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
            }
            this->pickedObject = guiImage;
            this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);        }
    }
}

void Editor::addGUIButtonControls() {

    static char GUIButtonName[32];
    ImGui::InputText("GUI Button Name", GUIButtonName, sizeof(GUIButtonName), ImGuiInputTextFlags_CharsNoBlank);

    static const AssetManager::AvailableAssetsNode* selectedAssetForGUIButton = nullptr;

    static char textureAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureButtonAssetTreeFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
    imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE,
                                      "GUIButton",
                                      &selectedAssetForGUIButton);

    static char GUIButtonNormalFileName[256] = {0};
    ImGui::InputText("Normal image", GUIButtonNormalFileName, sizeof(GUIButtonNormalFileName));
    ImGui::SameLine();
    if(selectedAssetForGUIButton != nullptr) {
        if(ImGui::Button("Set##GuiButN")) {
            strncpy(GUIButtonNormalFileName, selectedAssetForGUIButton->fullPath.c_str(), sizeof(GUIButtonNormalFileName)-1);
        }
    } else {
        ImGui::Button("Set##GuiButN");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No asset selected");
    }

    static char GUIButtonOnHoverFileName[256] = {0};
    ImGui::InputText("On hover image", GUIButtonOnHoverFileName, sizeof(GUIButtonOnHoverFileName));
    ImGui::SameLine();
    if(selectedAssetForGUIButton != nullptr) {
        if(ImGui::Button("Set##GuiButH")) {
            strncpy(GUIButtonOnHoverFileName, selectedAssetForGUIButton->fullPath.c_str(), sizeof(GUIButtonOnHoverFileName)-1);
        }
    } else {
        ImGui::Button("Set##GuiButN");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No asset selected");
    }

    static char GUIButtonOnClickFileName[256] = {0};
    ImGui::InputText("On click image", GUIButtonOnClickFileName, sizeof(GUIButtonOnClickFileName));
    ImGui::SameLine();
    if(selectedAssetForGUIButton != nullptr) {
        if(ImGui::Button("Set##GuiButC")) {
            strncpy(GUIButtonOnClickFileName, selectedAssetForGUIButton->fullPath.c_str(), sizeof(GUIButtonOnClickFileName)-1);
        }
    } else {
        ImGui::Button("Set##GuiButN");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No asset selected");
    }

    static char GUIButtonDisabledFileName[256] = {0};
    ImGui::InputText("Disabled image", GUIButtonDisabledFileName, sizeof(GUIButtonDisabledFileName));
    ImGui::SameLine();
    if(selectedAssetForGUIButton != nullptr) {
        if(ImGui::Button("Set##GuiButD")) {
            strncpy(GUIButtonDisabledFileName, selectedAssetForGUIButton->fullPath.c_str(), sizeof(GUIButtonDisabledFileName)-1);
        }
    } else {
        ImGui::Button("Set##GuiButN");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No asset selected");
    }

    static size_t selectedLayerIndex = 0;
    if (world->guiLayers.size() == 0) {
        world->guiLayers.push_back(new GUILayer(world->graphicsWrapper, world->debugDrawer, 10));
    }
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < world->guiLayers.size(); ++i) {
            bool isThisLayerSelected = selectedLayerIndex == i;
            if (ImGui::Selectable(std::to_string(i).c_str(), isThisLayerSelected)) {
                selectedLayerIndex = i;
            }
            if (isThisLayerSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if(strlen(GUIButtonNormalFileName) == 0) {
        ImGui::Button("Add GUI Button");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("Normal image must be set");
    } else {
        if (ImGui::Button("Add GUI Button")) {
            std::vector<std::string> fileNames;

            fileNames.push_back(std::string(GUIButtonNormalFileName));
            if (strlen(GUIButtonOnHoverFileName) > 0) {
                fileNames.push_back(std::string(GUIButtonOnHoverFileName));
                if (strlen(GUIButtonOnClickFileName) > 0) {
                    fileNames.push_back(std::string(GUIButtonOnClickFileName));
                    if (strlen(GUIButtonDisabledFileName) > 0) {
                        fileNames.push_back(std::string(GUIButtonDisabledFileName));
                    }
                }
            }
            std::string name;
            if(strlen(GUIButtonName) != 0) {
                name = GUIButtonName;
            } else {
                name = "GuiButton";
            }

            GUIButton *guiButton = new GUIButton(world->getNextObjectID(), world->assetManager, world->apiInstance,
                                                 name,
                                                 fileNames);
            guiButton->set2dWorldTransform(
                    glm::vec2(world->options->getScreenWidth() / 2.0f, world->options->getScreenHeight() / 2.0f), 0.0f);
            world->guiElements[guiButton->getWorldObjectID()] = guiButton;
            world->guiLayers[selectedLayerIndex]->addGuiElement(guiButton);

            if(this->pickedObject != nullptr ) {
                this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
            }
            this->pickedObject = guiButton;
            this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
        }
    }
}

void Editor::addGUIAnimationControls() {

    /**
     * For a new GUI Image we need only name and filename
     */

    static char GUIAnimationName[32];
    ImGui::InputText("GUI Animation Name", GUIAnimationName, sizeof(GUIAnimationName), ImGuiInputTextFlags_CharsNoBlank);

    static int32_t newAnimationFrameSpeed = TICK_PER_SECOND;
    ImGui::DragInt("FrameSpeed", &newAnimationFrameSpeed, 1.0f, TICK_PER_SECOND);

    static bool isLooped = false;
    ImGui::Checkbox("Is Animation Looped", &isLooped);

    static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;

    static char textureAnimationAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureAnimationAssetTreeFilter", textureAnimationAssetFilter, sizeof(textureAnimationAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAnimationAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);

    imgGuiHelper->buildTreeFromAssets(filteredAssets,
                                      AssetManager::AssetTypes::Asset_type_TEXTURE, "GUIAnimation",
                                      &selectedAsset);

    static size_t selectedLayerIndex = 0;
    if (world->guiLayers.size() == 0) {
        world->guiLayers.push_back(new GUILayer(world->graphicsWrapper, world->debugDrawer, 10));
    }
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < world->guiLayers.size(); ++i) {
            bool isThisLayerSelected = selectedLayerIndex == i;
            if (ImGui::Selectable(std::to_string(i).c_str(), isThisLayerSelected)) {
                selectedLayerIndex = i;
            }
            if (isThisLayerSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
     }
     if(selectedAsset == nullptr) {
         ImGui::Button("Add GUI Animation");
         ImGui::SameLine();
         ImGuiHelper::ShowHelpMarker("No Asset Selected");
     } else {

         if (ImGui::Button("Add GUI Animation")) {

             std::vector<std::string> fileNames;
             fileNames.push_back(selectedAsset->fullPath);

             GUIAnimation *guiAnimation = new GUIAnimation(world->getNextObjectID(), world->assetManager,
                                                          std::string(GUIAnimationName),
                                                          fileNames, world->gameTime, newAnimationFrameSpeed, isLooped);
             guiAnimation->set2dWorldTransform(glm::vec2(world->options->getScreenWidth() / 2.0f, world->options->getScreenHeight() / 2.0f), 0.0f);
             world->guiElements[guiAnimation->getWorldObjectID()] = guiAnimation;
             world->guiLayers[selectedLayerIndex]->addGuiElement(guiAnimation);
             if(this->pickedObject != nullptr ) {
                 this->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
             }
             this->pickedObject = guiAnimation;
             this->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
         }
     }
}

void Editor::addGUILayerControls() {
    static  int32_t levelSlider = 0;
    ImGui::DragInt("Layer level", &levelSlider, 1, 1, 128);
    if (ImGui::Button("Add GUI Layer")) {
        world->guiLayers.push_back(new GUILayer(world->graphicsWrapper, world->debugDrawer, (uint32_t)levelSlider));
    }
}

void Editor::addParticleEmitterEditor() {
    /**
     * For a new GUI Image we need only name and filename
     */
    static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;

    static char textureAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureAssetTreeEmitterFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
    imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE,
                                      "ParticleEmitter",
                                      &selectedAsset);
    static char particleEmitterName[32] = {0};
    ImGui::InputText("Particle Emitter Name", particleEmitterName, sizeof(particleEmitterName), ImGuiInputTextFlags_CharsNoBlank);
    static glm::vec3 startPosition;
    ImGui::InputFloat3("Particle Emitter Position", glm::value_ptr(startPosition));
    static float startSphereR;
    ImGui::InputFloat("Particle Emitter radius", &startSphereR);
    static int maxCount;
    ImGui::InputInt("Maximum particle count", &maxCount);
    static int lifeTime;
    ImGui::InputInt("Particle life time", &lifeTime);
    static glm::vec2 size;
    ImGui::InputFloat2("Particle size", glm::value_ptr(size));

    if(selectedAsset == nullptr) {
        ImGui::Button("Add Particle Emitter");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No Asset Selected!");
    } else if(strlen(particleEmitterName) == 0) {
        ImGui::Button("Add Particle Emitter");
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("No Name Set!");
    } else {
        if (ImGui::Button("Add Particle Emitter")) {
            std::shared_ptr<Emitter> newEmitter = std::make_shared<Emitter>(world->getNextObjectID(), particleEmitterName, world->assetManager, selectedAsset->fullPath,
                                                                            startPosition, glm::vec3(startSphereR, startSphereR, startSphereR), size, maxCount,
                                                                            lifeTime);
            world->emitters[newEmitter->getWorldObjectID()] = (newEmitter);
        }
    }
}

void Editor::addSkyBoxControls() {
    //first, build a tree for showing directories with textures in them.
    static const AssetManager::AvailableAssetsNode* selectedSkyBoxAsset = nullptr;
    static const AssetManager::AvailableAssetsNode* selectedAssetDirectory = nullptr;

    if(selectedAssetDirectory == nullptr) {
        static char skyBoxAssetFilter[32] = {0};
        ImGui::InputText("Filter Assets ##SkyBoxAssetTreeFilter", skyBoxAssetFilter, sizeof(skyBoxAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
        std::string skyBoxAssetFilterStr = skyBoxAssetFilter;
        std::transform(skyBoxAssetFilterStr.begin(), skyBoxAssetFilterStr.end(), skyBoxAssetFilterStr.begin(),::tolower);
        const AssetManager::AvailableAssetsNode *filteredAssets = world->assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, skyBoxAssetFilterStr);

        imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_DIRECTORY, "SkyBox", &selectedSkyBoxAsset);

        if(ImGui::Button("Set Directory##SkyBox")) {
            selectedAssetDirectory = selectedSkyBoxAsset;
        }
    } else {
        imgGuiHelper->buildTreeFromAssets(selectedAssetDirectory, AssetManager::Asset_type_TEXTURE, "SkyBox", &selectedSkyBoxAsset);
        if(ImGui::Button("Reset Directory##SkyBox")) {
            selectedAssetDirectory = nullptr;
            selectedSkyBoxAsset = nullptr;
        }

        static char skyBoxRightFileName[256] = {0};
        ImGui::InputText("Right image", skyBoxRightFileName, sizeof(skyBoxRightFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxRight")) {
                strncpy(skyBoxRightFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxRightFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxRight");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        static char skyBoxLeftFileName[256] = {0};
        ImGui::InputText("Left image", skyBoxLeftFileName, sizeof(skyBoxLeftFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxLeft")) {
                strncpy(skyBoxLeftFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxLeftFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxLeft");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        static char skyBoxTopFileName[256] = {0};
        ImGui::InputText("Top image", skyBoxTopFileName, sizeof(skyBoxTopFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxTop")) {
                strncpy(skyBoxTopFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxTopFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxTop");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        static char skyBoxBottomFileName[256] = {0};
        ImGui::InputText("Bottom image", skyBoxBottomFileName, sizeof(skyBoxBottomFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxBottom")) {
                strncpy(skyBoxBottomFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxBottomFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxBottom");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        static char skyBoxFrontFileName[256] = {0};
        ImGui::InputText("Front image", skyBoxFrontFileName, sizeof(skyBoxFrontFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxFront")) {
                strncpy(skyBoxFrontFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxFrontFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxFront");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        static char skyBoxBackFileName[256] = {0};
        ImGui::InputText("Back image", skyBoxBackFileName, sizeof(skyBoxBackFileName));
        ImGui::SameLine();
        if(selectedSkyBoxAsset != nullptr) {
            if(ImGui::Button("Set##skyBoxBack")) {
                strncpy(skyBoxBackFileName, selectedSkyBoxAsset->name.c_str(), sizeof(skyBoxBackFileName)-1);
            }
        } else {
            ImGui::Button("Set##skyBoxBack");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("No asset selected");
        }

        if(strlen(skyBoxRightFileName) != 0 &&
                strlen(skyBoxLeftFileName) != 0 &&
                strlen(skyBoxTopFileName) != 0 &&
                strlen(skyBoxBottomFileName) != 0 &&
                strlen(skyBoxFrontFileName) != 0 &&
                strlen(skyBoxBackFileName) != 0   ) {
            if(ImGui::Button("Change Sky Box")) {
                SkyBox* newSkyBox = new SkyBox(world->getNextObjectID(), world->assetManager, selectedAssetDirectory->fullPath, skyBoxRightFileName, skyBoxLeftFileName, skyBoxTopFileName, skyBoxBottomFileName, skyBoxBackFileName, skyBoxFrontFileName);
                delete world->sky;
                world->sky = newSkyBox;
            }
        } else {
            ImGui::Button("Change Sky Box");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("Some Elements are empty");
        }


    }


}

void Editor::drawNodeEditor() {
    if(this->nodeGraph == nullptr) {
        this->createNodeGraph();
    }

    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Example: Custom Node Graph", &this->showNodeGraph)) {
        ImGui::End();
        return;
    }
    ImGui::ShowDemoWindow();

    this->nodeGraph->display();
    static long handleId = 0;
    if (handleId != 0) {
        ImGui::OpenPopup("Keep pipeline active");
    }
    if (ImGui::BeginPopupModal("Keep pipeline active", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(handleId == 0) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::Text("Do you want to keep the pipeline active?\n\nIt will reset automatically in 10 seconds of game time, after you close the editor.");
        ImGui::Separator();

        if (ImGui::Button("Keep", ImVec2(120, 0))) {
            world->cancelTimedEventAPI(handleId);
            handleId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Revert", ImVec2(120, 0))) {
            world->cancelTimedEventAPI(handleId);
            world->renderPipeline = world->renderPipelineBackup;
            world->renderPipelineBackup = nullptr;
            handleId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if(this->pipelineExtension->isPipelineBuilt()) {
        if (ImGui::Button("Activate")) {
            std::shared_ptr<GraphicsPipeline> builtRenderPipeline = this->pipelineExtension->handOverBuiltPipeline();
            std::vector<LimonTypes::GenericParameter> emptyParameters;
            for(auto& stage:builtRenderPipeline->getStages()) {
                for(auto& method:stage.renderMethods) {
                    if(!method.getInitialized()) {
                        method.initialize(emptyParameters);
                    }
                }
            }
            std::vector<LimonTypes::GenericParameter> empty;
            handleId = world->addTimedEventAPI(10000, true,
                                        [&](const std::vector<LimonTypes::GenericParameter> &) {
                                            world->renderPipeline = world->renderPipelineBackup;
                                            world->renderPipelineBackup = nullptr;
                                            handleId = 0;
                                        },
                                        empty);
            world->renderPipelineBackup = world->renderPipeline;
            world->renderPipeline = builtRenderPipeline;
            world->setupRenderForPipeline();
        }
    }

    if(ImGui::Button("Save")) {
        this->nodeGraph->serialize("./Data/nodeGraph.xml");
        this->nodeGraph->addMessage("Serialization done.");
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel")){
        this->showNodeGraph = false;
    }
    ImGui::End();
}

void Editor::createNodeGraph() {
    std::vector<NodeType*> nodeTypeVector;

    //start with predefined types

    NodeType* screen = new NodeType{"Screen", false, "", nullptr,{}, {}, true, {}};
    screen->inputConnections.push_back(ConnectionDesc{"Color", "Texture"});
    screen->inputConnections.push_back(ConnectionDesc{"Depth", "Texture"});
    nodeTypeVector.push_back(screen);

    NodeType* blend = new NodeType{"Blend", true, "", nullptr,{}, {}, false, {}};
    blend->inputConnections.push_back(ConnectionDesc{"Input1", "Texture"});
    blend->inputConnections.push_back(ConnectionDesc{"Input2", "Texture"});
    blend->inputConnections.push_back(ConnectionDesc{"Input3", "Texture"});
    blend->outputConnections.push_back(ConnectionDesc{"output", "Texture"});
    nodeTypeVector.push_back(blend);

    this->iterationExtension = new IterationExtension();

    NodeType* iterate = new NodeType{"Iterate", false, "IterationExtension", [](const NodeType* nodeType[[gnu::unused]]) -> NodeExtension* {return new IterationExtension();},
                      {{"Input", "Texture"},},
                       {{"Output", "Texture"},},false, {}};
    nodeTypeVector.push_back(iterate);

    std::vector<std::shared_ptr<GraphicsProgram>> programs = world->getAllAvailablePrograms();

    RenderMethods renderMethods = world->buildRenderMethods();

    this->pipelineExtension = new PipelineExtension(world->graphicsWrapper, world->renderPipeline, world->assetManager, world->options, GraphicsPipeline::getRenderMethodNames(), renderMethods);

    for(auto program:programs) {
        std::string programName = program->getProgramName();
        size_t startof, endof;
        endof=programName.find_last_of("/\\");
        startof = programName.substr(0,endof).find_last_of("/\\") +1;
        std::string nodeName = programName.substr(startof, endof - startof);
        NodeType* type = new NodeType{nodeName.c_str(), false, "PipelineStageExtension", nullptr, {}, {}, true, {}};

        auto uniformMap = program->getUniformMap();
        for(auto uniform:uniformMap) {

            if (uniform.first.rfind("pre_", 0) != 0) {
                continue;
            }

            if (!(uniform.second->type == Uniform::VariableTypes::CUBEMAP ||
                  uniform.second->type == Uniform::VariableTypes::CUBEMAP_ARRAY ||
                  uniform.second->type == Uniform::VariableTypes::TEXTURE_2D ||
                  uniform.second->type == Uniform::VariableTypes::TEXTURE_2D_ARRAY)) {//if not texture
                continue;
            }

            ConnectionDesc desc;
            desc.name = uniform.first;
            switch (uniform.second->type) {
                case Uniform::VariableTypes::CUBEMAP           : desc.type = "Cubemap"; break;
                case Uniform::VariableTypes::CUBEMAP_ARRAY     : desc.type = "Cubemap array"; break;
                case Uniform::VariableTypes::TEXTURE_2D        : desc.type = "Texture"; break;
                case Uniform::VariableTypes::TEXTURE_2D_ARRAY  : desc.type = "Texture array"; break;
                case Uniform::VariableTypes::BOOL              : desc.type = "Boolean"; break;
                case Uniform::VariableTypes::INT               : desc.type = "Integer"; break;
                case Uniform::VariableTypes::FLOAT             : desc.type = "Float"; break;
                case Uniform::VariableTypes::FLOAT_VEC2        : desc.type = "Vector2"; break;
                case Uniform::VariableTypes::FLOAT_VEC3        : desc.type = "Vector3"; break;
                case Uniform::VariableTypes::FLOAT_VEC4        : desc.type = "Vector4"; break;
                case Uniform::VariableTypes::FLOAT_MAT4        : desc.type = "Matrix4"; break;
                case Uniform::VariableTypes::UNDEFINED         : desc.type = "Undefined"; break;
            }
            type->inputConnections.push_back(desc);
        }

        auto outputMap = program->getOutputMap();
        for(const auto& output:outputMap) {
            ConnectionDesc desc;
            desc.name = output.first;
            switch (output.second.first) {
                case Uniform::VariableTypes::CUBEMAP           : desc.type = "Cubemap"; break;
                case Uniform::VariableTypes::CUBEMAP_ARRAY     : desc.type = "Cubemap array"; break;
                case Uniform::VariableTypes::TEXTURE_2D        : desc.type = "Texture"; break;
                case Uniform::VariableTypes::TEXTURE_2D_ARRAY  : desc.type = "Texture array"; break;
                case Uniform::VariableTypes::BOOL              : desc.type = "Boolean"; break;
                case Uniform::VariableTypes::INT               : desc.type = "Integer"; break;
                case Uniform::VariableTypes::FLOAT             : desc.type = "Float"; break;
                case Uniform::VariableTypes::FLOAT_VEC2        : desc.type = "Vector2"; break;
                case Uniform::VariableTypes::FLOAT_VEC3        : desc.type = "Vector3"; break;
                case Uniform::VariableTypes::FLOAT_VEC4        : desc.type = "Vector4"; break;
                case Uniform::VariableTypes::FLOAT_MAT4        : desc.type = "Matrix4"; break;
                case Uniform::VariableTypes::UNDEFINED         : desc.type = "Undefined"; break;
            }
            type->outputConnections.push_back(desc);
        }
        PipelineStageExtension::ProgramNameInfo programNameInfo;
        programNameInfo.vertexShaderName = program->getVertexShaderFile();
        programNameInfo.geometryShaderName = program->getGeometryShaderFile();
        programNameInfo.fragmentShaderName = program->getFragmentShaderFile();

        type->nodeExtensionConstructor = [=](const NodeType* nodeType[[gnu::unused]]) ->NodeExtension* {return new PipelineStageExtension(this->pipelineExtension, programNameInfo);};
        type->extraVariables["vertexShaderName"] = program->getVertexShaderFile();
        type->extraVariables["geometryShaderName"] = program->getGeometryShaderFile();
        type->extraVariables["fragmentShaderName"] = program->getFragmentShaderFile();

        nodeTypeVector.push_back(type);
    }

    std::unordered_map<std::string, std::function<EditorExtension*()>> possibleEditorExtensions;
    possibleEditorExtensions["PipelineExtension"] = [=]() ->EditorExtension* {return this->pipelineExtension;};

    std::unordered_map<std::string, std::function<NodeExtension*(const NodeType*)>> possibleNodeExtensions;
    possibleNodeExtensions["PipelineStageExtension"] = [=](const NodeType* nodeType) ->NodeExtension* {return new PipelineStageExtension(nodeType, this->pipelineExtension);};
    possibleNodeExtensions["IterationExtension"] = [=](const NodeType*) -> NodeExtension* {return new IterationExtension();};

    this->nodeGraph = NodeGraph::deserialize("./Data/nodeGraph.xml", possibleEditorExtensions, possibleNodeExtensions);

    bool freshNodeGraphCreated = false;
    if(this->nodeGraph == nullptr) {
        std::cerr << "No custom Nodegraph found, using the default." << std::endl;
        this->nodeGraph = NodeGraph::deserialize("./Engine/nodeGraph.xml", possibleEditorExtensions, possibleNodeExtensions);
        if(this->nodeGraph == nullptr) {
            std::cerr << "Default Node deserialize failed too, using empty node graph" << std::endl;
            this->nodeGraph = new NodeGraph(nodeTypeVector, false, this->pipelineExtension);
            freshNodeGraphCreated = true;
        }
    }

    if(!freshNodeGraphCreated) {
        // we loaded an old nodegraph. What if the available programs changed? if user add new programs, we should add them. If user removed programs, we should mark the pipeline as invalid, and warn.
        // First check if any node type we don't have is defined
        std::vector<const NodeType *> oldDefinedNodeTypes = this->nodeGraph->getNodeTypes();
        for(const NodeType* oldNodeType: oldDefinedNodeTypes) {
            bool nodeTypeFound = false;
            for(NodeType* newNodeType: nodeTypeVector) {
                if(newNodeType->isSameNodeType(*oldNodeType)) {
                    nodeTypeFound = true;
                    break;
                }
            }
            if(!nodeTypeFound) {
                // a node type that is unknown is found mark pipeline as invalid
                this->pipelineExtension->setNodeGraphValid(false);
                this->pipelineExtension->addError("Old Node type " + oldNodeType->name + " Not found, this graph is invalid");
            }
        }
        //now do it in reverse, and try to add new programs we found, to the nodeGraph
        for( NodeType* newNodeType: nodeTypeVector) {
            bool nodeTypeFound = false;
            for (const NodeType *oldNodeType: oldDefinedNodeTypes) {
                if (newNodeType->name == oldNodeType->name) {
                    //TODO it is possible, that there is a node type that has the same name, but it is different. we need to replace them
                    nodeTypeFound = true;
                    break;
                }
            }
            if (!nodeTypeFound) {
                if (this->nodeGraph->addNodeType(newNodeType)) {
                    std::cout << "New node type " << newNodeType->name << " added" << std::endl;
                } else {
                    std::cerr << "New node type " << newNodeType->name << " should be added, but rejected!" << std::endl;
                }
            }
        }
    }
}

void Editor::update(InputHandler &inputHandler) {
    if(!world->currentPlayersSettings->editorShown || inputHandler.getInputStates().getInputEvents(InputStates::Inputs::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
        if(world->handlePlayerInput(inputHandler)) {
            world->handleQuitRequest();
            return;
        }
    }
}
