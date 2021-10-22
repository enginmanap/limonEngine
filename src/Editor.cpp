//
// Created by engin on 28/09/2021.
//

#include <Assets/Animations/AnimationLoader.h>
#include "Editor.h"
#include "World.h"
#include "ImGuiHelper.h"
#include "Camera.h"
#include "GameObjects/Model.h"
#include "GameObjects/ModelGroup.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Players/MenuPlayer.h"
#include "GameObjects/Players/FreeCursorPlayer.h"
#include "GameObjects/Players/FreeMovingPlayer.h"
#include "GameObjects/TriggerObject.h"

#include "Assets/Animations/AnimationCustom.h"
#include "Assets/Animations/AnimationInterface.h"
#include "AnimationSequencer.h"
#include "WorldSaver.h"
#include "AI/AIMovementGrid.h"

//This method is used only for ImGui loaded animations list generation
bool getNameOfLoadedAnimation(void* data, int index, const char** outText) {
    auto& animations = *static_cast<std::vector<AnimationCustom> *>(data);
    if(index < 0 || (uint32_t)index >= animations.size()) {
        return false;
    }
    *outText = animations.at(index).getName().c_str();
    return true;

}

void Editor::renderEditor(World& world) {

    world.imgGuiHelper->NewFrame();
    if(world.showNodeGraph) {
        world.drawNodeEditor();
        world.imgGuiHelper->RenderDrawLists();
        return;
    }
    /* window definitions */
    {
        ImGui::Begin("Editor");
        if(world.guiPickMode == false) {
            if (ImGui::Button("Switch to GUI selection mode")) {
                world.guiPickMode = true;
            }
        } else {
            if (ImGui::Button("Switch to World selection mode")) {
                world.guiPickMode = false;
            }
        }

        //list available elements
        static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;
        glm::vec3 newObjectPosition = world.camera->getPosition() + 10.0f * world.camera->getCenter();


        if (ImGui::CollapsingHeader("Add New Object")) {
            static char modelAssetFilter[32] = {0};
            ImGui::InputText("Filter Assets ##ModelsAssetTreeFilter", modelAssetFilter, sizeof(modelAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
            std::string modelAssetFilterStr = modelAssetFilter;
            std::transform(modelAssetFilterStr.begin(), modelAssetFilterStr.end(), modelAssetFilterStr.begin(), ::tolower);
            const AssetManager::AvailableAssetsNode* filteredAssets = world.assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_MODEL, modelAssetFilterStr);
            world.imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_MODEL,
                                              "Model",
                                              &selectedAsset);

            static float newObjectWeight;
            ImGui::SliderFloat("Weight", &newObjectWeight, 0.0f, 100.0f);

            ImGui::NewLine();
            if(selectedAsset == nullptr) {
                ImGui::Button("Add Object");
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("No Asset Selected!");
            } else {
                if(ImGui::Button("Add Object")) {
                    Model* newModel = new Model(world.getNextObjectID(), world.assetManager, newObjectWeight,
                                                selectedAsset->fullPath, false);
                    newModel->getTransformation()->setTranslate(newObjectPosition);
                    world.addModelToWorld(newModel);
                    newModel->getRigidBody()->activate();
                    world.pickedObject = static_cast<GameObject*>(newModel);
                }
            }
        }
        if(world.pickedObject != nullptr && world.pickedObject->getTypeID() == GameObject::MODEL) {
            ImGui::Separator();
            static float copyOffsets[3] { 0.25f, 0.25f, 0.25f};
            ImGui::DragFloat3("Copy position offsets", copyOffsets, 0.1f);
            if (ImGui::Button("Copy Selected object")) {

                Model* pickedModel = dynamic_cast<Model*>(world.pickedObject);
                Model* newModel = new Model(*pickedModel, world.getNextObjectID());
                newModel->getTransformation()->addTranslate(glm::vec3(copyOffsets[0], copyOffsets[1], copyOffsets[2]));
                world.addModelToWorld(newModel);
                //now we should apply the animations

                if(world.onLoadAnimations.find(pickedModel) != world.onLoadAnimations.end() &&
                        world.activeAnimations.find(pickedModel) != world.activeAnimations.end()) {
                    world.addAnimationToObject(newModel->getWorldObjectID(), world.activeAnimations[pickedModel]->animationIndex,
                                         true, true);
                }
                world.pickedObject = static_cast<GameObject*>(newModel);
            }

            if(ImGui::Button("Attach this object to another")) {
                world.objectToAttach = dynamic_cast<Model*>(world.pickedObject);
            }
            if(world.objectToAttach != nullptr) {
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + world.objectToAttach->getName());
            }
            if(world.objectToAttach!= nullptr && world.objectToAttach->getWorldObjectID() != world.pickedObject->getWorldObjectID()) {
                std::string savedObjectName = world.objectToAttach->getName();
                if (ImGui::Button("Attach saved object to current")) {
                    Model *pickedModel = dynamic_cast<Model *>(world.pickedObject);
                    int32_t attachedBoneID;
                    Transformation* pickedModelTransformation = pickedModel->getAttachmentTransform(attachedBoneID);

                    glm::vec3 translate, scale;
                    glm::quat orientation;
                    pickedModelTransformation->getDifferenceAddition(*world.objectToAttach->getTransformation(), translate,
                                                                     scale, orientation);
                    translate = translate * (1.0f / pickedModelTransformation->getScale());
                    translate = translate * pickedModelTransformation->getOrientation();
                    world.objectToAttach->getTransformation()->setTranslate(translate);
                    world.objectToAttach->getTransformation()->setScale(scale);
                    world.objectToAttach->getTransformation()->setOrientation(orientation);
                    world.objectToAttach->getTransformation()->setParentTransform(pickedModelTransformation);
                    world.objectToAttach->setParentObject(pickedModel, attachedBoneID);
                    pickedModel->addChild(world.objectToAttach);
                    world.objectToAttach = nullptr;
                }
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + savedObjectName);
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Current Object: " + world.pickedObject->getName());
            }
        }
        if(world.pickedObject != nullptr && world.pickedObject->getTypeID() == GameObject::PLAYER) {
            if(world.objectToAttach!= nullptr && world.objectToAttach->getWorldObjectID() != world.pickedObject->getWorldObjectID()) {
                if (ImGui::Button("Attach saved object to Player")) {
                    world.clearWorldRefsBeforeAttachment(world.objectToAttach);
                    world.physicalPlayer->setAttachedModel(world.objectToAttach);
                    world.startingPlayer.attachedModel = world.objectToAttach;
                    world.objectToAttach = nullptr;
                }
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Model Groups")) {
            static uint32_t selectedModelGroup = 0;

            if (ImGui::BeginCombo("Model Group##combobox", (selectedModelGroup == 0? "No Group Selected." : world.modelGroups[selectedModelGroup]->getName().c_str()))) {
                for (auto iterator = world.modelGroups.begin();
                     iterator != world.modelGroups.end(); ++iterator) {
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
            PhysicalRenderable* pickedPhysicalRenderable = dynamic_cast<PhysicalRenderable*>(world.pickedObject);
            if(pickedPhysicalRenderable != nullptr && selectedModelGroup != 0 && world.pickedObject->getWorldObjectID() != selectedModelGroup) {
                //now prevent adding to self

                if(ImGui::Button("Add model to group")) {
                    world.modelGroups[selectedModelGroup]->addChild(pickedPhysicalRenderable);
                }
            } else {
                ImGui::Button("Add model to group");
                if(world.pickedObject == nullptr) {
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No object Selected");
                }else {
                    if (world.pickedObject->getWorldObjectID() == selectedModelGroup) {
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
                    ModelGroup* modelGroup = new ModelGroup(world.graphicsWrapper, world.getNextObjectID(), std::string(modelGroupNameBuffer));
                    world.modelGroups[modelGroup->getWorldObjectID()] = modelGroup;

                }
            } else {
                ImGui::Button("Create Group");
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Name is mandatory!");

            }
        }
        if(ImGui::Button("Add Trigger Volume")) {

            TriggerObject* to = new TriggerObject(world.getNextObjectID(), world.apiInstance);
            to->getTransformation()->setTranslate(newObjectPosition);
            world.dynamicsWorld->addCollisionObject(to->getGhostObject(), World::CollisionTypes::COLLIDE_TRIGGER_VOLUME | World::CollisionTypes::COLLIDE_EVERYTHING,
                                                    World::CollisionTypes::COLLIDE_PLAYER | World::CollisionTypes::COLLIDE_EVERYTHING);
            world.triggers[to->getWorldObjectID()] = to;

            world.pickedObject = static_cast<GameObject*>(to);
        }

        if (ImGui::CollapsingHeader("Add New Light")) {

            if(ImGui::Button("Add Point Light")) {
                Light* newLight = new Light(world.graphicsWrapper, world.getNextObjectID(), Light::LightTypes::POINT, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                //world.lights.push_back(newLight);
                world.addLight(newLight);
                world.pickedObject = newLight;
            }
            if(world.directionalLightIndex == -1) {//Allow single directional light
                if(ImGui::Button("Add Directional Light")) {
                    Light* newLight = new Light(world.graphicsWrapper, world.getNextObjectID(), Light::LightTypes::DIRECTIONAL, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                    world.addLight(newLight);
                }
            }

        }

        if (ImGui::CollapsingHeader("Add GUI Elements##The header")) {
            ImGui::Indent( 16.0f );
            if (ImGui::CollapsingHeader("Add GUI Layer##The header")) {
                world.addGUILayerControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Text##The header")) {
                world.addGUITextControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Image##The header")) {
                world.addGUIImageControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Button##The header")) {
                world.addGUIButtonControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Animation##The header")) {
                world.addGUIAnimationControls();
            }
            ImGui::Unindent( 16.0f );
        }
        if (ImGui::CollapsingHeader("Add Particle Emitter ##The header")) {
            world.addParticleEmitterEditor();
        }

        if (ImGui::CollapsingHeader("Custom Animations")) {
            //list loaded animations
            int listbox_item_current = -1;//not static because I don't want user to select a item.
            ImGui::ListBox("Loaded animations", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&world.loadedAnimations), world.loadedAnimations.size(), 10);
            ImGui::Separator();


            ImGui::NewLine();
            static char loadAnimationNameBuffer[32];
            ImGui::Text("Load animation from file:");
            //double # because I don't want to show it
            ImGui::InputText("##LoadAnimationNameField", loadAnimationNameBuffer, sizeof(loadAnimationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);
            if (ImGui::Button("load animation")) {
                AnimationCustom *animation = AnimationLoader::loadAnimation("./Data/Animations/" + std::string(loadAnimationNameBuffer) + ".xml");
                if (animation == nullptr) {
                    world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation load failed");
                } else {
                    world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation loaded");
                    world.loadedAnimations.push_back(*animation);
                }
            }
        }
        ImGui::Separator();
        if(ImGui::CollapsingHeader("Player properties")) {
            if (ImGui::BeginCombo("Starting Type", world.startingPlayer.typeToString().c_str())) {
                for (auto iterator = World::PlayerInfo::typeNames.begin();
                     iterator != World::PlayerInfo::typeNames.end(); ++iterator) {
                    bool isThisTypeSelected = iterator->second == world.startingPlayer.typeToString();
                    if (ImGui::Selectable(iterator->second.c_str(), isThisTypeSelected)) {
                        world.startingPlayer.setType(iterator->second);
                    }
                    if (isThisTypeSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            static bool showError = false;
            if(ImGui::InputText("Custom Extension name", world.extensionNameBuffer, 31, ImGuiInputTextFlags_CharsNoBlank)) {
                showError = false;
            }

            if(world.startingPlayer.attachedModel != nullptr) {
                if(ImGui::Button("Disconnect Attachment##player attachment")) {
                    Model* attachedModel = world.startingPlayer.attachedModel;
                    world.startingPlayer.attachedModel = nullptr;
                    world.physicalPlayer->setAttachedModel(nullptr);
                    world.addModelToWorld(attachedModel);
                }
            }

            if(ImGui::Button("Apply##PlayerExtensionUpdate")) {
                std::string tempName = world.extensionNameBuffer;
                //find the starting player, and apply this change to it:
                Player* playerToUpdate = nullptr;
                switch (world.startingPlayer.type) {
                    case World::PlayerInfo::Types::DEBUG_PLAYER: playerToUpdate = world.debugPlayer; break;
                    case World::PlayerInfo::Types::EDITOR_PLAYER: playerToUpdate = world.editorPlayer; break;
                    case World::PlayerInfo::Types::PHYSICAL_PLAYER: playerToUpdate = world.physicalPlayer; break;
                    case World::PlayerInfo::Types::MENU_PLAYER: playerToUpdate = world.menuPlayer; break;
                }
                if(playerToUpdate != nullptr && tempName != "") {
                    PlayerExtensionInterface* extension = PlayerExtensionInterface::createExtension(tempName, world.apiInstance);
                    if(extension != nullptr) {
                        world.startingPlayer.extensionName = tempName;
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
                    for (size_t i = 0; i < world.onLoadActions.size(); i++) {
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
                TriggerObject::PutTriggerInGui(world.apiInstance, world.onLoadActions[onLoadTriggerIndex]->action,
                                               world.onLoadActions[onLoadTriggerIndex]->parameters,
                                               world.onLoadActions[onLoadTriggerIndex]->enabled, 100 + onLoadTriggerIndex);
                if (world.onLoadActions[world.onLoadActions.size() - 1]->enabled) {
                    //when user presses the enable button, add another and select it
                    onLoadTriggerIndex = world.onLoadActions.size();
                    world.onLoadActions.push_back(new World::ActionForOnload());
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
                const AssetManager::AvailableAssetsNode *filteredAssets = world.assetManager->getAvailableAssetsTreeFiltered(
                        AssetManager::Asset_type_SOUND, musicAssetFilterStr);
                world.imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_SOUND,
                                                  "Music",
                                                  &selectedSoundAsset);

                if (world.music != nullptr) {
                    ImGui::Text("Current Music: %s", world.music->getName().c_str());
                } else {
                    ImGui::Text("No music set for level. ");
                }

                if (selectedSoundAsset != nullptr) {
                    if (ImGui::Button("Change Music")) {
                        if (world.music != nullptr) {
                            world.music->stop();
                            delete world.music;
                        }
                        world.music = new Sound(world.getNextObjectID(), world.assetManager, selectedSoundAsset->fullPath);
                        world.music->setLoop(true);
                        world.music->setWorldPosition(glm::vec3(0, 0, 0), true);
                        world.music->play();
                    }
                } else {
                    ImGui::Button("Change Music");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No sound asset selected");
                }
            }
            if(ImGui::CollapsingHeader("SkyBox")) {
                ImGui::Indent(16.0f);
                world.addSkyBoxControls();
            }
            if(ImGui::CollapsingHeader("LoadingImage")) {
                ImGui::Indent(16.0f);
                static const AssetManager::AvailableAssetsNode* selectedLoadingImageAsset = nullptr;

                static char loadingImageAssetFilter[32] = {0};
                ImGui::InputText("Filter Assets ##TextureAssetTreeFilter", loadingImageAssetFilter, sizeof(loadingImageAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
                std::string loadingImageAssetFilterStr = loadingImageAssetFilter;
                std::transform(loadingImageAssetFilterStr.begin(), loadingImageAssetFilterStr.end(), loadingImageAssetFilterStr.begin(), ::tolower);
                const AssetManager::AvailableAssetsNode* filteredAssets = world.assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, loadingImageAssetFilterStr);
                world.imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE, "LoadingImage", &selectedLoadingImageAsset);

                if(selectedLoadingImageAsset == nullptr) {
                    ImGui::Button("Set Loading Image");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No Asset Selected!");
                } else {
                    if (ImGui::Button("Set Loading Image")) {
                        world.loadingImage = selectedLoadingImageAsset->fullPath;
                    }
                }
                if(world.loadingImage != "" ) {
                    ImGui::Text("Current Loading Image: %s", world.loadingImage.c_str() );
                } else {
                    ImGui::Text("No loading image set");
                }
            }


            if(ImGui::CollapsingHeader("ESC handling")) {
                ImGui::Indent(16.0f);
                ImGui::Text("By default, esc quits the game");

                if (ImGui::RadioButton("Quit Game", world.currentQuitResponse == World::QuitResponse::QUIT_GAME)) {
                    world.currentQuitResponse = World::QuitResponse::QUIT_GAME;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Return Previous", world.currentQuitResponse == World::QuitResponse::RETURN_PREVIOUS)) {
                    world.currentQuitResponse = World::QuitResponse::RETURN_PREVIOUS;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Load World", world.currentQuitResponse == World::QuitResponse::LOAD_WORLD)) {
                    world.currentQuitResponse = World::QuitResponse::LOAD_WORLD;
                }
                if (world.currentQuitResponse == World::QuitResponse::LOAD_WORLD) {
                    ImGui::InputText("Custom World file ", world.quitWorldNameBuffer, sizeof(world.quitWorldNameBuffer));
                    if (ImGui::Button("Apply##custom world file setting")) {
                        world.quitWorldName = world.quitWorldNameBuffer;
                    }
                }
            }

            ImGui::Unindent( 16.0f );

        }

        ImGui::Separator();

        ImGui::InputText("##save world name", world.worldSaveNameBuffer, sizeof(world.worldSaveNameBuffer));
        ImGui::SameLine();
        if(ImGui::Button("Save World")) {
            for(auto animIt = world.loadedAnimations.begin(); animIt != world.loadedAnimations.end(); animIt++) {
                if(animIt->serializeAnimation("./Data/Animations/")) {
                    world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation saved");
                } else {
                    world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation save failed");
                }
            }
            //before saving, set the connection state
            for (auto objectIt = world.disconnectedModels.begin(); objectIt != world.disconnectedModels.end(); ++objectIt) {
                world.disconnectObjectFromPhysics(*objectIt);
            }

            if(WorldSaver::saveWorld(world.worldSaveNameBuffer, &world)) {
                world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "World save successful");
            } else {
                world.options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "World save Failed");
            }
            //after save, set the states back
            for (auto objectIt = world.disconnectedModels.begin(); objectIt != world.disconnectedModels.end(); ++objectIt) {
                world.reconnectObjectToPhysics(*objectIt);
            }

        }
        if(ImGui::Button("Save AI walk Grid")) {
            if(world.grid != nullptr) {
                std::string AIWalkName = world.name.substr(0, world.name.find_last_of(".")) + ".aiwalk";
                world.grid->serializeXML(AIWalkName);
            }
        }
#ifdef CEREAL_SUPPORT
        if(ImGui::Button("Save AI walk Grid Binary")) {
            if(world.grid != nullptr) {
                std::string AIWalkName = world.name.substr(0, world.name.find_last_of(".")) + ".aiwalkb";
                std::ofstream os(AIWalkName, std::ios::binary);
                cereal::BinaryOutputArchive archive( os );

                archive(*grid);
            }
        }
#endif
        if(ImGui::Button("Convert models to binary")) {
            std::set<std::vector<std::string>> convertedAssets;
            for (auto objectIt = world.objects.begin(); objectIt != world.objects.end(); ++objectIt) {

                Model* model = dynamic_cast<Model*>(objectIt->second);
                if(model!= nullptr) {
                    model->convertAssetToLimon(convertedAssets);
                }
            }
            if(world.startingPlayer.attachedModel != nullptr) {
                world.startingPlayer.attachedModel->convertAssetToLimon(convertedAssets);
            }
        }

        if(ImGui::Button("Change Render Pipeline")) {
            world.showNodeGraph = true;
        }
        if (ImGui::CollapsingHeader("Render Debugging")) {
            static int listbox_item_current = -1;//not static because I don't want user to select a item.
            static ImGuiImageWrapper wrapper;//keeps selected texture and layer;
            std::vector<std::shared_ptr<Texture>> allTextures = world.renderPipeline->getTextures();

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
                ImGui::Image(&wrapper, size);
            }


        }
        if(ImGui::CollapsingHeader("List materials")) {
            static std::map<size_t, std::shared_ptr<Material>> allMaterials;
            if (allMaterials.empty()) {
                for (auto const &renderable: world.objects) {
                    std::vector<std::shared_ptr<Material>> currentMaterials = renderable.second->getMaterials();
                    for(auto const& material: currentMaterials) {
                        size_t hash = material->getHash();
                        if(allMaterials.find(hash) != allMaterials.end()) {
                            std::cerr << "collision find between " << material->getName() << " and " << allMaterials.find(hash)->second->getName() << "." << std::endl;
                        } else {
                            allMaterials[hash] = material;
                        }
                    }
                }
            }

            //listing
            static size_t selectedHash = 0;
            bool isSelected = false;
            ImGui::Text("Total material count is %llu", allMaterials.size());
            ImGui::BeginListBox("Materials");
            for (auto it = allMaterials.begin(); it != allMaterials.end(); it++) {
                isSelected = selectedHash == it->first;
                if (ImGui::Selectable((it->second->getName() + " -> " + std::to_string(it->first)).c_str(), isSelected)) {
                    selectedHash = it->first;
                }
            }
            ImGui::EndListBox();
            auto selectedMaterialIt = allMaterials.find(selectedHash);
            if(selectedMaterialIt != allMaterials.end()) {
                GameObject::ImGuiRequest request(glm::mat4(0), glm::mat4(0),
                                                                                glm::mat4(0), 0, 0, world.apiInstance);

                selectedMaterialIt->second->addImGuiEditorElements(request);
            }
        }
        ImGui::End();

        //ImGui::SetNextWindowSize(ImVec2(0,0), false);//true means set it only once

        ImGui::Begin("Selected Object Properties");
        std::string selectedName;
        if(world.pickedObject == nullptr) {
            selectedName = "No object selected";
        } else {
            selectedName = world.pickedObject->getName().c_str();
        }

        world.buildTreeFromAllGameObjects();

        if(world.pickedObject != nullptr) {
            GameObject::ImGuiResult objectEditorResult = world.pickedObject->addImGuiEditorElements(*world.request);

            switch(world.pickedObject->getTypeID()) {
                case GameObject::MODEL: {
                    Model* selectedObject = static_cast<Model*>(world.pickedObject);
                    if(objectEditorResult.updated) {
                        if(!selectedObject->isDisconnected()) {
                            world.dynamicsWorld->updateSingleAabb(selectedObject->getRigidBody());
                        }
                        world.updatedModels.push_back(selectedObject);
                    }
                    uint32_t removedActorID = 0;
                    if (objectEditorResult.removeAI) {
                        //remove AI requested
                        if (dynamic_cast<Model *>(world.pickedObject)->getAIID() != 0) {
                            removedActorID = dynamic_cast<Model *>(world.pickedObject)->getAIID();
                            world.actors.erase(dynamic_cast<Model *>(world.pickedObject)->getAIID());
                            dynamic_cast<Model *>(world.pickedObject)->detachAI();
                        }
                    }

                    if (objectEditorResult.addAI) {
                        std::cout << "adding AI to model " << std::endl;
                        if(removedActorID == 0) {
                            removedActorID = world.getNextObjectID();
                        }
                        //if remove and add is called in same frame, it means the type is changed, reuse the ID
                        ActorInterface *newEnemy = ActorInterface::createActor(objectEditorResult.actorTypeName, removedActorID, world.apiInstance);
                        Model* model = dynamic_cast<Model *>(world.pickedObject);
                        if(model != nullptr) {
                            newEnemy->setModel(model->getWorldObjectID());
                            model->attachAI(newEnemy);
                        } else {
                            std::cerr << "ActorInterface Model setting failed, because picked object is not a model." << std::endl;

                        }
                        world.addActor(newEnemy);
                    } else {
                        if(removedActorID != 0) {
                            world.unusedIDs.push(removedActorID);
                        }
                    }
                }
                    /* fall through */
/************** ATTENTION, NO BREAK ******************/
                case GameObject::GUI_TEXT:
                case GameObject::GUI_IMAGE:
                case GameObject::GUI_BUTTON:
                case GameObject::GUI_ANIMATION:
                {
                    Renderable* selectedObject = dynamic_cast<Renderable*>(world.pickedObject);
                    if(selectedObject != nullptr) {
                        //Now we are looking for animations
                        if (world.activeAnimations.find(selectedObject) != world.activeAnimations.end()) {
                            if (objectEditorResult.updated) {
                                world.activeAnimations[selectedObject]->originChange = true;
                            }

                            if (ImGui::Button(("Remove custom animation: " +
                                    world.loadedAnimations[world.activeAnimations[selectedObject]->animationIndex].getName()).c_str())) {
                                ImGui::OpenPopup("How To Remove Custom Animation");
                            }
                            if (ImGui::BeginPopupModal("How To Remove Custom Animation")) {
                                AnimationCustom &animationToRemove = world.loadedAnimations[world.activeAnimations[selectedObject]->animationIndex];
                                World::AnimationStatus *animationStatusToRemove = world.activeAnimations[selectedObject];
                                if (ImGui::Button("Set to Start##CustomAnimationRemoval")) {
                                    world.removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, 0);
                                    ImGui::CloseCurrentPopup();
                                }

                                if (ImGui::Button("Set to End##CustomAnimationRemoval")) {
                                    world.removeActiveCustomAnimation(animationToRemove, animationStatusToRemove,
                                                                animationToRemove.getDuration());
                                    ImGui::CloseCurrentPopup();
                                }
                                static float customTime = 0.0f;
                                ImGui::DragFloat("##CustomAnimationRemovalCustomTime", &customTime, 0.1, 0.0,
                                                 animationToRemove.getDuration());
                                ImGui::SameLine();
                                if (ImGui::Button("Set to custom time##CustomAnimationRemoval")) {
                                    world.removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, customTime);
                                    ImGui::CloseCurrentPopup();
                                }
                                if (ImGui::Button("Cancel##CustomAnimationRemoval")) {
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndPopup();

                            }
                        } else {
                            addAnimationDefinitionToEditor(world);
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
            switch (world.pickedObject->getTypeID()) {
                case GameObject::MODEL: {
                    if (world.disconnectedModels.find(world.pickedObject->getWorldObjectID()) != world.disconnectedModels.end()) {
                        if (ImGui::Button("reconnect to physics")) {
                            world.reconnectObjectToPhysicsRequest(static_cast<Model *>(world.pickedObject)->getWorldObjectID());//Request because that action will not be carried out on editor mode
                        }
                    } else {
                        if (ImGui::Button("Disconnect from physics")) {
                            world.disconnectObjectFromPhysicsRequest(static_cast<Model *>(world.pickedObject)->getWorldObjectID());
                        }
                        ImGui::Text(
                                "If object is placed in trigger volume, \ndisconnecting drastically improve performance.");
                    }

                    if (ImGui::Button("Remove This Object")) {
                        world.removeObject(world.pickedObject->getWorldObjectID());
                        world.pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::TRIGGER: {
                    if (ImGui::Button("Remove This Trigger")) {
                        world.removeTriggerObject(world.pickedObject->getWorldObjectID());
                        world.pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::GUI_TEXT:
                case GameObject::GUI_IMAGE:
                case GameObject::GUI_BUTTON:
                case GameObject::GUI_ANIMATION: {
                    if(objectEditorResult.remove) {
                        world.guiElements.erase(world.pickedObject->getWorldObjectID());
                        world.unusedIDs.push(world.pickedObject->getWorldObjectID());
                        delete world.pickedObject;
                        world.pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::LIGHT: {
                    if(objectEditorResult.remove) {
                        for (auto iterator = world.lights.begin(); iterator != world.lights.end(); ++iterator) {
                            if((*iterator)->getWorldObjectID() == world.pickedObject->getWorldObjectID()) {
                                world.unusedIDs.push(world.pickedObject->getWorldObjectID());
                                world.lights.erase(iterator);
                                break;
                            }
                        }
                        if(static_cast<Light*>(world.pickedObject)->getLightType() == Light::LightTypes::DIRECTIONAL) {
                            world.directionalLightIndex = -1;
                        }
                        delete world.pickedObject;
                        world.updateActiveLights(true);
                        world.pickedObject = nullptr;
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
    world.imgGuiHelper->RenderDrawLists();
}


void Editor::addAnimationDefinitionToEditor(World& world) {
    if (ImGui::CollapsingHeader("Custom animation properties")) {
        //If there is no animation setup ongoing, or there is one, but not for this model,
        //put start animation button.
        //else put time input, add and finalize buttons.
        if (world.animationInProgress == nullptr || world.animationInProgress->getAnimatingObject() != dynamic_cast<Renderable *>(world.pickedObject)) {

            static int listbox_item_current = 0;
            ImGui::Text("Loaded animations list");
            ImGui::ListBox("##Loaded animations listbox", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&world.loadedAnimations), world.loadedAnimations.size(), 10);

            if (ImGui::Button("Apply selected")) {
                world.addAnimationToObject(world.pickedObject->getWorldObjectID(), listbox_item_current,
                                     true, true);
            }

            ImGui::SameLine();
            if (ImGui::Button("Create new")) {
                if (world.animationInProgress == nullptr) {
                    world.animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(world.pickedObject));
                } else {
                    //ask for removal of the old work
                    delete world.animationInProgress;
                    world.animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(world.pickedObject));
                }
                // At this point we should know the animationInProgress is for current object
            }
        } else {
            ImGui::Text("Please use animation definition window.");
            bool finished, cancelled;
            world.animationInProgress->addAnimationSequencerToEditor(finished, cancelled);
            if (finished) {
                world.loadedAnimations.push_back(AnimationCustom(*world.animationInProgress->buildAnimationFromCurrentItems()));

                world.addAnimationToObject(world.pickedObject->getWorldObjectID(),
                                     world.loadedAnimations.size() - 1, true, true);
                delete world.animationInProgress;
                world.animationInProgress = nullptr;
            }
            if (cancelled) {
                delete world.animationInProgress;
                world.animationInProgress = nullptr;
            }
        }
    }
}

