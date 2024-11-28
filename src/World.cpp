   //
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include <random>
#include <Graphics/GraphicsPipeline.h>
#include <API/Graphics/RenderMethodInterface.h>
#include "NodeEditorExtensions/PipelineStageExtension.h"
#include "NodeEditorExtensions/PipelineExtension.h"
#include "NodeEditorExtensions/IterationExtension.h"
#include "nodeGraph/src/NodeGraph.h"

#include "Camera/PerspectiveCamera.h"
#include "BulletDebugDrawer.h"
#include "AI/AIMovementGrid.h"


#include "GameObjects/Players/FreeCursorPlayer.h"
#include "GameObjects/Players/FreeMovingPlayer.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Players/MenuPlayer.h"
#include "GameObjects/Light.h"
#include "GUI/GUILayer.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUIFPSCounter.h"
#include "GUI/GUITextDynamic.h"
#include "ImGuiHelper.h"
#include "WorldSaver.h"
#include "GameObjects/TriggerObject.h"
#include "Assets/Animations/AnimationLoader.h"
#include "Assets/Animations/AnimationCustom.h"
#include "AnimationSequencer.h"
#include "GUI/GUICursor.h"
#include "GameObjects/GUIText.h"
#include "GameObjects/GUIImage.h"
#include "GameObjects/GUIButton.h"
#include "GameObjects/GUIAnimation.h"
#include "GameObjects/ModelGroup.h"
#include "Graphics/PostProcess/QuadRender.h"
#include "Editor/Editor.h"

   const std::map<World::PlayerInfo::Types, std::string> World::PlayerInfo::typeNames =
    {
            { Types::PHYSICAL_PLAYER, "Physical"},
            { Types::DEBUG_PLAYER, "Debug"},
            { Types::EDITOR_PLAYER, "Editor"},
            { Types::MENU_PLAYER, "Menu" }
    };

SDL2MultiThreading::Condition VisibilityRequest::condition; //FIXME this variable doesn't looks like it belongs here

World::World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
             std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options *options)
        : assetManager(assetManager), options(options), graphicsWrapper(assetManager->getGraphicsWrapper()),
        alHelper(assetManager->getAlHelper()), name(name), fontManager(graphicsWrapper),
        startingPlayer(startingPlayerType) {
    strncpy(worldSaveNameBuffer, name.c_str(), sizeof(worldSaveNameBuffer) -1 );
    editor = std::make_unique<Editor>(this);
    // physics init
    broadphase = new btDbvtBroadphase();
    ghostPairCallback = new btGhostPairCallback();
    broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(
            ghostPairCallback);    // Needed once to enable ghost objects inside Bullet

    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    solver = new btSequentialImpulseConstraintSolver;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));
    debugDrawer = new BulletDebugDrawer(assetManager, options);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);

    apiGUILayer = new GUILayer(graphicsWrapper, debugDrawer, 1);
    apiGUILayer->setDebug(false);

    renderCounts = new GUIText(graphicsWrapper, getNextObjectID(), "Render Counts",
                               fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0", glm::vec3(204, 204, 0));
    renderCounts->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 170, options->getScreenHeight() - 36), 0);

    cursor = new GUICursor(graphicsWrapper, assetManager, "./Data/Textures/crosshair.png");

    cursor->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), 0);

    debugOutputGUI = new GUITextDynamic(graphicsWrapper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16),
                                        glm::vec3(0, 0, 0), 640, 380, options);
    debugOutputGUI->set2dWorldTransform(glm::vec2(320, options->getScreenHeight()-200), 0.0f);

    switch(startingPlayer.type) {
        case PlayerInfo::Types::PHYSICAL_PLAYER:
            physicalPlayer = new PhysicalPlayer(1, options, cursor, startingPlayer.position, startingPlayer.orientation, startingPlayerType.attachedModel);// 1 is reserved for physical player
            currentPlayer = physicalPlayer;
            break;
        case PlayerInfo::Types::DEBUG_PLAYER:
            debugPlayer = new FreeMovingPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
            currentPlayer = debugPlayer;
            break;
        case PlayerInfo::Types::EDITOR_PLAYER:
            editorPlayer = new FreeCursorPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
            currentPlayer = editorPlayer;
            break;
        case PlayerInfo::Types::MENU_PLAYER:
            menuPlayer = new MenuPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
            currentPlayer = menuPlayer;
            break;
    }
    
    quadRender = std::make_shared<QuadRender>(graphicsWrapper);
    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    playerCamera = new PerspectiveCamera("Player camera", options, currentPlayer->getCameraAttachment());//register is just below
    playerCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_STATIC);
    playerCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
    playerCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_BASIC);
    playerCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_TRANSPARENT);
    playerCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_ANIMATED);
    playerCamera->addRenderTag(HardCodedTags::PICKED_OBJECT);
    playerCamera->addTag(HardCodedTags::CAMERA_PLAYER);
    cullingResults.insert(std::make_pair(playerCamera, new std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>, VisibilityRequest::uint64_vector_hasher>()));//new camera, new visibility
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                           COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                           COLLIDE_MODELS | COLLIDE_EVERYTHING, worldAABBMin,
                                           worldAABBMax);
    switchPlayer(currentPlayer, *inputHandler); //switching to itself, to set the states properly. It uses camera so done after camera creation


    renderPipeline = GraphicsPipeline::deserialize("./Data/renderPipeline.xml", graphicsWrapper, assetManager, options, buildRenderMethods());

    if(renderPipeline == nullptr) {
        //use default if no custom is found
        std::cerr << "Render pipeline not found, loading default." << std::endl;
        renderPipeline = GraphicsPipeline::deserialize("./Engine/renderPipeline.xml", graphicsWrapper, assetManager, options, buildRenderMethods());
    }

    fpsCounter = new GUIFPSCounter(graphicsWrapper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                                   glm::vec3(204, 204, 0));
    fpsCounter->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);

    onLoadActions.push_back(new ActionForOnload());//this is here for editor, as if no action is added, editor would fail to allow setting the first one.

    modelIndicesBuffer.reserve(NR_MAX_MODELS);
    tempRenderedObjectsSet.reserve(NR_MAX_MODELS);

    activeLights.reserve(NR_TOTAL_LIGHTS);

    OptionsUtil::Options::Option<bool> multiThreadCullingOption = options->getOption<bool>(HASH("multiThreadedCulling"));
    renderInformationsOption = options->getOption<bool>(HASH("renderInformations"));
    multiThreadedCulling = multiThreadCullingOption.getOrDefault(true);

    /************ ImGui *****************************/
    // Setup ImGui binding
    imgGuiHelper = new ImGuiHelper(assetManager, options);
}

   RenderMethods World::buildRenderMethods() {
       RenderMethods renderMethods;

       renderMethods.renderParticleEmitters             = std::bind(&World::renderParticleEmitters,                     this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderGPUParticleEmitters          = std::bind(&World::renderGPUParticleEmitters,                  this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderGUITexts                     = std::bind(&World::renderGUITexts,                             this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderGUIImages                    = std::bind(&World::renderGUIImages,                            this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderEditor                       = std::bind(&World::ImGuiFrameSetup,                            this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderSky                          = std::bind(&World::renderSky,                                  this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderDebug                        = std::bind(&World::renderDebug,                                this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderCameraByTag                  = std::bind(&World::renderCameraByTag,                          this,               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       renderMethods.renderQuad                         = std::bind(&QuadRender::render,                                this->quadRender,   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

       renderMethods.getLightsByType = std::bind(&World::getLightIndexes, this, std::placeholders::_1);
       renderMethods.renderLight = std::bind(&World::renderLight, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
       return renderMethods;
   }

   bool World::checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName) {
     //FIXME this debug draw creates a flicker, because we redraw frames that surpass 60. we need duration for debug draw to prevent it
     //debugDrawer->drawLine(GLMConverter::GLMToBlt(from), camera.getRigidBody()->getCenterOfMassPosition(), btVector3(1,0,0));
     btCollisionWorld::AllHitsRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), GLMConverter::GLMToBlt(currentPlayer->getPosition()));

     dynamicsWorld->rayTest(
             GLMConverter::GLMToBlt(from),
             GLMConverter::GLMToBlt(currentPlayer->getPosition()),
             RayCallback
     );


     if (RayCallback.hasHit()) {
         btVector3 closestDistance = btVector3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
         bool hasSeen = false;
         for (int i = 0; i < RayCallback.m_collisionObjects.size(); ++i) {//it turns out, the hits are not ordered, so keep what we see
             btVector3 distance = RayCallback.m_hitPointWorld[i] - RayCallback.m_rayFromWorld;
             if(distance.length2() < closestDistance.length2()) {//length2 doesn't invoke sqtr, so it should be faster
                 GameObject *gameObject = static_cast<GameObject *>(RayCallback.m_collisionObjects[i]->getUserPointer());
                 if(gameObject->getName() == fromName) {
                     continue; //self should not change closest hit
                 }
                 closestDistance = distance;
                 if (gameObject->getTypeID() != GameObject::PLAYER && gameObject->getTypeID() != GameObject::TRIGGER ) { //trigger is ghost, so it should not block
                     if(hasSeen) {
                         //means we saw the player, and this is closer than player
                         return false;
                     }
                 }
                 if (gameObject->getTypeID() == GameObject::PLAYER) {
                     hasSeen = true;
                 }
             }
         }

         return hasSeen;
     }
     return false;//if ray did not hit anything, return false. This should never happen
 }

 /**
  * Simulates given time (it should be constantant), reads input, animates models etc.
  * Returns true if quit requested.
  * @param simulationTimeFrame
  * @param inputHandler
  * @return
  */
 void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler, uint64_t wallTime) {

     // If not in editor mode, dont let imgGuiHelper get input
     // if in editor mode, but player press editor button, dont allow imgui to process input
     // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
     if(!currentPlayersSettings->editorShown || inputHandler.getInputStates().getInputEvents(InputStates::Inputs ::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
         if(handlePlayerInput(inputHandler)) {
             handleQuitRequest();
             return;
         }
     }

     this->wallTime = wallTime;
     //Seperating physics step and visibility, because physics is used by camera, and camera is used by visibility
     if(currentPlayersSettings->worldSimulation) {
         //every time we call this method, we increase the time only by simulationTimeframe
         gameTime += simulationTimeFrame;
         dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
         currentPlayer->processPhysicsWorld(dynamicsWorld);
     }
     checkAndRunTimedEvents();//no londer requires to be in world simulation, because it checks both game time and wall time now
     if(playerCamera->isDirty()) {
         graphicsWrapper->setPlayerMatrices(playerCamera->getPosition(), playerCamera->getCameraMatrix(), gameTime);//this is required for any render
         alHelper->setListenerPositionAndOrientation(playerCamera->getPosition(), playerCamera->getCenter(), playerCamera->getUp());
     }

     if(currentPlayersSettings->worldSimulation) {
         for(const auto& emitter:emitters) {
             emitter.second->setupForTime(gameTime);
         }

         for(const auto& gpuEmitter:gpuParticleEmitters) {
             gpuEmitter.second->setupForTime(gameTime);
         }

         for(auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
             trigger->second->checkAndTrigger();
         }
         animateCustomAnimations();
         for(auto actorIt = actors.begin(); actorIt != actors.end(); ++actorIt) {
             ActorInterface::ActorInformation information = fillActorInformation(actorIt->second);
             actorIt->second->play(gameTime, information);
         }
         for (auto it = objects.begin(); it != objects.end(); ++it) {
             if (!it->second->getRigidBody()->isStaticOrKinematicObject() && it->second->getRigidBody()->isActive()) {
                 it->second->updateTransformFromPhysics();
                 Model* model = dynamic_cast<Model*>(it->second);
                 assert(model!= nullptr);
                 updatedModels.push_back(model);
             }
         }

         tempRenderedObjectsSet.clear();
         for (const auto &visibility: cullingResults) {
             for (const auto &visibleTags: *visibility.second){
                 for (const auto &visibleAssets: visibleTags.second) {
                     for (const auto &visibleObjectId: visibleAssets.second.first) {
                         if (tempRenderedObjectsSet.find(visibleObjectId) == tempRenderedObjectsSet.end()) {
                             objects[visibleObjectId]->setupForTime(gameTime);
                             tempRenderedObjectsSet.insert(visibleObjectId);
                         }
                     }
                 }
             }
         }

         //Player setup
         if(startingPlayer.attachedModel != nullptr) {
             startingPlayer.attachedModel->setupForTime(gameTime);
         }
     }

     for (size_t j = 0; j < activeLights.size(); ++j) {
         activeLights[j]->step(gameTime, playerCamera);
     }
     updateActiveLights(false);

     fillVisibleObjectsUsingTags();

     //FIXME moved out of fillVisible because for the time being we have 2 (fillVisibleObjects(), fillVisibleObjectsUsingTags()) once one is gone, these 2 clears should go in.
     updatedModels.clear();
     playerCamera->clearDirty();
    if(sky != nullptr) { // menu worlds don't have sky set, and WIP levels can miss it too.
        sky->step(playerCamera);
    }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }
    debugOutputGUI->setupForTime(gameTime);

    if(currentPlayersSettings->menuInteraction) {
        GUIButton* button = nullptr;

        GameObject* pointed = this->getPointedObject(COLLIDE_EVERYTHING, ~(COLLIDE_NOTHING));
        if(pointed != nullptr && pointed->getTypeID() == GameObject::GUI_BUTTON) {
            button = dynamic_cast<GUIButton*>(pointed);
        }

        if(button != nullptr) {
            if(this->hoveringButton != button) {
                if(this->hoveringButton != nullptr) {
                    this->hoveringButton->setOnHover(false);
                    this->hoveringButton->setOnClick(false);
                }
            }
            this->hoveringButton = button;
            button->setOnHover(true);
            if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
                if(inputHandler.getInputStates().getInputEvents(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
                    button->setOnClick(true);
                }
            } else {
                button->setOnClick(false);
            }
        } else {
            if(this->hoveringButton != nullptr) {
                this->hoveringButton->setOnHover(false);
                this->hoveringButton->setOnClick(false);
            }
        }
    }
}

void World::animateCustomAnimations() {
    // ATTENTION iterator is not increased in for, it is done manually.
    for(auto animIt = activeAnimations.begin(); animIt != activeAnimations.end();) {
        AnimationStatus* animationStatus = animIt->second;
        if(animationStatus->originChange) {
            //this means the origin has changed in editor mode, so we should update our origin of transformation.

            // First change the original transform to current, since user updated it
            animationStatus->originalTransformation.setTranslate(animationStatus->object->getTransformation()->getTranslate());
            animationStatus->originalTransformation.setScale(animationStatus->object->getTransformation()->getScale());
            animationStatus->originalTransformation.setOrientation(animationStatus->object->getTransformation()->getOrientation());

            animationStatus->object->getTransformation()->setScale(glm::vec3(1.0f,1.0f,1.0f));//then remove the change from object transform.
            animationStatus->object->getTransformation()->setTranslate(glm::vec3(0.0f,0.0f,0.0f));
            animationStatus->object->getTransformation()->setOrientation(glm::quat(1.0f,0.0f,0.0f, 0.0f));
            animationStatus->originChange = false;
        }
        const AnimationCustom* animationCustom = &loadedAnimations[animationStatus->animationIndex];
        if((animationStatus->loop ) || animationCustom->getDuration() / animationCustom->getTicksPerSecond() * 1000  + animationStatus->startTime >
                                             gameTime) {
            float ticksPerSecond;
            if (animationCustom->getTicksPerSecond() != 0) {
                ticksPerSecond = animationCustom->getTicksPerSecond();
            } else {
                ticksPerSecond = TICK_PER_SECOND;
            }
            float animationTime = fmod(((gameTime - animationStatus->startTime) / 1000.0f) * ticksPerSecond, animationCustom->getDuration());
            animationCustom->calculateTransform("", animationTime, *animationStatus->object->getTransformation());

            if(animationStatus->sound) {
                animationStatus->sound->setWorldPosition(
                animationStatus->object->getTransformation()->getTranslate());
            }
            animIt++;
        } else {
            //animation finish case, set final transform and remove
            float animationTime = animationCustom->getDuration();
            animationCustom->calculateTransform("", animationTime, *animationStatus->object->getTransformation());

            if(!animationStatus->wasKinematic && animationStatus->wasPhysical) {
                PhysicalRenderable* tempPointer = dynamic_cast<PhysicalRenderable*>(animationStatus->object);//this can be static cast, since it is not possible to be anything else.
                tempPointer->getRigidBody()->setCollisionFlags(tempPointer->getRigidBody()->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                tempPointer->getRigidBody()->setActivationState(ACTIVE_TAG);
            }

            if(animationStatus->sound) {
                animationStatus->sound->stop();
            }

            //now before deleting the animation, separate parent/child animations

            animationStatus->object->getTransformation()->getWorldTransform();//make sure propagates were run

            glm::vec3 tempScale, tempTranslate;
            glm::quat tempOrientation;
            tempScale       = animationStatus->object->getTransformation()->getScale();
            tempTranslate   = animationStatus->object->getTransformation()->getTranslate();
            tempOrientation = animationStatus->object->getTransformation()->getOrientation();

            animationStatus->object->getTransformation()->removeParentTransform();
            animationStatus->object->getTransformation()->setTranslate(tempTranslate);
            animationStatus->object->getTransformation()->setScale(tempScale);
            animationStatus->object->getTransformation()->setOrientation(tempOrientation);
            animationStatus->object->setCustomAnimation(false);

            options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_DEBUG, "Animation " + animationCustom->getName() + " finished, removing. ");
            delete animIt->second;
            animIt = activeAnimations.erase(animIt);

        }
    }
}

void World::resetVisibilityBufferForRenderPipelineChange() {
    for (auto &item: this->cullingResults) {
        item.second->clear();
    }
}

void World::resetCameraTagsFromPipeline(const std::map<std::string, std::vector<std::set<std::string>>> & cameraRenderTagListMap) {
    for (auto& cameraEntryForCulling:this->cullingResults) { //key is the camera
        for (const auto& renderTagListMapFromPipelineForCamera : cameraRenderTagListMap) {
            if(cameraEntryForCulling.first->hasTag(HashUtil::hashString(renderTagListMapFromPipelineForCamera.first))) {
                //we have a camera and a renderStage match, update the tag information.
                // in renderTagListMapFromPipelineForCamera we have a list, in the list each element is a set of tags. we want to convert them and create new entries based on that
                cameraEntryForCulling.second->clear();
                for(std::set<std::string> tagSet:renderTagListMapFromPipelineForCamera.second) {
                    std::vector<uint64_t> tempHashList;
                    for(std::string tagString: tagSet) {
                        uint64_t tempHash = HashUtil::hashString(tagString);
                        tempHashList.emplace_back(tempHash);
                    }
                    //One set is done, put it in the culling data structure
                    cameraEntryForCulling.second->insert(std::make_pair(tempHashList, std::unordered_map<uint32_t,std::pair<std::vector<uint32_t>, uint32_t>>()));
                }
            }
        }
    }
}

void* fillVisibleObjectPerCamera(const void* visibilityRequestRaw) {
       const VisibilityRequest* visibilityRequest = static_cast<const VisibilityRequest *>(visibilityRequestRaw);
       std::vector<long> lodDistances = visibilityRequest->lodDistancesOption.get();
       float skipRenderDistance = 0, skipRenderSize = 0, maxSkipRenderSize = 0;
       glm::mat4 viewMatrix;
       if(visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE ||
               visibilityRequest->camera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
           skipRenderDistance = visibilityRequest->skipRenderDistanceOption.get();
           skipRenderSize = visibilityRequest->skipRenderSizeOption.get();
           maxSkipRenderSize = visibilityRequest->maxSkipRenderSizeOption.get();
           viewMatrix = visibilityRequest->camera->getProjectionMatrix() * visibilityRequest->camera->getCameraMatrixConst();
       }
       for (auto objectIt = visibilityRequest->objects->begin(); objectIt != visibilityRequest->objects->end(); ++objectIt) {
           if(!visibilityRequest->camera->isDirty() && !objectIt->second->isDirtyForFrustum()) {
               continue; //if neither object nor camera dirty, no need to recalculate
           }
           Model *currentModel = dynamic_cast<Model *>(objectIt->second);
           for(const auto& tag: currentModel->getTags()) {
               if(visibilityRequest->camera->hasRenderTag(tag.hash)){
                   //does this camera have the entry for this tag?
                   const auto& tagEntries = visibilityRequest->findHashEntry(tag.hash);
                   if(tagEntries == visibilityRequest->visibility->end()) {
                       /**
                        * Lets understand what this means
                        * 1) model had this tag
                        * 2) camera for this culling request also has this render tag
                        * 3) visibility request itself doesn't have this tag
                        *
                        * why would 3 happen? Because a tag that camera can render, doesn't mean we have a stage that render is meaningful
                        * Stages are already filled in the World::cullingResults, so this is a case where we cancel
                        */
                       continue;

                       //create the tag entry
                       /*********************************************************************************************
                        * *********************************************************************************************
                        * *********************************************************************************************
                        * *********************************************************************************************
                        * *********************************************************************************************
                        * *********************************************************************************************
                        *
                        */
                        std::vector<uint64_t> temp;
                        temp.push_back(tag.hash);
                       (*visibilityRequest->visibility).insert(std::make_pair(temp, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>()));
                   }
                   //we matched a tag for this camera, we should add here, and then break so we don't add to others
                   bool isVisible = visibilityRequest->camera->isVisible(*currentModel);//find if visible
                   auto tagVisibilityEntry = visibilityRequest->findHashEntry(tag.hash);//no need to check, as we already created if didn't exist
                   auto assetVisibilityEntry = tagVisibilityEntry->second.find(currentModel->getAssetID());
                   if(isVisible) {
                       if (assetVisibilityEntry == tagVisibilityEntry->second.end()) {
                           tagVisibilityEntry->second[currentModel->getAssetID()] = std::make_pair(std::vector<uint32_t>(), SKIP_LOD_LEVEL);
                       }

                       uint32_t lod = World::getLodLevel(lodDistances, skipRenderDistance, skipRenderSize, maxSkipRenderSize, viewMatrix, visibilityRequest->playerPosition, objectIt->second);
                       if(lod != SKIP_LOD_LEVEL) {
                           tagVisibilityEntry->second[currentModel->getAssetID()].second = std::min(tagVisibilityEntry->second[currentModel->getAssetID()].second, lod);
                           //check if this thing is already in the list of things to render
                           auto objectIndexIterator = std::find(tagVisibilityEntry->second[currentModel->getAssetID()].first.begin(),
                                                                tagVisibilityEntry->second[currentModel->getAssetID()].first.end(), currentModel->getWorldObjectID());
                           if (objectIndexIterator == tagVisibilityEntry->second[currentModel->getAssetID()].first.end()) {
                               tagVisibilityEntry->second[currentModel->getAssetID()].first.emplace_back(currentModel->getWorldObjectID());
                           }
                       }
                   } else { //not visible
                       if(assetVisibilityEntry == tagVisibilityEntry->second.end()) {
                           //it was never in the visible set, ignore.
                       } else {
                           //this asset was in visible set, but was this game object in the visible set?
                           for (auto modelIdIterator = assetVisibilityEntry->second.first.begin(); modelIdIterator != assetVisibilityEntry->second.first.end(); modelIdIterator++) {
                               if((*modelIdIterator) == currentModel->getWorldObjectID()) {
                                   assetVisibilityEntry->second.first.erase(modelIdIterator);
                                   //so we removed the element. should we drop the entry itself?
                                   if(assetVisibilityEntry->second.first.empty()) {
                                       tagVisibilityEntry->second.erase(assetVisibilityEntry);
                                   }
                                   break;
                               }
                           }
                       }
                   }
               }
           }
       }
       return visibilityRequest->visibility;
   }

static int staticOcclusionThread(void* visibilityRequestRaw) {
    VisibilityRequest* visibilityRequest = static_cast<VisibilityRequest *>(visibilityRequestRaw);
    //std::cout << "Thread for  " << visibilityRequest->camera->getName() << " launched, waiting for condition" << std::endl;
    //std::cout << "Thread for  " << visibilityRequest->camera->getName() << " started" << std::endl;
    while(visibilityRequest->running) {
        visibilityRequest->frameCount.fetch_add(1);
        fillVisibleObjectPerCamera(visibilityRequestRaw);
        visibilityRequest->inProgressLock.unlock();
        //std::cout << "Processing done for camera " << visibilityRequest->camera->getName() << " now waiting for condition" << std::endl;
        VisibilityRequest::condition.waitCondition(visibilityRequest->blockMutex);
        visibilityRequest->inProgressLock.lock();
        //std::cout << "signal received by " << visibilityRequest->camera->getName() << " starting processing again" << std::endl;
    }
    visibilityRequest->inProgressLock.unlock();
    return 0;
}

std::map<VisibilityRequest*, SDL_Thread *> World::occlusionThreadManager() {
    std::map<VisibilityRequest*, SDL_Thread*> visibilityProcessing;
    for (auto &cameraVisibility: cullingResults) {
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &this->objects, cameraVisibility.second, currentPlayer->getPosition(), options);
        SDL_Thread* thread = SDL_CreateThread(staticOcclusionThread, request->camera->getName().c_str(), request);
        visibilityProcessing[request] = thread;
    }
    return visibilityProcessing;
}

void World::fillVisibleObjectsUsingTags() {
     //first clear up dirty cameras
    for (auto &it: cullingResults) {
        if (it.first->isDirty()) {
            for(auto renderEntries:*it.second) {
                renderEntries.second.clear();
            }
        }
    }
    if(multiThreadedCulling) {
        if (visibilityThreadPool.empty()) {
            visibilityThreadPool = occlusionThreadManager();
            for (const auto &item: visibilityThreadPool) {
                item.first->inProgressLock.lock();
                item.first->inProgressLock.unlock();
            }
        }

        //std::cout << "          new frame, trigger occlusion threads" << std::endl;
        uint32_t lastFrameCount = visibilityThreadPool.begin()->first->frameCount.load();
        VisibilityRequest::condition.signalWaiting();

        for (const auto &item: visibilityThreadPool) {
            while (item.first->frameCount == lastFrameCount) {
                //busy wait until frame starts
            }
            item.first->inProgressLock.lock();
            item.first->playerPosition = currentPlayer->getPosition();
            item.first->inProgressLock.unlock();
        }
    } else {
        if(visibilityThreadPool.empty()) {
            for (auto &cameraVisibility: cullingResults) {
                VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &this->objects, cameraVisibility.second, currentPlayer->getPosition(), options);
                visibilityThreadPool[request] = nullptr;
            }
        }
        for (const auto &item: visibilityThreadPool) {
            item.first->playerPosition = currentPlayer->getPosition();
            fillVisibleObjectPerCamera(item.first);
        }
    }
    for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
        //all cameras calculated, clear dirty for object
        objectIt->second->setCleanForFrustum();
    }
    for (auto &it: cullingResults) {
        if (it.first->isDirty()) {
            it.first->clearDirty();//clear after processing so we can check while processing
        }
    }
}

ActorInterface::ActorInformation World::fillActorInformation(ActorInterface *actor) {
    ActorInterface::ActorInformation information;
    Model* actorModel = dynamic_cast<Model*>(objects[actor->getModelID()]);
    if(actorModel != nullptr) {
        information.canSeePlayerDirectly = checkPlayerVisibility(
                actor->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight+1.0f, 0),
                actorModel->getName());
        if (currentPlayer->isDead()) {
            information.playerDead = true;
        }
        glm::vec3 front = actor->getFrontVector();
        glm::vec3 rayDir = currentPlayer->getPosition() - actor->getPosition();
        float cosBetween = glm::dot(normalize(front), normalize(rayDir));
        information.playerDistance = glm::length(rayDir);
        information.cosineBetweenPlayer = cosBetween;
        information.playerDirection = normalize(rayDir);
        if (cosBetween > 0) {
            information.isPlayerFront = true;
            information.isPlayerBack = false;
        } else {
            information.isPlayerFront = false;
            information.isPlayerBack = true;
        }

        //now we know if it is front or back. we can check up, down, left, right
        //remove the y component, and test for left, right
        glm::vec3 rayDirWithoutY = rayDir;
        rayDirWithoutY.y = 0;
        glm::vec3 frontWithoutY = front;
        frontWithoutY.y = 0;
        glm::vec3 crossBetween = cross(normalize(frontWithoutY), normalize(rayDirWithoutY));
        float cosineForSide = glm::dot(normalize(frontWithoutY), normalize(rayDirWithoutY));
        information.cosineBetweenPlayerForSide = cosineForSide;
        if (crossBetween.y > 0) {
            information.isPlayerRight = false;
            information.isPlayerLeft = true;
        } else {
            information.isPlayerRight = true;
            information.isPlayerLeft = false;
        }
        //now we need up and down. For that, normally we can remove z or x, but since camera is z alone at start, I will use x
        rayDir.x = 0;
        front.x = 0;
        crossBetween = glm::cross(normalize(front), normalize(rayDir));
        if (crossBetween.x > 0) {
            information.isPlayerUp = false;
            information.isPlayerDown = true;
        } else {
            information.isPlayerUp = true;
            information.isPlayerDown = false;
        }
        ActorInterface::InformationRequest requests = actor->getRequests();
        if (requests.routeToPlayer == true && routeThreads.find(actor->getWorldID()) == routeThreads.end()) {//if no thread is working for route to player
            std::vector<LimonTypes::GenericParameter> parameters;
            parameters.push_back(LimonTypes::GenericParameter());

            parameters[0].value.vectorValue = GLMConverter::GLMToLimon(
                    actor->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0));
            parameters.push_back(LimonTypes::GenericParameter());
            parameters[1].value.longValues[0] = 2;
            parameters[1].value.longValues[1] = actor->getWorldID();
            parameters[1].value.longValues[2] = information.maximumRouteDistance;

            std::function<std::vector<LimonTypes::GenericParameter>(
                    std::vector<LimonTypes::GenericParameter>)> functionToRun =
                    std::bind(&World::fillRouteInformation, this, std::placeholders::_1);
            routeThreads[actor->getWorldID()] = new SDL2MultiThreading::Thread("FillRouteForActor", functionToRun, parameters);
            routeThreads[actor->getWorldID()]->run();
        }

        if (routeThreads.find(actor->getWorldID()) != routeThreads.end() && routeThreads[actor->getWorldID()]->isThreadDone()) {
            const std::vector<LimonTypes::GenericParameter>* route = routeThreads[actor->getWorldID()]->getResult();
            for (size_t i = 0; i < route->size(); ++i) {
                information.routeToRequest.push_back(glm::vec3(GLMConverter::LimonToGLM(route->at(i).value.vectorValue)));
            }
            delete routeThreads[actor->getWorldID()];
            routeThreads.erase(actor->getWorldID());

            if (information.routeToRequest.empty()) {
                information.routeFound = false;
            } else {
                information.routeFound = true;
            }
            information.routeReady = true;
        }
    }
    return information;
}


   //std::function<std::vector<LimonTypes::ParameterRequest>(std::vector<LimonTypes::ParameterRequest>)> functionToRun

std::vector<LimonTypes::GenericParameter>
World::fillRouteInformation(std::vector<LimonTypes::GenericParameter> parameters) const {
    glm::vec3 fromPosition = glm::vec3(GLMConverter::LimonToGLM(parameters[0].value.vectorValue));
    uint32_t actorID = (uint32_t)parameters[1].value.longValues[1];
    uint32_t maximumDistance = (uint32_t)parameters[1].value.longValues[2];
    std::vector<glm::vec3> route;
    glm::vec3 playerPosWithGrid = currentPlayer->getPosition();
    bool isPlayerReachable = grid->setProperHeight(&playerPosWithGrid, AIMovementGrid::floatingHeight, 0.0f,
                                                      dynamicsWorld);
    if (isPlayerReachable && grid->coursePath(fromPosition, playerPosWithGrid, actorID, maximumDistance, &route)) {
        if (!route.empty()) {             
            //Normally, this information should be used for straightening the path, but not yet.             
            reverse(std::begin(route), std::end(route));
            }
       }
    std::vector<LimonTypes::GenericParameter> routeList;
    for (size_t i = 0; i < route.size(); ++i) {
        LimonTypes::GenericParameter pr;
        pr.value.vectorValue = GLMConverter::GLMToLimon(route[i]);
        routeList.push_back(pr);
    }
    return routeList;
}

   bool World::handlePlayerInput(InputHandler &inputHandler) {
    if(inputHandler.getInputStates().getInputEvents(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
        if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
            GameObject *gameObject = getPointedObject(COLLIDE_EVERYTHING, ~(COLLIDE_NOTHING));
            if (gameObject != nullptr) {//FIXME this looks like a left over
                if(pickedObject != nullptr ) {
                    pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                }
                pickedObject = gameObject;
                pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
            } else {
                pickedObject = nullptr;
            }
        }
    }

    if(inputHandler.getInputStates().getInputEvents(InputStates::Inputs::F4)) {
        /*
        bool debugDrawLines;
        options->getOptionOrDefault("DebugDrawLines", debugDrawLines, false);
        if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::F4)) {
            debugDrawLines = !debugDrawLines;
        }
        options->setOption("DebugDrawLines", debugDrawLines);
         */
        long cascadeCount;
        OptionsUtil::Options::Option<long> cascadeCountOption = options->getOption<long>(HASH("CascadeCount"));
        cascadeCount = cascadeCountOption.getOrDefault(4);
        if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::F4)) {
            cascadeCount--;
            if(cascadeCount < 0) {
                cascadeCount = cascadeCountOption.getOrDefault(4);
            }
        }
    }

    if (inputHandler.getInputStates().getInputEvents(InputStates::Inputs::EDITOR) && inputHandler.getInputStates().getInputStatus(InputStates::Inputs::EDITOR)) {
        if(editorPlayer == nullptr) {
            editorPlayer = new FreeCursorPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
            editorPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                                  COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                                  COLLIDE_MODELS | COLLIDE_EVERYTHING,
                                                  worldAABBMin, worldAABBMax);

        }
        if(!currentPlayersSettings->editorShown) {
            switchPlayer(editorPlayer, inputHandler);
        } else {
            switchPlayer(beforePlayer, inputHandler);
        }
    }
    //if not in editor mode and press debug
    if (!currentPlayersSettings->editorShown && inputHandler.getInputStates().getInputEvents(InputStates::Inputs::DEBUG) && inputHandler.getInputStates().getInputStatus(InputStates::Inputs::DEBUG)) {
        if(currentPlayersSettings->debugMode != Player::DEBUG_ENABLED) {
            if(debugPlayer == nullptr) {
                debugPlayer = new FreeMovingPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
                debugPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                                     COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                                     COLLIDE_MODELS | COLLIDE_EVERYTHING,
                                                     worldAABBMin, worldAABBMax);

            }
            switchPlayer(debugPlayer, inputHandler);
        } else {
            if(physicalPlayer == nullptr) {
                physicalPlayer = new PhysicalPlayer(1, options, cursor, startingPlayer.position, startingPlayer.orientation, startingPlayer.attachedModel);
                physicalPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                                        COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                                        COLLIDE_MODELS | COLLIDE_EVERYTHING,
                                                        worldAABBMin, worldAABBMax);

            }
            switchPlayer(physicalPlayer, inputHandler);
        }
    }

       currentPlayer->processInput(inputHandler.getInputStates(), gameTime);

    if(inputHandler.getInputStates().getInputEvents(InputStates::Inputs::QUIT) &&  inputHandler.getInputStates().getInputStatus(InputStates::Inputs::QUIT)) {
        return true;
    } else {
        return false;
    }
}

void World:: render() {
    renderPipeline->render();
}

void World::renderGUIImages(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
    cursor->renderWithProgram(renderProgram, 0);

    for (auto it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->renderImageWithProgram(renderProgram);
    }
    //render API gui layer
    apiGUILayer->renderImageWithProgram(renderProgram);

}

void World::renderGUITexts(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
    for (auto it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->renderTextWithProgram(renderProgram);
    }
    //render API gui layer
    apiGUILayer->renderTextWithProgram(renderProgram);

    uint32_t triangle, line;
    graphicsWrapper->getRenderTriangleAndLineCount(triangle, line);
    renderCounts->updateText("Tris: " + std::to_string(triangle) + ", lines: " + std::to_string(line));
    bool renderInformations;
    renderInformations = renderInformationsOption.getOrDefault(false);
    if (renderInformations) {
        renderCounts->renderWithProgram(renderProgram, 0);
        debugOutputGUI->renderWithProgram(renderProgram, 0);
        fpsCounter->renderWithProgram(renderProgram, 0);
    }
}

void World::renderParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
     for(const auto& emitter:emitters) {
         emitter.second->renderWithProgram(renderProgram, 0);
     }
}

void World::renderGPUParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   for(const auto& gpuParticleEmitter:gpuParticleEmitters) {
       gpuParticleEmitter.second->renderWithProgram(renderProgram, 0);
   }
}

void World::renderDebug(const std::shared_ptr<GraphicsProgram>& renderProgram [[gnu::unused]], const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   dynamicsWorld->debugDrawWorld();
    for (const auto &drawLinePair: options->getLogger()->getDrawLines()) {
        if(!drawLinePair.second.empty()) {
            for (const auto &line: drawLinePair.second) {
                debugDrawer->drawLine(GLMConverter::GLMToBlt(line.from), GLMConverter::GLMToBlt(line.to), GLMConverter::GLMToBlt(line.fromColor), GLMConverter::GLMToBlt(line.toColor));
            }
        }
    }
   if (dynamicsWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
       debugDrawer->drawLine(btVector3(0, 0, 0), btVector3(0, 250, 0), btVector3(1, 1, 1));
       //draw the ai-grid
       if (grid != nullptr) {
           grid->debugDraw(debugDrawer);
       }
   }
   debugDrawer->flushDraws();
}

void World::renderCameraByTag(const std::shared_ptr<GraphicsProgram> &renderProgram, const std::string &cameraName, const std::vector<HashUtil::HashedString> &tags) const {
   uint64_t hashedCameraTag = HashUtil::hashString(cameraName);
   tempRenderedObjectsSet.clear();

    for (const auto &visibilityEntry: cullingResults) {
        if (visibilityEntry.first->hasTag(hashedCameraTag)) { //if this camera doesn't match the tag, then just ignore
            std::vector<uint64_t> alreadyRenderedTagHashes;
            for (const auto &renderTag: tags) {
                if(std::find(alreadyRenderedTagHashes.begin(), alreadyRenderedTagHashes.end(), renderTag.hash) != alreadyRenderedTagHashes.end()) {
                    continue;
                }
                const auto& taggedEntries = VisibilityRequest::findHashEntry(visibilityEntry.second,renderTag.hash);
                if(taggedEntries != visibilityEntry.second->end()) {
                    //We found what we will render, all these entries will render, so all the tags should be considered rendered
                    for (const auto &item: taggedEntries->first){
                        alreadyRenderedTagHashes.emplace_back(item);
                    }
                    //there are tagged entries, we should iterate and render
                    for (const auto &assetVisibility: taggedEntries->second){
                        //we don't care about the asset part, but knowing they are all same asset means instanced rendering
                        if(!assetVisibility.second.first.empty()) {
                            const auto& perAssetElement = assetVisibility.second;
                            //if not empty, then lets find a sample
                            uint32_t modelId = *perAssetElement.first.begin();
                            Model *sampleModel = dynamic_cast<Model *>(objects.at(modelId));
                            if (sampleModel == nullptr) {
                                std::cerr << "Sample model detection got a non model object for id " << modelId << " this should not have happened" << std::endl;
                                continue;
                            }
                            sampleModel->renderWithProgramInstanced(perAssetElement.first, *(renderProgram), perAssetElement.second);
                        }
                    }
                }
            }
            //now recursively render the player attachments, no visibility check.
            if (!currentPlayer->isDead() && startingPlayer.attachedModel != nullptr) {//don't render attached model if dead
                std::vector<uint32_t> alreadyRenderedModelIds;
                for (const auto &renderTag: tags) {
                    renderPlayerAttachmentsRecursiveByTag(startingPlayer.attachedModel, renderTag.hash,
                                                          renderProgram, alreadyRenderedModelIds);//Starting player, because we don't wanna render when in editor mode
                }
            }
        }
    }
}

void World::renderPlayerAttachmentsRecursiveByTag(PhysicalRenderable *attachment, uint64_t renderTag, const std::shared_ptr<GraphicsProgram> &renderProgram,
                                                  std::vector<uint32_t> &alreadyRenderedModelIds) const{
    if(attachment == nullptr) {
        return;
    }
    GameObject* attachmentObject = dynamic_cast<GameObject*>(attachment);
    if(attachmentObject == nullptr) {
        //FIXME there is no logical explanation for something to be a PhysicalRenderable and not a game object
        // the object is not a game object. We should render and return, as no tag checks possible
        attachment->renderWithProgram(renderProgram, 0);
        return;
    }
    std::vector<PhysicalRenderable *> children;
    if(attachmentObject->getTypeID() == GameObject::MODEL) {
        children = (static_cast<Model*>(attachment))->getChildren();
    } else if(attachmentObject->getTypeID() == GameObject::MODEL_GROUP) {
        //the group has the tag, everything under should be rendered.
        children = (static_cast<ModelGroup*>(attachment))->getChildren();
    }
    if(std::find(alreadyRenderedModelIds.begin(), alreadyRenderedModelIds.end(), attachmentObject->getWorldObjectID()) == alreadyRenderedModelIds.end() && attachmentObject->hasTag(renderTag)) {
        alreadyRenderedModelIds.emplace_back(attachmentObject->getWorldObjectID());
        std::vector<uint32_t> temp;
        temp.push_back(attachmentObject->getWorldObjectID());
        if(attachmentObject->getTypeID() == GameObject::MODEL) {
            (static_cast<Model *>(attachment))->renderWithProgramInstanced(temp, *(renderProgram), 0);//it is guaranteed to be very close to the player.
        }
    }
    for (const auto &child: children) {
        renderPlayerAttachmentsRecursiveByTag(child, renderTag, renderProgram, alreadyRenderedModelIds);
    }
 }

void World::renderSky(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   if (sky != nullptr) {
       sky->renderWithProgram(renderProgram, 0);
   }
}

void World::renderLight(unsigned int lightIndex, unsigned int renderLayer, const std::shared_ptr<GraphicsProgram> &renderProgram) const {
   renderProgram->setUniform("renderLightIndex", (int) lightIndex);
    renderProgram->setUniform("renderLightLayer", (int) renderLayer);
   Light* selectedLight = lights[lightIndex];
        Camera* lightCamera = selectedLight->getCameras()[renderLayer];

    const auto &selectedVisibilities = cullingResults.find(lightCamera);
    if (selectedVisibilities != cullingResults.end()) {
        std::set<uint64_t> alreadyRenderedTagHashes;

        for (const auto &renderTag: lightCamera->getRenderTags()) {
            if (alreadyRenderedTagHashes.find(renderTag.hash) != alreadyRenderedTagHashes.end()) {
                continue;
            }
            const auto &taggedVisibilities = VisibilityRequest::findHashEntry(selectedVisibilities->second, renderTag.hash);
            if (taggedVisibilities != selectedVisibilities->second->end()) {
                for (const auto &item: taggedVisibilities->first) {
                    alreadyRenderedTagHashes.insert(item);
                }
                //so all objects that needs rendering is here, now render
                for (const auto &assetIt: taggedVisibilities->second) {
                    const auto &perAssetElement = assetIt.second;
                    if (!perAssetElement.first.empty()) {
                        uint32_t modelId = *perAssetElement.first.begin();
                        Model *sampleModel = dynamic_cast<Model *>(objects.at(modelId));
                        if (sampleModel == nullptr) {
                            std::cerr << "Sample model detection got a non model object for id " << modelId << " this should not have happened" << std::endl;
                            continue;
                        }
                        sampleModel->renderWithProgramInstanced(perAssetElement.first, *(renderProgram), perAssetElement.second);
                    }
                }
            }
        }
    }
}

/**
 * This method checks if we are in editor mode, and if we are, enables ImGui windows
 * It also fills the windows with relevant parameters.
 */
void World::ImGuiFrameSetup(std::shared_ptr<GraphicsProgram> graphicsProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) {//TODO not const because it removes the object. Should be separated
   if(!currentPlayersSettings->editorShown) {
       return;
   }
   delete request;
   request = new ImGuiRequest(playerCamera->getCameraMatrix(), playerCamera->getProjectionMatrix(),
                              graphicsWrapper->getGUIOrthogonalProjectionMatrix(), options->getScreenHeight(), options->getScreenWidth(), playerCamera, apiInstance);

   //Render Trigger volumes
   for (auto it = triggers.begin(); it != triggers.end(); ++it) {
       it->second->render(debugDrawer);
   }
   //Render player place holder
   if (physicalPlayer != nullptr) {
       if (playerPlaceHolder == nullptr) {
           std::string assetFile;
           glm::vec3 scale;
           physicalPlayer->getRenderProperties(assetFile, scale);
           playerPlaceHolder = new Model(getNextObjectID(), assetManager, 0, assetFile, true);
           playerPlaceHolder->getTransformation()->setScale(scale);
       }

       startingPlayer.orientation = physicalPlayer->getLookDirection();
       startingPlayer.position = physicalPlayer->getPosition();

       playerPlaceHolder->getTransformation()->setTranslate(physicalPlayer->getPosition());
       playerPlaceHolder->getTransformation()->setOrientation(physicalPlayer->getLookDirectionQuaternion());
       std::vector<uint32_t> temp;
       temp.push_back(playerPlaceHolder->getWorldObjectID());
       playerPlaceHolder->renderWithProgramInstanced(temp, *(graphicsProgram.get()), 0);
   }
   editor->renderEditor();
}

void World::removeActiveCustomAnimation(const AnimationCustom &animationToRemove,
        const World::AnimationStatus *animationStatusToRemove,
        float animationTime) {
   if(animationStatusToRemove->sound) {
       animationStatusToRemove->sound->stop();
   }

   animationToRemove.calculateTransform("", animationTime, *animationStatusToRemove->object->getTransformation());

   if(!animationStatusToRemove->wasKinematic && animationStatusToRemove->wasPhysical) {
       PhysicalRenderable* tempPointer = dynamic_cast<PhysicalRenderable*>(animationStatusToRemove->object);//this can be static cast, since it is not possible to be anything else.
       tempPointer->getRigidBody()->setCollisionFlags(tempPointer->getRigidBody()->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
       tempPointer->getRigidBody()->setActivationState(ACTIVE_TAG);
   }

   //now before deleting the animation, separate parent/child animations
   glm::vec3 tempScale, tempTranslate;
   glm::quat tempOrientation;
   tempScale       = animationStatusToRemove->object->getTransformation()->getScale();
   tempTranslate   = animationStatusToRemove->object->getTransformation()->getTranslate();
   tempOrientation = animationStatusToRemove->object->getTransformation()->getOrientation();

   animationStatusToRemove->object->getTransformation()->removeParentTransform();
   animationStatusToRemove->object->getTransformation()->setTranslate(tempTranslate);
   animationStatusToRemove->object->getTransformation()->setScale(tempScale);
   animationStatusToRemove->object->getTransformation()->setOrientation(tempOrientation);
   animationStatusToRemove->object->setCustomAnimation(false);

   //now remove active animations
   Renderable* objectOfAnimation = animationStatusToRemove->object;
    delete activeAnimations[objectOfAnimation];
   activeAnimations.erase(objectOfAnimation);

   if(onLoadAnimations.find(objectOfAnimation) != onLoadAnimations.end()) {
       onLoadAnimations.erase(objectOfAnimation);
   }
}

   void World::addGUITextControls() {
    /**
     * we need these set:
     * 1) font
     * 2) font size
     * 3) name
     *
     */

    static int fontSize = 32;

    std::set<std::pair<std::string, uint32_t>> loadedFonts = fontManager.getLoadedFonts();
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
    if (guiLayers.size() == 0) {
        guiLayers.push_back(new GUILayer(graphicsWrapper, debugDrawer, 10));
    }
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < guiLayers.size(); ++i) {
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
        GUIText *guiText = new GUIText(graphicsWrapper, getNextObjectID(), GUITextName,
                                       fontManager.getFont(selectedFontName, fontSize), "New Text", glm::vec3(0, 0, 0));
        guiText->set2dWorldTransform(
                glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
        guiElements[guiText->getWorldObjectID()] = guiText;
        guiLayers[selectedLayerIndex]->addGuiElement(guiText);
        if(pickedObject != nullptr ) {
            pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
        }
        pickedObject = guiText;
        pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
    }
}

World::~World() {

    if(!routeThreads.empty()) {
        std::cout << "Waiting for AI route threads to finish. " << std::endl;
        for (auto threadIt = routeThreads.begin(); threadIt != routeThreads.end(); ++threadIt) {
            threadIt->second->waitUntilDone();
        }
        std::cout << "AI route threads to finished." << std::endl;
    }

    delete dynamicsWorld;
    delete animationInProgress;

    if(this->music != nullptr) {
        delete this->music;
    }

    //FIXME clear GUIlayer elements
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        delete (*it).second;
    }

    for (auto it = triggers.begin(); it != triggers.end(); ++it) {
        delete (*it).second;
    }

    delete sky;

    for (std::vector<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        for (Camera* camera : (*it)->getCameras()) {
            cullingResults.erase(camera);
        }
        delete (*it);
    }

    for (auto it = onLoadActions.begin(); it != onLoadActions.end(); ++it) {
        delete (*it)->action;
        delete (*it);
    }

    delete debugDrawer;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;
    delete ghostPairCallback;

    delete grid;
    delete playerCamera;
    delete physicalPlayer;
    delete debugPlayer;
    delete editorPlayer;
    delete menuPlayer;

    delete apiGUILayer;
    delete renderCounts;
    delete fpsCounter;
    delete cursor;
    delete debugOutputGUI;

    delete imgGuiHelper;

    for (auto &item: visibilityThreadPool) {
        delete item.first;
        if (item.second) {
            SDL_WaitThread(item.second, NULL);
        }
    }
}

bool World::addModelToWorld(Model *xmlModel) {
    if(objects.find(xmlModel->getWorldObjectID()) != objects.end()) {
        //the object is already registered. fail
        return false;
    }
    xmlModel->getTransformation()->getWorldTransform();
    objects[xmlModel->getWorldObjectID()] = xmlModel;
    rigidBodies.push_back(xmlModel->getRigidBody());
    xmlModel->updateAABB();
    if(xmlModel->isDisconnected()) {
        disconnectedModels.insert(xmlModel->getWorldObjectID());
        dynamicsWorld->removeRigidBody(xmlModel->getRigidBody());
    } else {
        if(xmlModel->isAnimated()) {
            dynamicsWorld->addRigidBody(xmlModel->getRigidBody(), COLLIDE_MODELS | COLLIDE_KINEMATIC_MODELS,
                                        COLLIDE_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
        } else {
            if(xmlModel->getRigidBody()->isStaticObject()) {
                dynamicsWorld->addRigidBody(xmlModel->getRigidBody(), COLLIDE_MODELS | COLLIDE_STATIC_MODELS,
                                            COLLIDE_DYNAMIC_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
            } else if(xmlModel->getRigidBody()->isKinematicObject()) {
                dynamicsWorld->addRigidBody(xmlModel->getRigidBody(), COLLIDE_MODELS | COLLIDE_KINEMATIC_MODELS,
                                            COLLIDE_DYNAMIC_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
            } else {
                dynamicsWorld->addRigidBody(xmlModel->getRigidBody(), COLLIDE_MODELS | COLLIDE_DYNAMIC_MODELS,
                                            COLLIDE_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
            }
        }
    }
    btVector3 aabbMin, aabbMax;
    xmlModel->getRigidBody()->getAabb(aabbMin, aabbMax);

    updateWorldAABB(GLMConverter::BltToGLM(aabbMin), GLMConverter::BltToGLM(aabbMax));
    updatedModels.push_back(xmlModel);
    return true;

}

bool World::addGUIElementToWorld(GUIRenderable *guiRenderable, GUILayer *guiLayer) {
    GameObject* object = dynamic_cast<GameObject*>(guiRenderable);
    if(object == nullptr) {
        return false;
    }

    if (guiElements.find(object->getWorldObjectID()) != guiElements.end()) {
        //the object is already registered. fail
        return false;
    }
    guiElements[object->getWorldObjectID()] = guiRenderable;
    guiLayer->addGuiElement(guiRenderable);
    return true;
}

void World::updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
    worldAABBMin = glm::vec3(std::min(aabbMin.x, worldAABBMin.x), std::min(aabbMin.y, worldAABBMin.y), std::min(aabbMin.z, worldAABBMin.z));
    worldAABBMax = glm::vec3(std::max(aabbMax.x, worldAABBMax.x), std::max(aabbMax.y, worldAABBMax.y), std::max(aabbMax.z, worldAABBMax.z));
}

void World::addActor(ActorInterface *actor) {
    this->actors[actor->getWorldID()] = actor;
}

void World::createGridFrom(const glm::vec3 &aiGridStartPoint) {
    if(grid != nullptr) {
        delete grid;
        grid = nullptr;
    }
#ifdef CEREAL_SUPPORT
    std::string AIWalkBinaryName = this->name.substr(0, this->name.find_last_of(".")) + ".aiwalkb";
    grid = AIMovementGrid::deserializeBinary(AIWalkBinaryName);
#endif
    if(grid == nullptr) {
        std::string AIWalkName = this->name.substr(0, this->name.find_last_of(".")) + ".aiwalk";
        grid = AIMovementGrid::deserialize(AIWalkName);
        if (grid == nullptr) {
            grid = new AIMovementGrid(aiGridStartPoint, dynamicsWorld, worldAABBMin, worldAABBMax, COLLIDE_PLAYER,
                                      COLLIDE_STATIC_MODELS | COLLIDE_EVERYTHING);
        }
    }
}

void World::setSky(SkyBox *skyBox) {
    if(sky!= nullptr) {
        delete sky;
    }
    sky = skyBox;
}

void World::addLight(Light *light) {
    this->lights.push_back(light);
    if(light->getLightType() == Light::LightTypes::DIRECTIONAL) {
        directionalLightIndex = (uint32_t)lights.size()-1;
    }
    const std::vector<Camera*>& cameras = light->getCameras();
    for(Camera* camera : cameras) {
        cullingResults.insert(std::make_pair(camera,
                                             new std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t, std::pair<std::vector<uint32_t>, uint32_t>>, VisibilityRequest::uint64_vector_hasher>()));
    }
    updateActiveLights(false);
}

uint32_t World::addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                              const std::string *soundToPlay) {
    AnimationStatus* as = new AnimationStatus;
    PhysicalRenderable* physicalPointer = nullptr;
    if(objects.find(modelID) != objects.end()) {
        as->object = objects[modelID];
        physicalPointer = objects[modelID];
        as->wasPhysical = true;
    } else if(guiElements.find(modelID) != guiElements.end()) {
        as->object = guiElements[modelID];
    } else {
        //object is not model or GUI, so object is not found. Return 0
        std::cerr << "add animation called for non existent object, skipping. " << std::endl;
        delete as;
        return 0;
    }
    if(loadedAnimations.size() <= animationID) {
        std::cerr << "add animation called for non existent animation, skipping. " << std::endl;
        delete as;
        return 0;
    }
    as->animationIndex = animationID;
    as->loop = looped;
    if(physicalPointer != nullptr) {
        as->wasKinematic = physicalPointer->getRigidBody()->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    as->startTime = gameTime;
    if(activeAnimations.count(as->object) != 0) {
        options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_WARN, "Model had custom animation, overriding.");
        as->wasKinematic = activeAnimations[as->object]->wasKinematic;
        if(activeAnimations[as->object]->loop) {
            as->originalTransformation = activeAnimations[as->object]->originalTransformation;//if looped animation, start new one from origin
        } else {
            //if not looped animation, start from end of the old one
            const AnimationCustom* oldAnimation = &loadedAnimations[activeAnimations[as->object]->animationIndex];
            float duration = oldAnimation->getDuration();
            oldAnimation->calculateTransform("", duration, *as->object->getTransformation());

            as->object->getTransformation()->getWorldTransform();//make sure it propagated as it should.
            //now before deleting the animation, separate parent/child animations
            glm::vec3 tempScale, tempTranslate;
            glm::quat tempOrientation;
            tempScale       = as->object->getTransformation()->getScale();
            tempTranslate   = as->object->getTransformation()->getTranslate();
            tempOrientation = as->object->getTransformation()->getOrientation();

            as->object->getTransformation()->removeParentTransform();
            as->object->getTransformation()->setTranslate(tempTranslate);
            as->object->getTransformation()->setScale(tempScale);
            as->object->getTransformation()->setOrientation(tempOrientation);
            as->object->setCustomAnimation(false);
            as->originalTransformation = *as->object->getTransformation();

        }
        delete activeAnimations[as->object];
    } else {
        as->originalTransformation = *as->object->getTransformation();
    }
    //we should animate child, and keep parent, so we should attach to the object itself
    as->object->getTransformation()->setScale(glm::vec3(1.0f,1.0f,1.0f));
    as->object->getTransformation()->setTranslate(glm::vec3(0.0f,0.0f,0.0f));
    as->object->getTransformation()->setOrientation(glm::quat(1.0f,0.0f,0.0f, 0.0f));

    //now set the parent/child
    as->object->getTransformation()->setParentTransform(&as->originalTransformation);
    as->object->setCustomAnimation(true);
    if(physicalPointer != nullptr) {
        physicalPointer->getRigidBody()->setCollisionFlags(physicalPointer->getRigidBody()->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        physicalPointer->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);
    }
    if(startOnLoad) {
        onLoadAnimations.insert(as->object);
        as->startTime = 0;
    } else {
        as->startTime = gameTime;
    }

    if(soundToPlay != nullptr) {
        as->sound = std::make_unique<Sound>(this->getNextObjectID(), assetManager, *soundToPlay);
        as->sound->setLoop(looped);
        as->sound->setWorldPosition(as->object->getTransformation()->getTranslate());
        as->sound->play();
    }
    activeAnimations[as->object] = as;
    return modelID;
}


bool
World::generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index) {
    bool isAllSet = true;
    std::set<std::string> passDescriptions;//multi select is build on first occurance, so pass other times
    for (size_t i = 0; i < runParameters.size(); ++i) {
        LimonTypes::GenericParameter& parameter = runParameters[i];
        if(passDescriptions.find(parameter.description) != passDescriptions.end()) {
            continue;
        }
        switch(parameter.requestType) {

            case LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT: {
                //we get a multi select, we should build the multiselect. first one is the selected element
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(), parameter.value.stringValue)) {
                    for (size_t j = i+1; j < runParameters.size(); ++j) {//passing i because it should be repeated in the list
                        if(runParameters[j].requestType == LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT && runParameters[j].description == runParameters[i].description) {
                            bool isThisElementSelected = (std::strcmp(runParameters[j].value.stringValue,parameter.value.stringValue) == 0);

                            if (ImGui::Selectable(runParameters[j].value.stringValue, isThisElementSelected)) {
                                strncpy(parameter.value.stringValue,runParameters[j].value.stringValue, sizeof(runParameters[j].value.stringValue));
                                parameter.isSet = true;
                            }
                            if(isThisElementSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            //now add this to pass list
            passDescriptions.insert(parameter.description);
            break;
            case LimonTypes::GenericParameter::RequestParameterTypes::MODEL: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
                std::string currentObject;
                if (parameter.isSet) {
                    if(objects.find((uint32_t) (parameter.value.longValue)) != objects.end()) {
                        currentObject = dynamic_cast<Model *>(objects[(uint32_t) (parameter.value.longValue)])->getName();
                    } else {
                        parameter.isSet = false;
                        currentObject = "Not selected";
                        isAllSet = false;
                    }
                } else {
                    currentObject = "Not selected";
                    isAllSet = false;
                }
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                      currentObject.c_str())) {
                    for (auto it = objects.begin(); it != objects.end(); it++) {
                        Model* currentModel = dynamic_cast<Model *>(it->second);
                        if(currentModel == nullptr) {
                            std::cerr << "Object cast to model failed" << std::endl;
                            continue;
                        }
                        bool isThisModelSelected =  (currentObject == currentModel->getName());

                        if (ImGui::Selectable(currentModel->getName().c_str(), isThisModelSelected)) {
                            parameter.value.longValue = static_cast<long>(currentModel->getWorldObjectID());
                            parameter.isSet = true;
                        }
                        if(isThisModelSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
                break;
            case LimonTypes::GenericParameter::RequestParameterTypes::ANIMATION: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
                std::string currentAnimation;
                if (parameter.isSet) {
                    currentAnimation = loadedAnimations[static_cast<uint32_t >(parameter.value.longValue)].getName();
                } else {
                    currentAnimation = "Not selected";
                    isAllSet = false;
                }
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                      currentAnimation.c_str())) {
                    for (uint32_t j = 0; j < loadedAnimations.size(); ++j) {

                        bool isThisAnimationSelected =  (currentAnimation == loadedAnimations[j].getName().c_str());

                        if (ImGui::Selectable(loadedAnimations[j].getName().c_str(), isThisAnimationSelected)) {
                            parameter.value.longValue = static_cast<long>(j);
                            parameter.isSet = true;
                        }

                        if(isThisAnimationSelected) {
                            ImGui::SetItemDefaultFocus();
                        }

                    }
                    ImGui::EndCombo();
                }
                break;
            }
            case LimonTypes::GenericParameter::RequestParameterTypes::GUI_TEXT: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
                std::string currentGUIText;
                if (parameter.isSet) {
                    if(guiElements.find(static_cast<uint32_t >(parameter.value.longValue)) != guiElements.end()) {
                        GameObject* guiGameObject = dynamic_cast<GameObject*>(guiElements[static_cast<uint32_t >(parameter.value.longValue)]);
                        if(guiGameObject != nullptr && guiGameObject->getTypeID() == GameObject::ObjectTypes::GUI_TEXT) {
                            currentGUIText = guiGameObject->getName();
                        }
                    } else {
                        parameter.isSet = false;
                        currentGUIText = "Not selected";
                        isAllSet = false;
                    }
                } else {
                    currentGUIText = "Not selected";
                    isAllSet = false;
                }
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                      currentGUIText.c_str())) {
                    for(auto it = guiElements.begin(); it != guiElements.end(); it++) {
                        bool isThisGUITextSelected = false;
                        GameObject* guiGameObject = dynamic_cast<GameObject*>(it->second);
                        if(guiGameObject != nullptr && guiGameObject->getTypeID() == GameObject::ObjectTypes::GUI_TEXT) {
                            isThisGUITextSelected = currentGUIText == guiGameObject->getName();
                            if (ImGui::Selectable(guiGameObject->getName().c_str(), isThisGUITextSelected)) {
                                parameter.value.longValue = static_cast<long>(it->first);
                                parameter.isSet = true;
                            }

                            if(isThisGUITextSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
                break;
            case LimonTypes::GenericParameter::RequestParameterTypes::SWITCH: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::BOOLEAN;
                bool isSelected;
                if (parameter.isSet) {
                    isSelected = parameter.value.boolValue;
                } else {
                    isSelected = false;
                    isAllSet = false;
                }
                if (ImGui::Checkbox((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                    &isSelected)) {
                    parameter.isSet = true;
                    parameter.value.boolValue = isSelected;
                };
            }
                break;
            case LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
                if (!parameter.isSet) {
                    isAllSet = false;
                }
                if (ImGui::InputText((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                     parameter.value.stringValue, sizeof(parameter.value.stringValue))) {
                    parameter.isSet = true;
                };
            }
                break;
            case LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER: {
                switch (parameter.valueType) {
                    case LimonTypes::GenericParameter::ValueTypes::DOUBLE: {
                        parameter.valueType = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
                        if (!parameter.isSet) {
                            isAllSet = false;
                        }
                        float value = parameter.value.doubleValue;
                        if (ImGui::DragFloat((parameter.description + "##triggerParam" + std::to_string(i) + "##" +
                                              std::to_string(index)).c_str(),
                                             &value, sizeof(parameter.value.doubleValue))) {
                            parameter.value.doubleValue = value;
                            parameter.isSet = true;
                        };
                    }
                    break;
                    case LimonTypes::GenericParameter::ValueTypes::LONG:
                    default: {
                        parameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
                        if (!parameter.isSet) {
                            isAllSet = false;
                        }
                        int value = parameter.value.longValue;
                        if (ImGui::DragInt((parameter.description + "##triggerParam" + std::to_string(i) + "##" +
                                            std::to_string(index)).c_str(),
                                           &value, sizeof(parameter.value.longValue))) {
                            parameter.value.longValue = value;
                            parameter.isSet = true;
                        };
                    }
                        break;
                }
            }
            break;
            case LimonTypes::GenericParameter::RequestParameterTypes::TRIGGER: {
                parameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG_ARRAY;
                parameter.value.longValues[0] = 3;//including self
                std::string currentObject;
                if (parameter.isSet) {
                    currentObject = dynamic_cast<TriggerObject*>(triggers[(uint32_t) (parameter.value.longValues[1])])->getName();
                } else {
                    currentObject = "Not selected";
                    isAllSet = false;
                }
                std::string label = parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index);
                if (ImGui::BeginCombo(label.c_str(), currentObject.c_str())) {
                    for (auto it = triggers.begin(); it != triggers.end(); it++) {

                        bool isThisTriggerSelected = currentObject == it->second->getName();

                        if (ImGui::Selectable((it->second)->getName().c_str(), isThisTriggerSelected)) {
                            parameter.value.longValues[1] = static_cast<long>((it->second)->getWorldObjectID());
                            parameter.isSet= true;
                        }

                        if(isThisTriggerSelected) {
                            ImGui::SetItemDefaultFocus();
                        }

                    }
                    ImGui::EndCombo();

                }
                //we need user to select which of the triggers are wanted
                int RadioButtonValue = parameter.value.longValues[2];
                ImGui::BeginGroup();
                ImGui::RadioButton(("First Enter##" + label).c_str(), &RadioButtonValue, 1);
                ImGui::RadioButton(("Enter##" + label).c_str(), &RadioButtonValue, 2);
                ImGui::RadioButton(("Exit##" + label).c_str(), &RadioButtonValue, 3);
                ImGui::EndGroup();
                parameter.value.longValues[2] = RadioButtonValue;
            }
            break;

            case LimonTypes::GenericParameter::RequestParameterTypes::COORDINATE:
            case LimonTypes::GenericParameter::RequestParameterTypes::TRANSFORM:
                std::cerr << "These parameter types are not handled!" << std::endl;

        }
    }
    return isAllSet;
}

uint32_t World::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &name, const std::string &text,
                           const glm::vec3 &color,
                           const glm::vec2 &position, float rotation) {
    GUIText* tr = new GUIText(graphicsWrapper, getNextObjectID(), name, fontManager.getFont(fontFilePath, fontSize),
                              text, color);
    glm::vec2 screenPosition;
    screenPosition.x = position.x * this->options->getScreenWidth();
    screenPosition.y = position.y * this->options->getScreenHeight();

    tr->set2dWorldTransform(screenPosition, rotation);
    guiElements[tr->getWorldObjectID()] = tr;
    apiGUILayer->addGuiElement(tr);
    return tr->getWorldObjectID();
}

uint32_t World::addGuiImageAPI(const std::string &imageFilePath, const std::string &name,
                            const LimonTypes::Vec2 &position, const LimonTypes::Vec2 &scale, float rotation) {
    GUIImage* guiImage = new GUIImage(getNextObjectID(), options, assetManager, name, imageFilePath);

    glm::vec2 screenPosition;
    screenPosition.x = position.x * this->options->getScreenWidth();
    screenPosition.y = position.y * this->options->getScreenHeight();

    glm::vec2 screenScale;
    screenScale.x = scale.x * this->options->getScreenWidth() /2;
    screenScale.y = scale.y * this->options->getScreenHeight() /2;

    guiImage->setScale(screenScale);
    guiImage->set2dWorldTransform(screenPosition, rotation);
    guiElements[guiImage->getWorldObjectID()] = guiImage;
    apiGUILayer->addGuiElement(guiImage);
    return guiImage->getWorldObjectID();
}

bool World::removeGuiElement(uint32_t guiElementID) {
    if(guiElements.find(guiElementID) != guiElements.end()) {
        GUIRenderable* temp = guiElements[guiElementID];
        if(hoveringButton != nullptr && hoveringButton->getWorldObjectID() == guiElementID ) {
            hoveringButton = nullptr;
        }
        //remove any active animations
        if(activeAnimations.find(temp) != activeAnimations.end()) {
            delete activeAnimations[temp];
            activeAnimations.erase(temp);
        }
        onLoadAnimations.erase(temp);

        guiElements.erase(guiElementID);
        delete temp;
        return true;
    }
    return false;
}

std::vector<LimonTypes::GenericParameter> World::getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID) {
    std::vector<LimonTypes::GenericParameter> result;
    if(triggers.find(triggerObjectID) != triggers.end()) {
        TriggerObject* to = triggers[triggerObjectID];
        result = to->getResultOfCode(triggerCodeID);
    }

    return result;
}

bool World::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    if(guiElements.find(guiTextID) != guiElements.end()) {
        dynamic_cast<GUITextBase*>(guiElements[guiTextID])->updateText(newText);
        return true;
    }
    return false;
}


bool World::removeTriggerObject(uint32_t triggerobjectID) {
    if(triggers.find(triggerobjectID) != triggers.end()) {
        TriggerObject* objectToRemove = triggers[triggerobjectID];
        dynamicsWorld->removeCollisionObject(objectToRemove->getGhostObject());
        //delete object itself
        delete triggers[triggerobjectID];
        triggers.erase(triggerobjectID);
        unusedIDs.push(triggerobjectID);
        return true;
    }
    return false;//not successful
}

bool World::removeObject(uint32_t objectID, const bool &removeChildren) {
    Model* modelToRemove = findModelByID(objectID);
    if(modelToRemove == nullptr) {
        return false;
    }
    dynamicsWorld->removeRigidBody(modelToRemove->getRigidBody());
    //disconnect AI

    if (modelToRemove!= nullptr && modelToRemove->getAIID() != 0) {
        unusedIDs.push(modelToRemove->getAIID());
        actors.erase(modelToRemove->getAIID());
    }
    //remove any active animations
    if(activeAnimations.find(modelToRemove) != activeAnimations.end()) {
        delete activeAnimations[modelToRemove];
        activeAnimations.erase(modelToRemove);
    }
    onLoadAnimations.erase(modelToRemove);

    bool removalDone = false;
    //of course we need to remove from the tag visibility lists too
    for (auto &perCameraVisibility: cullingResults) {
        for(auto perTagVisibilityIt = perCameraVisibility.second->begin(); perTagVisibilityIt != perCameraVisibility.second->end(); perTagVisibilityIt++) {
            auto assetSet = perTagVisibilityIt->second.find(modelToRemove->getAssetID());
            if (assetSet != perTagVisibilityIt->second.end()) {
                //found the asset, is the model in it?
                for(auto modelIdIterator = assetSet->second.first.begin(); modelIdIterator != assetSet->second.first.end(); modelIdIterator++) {
                    if((*modelIdIterator) == modelToRemove->getWorldObjectID()) {
                        //model in it, remove
                        assetSet->second.first.erase(modelIdIterator);
                        removalDone = true;
                        if(assetSet->second.first.empty()) {
                            //the model we removed was the only model, we should drop the whole asset
                            perTagVisibilityIt->second.erase(modelToRemove->getAssetID());
                            break;//we have been iterating in the asset set, which doesn't exist anymore
                        }
                    }
                }
                if(removalDone) {
                    break;
                }
            }
        }
    }

	//remove its children
    if(removeChildren)
    {
        std::vector<PhysicalRenderable*> children=objects[objectID]->getChildren();
        for (auto child = children.begin(); child != children.end(); ++child) {
            Model* model = dynamic_cast<Model*>(*child);
            if(model!= nullptr) {//FIXME this eliminates non model childs
                removeObject(model->getWorldObjectID());
            }
        }
    }
    
    //delete object itself
    delete modelToRemove;
    objects.erase(objectID);
    unusedIDs.push(objectID);


    return true;
}

void World::afterLoadFinished() {
    resetTagsAndRefillCulling();
    for (size_t i = 0; i < onLoadActions.size(); ++i) {
        if(onLoadActions[i]->action == nullptr) {
            std::cerr << "There was an onload action defined but action is not loaded, skipping." << std::endl;
            continue;
        }
        if(onLoadActions[i]->enabled) {
            std::cout << "running trigger " << onLoadActions[i]->action->getName() << std::endl;
            onLoadActions[i]->action->run(onLoadActions[i]->parameters);
        }
    }

    if(music != nullptr) {
        music->play();
    }

    //setup request
    request = new ImGuiRequest(playerCamera->getCameraMatrix(), playerCamera->getProjectionMatrix(),
                               graphicsWrapper->getGUIOrthogonalProjectionMatrix(), options->getScreenHeight(), options->getScreenWidth(), playerCamera, apiInstance);

    if(startingPlayer.extensionName != "") {
        PlayerExtensionInterface *playerExtension =PlayerExtensionInterface::createExtension(startingPlayer.extensionName, apiInstance);
        this->currentPlayer->setPlayerExtension(playerExtension);
        strncpy(extensionNameBuffer, startingPlayer.extensionName.c_str(), sizeof(extensionNameBuffer)-1);
    }
}

bool World::disconnectObjectFromPhysics(uint32_t objectWorldID) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    Model* model = dynamic_cast<Model*>(objects.at(objectWorldID));
    if(model == nullptr) {
        return false;//fail
    }

    model->disconnectFromPhysicsWorld(dynamicsWorld);
    return true;
}

bool World::reconnectObjectToPhysics(uint32_t objectWorldID) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    Model* model = dynamic_cast<Model*>(objects.at(objectWorldID));
    if(model == nullptr) {
        return false;//fail
    }
    model->connectToPhysicsWorld(dynamicsWorld, COLLIDE_MODELS, COLLIDE_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
    return true;
}

bool World::disconnectObjectFromPhysicsRequest(uint32_t objectWorldID) {
   if(objects.find(objectWorldID) == objects.end()) {
       return false;//fail
   }
   Model* model = dynamic_cast<Model*>(objects.at(objectWorldID));
   if(model == nullptr) {
       return false;//fail
   }
   disconnectedModels.insert(model->getWorldObjectID());
   return true;
}

bool World::reconnectObjectToPhysicsRequest(uint32_t objectWorldID) {
   if(objects.find(objectWorldID) == objects.end()) {
       return false;//fail
   }
   Model* model = dynamic_cast<Model*>(objects.at(objectWorldID));
   if(model == nullptr) {
       return false;//fail
   }
   disconnectedModels.erase(model->getWorldObjectID());
   return true;
}

bool World::attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    objects[objectWorldID]->setSoundAttachmentAndPlay(std::make_unique<Sound>(getNextObjectID(), assetManager, soundPath));
    return true;
}

bool World::detachSoundFromObject(uint32_t objectWorldID) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    objects[objectWorldID]->detachSound();
    return true;
}

uint32_t World::playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped) {
    std::unique_ptr<Sound> sound = std::make_unique<Sound>(getNextObjectID(), assetManager, soundPath);
    sound->setLoop(looped);
    sound->setWorldPosition(position, positionRelative);
    sound->play();
    uint32_t soundID = sound->getWorldObjectID();
    sounds[soundID] = std::move(sound);
    return soundID;
}

void World::addGUIImageControls() {
    /**
     * For a new GUI Image we need only name and filename
     */
    static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;

    static char textureAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureAssetTreeFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
    imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE,
                                      "GUIImage",
                                      &selectedAsset);

    static size_t selectedLayerIndex = 0;
    if (guiLayers.size() == 0) {
        guiLayers.push_back(new GUILayer(graphicsWrapper, debugDrawer, 10));
    }
    static char GUIImageName[32];
    ImGui::InputText("GUI Image Name", GUIImageName, sizeof(GUIImageName), ImGuiInputTextFlags_CharsNoBlank);
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < guiLayers.size(); ++i) {
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
            GUIImage *guiImage = new GUIImage(this->getNextObjectID(), options, assetManager, std::string(GUIImageName),
                                              selectedAsset->fullPath);
            guiImage->set2dWorldTransform(
                    glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
            guiElements[guiImage->getWorldObjectID()] = guiImage;
            guiLayers[selectedLayerIndex]->addGuiElement(guiImage);
            if(pickedObject != nullptr ) {
                pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
            }
            pickedObject = guiImage;
            pickedObject->addTag(HardCodedTags::PICKED_OBJECT);        }
    }
}

   void World::addParticleEmitterEditor() {
       /**
        * For a new GUI Image we need only name and filename
        */
       static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;

       static char textureAssetFilter[32] = {0};
       ImGui::InputText("Filter Assets ##TextureAssetTreeEmitterFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
       std::string textureAssetFilterStr = textureAssetFilter;
       std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
       const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
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
               std::shared_ptr<Emitter> newEmitter = std::make_shared<Emitter>(this->getNextObjectID(), particleEmitterName, this->assetManager, selectedAsset->fullPath,
                                                                               startPosition, glm::vec3(startSphereR, startSphereR, startSphereR), size, maxCount,
                                                                               lifeTime);
               this->emitters[newEmitter->getWorldObjectID()] = (newEmitter);
           }
       }
   }

void World::switchPlayer(Player *targetPlayer, InputHandler &inputHandler) {
    //we should reconnect disconnected object if switching to editor mode, because we use physics for pickup
    if(targetPlayer->getWorldSettings().editorShown && (currentPlayersSettings == nullptr ||!currentPlayersSettings->editorShown)) {
        //switching to editor shown. reconnect all disconnected objects, if no currentPlayer, starting with editor, reconnect
        for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
            reconnectObjectToPhysics(*objectIt);
        }
    } else if(!targetPlayer->getWorldSettings().editorShown && (currentPlayersSettings  == nullptr || currentPlayersSettings->editorShown)) {
        //if exiting editor mode, or starting not editor mode
        for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
            disconnectObjectFromPhysics(*objectIt);
        }
    }

    currentPlayersSettings = &(targetPlayer->getWorldSettings());

    setupForPlay(inputHandler);

    //now all settings done, switch player
    beforePlayer = currentPlayer;
    currentPlayer = targetPlayer;
    targetPlayer->ownControl(beforePlayer->getPosition(), beforePlayer->getLookDirection());

    dynamicsWorld->updateAabbs();
    playerCamera->setCameraAttachment(currentPlayer->getCameraAttachment());

}

void World::setupForPlay(InputHandler &inputHandler) {
    if(currentPlayersSettings->debugMode == Player::DEBUG_ENABLED) {
        dynamicsWorld->getDebugDrawer()->setDebugMode(
                dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE | dynamicsWorld->getDebugDrawer()->DBG_DrawAabb | dynamicsWorld->getDebugDrawer()->DBG_DrawConstraints | dynamicsWorld->getDebugDrawer()->DBG_DrawConstraintLimits);
        for (size_t i = 0; i < guiLayers.size(); ++i) {
            guiLayers[i]->setDebug(true);
        }
        options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_INFO, "Debug enabled");
    } else if(currentPlayersSettings->debugMode == Player::DEBUG_DISABLED) {
        dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
        for (size_t i = 0; i < guiLayers.size(); ++i) {
            guiLayers[i]->setDebug(false);
        }
    }
    if(currentPlayersSettings->audioPlaying) {
        alHelper->resumePlay();
        if(this->music != nullptr) {
            //on world change, the music is stopped. If we are returning, we should start it again
            if (this->music->getState() == Sound::State::PAUSED) {
                this->music->resume();
            }
        }
    } else {
        alHelper->pausePlay();
    }

    if(currentPlayersSettings->cursorFree) {
        inputHandler.setMouseModeFree();
        cursor->hide();
    } else {
        inputHandler.setMouseModeRelative();
        cursor->unhide();
    }

    if(currentPlayersSettings->resetAnimations) {
        //when switching to editor mode, return all objects that are custom animated without triggers
        //to original position
        for(auto it = onLoadAnimations.begin(); it != onLoadAnimations.end(); it++) {
            if(activeAnimations.find(*it) != activeAnimations.end()) {
                (*it)->getTransformation()->setTranslate(glm::vec3(0.0f, 0.0f, 0.0f));
                (*it)->getTransformation()->setScale(glm::vec3(1.0f, 1.0f, 1.0f));
                (*it)->getTransformation()->setOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            }
        }
    }

    if(currentPlayersSettings->menuInteraction) {
        guiPickMode = true;
    } else {
        guiPickMode = false;
    }
}

void World::setupForPauseOrStop() {
    if(this->music != nullptr) {
        this->music->pause();
    }
}

void World::addGUIButtonControls() {

    static char GUIButtonName[32];
    ImGui::InputText("GUI Button Name", GUIButtonName, sizeof(GUIButtonName), ImGuiInputTextFlags_CharsNoBlank);

    static const AssetManager::AvailableAssetsNode* selectedAssetForGUIButton = nullptr;

    static char textureAssetFilter[32] = {0};
    ImGui::InputText("Filter Assets ##TextureButtonAssetTreeFilter", textureAssetFilter, sizeof(textureAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
    std::string textureAssetFilterStr = textureAssetFilter;
    std::transform(textureAssetFilterStr.begin(), textureAssetFilterStr.end(), textureAssetFilterStr.begin(), ::tolower);
    const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);
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
    if (guiLayers.size() == 0) {
        guiLayers.push_back(new GUILayer(graphicsWrapper, debugDrawer, 10));
    }
    if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
        for (size_t i = 0; i < guiLayers.size(); ++i) {
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

            GUIButton *guiButton = new GUIButton(this->getNextObjectID(), assetManager, apiInstance,
                                                 name,
                                                 fileNames);
            guiButton->set2dWorldTransform(
                    glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
            guiElements[guiButton->getWorldObjectID()] = guiButton;
            guiLayers[selectedLayerIndex]->addGuiElement(guiButton);

            if(pickedObject != nullptr ) {
                pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
            }
            pickedObject = guiButton;
            pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
        }
    }
}

   void World::addGUIAnimationControls() {

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
       const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureAssetFilterStr);

       imgGuiHelper->buildTreeFromAssets(filteredAssets,
                                         AssetManager::AssetTypes::Asset_type_TEXTURE, "GUIAnimation",
                                         &selectedAsset);

       static size_t selectedLayerIndex = 0;
       if (guiLayers.size() == 0) {
           guiLayers.push_back(new GUILayer(graphicsWrapper, debugDrawer, 10));
       }
       if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
           for (size_t i = 0; i < guiLayers.size(); ++i) {
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

                GUIAnimation *guiAnimation = new GUIAnimation(this->getNextObjectID(), assetManager,
                                                             std::string(GUIAnimationName),
                                                             fileNames, gameTime, newAnimationFrameSpeed, isLooped);
                guiAnimation->set2dWorldTransform(glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
                guiElements[guiAnimation->getWorldObjectID()] = guiAnimation;
                guiLayers[selectedLayerIndex]->addGuiElement(guiAnimation);
                if(pickedObject != nullptr ) {
                    pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                }
                pickedObject = guiAnimation;
                pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
            }
        }
   }

void World::addGUILayerControls() {
    static  int32_t levelSlider = 0;
    ImGui::DragInt("Layer level", &levelSlider, 1, 1, 128);
    if (ImGui::Button("Add GUI Layer")) {
        this->guiLayers.push_back(new GUILayer(graphicsWrapper, debugDrawer, (uint32_t)levelSlider));
    }
}

bool World::handleQuitRequest() {
    switch(currentQuitResponse) {
        case QuitResponse::LOAD_WORLD:
            apiInstance->returnToWorld(quitWorldName);
            break;
        case QuitResponse::RETURN_PREVIOUS:
            apiInstance->returnPreviousWorld();
            break;
        case QuitResponse::QUIT_GAME:
            apiInstance->quitGame();

    }
    return true;
}

std::string World::getName() {
    return this->name;
}

uint32_t World::addModelApi(const std::string &modelFilePath, float modelWeight, bool physical,
                            const glm::vec3 &position,
                            const glm::vec3 &scale, const glm::quat &orientation) {
    uint32_t objectID = this->getNextObjectID();

    Model* newModel = new Model(objectID, assetManager, modelWeight, modelFilePath, !physical);//the physical is reversed because parameter here is "disconnected"
    newModel->getTransformation()->setTranslate(position);
    newModel->getTransformation()->setScale(scale);
    newModel->getTransformation()->setOrientation(orientation);

    this->addModelToWorld(newModel);

    return objectID;
}

bool World::applyForceAPI(uint32_t objectID, const LimonTypes::Vec4 &forcePosition, const LimonTypes::Vec4 &forceAmount) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getRigidBody()->activate(true);
    btVector3 forcePositionRelative = GLMConverter::LimonToBlt(forcePosition) - model->getRigidBody()->getCenterOfMassTransform().getOrigin();

    model->getRigidBody()->applyForce(GLMConverter::LimonToBlt(forceAmount), forcePositionRelative);
    return true;
}

bool World::applyForceToPlayerAPI(const LimonTypes::Vec4 &forceAmount) {
    if(physicalPlayer == nullptr) {
        return false;
    }
    physicalPlayer->getRigidBody()->activate(true);
    physicalPlayer->getRigidBody()->applyForce(GLMConverter::LimonToBlt(forceAmount), btVector3(0,0,0));

    return true;
}

bool World::setModelTemporaryAPI(uint32_t modelID, bool temporary) {
    Model* model = findModelByID(modelID);
    if(model == nullptr) {
        return false;
    } else {
        model->setTemporary(temporary);
        return true;
    }
}

std::vector<LimonTypes::GenericParameter> World::rayCastToCursorAPI() {
   /**
    * * If nothing is hit, returns empty vector
    * returns these values:
    * 1) objectID for what is under the cursor
    * 2) hit coordinates
    * 3) hit normal
    * 4) If object has AI, id of that AI
    */
   std::vector<LimonTypes::GenericParameter>result;
   glm::vec3 position, normal;
   GameObject* gameObject = this->getPointedObject(COLLIDE_EVERYTHING, COLLIDE_MODELS |COLLIDE_EVERYTHING, &position, &normal);

   if(gameObject == nullptr) {
       return result;
   }
   LimonTypes::GenericParameter objectIDParam;
   objectIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
   objectIDParam.value.longValue = gameObject->getWorldObjectID();
   result.push_back(objectIDParam);

   LimonTypes::GenericParameter positionParam;
   positionParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
   positionParam.value.vectorValue.x = position.x;
   positionParam.value.vectorValue.y = position.y;
   positionParam.value.vectorValue.z = position.z;
   result.push_back(positionParam);

   LimonTypes::GenericParameter normalParam;
   normalParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
   normalParam.value.vectorValue.x = normal.x;
   normalParam.value.vectorValue.y = normal.y;
   normalParam.value.vectorValue.z = normal.z;
   result.push_back(normalParam);

   if(gameObject->getTypeID() == GameObject::MODEL) {
       Model * foundModel = dynamic_cast<Model *>(gameObject);
       if (foundModel != nullptr && foundModel->getAIID() != 0) {
           LimonTypes::GenericParameter aiIDParam;
           aiIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
           aiIDParam.value.longValue = foundModel->getAIID();
           result.push_back(aiIDParam);
       }
   }

   return result;
}

std::vector<LimonTypes::GenericParameter> World::getObjectTransformationAPI(uint32_t objectID) const {
    std::vector<LimonTypes::GenericParameter> result;
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return result;
    }
    const Transformation* transformation = model->getTransformation();

    LimonTypes::GenericParameter translate;
    translate.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    translate.value.vectorValue = GLMConverter::GLMToLimon(transformation->getTranslate());
    result.push_back(translate);

    LimonTypes::GenericParameter scale;
    scale.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    scale.value.vectorValue = GLMConverter::GLMToLimon(transformation->getScale());
    result.push_back(scale);

    LimonTypes::GenericParameter orientation;
    orientation.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    orientation.value.vectorValue.x = transformation->getOrientation().x;
    orientation.value.vectorValue.y = transformation->getOrientation().y;
    orientation.value.vectorValue.z = transformation->getOrientation().z;
    orientation.value.vectorValue.w = transformation->getOrientation().w;
    result.push_back(orientation);

    return result;
}

std::vector<LimonTypes::GenericParameter> World::getObjectTransformationMatrixAPI(uint32_t objectID) const {
   std::vector<LimonTypes::GenericParameter> result;
   if(objects.find(objectID) == objects.end()) {
       return result;
   }

   LimonTypes::GenericParameter transform;
   transform.valueType = LimonTypes::GenericParameter::ValueTypes::MAT4;
   transform.value.matrixValue = GLMConverter::GLMToLimon(objects.at(objectID)->getTransformation()->getWorldTransform());

   result.push_back(transform);
   return result;
}

GameObject * World::getPointedObject(int collisionType, int filterMask,
                                     glm::vec3 *collisionPosition, glm::vec3 *collisionNormal) const {
    glm::vec3 from, lookDirection;
    currentPlayer->getWhereCameraLooks(from, lookDirection);

    if(guiPickMode) {
        GameObject* pickedGuiElement = nullptr;
        uint32_t pickedLevel = 0;
        //then we don't need to rayTest. We can get the picked object directly by coordinate.
        for (size_t i = 0; i < guiLayers.size(); ++i) {
            GameObject* pickedGuiTemp = dynamic_cast<GameObject*>(guiLayers[i]->getRenderableFromCoordinate(cursor->getTranslate()));
            if(pickedGuiTemp != nullptr && guiLayers[i]->getLevel() >= pickedLevel) {
                pickedGuiElement = pickedGuiTemp;//because we are iterating all the levels
            }
        }
        return pickedGuiElement;
    } else {
        //we want to extend to vector to world AABB limit
        float maxFactor = 1; //don't allow making ray smaller than unit by setting 1.

        if (lookDirection.x > 0) {
            //so we are looking at positive x. determine how many times the ray x we need
            maxFactor = std::max(maxFactor,(worldAABBMax.x - from.x) / lookDirection.x);
        } else {
            maxFactor = std::max(maxFactor,(worldAABBMin.x - from.x) /
                                           lookDirection.x); //Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
        }

        if (lookDirection.y > 0) {
            std::max(maxFactor, (worldAABBMax.y - from.y) / lookDirection.y);
        } else {
            std::max(maxFactor, (worldAABBMin.y - from.y) /
                                lookDirection.y);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
        }

        if (lookDirection.z > 0) {
            std::max(maxFactor, (worldAABBMax.z - from.z) / lookDirection.z);
        } else {
            std::max(maxFactor, (worldAABBMin.z - from.z) /
                                lookDirection.z);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
        }
        lookDirection = lookDirection * maxFactor;
        glm::vec3 to = lookDirection + from;
        btCollisionWorld::ClosestRayResultCallback RayCallback(GLMConverter::GLMToBlt(from),
                                                               GLMConverter::GLMToBlt(to));
        RayCallback.m_collisionFilterGroup = collisionType;
        RayCallback.m_collisionFilterMask = filterMask;

        dynamicsWorld->rayTest(
                GLMConverter::GLMToBlt(from),
                GLMConverter::GLMToBlt(to),
                RayCallback
        );

        //debugDrawer->flushDraws();
        if (RayCallback.hasHit()) {
            if(collisionPosition != nullptr) {
                *collisionPosition = GLMConverter::BltToGLM(RayCallback.m_hitPointWorld);
            }
            if(collisionNormal != nullptr) {
                *collisionNormal = GLMConverter::BltToGLM(RayCallback.m_hitNormalWorld);
            }
            return static_cast<GameObject *>(RayCallback.m_collisionObject->getUserPointer());
        } else {
            return nullptr;
        }
    }
}

bool World::interactWithAIAPI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation) const {
   if(actors.find(AIID) == actors.end()) {
       return false;
   }
   return actors.at(AIID)->interaction(interactionInformation);
}

void World::interactWithPlayerAPI(std::vector<LimonTypes::GenericParameter> &interactionInformation) const {
    if(this->physicalPlayer != nullptr) {
       this->physicalPlayer->interact(apiInstance, interactionInformation);
    }
}

void World::simulateInputAPI(InputStates input) {
    if(this->physicalPlayer != nullptr) {
        input.setSimulated(true);
        this->physicalPlayer->processInput(input, gameTime);
    }
}

long World::addTimedEventAPI(uint64_t waitTime, bool useWallTime, std::function<void(const std::vector<LimonTypes::GenericParameter> &)> methodToCall,
                             std::vector<LimonTypes::GenericParameter> parameters) {
    long handleId = timedEventHandleIndex++;
    uint64_t callTime = useWallTime ? this->wallTime : this->gameTime;
    callTime += waitTime;
    timedEvents.emplace(handleId, callTime, useWallTime, std::move(methodToCall), std::move(parameters));
    return handleId;
}

bool World::cancelTimedEventAPI(long handleId) {
    std::vector<TimedEvent>& container = Container(timedEvents);
    for (size_t i = 0; i < container.size(); ++i) {
        if(container[i].handleId == handleId) {
            container[i].active = false;
            return true;
        }
    }
    return false;
}


void World::checkAndRunTimedEvents() {
    //we need to check 2 different things
    while(!timedEvents.empty()) {
        bool nothingToRun = true;
        //if there are waiting timed events we need to check if they need to run
        if(timedEvents.top().useWallTime && timedEvents.top().callTime <= wallTime) {
            timedEvents.top().run();
            timedEvents.pop();
            nothingToRun = false;
        }
        if(!timedEvents.top().useWallTime && timedEvents.top().callTime <= gameTime){
            timedEvents.top().run();
            timedEvents.pop();
            nothingToRun = false;
        }
        if(nothingToRun) {
            break;
        }
    }
}

uint32_t World::getPlayerAttachedModelAPI() {
    if(this->startingPlayer.attachedModel != nullptr) {
        return this->startingPlayer.attachedModel->getWorldObjectID();
    }
    return 0;
}

std::vector<uint32_t> World::getModelChildrenAPI(uint32_t modelID) {
    std::vector<uint32_t> result;
    Model* model = findModelByID(modelID);
    if(model != nullptr) {
        std::vector<PhysicalRenderable*> children = model->getChildren();
        for (auto child = children.begin(); child != children.end(); ++child) {
            Model* model = dynamic_cast<Model*>(*child);
            if(model!= nullptr) {//FIXME this eliminates non model childs
                result.push_back(model->getWorldObjectID());
            }
        }
   }
   return result;
}

std::string World::getModelAnimationNameAPI(uint32_t modelID) {
    Model* model = findModelByID(modelID);
        if(model != nullptr) {
            return model->getAnimationName();
        }

    return "";
}

bool World::getModelAnimationFinishedAPI(uint32_t modelID) {
    Model* model = findModelByID(modelID);
    if(model != nullptr) {
        return model->isAnimationFinished();
    }

    return false;
}

bool World::setModelAnimationAPI(uint32_t modelID, const std::string& animationName, bool isLooped) {
    Model* model = findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimation(animationName, isLooped);
        return true;
    }
    return false;
}

bool World::setModelAnimationSpeedAPI(uint32_t modelID, float speed) {
    if(speed < 0.001f) {
        return false;
    }
    Model* model = findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimationTimeScale(speed);
        return true;
    }
    return false;
}

bool World::setModelAnimationWithBlendAPI(uint32_t modelID, const std::string& animationName, bool isLooped, long blendTime) {
    Model* model = findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimationWithBlend(animationName, isLooped, blendTime);
        return true;
    }
    return false;
}

LimonTypes::Vec4 World::getPlayerModelOffsetAPI() {
   if(this->startingPlayer.attachedModel != nullptr) {
       if(physicalPlayer != nullptr) {
           return GLMConverter::GLMToLimon(physicalPlayer->getAttachedModelOffset());
       }
   }
   return LimonTypes::Vec4(0,0,0);
}

bool World::setPlayerModelOffsetAPI(LimonTypes::Vec4 newOffset) {
    if(this->startingPlayer.attachedModel != nullptr) {
        if(physicalPlayer != nullptr) {
            physicalPlayer->setAttachedModelOffset(glm::vec3(GLMConverter::LimonToGLM(newOffset)));
            return true;
        }
    }
    return false;
}

void World::killPlayerAPI() {
    this->currentPlayer->setDead();
}

Model* World::findModelByIDChildren(PhysicalRenderable* parent,uint32_t modelID) const {
    if(parent == nullptr) {
        return nullptr;
    }
    Model* model = dynamic_cast<Model*>(parent);
    if(model != nullptr) {//FIXME non model attachments are filtered
        if(model->getWorldObjectID() == modelID) {
            return model;
        }else {
            for (auto iterator = model->getChildren().begin(); iterator != model->getChildren().end(); ++iterator) {
                Model* child = findModelByIDChildren(*iterator, modelID);
                if(child != nullptr) {
                    return child;
                }
            }
        }
    }
    return nullptr;
}

bool World::changeRenderPipeline(const std::string &pipelineFileName) {
    std::unique_ptr<GraphicsPipeline> newPipeline = GraphicsPipeline::deserialize(pipelineFileName, this->graphicsWrapper, assetManager, options, buildRenderMethods());
    if(newPipeline != nullptr) {
        this->renderPipeline = std::move(newPipeline);
        //reset the
        resetTagsAndRefillCulling();
        return true;
    }
    return false;
}

   void World::resetTagsAndRefillCulling() {
       resetVisibilityBufferForRenderPipelineChange();
       resetCameraTagsFromPipeline(renderPipeline->getCameraTagToRenderTagSetMap());
       fillVisibleObjectsUsingTags();
   }


   Model *World::findModelByID(uint32_t modelID) const {
    if(startingPlayer.attachedModel != nullptr) {
        Model* playerAttachment = findModelByIDChildren(startingPlayer.attachedModel, modelID);
        if(playerAttachment != nullptr) {
            return playerAttachment;
        }
    }

    if(objects.find(modelID) != objects.end()) {
        Model* model = dynamic_cast<Model*>(objects.at(modelID));
        if(model != nullptr) {
            return model;
        }
    }
    return nullptr;
}

bool World::attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID) {
    PhysicalRenderable* objectToAttach;
    PhysicalRenderable* objectToAttachTo;
    if(objectID == objectToAttachToID) {
        //can't attach to self
        return false;
    }

    Transformation* transform1,* transform2;

    objectToAttach = findModelByID(objectID);
    if(objectToAttach == nullptr) {
        return false;
    }
    transform1 = objectToAttach->getTransformation();

    objectToAttachTo = findModelByID(objectToAttachToID);
    if(objectToAttachTo == nullptr) {
        return false;
    }
    transform2 = objectToAttachTo->getTransformation();

    //The offset removal of the parent is not exposed via API. We should manually remove that from the object
    transform1->addTranslate(-1 * objectToAttachTo->getCenterOffset());

    transform1->setParentTransform(transform2);
    objectToAttach->setParentObject(objectToAttachTo);
    objectToAttachTo->addChild(objectToAttach);


    glm::vec3 translate, scale;
    glm::quat orientation;
    objectToAttachTo->getTransformation()->getDifferenceStacked(*(transform1), translate,
                                                    scale, orientation);
    GLMUtils::printVector(translate);
    return true;
}

bool World::setObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4 &position) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }

    model->getTransformation()->setTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
    return true;
}

bool World::setObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4 &scale) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }

    model->getTransformation()->setScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
    return true;
}

bool World::setObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }

    glm::quat orientationQuat(orientation.w,
                              orientation.x,
                              orientation.y,
                              orientation.z);
    model->getTransformation()->setOrientation(orientationQuat);
    return true;
}

bool World::addObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4 &position) {
   Model* model = findModelByID(objectID);
   if(model == nullptr) {
       return false;
   }

   model->getTransformation()->addTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
   return true;
}

bool World::addObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4 &scale) {
   Model* model = findModelByID(objectID);
   if(model == nullptr) {
       return false;
   }

   model->getTransformation()->addScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
   return true;
}

bool World::addObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
   Model* model = findModelByID(objectID);
   if(model == nullptr) {
       return false;
   }

   glm::quat orientationQuat(orientation.w,
                             orientation.x,
                             orientation.y,
                             orientation.z);
   model->getTransformation()->addOrientation(orientationQuat);
   return true;
}


struct LightCloserToPlayer {
    glm::vec3 playerPosition;
    LightCloserToPlayer(glm::vec3 playerPosition) : playerPosition(playerPosition) {}
   bool operator()(Light const *a, Light const *b) const {
       return glm::length2(a->getPosition() - playerPosition) <
              glm::length2(b->getPosition() - playerPosition);
   }
};
void World::updateActiveLights(bool forceUpdate) {
    if(!forceUpdate) {
        // if player is not moved around and lights didn't move around, don't update
        float distance = glm::distance(currentPlayer->getPosition(), lastLightUpdatePlayerPosition);
        if (distance < 1.0f) {
            for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
                if (lights[lightIndex]->isFrustumChanged()) {
                    forceUpdate = true;
                    break;
                }
            }
        } else {
            forceUpdate = true;
        }
        if (!forceUpdate) {
            return;
        }
    }

    lastLightUpdatePlayerPosition = currentPlayer->getPosition();
    activeLights.clear();

    // we have NR_POINT lights, and directional lights. we should have 1 directional light, and rest point lights.
    uint32_t fullLightsIndex = 0;
    for (; fullLightsIndex < lights.size() && activeLights.size() < NR_POINT_LIGHTS; ++fullLightsIndex) {
        if(lights[fullLightsIndex]->getLightType() != Light::LightTypes::DIRECTIONAL) {
            activeLights.push_back(lights[fullLightsIndex]);
            lights[fullLightsIndex]->setFrustumChanged(true);//since we needed update, force update;
        }
    }
    if(lights.size() > NR_POINT_LIGHTS) {
        std::sort(activeLights.begin(), activeLights.end(), LightCloserToPlayer(currentPlayer->getPosition()));
        for (; fullLightsIndex < lights.size(); ++fullLightsIndex) {
            if(lights[fullLightsIndex]->getLightType() == Light::LightTypes::DIRECTIONAL) {
                continue;
            }
            uint32_t insertIndex = NR_POINT_LIGHTS-1;
            while(insertIndex > 0 && (glm::length2(lights[fullLightsIndex]->getPosition() - currentPlayer->getPosition()) <
                    glm::length2(activeLights[insertIndex]->getPosition() - currentPlayer->getPosition()))) {
                activeLights[insertIndex] = activeLights[insertIndex-1];
                insertIndex--;
            }
            if(insertIndex != NR_POINT_LIGHTS-1) {
                activeLights[insertIndex] = lights[fullLightsIndex];
                lights[fullLightsIndex]->setFrustumChanged(true);
            } else {
                //this means the light will not be used, there for it will not be needed for frustum culled;
                lights[fullLightsIndex]->setFrustumChanged(false);//since we needed update, force update;
            }
        }
    }

    //at this point, add the directional light to the end
    if(directionalLightIndex != -1) {
        activeLights.push_back(lights[directionalLightIndex]);
        lights[directionalLightIndex]->setFrustumChanged(true);
    }

    for (size_t lightIndex = 0; lightIndex < activeLights.size(); ++lightIndex) {
        const Light* currentLight = activeLights[lightIndex];
        graphicsWrapper->setLight(
                lightIndex,
                currentLight->getAttenuation(),
                currentLight->getShadowMatrices(),
                currentLight->getPosition(),
                currentLight->getColor(),
                currentLight->getAmbientColor(),
                static_cast<int>(currentLight->getLightType()),
                currentLight->getActiveDistance()
                );
    }

    for (uint32_t i = activeLights.size(); i < NR_TOTAL_LIGHTS; ++i) {
        graphicsWrapper->removeLight(i);
    }

}

   void World::clearWorldRefsBeforeAttachment(PhysicalRenderable *attachment) {
       GameObject* gameObject = dynamic_cast<GameObject*>(attachment);
       if(gameObject != nullptr) {
           objects.erase(gameObject->getWorldObjectID());
           dynamicsWorld->removeRigidBody(attachment->getRigidBody());
           for (auto iterator = rigidBodies.begin(); iterator != rigidBodies.end(); ++iterator) {
               if ((*iterator) == attachment->getRigidBody()) {
                   rigidBodies.erase(iterator);
                   break;
               }
           }
       }
       attachment->disconnectFromPhysicsWorld(dynamicsWorld);
       for (auto childIt = attachment->getChildren().begin();
            childIt != attachment->getChildren().end(); ++childIt) {
           clearWorldRefsBeforeAttachment(*childIt);
       }
   }

bool World::addPlayerAttachmentUsedIDs(const PhysicalRenderable *attachment, std::set<uint32_t> &usedIDs,
                                       uint32_t &maxID) {
    if(attachment == nullptr) {
        return true;
    }
    const GameObject* gameObjectOfTheSame = dynamic_cast<const GameObject*>(attachment);
    if(gameObjectOfTheSame == nullptr) {
        std::cerr << "Player attachment is not GameObject, that should never happen." << std::endl;
        return false;
    }
    auto result = usedIDs.insert(gameObjectOfTheSame->getWorldObjectID());
    if(result.second == false) {
        std::cerr << "world ID repetition on player attachment detected! ID was " << gameObjectOfTheSame->getWorldObjectID() << std::endl;
        return false;
    }
    maxID = std::max(maxID, gameObjectOfTheSame->getWorldObjectID());
    for (auto child = attachment->getChildren().begin(); child != attachment->getChildren().end(); ++child) {
        if(!addPlayerAttachmentUsedIDs(*child, usedIDs, maxID)) {
            return false;
        }
    }
    return true;
}

bool World::verifyIDs() {
    std::set<uint32_t > usedIDs;
    uint32_t maxID = 0;
    usedIDs.insert(1);//reserved for physicalPlayer
    /** there are 3 places that has IDs,
     * 1) sky
     * 2) objects
     * 3) AIs
     */
    //put sky first, since it is guaranteed to be single
    if(this->sky != nullptr) {
        usedIDs.insert(this->sky->getWorldObjectID());
        maxID = this->sky->getWorldObjectID();
    }

    for(auto object = objects.begin(); object != objects.end(); object++) {
        auto result = usedIDs.insert(object->first);
        if(result.second == false) {
            std::cerr << "world ID repetition on object detected! with id " << object->first << std::endl;
            return false;
        }
        maxID = std::max(maxID,object->first);
    }

    for(auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
        auto result = usedIDs.insert(trigger->first);
        if(result.second == false) {
            std::cerr << "world ID repetition on trigger detected! with id " << trigger->first << std::endl;
            return false;
        }
        maxID = std::max(maxID,trigger->first);
    }

    for(auto actor = actors.begin(); actor != actors.end(); actor++) {
        auto result = usedIDs.insert(actor->first);
        if(result.second == false) {
            std::cerr << "world ID repetition detected! ActorInterface with id " << actor->first << std::endl;
            return false;
        }
        maxID = std::max(maxID,actor->first);
    }

    for (auto guiElement = guiElements.begin(); guiElement != guiElements.end(); ++guiElement) {
        auto result = usedIDs.insert(guiElement->first);
        if(result.second == false) {
            std::cerr << "world ID repetition detected! gui element with id " << guiElement->first << std::endl;
            return false;
        }
        maxID = std::max(maxID, guiElement->first);
    }

    for (auto modelGroup = modelGroups.begin(); modelGroup != modelGroups.end(); ++modelGroup) {
        auto result = usedIDs.insert(modelGroup->first);
        if(result.second == false) {
            std::cerr << "world ID repetition on trigger detected! gui element with id " << modelGroup->first << std::endl;
            return false;
        }
        maxID = std::max(maxID, modelGroup->first);
    }

    if (!addPlayerAttachmentUsedIDs(startingPlayer.attachedModel, usedIDs, maxID)) {
        return false;
    }

    uint32_t unusedIDCount = 0;
    for(uint32_t index = 1; index <= maxID; index++) {
        if(usedIDs.count(index) != 1) {
            unusedIDs.push(index);
            unusedIDCount++;
        }
    }
    std::cout << "World load found " << maxID - unusedIDCount << " objects and " << unusedIDCount << " unused IDs." << std::endl;
    OptionsUtil::Options::Option<std::string> dataDirectoryOption = options->getOption<std::string>(HASH("dataDirectory"));
    std::cout << "Data directory is " << dataDirectoryOption.get() << std::endl;
    nextWorldID = maxID+1;
    return true;
}

void World::addSkyBoxControls() {
    //first, build a tree for showing directories with textures in them.
    static const AssetManager::AvailableAssetsNode* selectedSkyBoxAsset = nullptr;
    static const AssetManager::AvailableAssetsNode* selectedAssetDirectory = nullptr;

    if(selectedAssetDirectory == nullptr) {
        static char skyBoxAssetFilter[32] = {0};
        ImGui::InputText("Filter Assets ##SkyBoxAssetTreeFilter", skyBoxAssetFilter, sizeof(skyBoxAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
        std::string skyBoxAssetFilterStr = skyBoxAssetFilter;
        std::transform(skyBoxAssetFilterStr.begin(), skyBoxAssetFilterStr.end(), skyBoxAssetFilterStr.begin(),::tolower);
        const AssetManager::AvailableAssetsNode *filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, skyBoxAssetFilterStr);

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
                SkyBox* newSkyBox = new SkyBox(getNextObjectID(), assetManager, selectedAssetDirectory->fullPath, skyBoxRightFileName, skyBoxLeftFileName, skyBoxTopFileName, skyBoxBottomFileName, skyBoxBackFileName, skyBoxFrontFileName);
                delete this->sky;
                this->sky = newSkyBox;
            }
        } else {
            ImGui::Button("Change Sky Box");
            ImGui::SameLine();
            ImGuiHelper::ShowHelpMarker("Some Elements are empty");
        }


    }


}

bool World::addLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4 &position) {
    Light* light = nullptr;
    for (size_t i = 0; i < lights.size(); ++i) {
        if(lights[i]->getWorldObjectID() == lightID){
            light = lights[i];
            break;
        }
    }
    if(light == nullptr) {
        return false;
    }

    light->setPosition(light->getPosition() + glm::vec3(GLMConverter::LimonToGLM(position)), this->playerCamera);
    return true;
}

bool World::setLightColorAPI(uint32_t lightID, const LimonTypes::Vec4& color) {
    Light* light = nullptr;
    for (size_t i = 0; i < lights.size(); ++i) {
        if(lights[i]->getWorldObjectID() == lightID){
            light = lights[i];
            break;
        }
    }
    if(light == nullptr) {
        return false;
    }

    light->setColor(glm::vec3(GLMConverter::LimonToGLM(color)));
    return true;
}

void World::drawNodeEditor() {
    if(this->nodeGraph == nullptr) {
        createNodeGraph();
    }

    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Example: Custom Node Graph", &showNodeGraph)) {
        ImGui::End();
        return;
    }
    ImGui::ShowDemoWindow();

    nodeGraph->display();
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
            cancelTimedEventAPI(handleId);
            handleId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Revert", ImVec2(120, 0))) {
            cancelTimedEventAPI(handleId);
            this->renderPipeline = this->renderPipelineBackup;
            this->renderPipelineBackup = nullptr;
            handleId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if(pipelineExtension->isPipelineBuilt()) {
        if (ImGui::Button("Activate")) {
            std::shared_ptr<GraphicsPipeline> builtRenderPipeline = pipelineExtension->handOverBuiltPipeline();
            std::vector<LimonTypes::GenericParameter> emptyParameters;
            for(auto& stage:builtRenderPipeline->getStages()) {
                for(auto& method:stage.renderMethods) {
                    if(!method.getInitialized()) {
                        method.initialize(emptyParameters);
                    }
                }
            }
            std::vector<LimonTypes::GenericParameter> empty;
            handleId = addTimedEventAPI(10000, true,
                                        [&](const std::vector<LimonTypes::GenericParameter> &) {
                                            this->renderPipeline = this->renderPipelineBackup;
                                            this->renderPipelineBackup = nullptr;
                                            handleId = 0;
                                        },
                                        empty);
            this->renderPipelineBackup = this->renderPipeline;
            this->renderPipeline = builtRenderPipeline;
        }
    }

    if(ImGui::Button("Save")) {
        nodeGraph->serialize("./Data/nodeGraph.xml");
        nodeGraph->addMessage("Serialization done.");
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel")){
        showNodeGraph = false;
    }
    ImGui::End();
}

void World::createNodeGraph() {
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

    iterationExtension = new IterationExtension();

    NodeType* iterate = new NodeType{"Iterate", false, "IterationExtension", [](const NodeType* nodeType[[gnu::unused]]) -> NodeExtension* {return new IterationExtension();},
                      {{"Input", "Texture"},},
                       {{"Output", "Texture"},},false, {}};
    nodeTypeVector.push_back(iterate);

    std::vector<std::shared_ptr<GraphicsProgram>> programs = getAllAvailablePrograms();

    RenderMethods renderMethods = buildRenderMethods();

    pipelineExtension = new PipelineExtension(graphicsWrapper, renderPipeline, assetManager, options, GraphicsPipeline::getRenderMethodNames(), renderMethods);

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

        type->nodeExtensionConstructor = [=](const NodeType* nodeType[[gnu::unused]]) ->NodeExtension* {return new PipelineStageExtension(pipelineExtension, programNameInfo);};
        type->extraVariables["vertexShaderName"] = program->getVertexShaderFile();
        type->extraVariables["geometryShaderName"] = program->getGeometryShaderFile();
        type->extraVariables["fragmentShaderName"] = program->getFragmentShaderFile();

        nodeTypeVector.push_back(type);
    }

    std::unordered_map<std::string, std::function<EditorExtension*()>> possibleEditorExtensions;
    possibleEditorExtensions["PipelineExtension"] = [=]() ->EditorExtension* {return pipelineExtension;};

    std::unordered_map<std::string, std::function<NodeExtension*(const NodeType*)>> possibleNodeExtensions;
    possibleNodeExtensions["PipelineStageExtension"] = [=](const NodeType* nodeType) ->NodeExtension* {return new PipelineStageExtension(nodeType, pipelineExtension);};
    possibleNodeExtensions["IterationExtension"] = [=](const NodeType*) -> NodeExtension* {return new IterationExtension();};

    nodeGraph = NodeGraph::deserialize("./Data/nodeGraph.xml", possibleEditorExtensions, possibleNodeExtensions);

    bool freshNodeGraphCreated = false;
    if(nodeGraph == nullptr) {
        std::cerr << "No custom Nodegraph found, using the default." << std::endl;
        nodeGraph = NodeGraph::deserialize("./Engine/nodeGraph.xml", possibleEditorExtensions, possibleNodeExtensions);
        if(nodeGraph == nullptr) {
            std::cerr << "Default Node deserialize failed too, using empty node graph" << std::endl;
            nodeGraph = new NodeGraph(nodeTypeVector, false, pipelineExtension);
            freshNodeGraphCreated = true;
        }
    }

    if(!freshNodeGraphCreated) {
        // we loaded an old nodegraph. What if the available programs changed? if user add new programs, we should add them. If user removed programs, we should mark the pipeline as invalid, and warn.
        // First check if any node type we don't have is defined
        std::vector<const NodeType *> oldDefinedNodeTypes = nodeGraph->getNodeTypes();
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
                pipelineExtension->setNodeGraphValid(false);
                pipelineExtension->addError("Old Node type " + oldNodeType->name + " Not found, this graph is invalid");
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
                if (nodeGraph->addNodeType(newNodeType)) {
                    std::cout << "New node type " << newNodeType->name << " added" << std::endl;
                } else {
                    std::cerr << "New node type " << newNodeType->name << " should be added, but rejected!" << std::endl;
                }
            }
        }
    }
}

   std::vector<std::shared_ptr<GraphicsProgram>> World::getAllAvailablePrograms() {
       const AssetManager::AvailableAssetsNode* availableAssetsTree = assetManager->getAvailableAssetsTree();
       std::vector<std::shared_ptr<GraphicsProgram>> programs;
       getAllAvailableProgramsRecursive(availableAssetsTree, programs);
       return programs;
   }

   void World::getAllAvailableProgramsRecursive(const AssetManager::AvailableAssetsNode *currentNode, std::vector<std::shared_ptr<GraphicsProgram>> &programs) {
       if (currentNode->assetType == AssetManager::Asset_type_DIRECTORY) {
           const AssetManager::AvailableAssetsNode *vertexShaderNode = nullptr;
           const AssetManager::AvailableAssetsNode *geometryShaderNode = nullptr;
           const AssetManager::AvailableAssetsNode *fragmentShaderNode = nullptr;
           for (auto childNode:currentNode->children) {
               if (childNode->assetType == AssetManager::Asset_type_DIRECTORY) {
                   getAllAvailableProgramsRecursive(childNode, programs);
               } else if (childNode->assetType == AssetManager::Asset_type_GRAPHICSPROGRAM) {
                   if ("vertex.glsl" == childNode->name) { vertexShaderNode = childNode; }
                   else if ("geometry.glsl" == childNode->name) { geometryShaderNode = childNode; }
                   else if ("fragment.glsl" == childNode->name) { fragmentShaderNode = childNode; }
               } else {
                   continue;
               }
           }
           if (vertexShaderNode != nullptr && fragmentShaderNode != nullptr) {
               //if we have both vertex and fragment, then this is a valid program. Geometry is optional.
               if(vertexShaderNode->fullPath == "./Data/Shaders/Particles/vertex.glsl" &&
               fragmentShaderNode->fullPath == "./Data/Shaders/Particles/fragment.glsl"
               ) {
                   std::cerr << "Found the particles asset" << std::endl;
               }

               std::shared_ptr<GraphicsProgram> foundProgram;
               if (geometryShaderNode != nullptr) {
                   foundProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                                    vertexShaderNode->fullPath,
                                                                    geometryShaderNode->fullPath,
                                                                    fragmentShaderNode->fullPath,
                                                                    false);
               } else {
                   foundProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                                    vertexShaderNode->fullPath,
                                                                    fragmentShaderNode->fullPath,
                                                                    false);
               }
               for(auto oldProgram:programs) {
                   if(oldProgram->getProgramName() == foundProgram->getProgramName()) {
                       std::cout << "wtf" << std::endl;
                   }
               }
               programs.emplace_back(foundProgram);
           }
       }

   }

   bool World::enableParticleEmitter(uint32_t particleEmitterId) {
    auto emitterIt = emitters.find(particleEmitterId);
    if(emitterIt == emitters.end()) {
        return false;
    }
    emitterIt->second->setEnabled(true);
    return true;
   }

   bool World::disableParticleEmitter(uint32_t particleEmitterId) {
       auto emitterIt = emitters.find(particleEmitterId);
       if(emitterIt == emitters.end()) {
           return false;
       }
       emitterIt->second->setEnabled(false);
       return true;
   }

uint32_t World::addParticleEmitter(const std::string &name,
                                   const std::string& textureFile,
                                   const LimonTypes::Vec4& startPosition,
                                   const LimonTypes::Vec4& maxStartDistances,
                                   const LimonTypes::Vec2& size,
                                   uint32_t count,
                                   uint32_t lifeTime,
                                   float particlePerMs,
                                   bool continuouslyEmit){
   //validate first:
   if(count > 10000) {
       std::cerr << "Can't create particle emitter with more than 10000 particles" << std::endl;
       return 0;
   }

    if(count < 1) {
        std::cerr << "Can't create particle emitter with 0 particles" << std::endl;
        return 0;
    }
   std::shared_ptr<Emitter> newEmitter;
   if(particlePerMs <= 0) {
       newEmitter = std::make_shared<Emitter>(this->getNextObjectID(), name,
                                              this->assetManager, textureFile,
                                              GLMConverter::LimonToGLMV3(startPosition),
                                              GLMConverter::LimonToGLMV3(maxStartDistances),
                                              GLMConverter::LimonToGLM(size),
                                              count, lifeTime);
   } else {
       newEmitter = std::make_shared<Emitter>(this->getNextObjectID(), name,
                                              this->assetManager, textureFile,
                                              GLMConverter::LimonToGLMV3(startPosition),
                                              GLMConverter::LimonToGLMV3(maxStartDistances),
                                              GLMConverter::LimonToGLM(size), count,
                                              lifeTime, particlePerMs);
   }
   newEmitter->setContinuousEmit(continuouslyEmit);
   this->emitters[newEmitter->getWorldObjectID()] = (newEmitter);
   return newEmitter->getWorldObjectID();
}

bool World::removeParticleEmitter(uint32_t emitterID) {
       auto emitterIT = this->emitters.find(emitterID);
        if(emitterIT == this->emitters.end()) {
            return false;
        }
        this->emitters.erase(emitterIT);
        return true;
}

bool World::setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset) {
       auto emitterIT = this->emitters.find(emitterID);
       if(emitterIT == this->emitters.end()) {
           return false;
       }
       emitterIT->second->setSpeedMultiplier(GLMConverter::LimonToGLMV3(speedMultiplier));
       emitterIT->second->setSpeedOffset(GLMConverter::LimonToGLMV3(speedOffset));
       return true;
}

bool World::setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4& gravity) {
       auto emitterIT = this->emitters.find(emitterID);
       if(emitterIT == this->emitters.end()) {
           return false;
       }
       emitterIT->second->setGravity(GLMConverter::LimonToGLMV3(gravity));
       return true;
}