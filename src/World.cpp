//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "WorldAPIAccessor.h"
#include <Graphics/GraphicsPipeline.h>
#include "NodeEditorExtensions/PipelineStageExtension.h"

#include "Camera/PerspectiveCamera.h"
#include "BulletDebugDrawer.h"
#include "AI/AIMovementGrid.h"

#include "GameObjects/Players/FreeCursorPlayer.h"
#include "GameObjects/Players/FreeMovingPlayer.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Players/MenuPlayer.h"
#include "GameObjects/Players/EditorPlayer.h"
#include "GameObjects/Light.h"
#include "GUI/GUILayer.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUIFPSCounter.h"
#include "GUI/GUITextDynamic.h"
#include "ImGuiHelper.h"
#include "GameObjects/TriggerObject.h"
#include "Assets/Animations/AnimationCustom.h"
#include "AnimationSequencer.h"
#include "GUI/GUICursor.h"
#include "GameObjects/GUIText.h"
#include "GameObjects/GUIButton.h"
#include "GameObjects/ModelGroup.h"
#include "Graphics/PostProcess/QuadRender.h"
#include "Editor/Editor.h"
#include "Occlusion/RenderList.h"
#include "Occlusion/VisibilityManager.h"
#include "Profiler/ProfilerMacros.h"

const std::map<World::PlayerInfo::Types, std::string> World::PlayerInfo::typeNames =
    {
            { Types::PHYSICAL_PLAYER, "Physical"},
            { Types::DEBUG_PLAYER, "Debug"},
            { Types::EDITOR_PLAYER, "Editor"},
            { Types::MENU_PLAYER, "Menu" }
    };

void World::setupRenderForPipeline() const {
}

World::World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
                std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options *options, ProfilerSystem* profilerSystem)
        : assetManager(assetManager), options(options), profilerSystem(profilerSystem),
        graphicsWrapper(assetManager->getGraphicsWrapper()), alHelper(assetManager->getAlHelper()), name(name),
        fontManager(graphicsWrapper), startingPlayer(startingPlayerType) {
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
            editorPlayer = new EditorPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation, inputHandler);
            currentPlayer = editorPlayer;
            break;
        case PlayerInfo::Types::MENU_PLAYER:
            menuPlayer = new MenuPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation);
            currentPlayer = menuPlayer;
            break;
    }
    
    quadRender = std::make_shared<QuadRender>(graphicsWrapper);
    visibilityManager = std::make_unique<VisibilityManager>(this);
    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    playerCamera = new PerspectiveCamera("Player camera", options, currentPlayer->getCameraAttachment());//register is just below
    playerCamera->addTag(HardCodedTags::CAMERA_PLAYER);
    visibilityManager->addCamera(playerCamera);
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                           COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                           COLLIDE_MODELS | COLLIDE_EVERYTHING, worldAABBMin,
                                           worldAABBMax);
    switchPlayer(currentPlayer, *inputHandler); //switching to itself, to set the states properly. It uses camera so done after camera creation

    OptionsUtil::Options::Option<std::string> renderPipelineOption = options->getOption<std::string>(HASH("StartingRenderPipeline"));
    renderPipeline = GraphicsPipeline::deserialize(renderPipelineOption.get(), graphicsWrapper, assetManager, options, buildRenderMethods());

    if(renderPipeline == nullptr) {
        //use default if no custom is found
        std::cerr << "Render pipeline not found, loading default." << std::endl;
        renderPipeline = GraphicsPipeline::deserialize("./Engine/renderPipeline.xml", graphicsWrapper, assetManager, options, buildRenderMethods());
    }
    if (renderPipeline == nullptr) {
        std::cerr << "Default render pipeline not found, please check if your installation is correct. Exiting" << std::endl;
        std::exit(-1);
    }
    setupRenderForPipeline();

    fpsCounter = new GUIFPSCounter(graphicsWrapper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                                   glm::vec3(204, 204, 0));
    fpsCounter->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);

    onLoadActions.push_back(new ActionForOnload());//this is here for editor, as if no action is added, editor would fail to allow setting the first one.

    modelIndicesBuffer.reserve(NR_MAX_MODELS);
    tempRenderedObjectsSet.reserve(NR_MAX_MODELS);

    renderInformationsOption = options->getOption<bool>(HASH("renderInformations"));
    maxLightsOption = options->getOption<long>(HASH("maximumLights"));
    activeLights.reserve(maxLightsOption.getOrDefault(4));
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
       renderMethods.renderLight = std::bind(&World::renderLight, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
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
                 if (gameObject->getTypeID() != GameObject::ObjectTypes::PLAYER && gameObject->getTypeID() != GameObject::ObjectTypes::TRIGGER ) { //trigger is ghost, so it should not block
                     if(hasSeen) {
                         //means we saw the player, and this is closer than player
                         return false;
                     }
                 }
                 if (gameObject->getTypeID() == GameObject::ObjectTypes::PLAYER) {
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
    PROFILE_SIMULATION("World::play");
     editor->update(inputHandler);

     this->wallTime = wallTime;
     //Seperating physics step and visibility, because physics is used by camera, and camera is used by visibility
     if(currentPlayersSettings->worldSimulation) {
         //every time we call this method, we increase the time only by simulationTimeframe
         gameTime += simulationTimeFrame;
         {
             PROFILE_SIMULATION("World::play::PhysicsSimulation");
             dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
         }
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
             }
         }

         tempRenderedObjectsSet.clear();//used to choose which models needs setup for time
         for (const auto &visibility: visibilityManager->getCullingResults()) {
             for (auto &visibleTags: *visibility.second){
                 for (auto it = visibleTags.second.getIterator(); !it.isEnd(); ++it) {
                     for (glm::uvec4 meshRenderInfo:it.get().indices) {
                         if (tempRenderedObjectsSet.find(meshRenderInfo.x) == tempRenderedObjectsSet.end()) {
                             objects[meshRenderInfo.x]->setupForTime(gameTime);
                             tempRenderedObjectsSet.insert(meshRenderInfo.x);
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

     updateActiveLights(false);
     for (size_t j = 0; j < activeLights.size(); ++j) {
         activeLights[j]->step(gameTime, playerCamera);
     }
    uploadActiveLightsToGPU();
     visibilityManager->update();

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
        if(pointed != nullptr && pointed->getTypeID() == GameObject::ObjectTypes::GUI_BUTTON) {
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
    PROFILE_SIMULATION("World::animateCustomAnimations");
    // ATTENTION iterator is not increased in for, it is done manually.
    for(auto animIt = activeAnimations.begin(); animIt != activeAnimations.end();) {
        AnimationStatus* animationStatus = animIt->second;
        if(animationStatus->originChange) {
            //this means the origin has changed in editor mode, so we should update our origin of transformation.

            // First change the original transform to current, since user updated it
            animationStatus->originalTransformation.setTransformations(animationStatus->object->getTransformation()->getTranslate()
            ,animationStatus->object->getTransformation()->getScale()
            ,animationStatus->object->getTransformation()->getOrientation());

            //then remove the change from object transform.
            animationStatus->object->getTransformation()->setTransformations(glm::vec3(0.0f,0.0f,0.0f)
            , glm::vec3(1.0f,1.0f,1.0f)
            , glm::quat(1.0f,0.0f,0.0f, 0.0f));
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
            animationStatus->object->getTransformation()->setTransformations(tempTranslate
            , tempScale
            , tempOrientation);
            animationStatus->object->setCustomAnimation(false);

            options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_DEBUG, "Animation " + animationCustom->getName() + " finished, removing. ");
            delete animIt->second;
            animIt = activeAnimations.erase(animIt);

        }
    }
}

void World::setPlayerAttachmentsForChangedBoneTransforms(Model *playerAttachment) {
    if (playerAttachment == nullptr) {
        return;
    }
    if (playerAttachment->getRigId() == 0) {
        playerAttachment->setRigId(this->getNextRigId());
    }
    if(playerAttachment->isAnimated()) {
            changedBoneTransforms.emplace(playerAttachment->getRigId(), playerAttachment->getBoneTransforms());
    }
    for (auto& child:playerAttachment->getChildren()) {
        Model* childModel = dynamic_cast<Model*>(child);
        setPlayerAttachmentsForChangedBoneTransforms(childModel);
    }
}

ActorInterface::ActorInformation World::fillActorInformation(ActorInterface *actor) {
    PROFILE_SIMULATION("World::fillActorInformation");
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
        information.playerDirection = normalize(rayDir);
        information.cosineBetweenPlayer = cosBetween;
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
        if ((requests.routeToPlayer || requests.routeToCustomPosition) && routeThreads.find(actor->getWorldID()) == routeThreads.end()) {
            glm::vec3 destination = requests.routeToPlayer
                ? currentPlayer->getPosition()
                : requests.customPosition;

            std::vector<LimonTypes::GenericParameter> parameters;
            parameters.push_back(LimonTypes::GenericParameter());
            parameters[0].value.vectorValue = GLMConverter::GLMToLimon(
                    actor->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0));
            parameters.push_back(LimonTypes::GenericParameter());
            parameters[1].value.longValues[0] = 2;
            parameters[1].value.longValues[1] = actor->getWorldID();
            parameters[1].value.longValues[2] = information.maximumRouteDistance;
            parameters.push_back(LimonTypes::GenericParameter());
            parameters[2].value.vectorValue = GLMConverter::GLMToLimon(destination);

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
    PROFILE_SIMULATION("World::fillRouteInformation");
    glm::vec3 fromPosition = glm::vec3(GLMConverter::LimonToGLM(parameters[0].value.vectorValue));
    uint32_t actorID = (uint32_t)parameters[1].value.longValues[1];
    uint32_t maximumDistance = (uint32_t)parameters[1].value.longValues[2];
    std::vector<glm::vec3> route;
    glm::vec3 destinationWithGrid = glm::vec3(GLMConverter::LimonToGLM(parameters[2].value.vectorValue));
    bool isDestinationReachable = grid->setProperHeight(&destinationWithGrid, AIMovementGrid::floatingHeight, 0.0f,
                                                      dynamicsWorld);
    if (isDestinationReachable && grid->coursePath(fromPosition, destinationWithGrid, actorID, maximumDistance, &route)) {
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
                if(editor->pickedObject != nullptr ) {
                    editor->pickedObject->removeTag(HardCodedTags::PICKED_OBJECT);
                }
                editor->pickedObject = gameObject;
                editor->pickedObject->addTag(HardCodedTags::PICKED_OBJECT);
            } else {
                editor->pickedObject = nullptr;
            }
        }
    }

    if(inputHandler.getInputStates().getInputEvents(InputStates::Inputs::F4)) {
        OptionsUtil::Options::Option<bool> debugDrawLinesOption = options->getOption<bool>(HASH("DebugDrawLines"));
        bool debugDrawLines = debugDrawLinesOption.getOrDefault(false);
        if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::F4)) {
            debugDrawLines = !debugDrawLines;
        }
        debugDrawLinesOption.set(debugDrawLines);
        /*
        long cascadeCount;
        OptionsUtil::Options::Option<long> cascadeCountOption = options->getOption<long>(HASH("CascadeCount"));
        cascadeCount = cascadeCountOption.getOrDefault(4);
        if(inputHandler.getInputStates().getInputStatus(InputStates::Inputs::F4)) {
            cascadeCount--;
            if(cascadeCount < 0) {
                cascadeCount = cascadeCountOption.getOrDefault(4);
            }
        }
        */
    }

    if (inputHandler.getInputStates().getInputEvents(InputStates::Inputs::EDITOR) && inputHandler.getInputStates().getInputStatus(InputStates::Inputs::EDITOR)) {
        if(editorPlayer == nullptr) {
            editorPlayer = new EditorPlayer(options, cursor, startingPlayer.position, startingPlayer.orientation, &inputHandler);
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

void World::render() {
    PROFILE_RENDERING("World::render");
    renderPipeline->render();
}

void World::renderGUIImages(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
    PROFILE_RENDERING("World::renderGUIImages");
    cursor->renderWithProgram(renderProgram, 0);

    for (auto it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->renderImageWithProgram(renderProgram);
    }
    //render API gui layer
    apiGUILayer->renderImageWithProgram(renderProgram);

}

void World::renderGUITexts(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
    PROFILE_RENDERING("World::renderGUITexts");
    for (auto it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->renderTextWithProgram(renderProgram);
    }
    //render API gui layer
    apiGUILayer->renderTextWithProgram(renderProgram);

    uint32_t triangle, line;
    graphicsWrapper->getRenderTriangleAndLineCount(triangle, line);
    renderCounts->updateText("Tris: " + std::to_string(triangle) + ", lines: " + std::to_string(line));
    bool renderInformations =renderInformationsOption.getOrDefault(false);
    if (renderInformations) {
        renderCounts->renderWithProgram(renderProgram, 0);
        debugOutputGUI->renderWithProgram(renderProgram, 0);
        fpsCounter->renderWithProgram(renderProgram, 0);
    }
}

void World::renderParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
     PROFILE_RENDERING("World::renderParticleEmitters");
     for(const auto& emitter:emitters) {
         emitter.second->renderWithProgram(renderProgram, 0);
     }
}

void World::renderGPUParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   PROFILE_RENDERING("World::renderGPUParticleEmitters");
   for(const auto& gpuParticleEmitter:gpuParticleEmitters) {
       gpuParticleEmitter.second->renderWithProgram(renderProgram, 0);
   }
}

void World::renderDebug(const std::shared_ptr<GraphicsProgram>& renderProgram [[gnu::unused]], const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   PROFILE_RENDERING("World::renderDebug");
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
    PROFILE_RENDERING("World::renderCameraByTag");
    uint64_t hashedCameraTag = HashUtil::hashString(cameraName);
    for (const auto &visibilityEntry: visibilityManager->getCullingResults()) {
        if (visibilityEntry.first->hasTag(hashedCameraTag)) { //This is a request for this camera
            std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>& renderLists = *visibilityEntry.second;
            //First  recursively render the player attachments, no visibility check.
            if (!currentPlayer->isDead() && startingPlayer.attachedModel != nullptr) {
                //don't render attached model if dead
                std::vector<uint32_t> alreadyRenderedModelIds;
                for (const auto &renderTag: tags) {
                    renderPlayerAttachmentsRecursiveByTag(startingPlayer.attachedModel, renderTag.hash,
                                                          renderProgram, alreadyRenderedModelIds);//Starting player, because we don't wanna render when in editor mode
                }
            }
            for (auto& renderListEntry: renderLists) {
                if (!VisibilityRequest::vectorComparator(renderListEntry.first, tags)) {
                    continue;
                }
                const RenderList& renderList = renderListEntry.second;
                renderList.render(graphicsWrapper, renderProgram);
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
    std::vector<Attachable *> children;
    if(attachmentObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
        children = (static_cast<Model*>(attachment))->getChildren();
    } else if(attachmentObject->getTypeID() == GameObject::ObjectTypes::MODEL_GROUP) {
        //the group has the tag, everything under should be rendered.
        children = (static_cast<ModelGroup*>(attachment))->getChildren();
    }
    if(std::find(alreadyRenderedModelIds.begin(), alreadyRenderedModelIds.end(), attachmentObject->getWorldObjectID()) == alreadyRenderedModelIds.end() && attachmentObject->hasTag(renderTag)) {
        alreadyRenderedModelIds.emplace_back(attachmentObject->getWorldObjectID());
        if(attachmentObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
            static_cast<Model *>(attachment)->convertToRenderList(0,0).render(graphicsWrapper, renderProgram);
        }
    }
    for (const auto &child: children) {
        PhysicalRenderable* physChild = dynamic_cast<PhysicalRenderable*>(child);
        if(physChild != nullptr) {
            renderPlayerAttachmentsRecursiveByTag(physChild, renderTag, renderProgram, alreadyRenderedModelIds);
        }
    }
 }

void World::renderSky(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const {
   PROFILE_RENDERING("World::renderSky");
   if (sky != nullptr) {
       sky->renderWithProgram(renderProgram, 0);
   }
}

void World::renderLight(unsigned int lightIndex, unsigned int renderLayer, const std::shared_ptr<GraphicsProgram> &renderProgram, const std::vector<HashUtil::HashedString> &tags) const {
    PROFILE_RENDERING("World::renderLight");
    Light* selectedLight = activeLights[lightIndex];
    Camera* lightCamera = selectedLight->getCameras()[renderLayer];
    if (lightCamera->isDirty()) {
        //TODO: following check fixes the point lights, but it is a hack, meaning we need to handle this properly.
         if (lightCamera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
        graphicsWrapper->clearDepthBuffer();
        }
        renderProgram->setUniform("renderLightIndex", (int) lightIndex);
        renderProgram->setUniform("renderLightLayer", (int) renderLayer);
        std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>* cullingResult = visibilityManager->getCullingResults().at(lightCamera);
        for (const auto& iterator:*cullingResult) {
            if (!VisibilityRequest::vectorComparator(iterator.first, tags)) {
                continue;
            }
            const RenderList& renderList = iterator.second;
            renderList.render(graphicsWrapper, renderProgram);
        }
        lightCamera->clearDirty();
    }
}

/**
 * This method checks if we are in editor mode, and if we are, enables ImGui windows
 * It also fills the windows with relevant parameters.
 */
void World::ImGuiFrameSetup(std::shared_ptr<GraphicsProgram> graphicsProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) {//TODO not const because it removes the object. Should be separated
   PROFILE_RENDERING("World::ImGuiFrameSetup");
   if(!currentPlayersSettings->editorShown) {
       return;
   }
   delete editor->request;
   Editor* editorPtr = editor.get();
   GenerateEditorElementsCallback generateEditorElementsForParameters = [editorPtr](std::vector<LimonTypes::GenericParameter> &parameters, uint32_t index) {
       return editorPtr->generateEditorElementsForParameters(parameters, index);
   };
   editor->request = new ImGuiRequest(playerCamera->getCameraMatrix(), playerCamera->getProjectionMatrix(),
                              graphicsWrapper->getGUIOrthogonalProjectionMatrix(), options->getScreenHeight(), options->getScreenWidth(), playerCamera, generateEditorElementsForParameters, editor->imgGuiHelper);

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

       playerPlaceHolder->getTransformation()->setTransformations(physicalPlayer->getPosition()
       ,physicalPlayer->getLookDirectionQuaternion());
       graphicsProgram->setUniform("renderModelIMGUI", 1);
       playerPlaceHolder->convertToRenderList(0,0).render(graphicsWrapper, graphicsProgram);
       graphicsProgram->setUniform("renderModelIMGUI", 0);
   }
   editor->renderEditor(graphicsProgram);
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
   animationStatusToRemove->object->getTransformation()->setTransformations(tempTranslate
   , tempScale
   , tempOrientation);
   animationStatusToRemove->object->setCustomAnimation(false);

   //now remove active animations
   Renderable* objectOfAnimation = animationStatusToRemove->object;
    delete activeAnimations[objectOfAnimation];
   activeAnimations.erase(objectOfAnimation);

   if(onLoadAnimations.find(objectOfAnimation) != onLoadAnimations.end()) {
       onLoadAnimations.erase(objectOfAnimation);
   }
}

World::~World() {
    // Stop visibility threads, otherwise race condition.
    visibilityManager->stop();

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
            visibilityManager->removeCamera(camera);
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
    delete apiAccessor;

}

bool World::addModelToWorld(Model *xmlModel) {
    if(objects.find(xmlModel->getWorldObjectID()) != objects.end()) {
        //the object is already registered. fail
        return false;
    }
    xmlModel->getTransformation()->getWorldTransform();
    objects[xmlModel->getWorldObjectID()] = xmlModel;
    if (xmlModel->isAnimated()) {
        xmlModel->setRigId(getNextRigId());
    }
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
    light->setFrustumChanged(true);//ensure updateActiveLights picks it up even if player hasn't moved
    //we don't add it to visibility manager threads, because we don't know if it will activate or not.
    updateActiveLights(false);
}

   void World::setupRender() {
    for (auto& boneTransformPair: changedBoneTransforms) {
        graphicsWrapper->setBoneTransforms(boneTransformPair.first, *(boneTransformPair.second));
    }
    changedBoneTransforms.clear();
   }

void World::afterLoadFinished() {
    alHelper->setDistanceModel(soundDistanceModel);
    visibilityManager->onPipelineChange();
    for (size_t i = 0; i < onLoadActions.size(); ++i) {
        if(onLoadActions[i]->action == nullptr) {
            std::cerr << "There was an onload action defined but action is not loaded, skipping." << std::endl;
            continue;
        }
        if(onLoadActions[i]->enabled) {
            std::cout << "running trigger " << onLoadActions[i]->action->getName() << std::endl;
            onLoadActions[i]->action->run(onLoadActions[i]->action->getParameters());
        }
    }

    if(music != nullptr) {
        music->play();
    }

    if(!startingPlayer.extensionName.empty()) {
        PlayerExtensionInterface *playerExtension = PlayerExtensionInterface::createExtension(startingPlayer.extensionName, apiInstance);
        if(playerExtension == nullptr) {
            std::cerr << "Player extension '" << startingPlayer.extensionName << "' not found. Is the correct trigger library loaded?" << std::endl;
        } else {
            playerExtension->setParameters(startingPlayer.parameters);
            this->currentPlayer->setPlayerExtension(playerExtension);
            this->currentPlayer->setCameraOverride(playerExtension->getCustomCameraAttachment());
            playerCamera->setCameraAttachment(currentPlayer->getCameraAttachment());
        }
    }

    for(auto& kv : sounds) {
        if(kv.second->isAutoPlay()) {
            kv.second->play();
        }
    }

    for(auto& kv : objects) {
        Model* model = dynamic_cast<Model*>(kv.second);
        if(model != nullptr && model->isAnimated()) {
            model->setupForTime(gameTime);
        }
    }

    this->visibilityManager->start();
}

void World::switchPlayer(Player *targetPlayer, InputHandler &inputHandler) {
    //we should reconnect disconnected object if switching to editor mode, because we use physics for pickup
    if(targetPlayer->getWorldSettings().editorShown && (currentPlayersSettings == nullptr ||!currentPlayersSettings->editorShown)) {
        //switching to editor shown. reconnect all disconnected objects, if no currentPlayer, starting with editor, reconnect
        for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
            apiAccessor->reconnectObjectToPhysics(*objectIt);
        }
    } else if(!targetPlayer->getWorldSettings().editorShown && (currentPlayersSettings  == nullptr || currentPlayersSettings->editorShown)) {
        //if exiting editor mode, or starting not editor mode
        for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
            apiAccessor->disconnectObjectFromPhysics(*objectIt);
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
                (*it)->getTransformation()->setTransformations(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
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
    visibilityManager->stop(); // Stop visibility threads when the world is paused or stopped
    if(this->music != nullptr) {
        this->music->pause();
    }
}

void World::setupForUnpause() {
    this->visibilityManager->start();
    if(this->music != nullptr) {
        this->music->resume();
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

   btVector3 World::extendRayToWorldAABB(glm::vec3 from, glm::vec3 direction) const {
       //we want to extend to vector to world AABB limit
       float maxFactor = 1; //don't allow making ray smaller than unit by setting 1.

       if (direction.x > 0) {
           //so we are looking at positive x. determine how many times the ray x we need
           maxFactor = std::max(maxFactor,(worldAABBMax.x - from.x) / direction.x);
       } else {
           maxFactor = std::max(maxFactor,(worldAABBMin.x - from.x) /
                                          direction.x); //Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
       }

       if (direction.y > 0) {
           maxFactor =std::max(maxFactor, (worldAABBMax.y - from.y) / direction.y);
       } else {
           maxFactor = std::max(maxFactor, (worldAABBMin.y - from.y) /
                                           direction.y);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
       }

       if (direction.z > 0) {
           maxFactor = std::max(maxFactor, (worldAABBMax.z - from.z) / direction.z);
       } else {
           maxFactor = std::max(maxFactor, (worldAABBMin.z - from.z) /
                                           direction.z);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
       }
       direction = direction * maxFactor;
       return GLMConverter::GLMToBlt(direction + from);
   }

GameObject* World::rayCastClosest(glm::vec3 from, glm::vec3 direction, int collisionType, int filterMask,
                                     glm::vec3 *collisionPosition, glm::vec3 *collisionNormal) const {
    btVector3 to = extendRayToWorldAABB(from, direction);
    btCollisionWorld::ClosestRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), to);
    RayCallback.m_collisionFilterGroup = collisionType;
    RayCallback.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(
            GLMConverter::GLMToBlt(from), to,
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

GameObject* World::rayCastClosestOther(const glm::vec3 from, const glm::vec3 direction, const int collisionType, const int filterMask,
                                        const GameObject* ignoreObject,
                                     glm::vec3 *collisionPosition, glm::vec3 *collisionNormal) const {
    btVector3 to = extendRayToWorldAABB(from, direction);
    btCollisionWorld::AllHitsRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), to);
    RayCallback.m_collisionFilterGroup = collisionType;
    RayCallback.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(
            GLMConverter::GLMToBlt(from), to,
            RayCallback
    );

    float closestDistance = std::numeric_limits<float>::max();
    size_t closestIndex = std::numeric_limits<size_t>::max();
    if (RayCallback.hasHit()) {
        for (int i = 0; i < RayCallback.m_collisionObjects.size(); ++i) {
            if (RayCallback.m_collisionObjects[i]->getUserPointer() != nullptr) {
                GameObject* hitObject = static_cast<GameObject *>(RayCallback.m_collisionObjects[i]->getUserPointer());
                if(hitObject->getWorldObjectID() != ignoreObject->getWorldObjectID()) {
                    float distance = glm::distance2(from, GLMConverter::BltToGLM(RayCallback.m_hitPointWorld[i]));
                    if(distance < closestDistance) {
                        closestDistance = distance;
                        closestIndex = i;
                    }
                }
            }
        }
        if(closestIndex == std::numeric_limits<size_t>::max()) {
            return nullptr;
        }
        *collisionPosition = GLMConverter::BltToGLM(RayCallback.m_hitPointWorld[closestIndex]);
        *collisionNormal = GLMConverter::BltToGLM(RayCallback.m_hitNormalWorld[closestIndex]);
        return static_cast<GameObject *>(RayCallback.m_collisionObjects[closestIndex]->getUserPointer());
    }
    return nullptr;
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
        return rayCastClosest(from, lookDirection, collisionType, filterMask, collisionPosition, collisionNormal);
    }
}

void World::checkAndRunTimedEvents() {
    while(!timedEvents.empty()) {
        const auto& event = timedEvents.top();

        // Check if the event should run
        if ((event.useWallTime && event.callTime <= wallTime) ||
            (!event.useWallTime && event.callTime <= gameTime)) {
            // Run the event
            event.run();
            // Remove the event from the queue
            timedEvents.pop();
            } else {
                // No more events to process
                break;
            }
    }
}

   Model *World::findModelByID(uint32_t modelID) const {
    if(startingPlayer.attachedModel != nullptr) {
        Model * playerAttachment = dynamic_cast<Model *>(findAttachableInSubtree(startingPlayer.attachedModel, modelID));
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

Attachable* World::findAttachableInSubtree(Attachable *root, uint32_t objectID) {
    if(root == nullptr) return nullptr;
    const GameObject* go = dynamic_cast<const GameObject*>(root);
    if(go != nullptr && go->getWorldObjectID() == objectID) return root;
    for(Attachable* child : root->getChildren()) {
        Attachable* found = findAttachableInSubtree(child, objectID);
        if(found != nullptr) return found;
    }
    return nullptr;
}

Attachable* World::findAttachableByID(uint32_t objectID) const {
    if(startingPlayer.attachedModel != nullptr) {
        Attachable* playerAttachment = findAttachableInSubtree(startingPlayer.attachedModel, objectID);
        if(playerAttachment != nullptr) {
            return playerAttachment;
        }
    }

    auto objectIt = objects.find(objectID);
    if(objectIt != objects.end()) {
        return objectIt->second;
    }
    auto groupIt = modelGroups.find(objectID);
    if(groupIt != modelGroups.end()) {
        return groupIt->second;
    }
    auto triggerIt = triggers.find(objectID);
    if(triggerIt != triggers.end()) {
        return triggerIt->second;
    }
    auto emitterIt = emitters.find(objectID);
    if(emitterIt != emitters.end()) {
        return emitterIt->second.get();
    }
    auto gpuEmitterIt = gpuParticleEmitters.find(objectID);
    if(gpuEmitterIt != gpuParticleEmitters.end()) {
        return gpuEmitterIt->second.get();
    }
    for(Light* light : lights) {
        if(light->getWorldObjectID() == objectID) {
            return light;
        }
    }
    auto soundIt = sounds.find(objectID);
    if(soundIt != sounds.end()) {
        return soundIt->second.get();
    }
    return nullptr;
}

void World::addSound(Sound* sound) {
    sounds[sound->getWorldObjectID()] = std::unique_ptr<Sound>(sound);
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
    if(forceUpdate) {
        //force update means a light was removed, so if directional light exists, it is in wrong index now. find and update
        for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
            if (lights[lightIndex]->getLightType() == Light::LightTypes::DIRECTIONAL) {
                directionalLightIndex = lightIndex;
                break;
            }
        }
    } else {
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
    std::vector<Light *> previousActiveLights = activeLights;
    activeLights.clear();
    std::vector<Light *> culledPointLights;
    for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
        Light *currentLight = lights[lightIndex];
        if (currentLight->getLightType() == Light::LightTypes::POINT) {
            if (playerCamera->isVisible(currentLight->getPosition(), currentLight->getActiveDistance())) {
                culledPointLights.emplace_back(currentLight);
            }
        }
    }

    if (culledPointLights.size() <= static_cast<size_t>(maxLightsOption.getOrDefault(4) - 1)) {
        activeLights.insert(activeLights.end(), culledPointLights.begin(), culledPointLights.end());
        if (directionalLightIndex != -1) {
            activeLights.emplace_back(lights[directionalLightIndex]);
        }
    } else {
        uint32_t pointLightCount = maxLightsOption.getOrDefault(4);
        if (directionalLightIndex != -1) {
            pointLightCount--;
        }
        //this is the case we can't actually activate all the point light, sort and only activate the closest ones
        std::sort(culledPointLights.begin(), culledPointLights.end(), LightCloserToPlayer(currentPlayer->getPosition()));
        for (uint32_t i = 0; i < pointLightCount; ++i) {
            activeLights.emplace_back(culledPointLights[i]);
        }
        if (directionalLightIndex != -1) {
            activeLights.emplace_back(lights[directionalLightIndex]);
        }
    }

    // Now update visibility manager with new lights we just set
    //Removal part
    for (Light* light : previousActiveLights) {
        bool stillActive = false;
        for (Light* newLight : activeLights) {
            if (newLight == light) { stillActive = true; break; }
        }
        if (!stillActive) {
            for (Camera* camera : light->getCameras()) {
                visibilityManager->removeCamera(camera);
            }
        }
    }
    //Adding part
    for (Light* light : activeLights) {
        bool wasActive = false;
        for (Light* oldLight : previousActiveLights) {
            if (oldLight == light) { wasActive = true; break; }
        }
        if (!wasActive) {
            for (Camera* camera : light->getCameras()) {
                visibilityManager->addCamera(camera);
            }
            light->setFrustumChanged(true);
        }
    }
}
void World::uploadActiveLightsToGPU() const {
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

    for (int i = activeLights.size(); i < static_cast<int>(maxLightsOption.getOrDefault(4)); ++i) {
        graphicsWrapper->removeLight(i);
    }
}

   void World::clearWorldRefsBeforeAttachment(PhysicalRenderable *attachment, const bool removeChildren) {
       Model *modelToClear = dynamic_cast<Model *>(attachment);
       if (modelToClear != nullptr) {
           modelToClear->disconnectFromPhysicsWorld(dynamicsWorld);
           for (auto iterator = rigidBodies.begin(); iterator != rigidBodies.end(); ++iterator) {
               if ((*iterator) == attachment->getRigidBody()) {
                   rigidBodies.erase(iterator);
                   break;
               }
           }

           //disconnect AI
           if (modelToClear->getAIID() != 0) {
               unusedIDs.push(modelToClear->getAIID());
               actors.erase(modelToClear->getAIID());
           }
           //remove any active animations
           if (activeAnimations.find(modelToClear) != activeAnimations.end()) {
               delete activeAnimations[modelToClear];
               activeAnimations.erase(modelToClear);
           }
           onLoadAnimations.erase(modelToClear);

           //of course we need to remove from the tag visibility lists too
           for (auto &perCameraVisibility: visibilityManager->getCullingResults()) {
               for (auto perTagVisibilityIt = perCameraVisibility.second->begin(); perTagVisibilityIt != perCameraVisibility.second->end(); ++perTagVisibilityIt) {
                   perTagVisibilityIt->second.removeModelFromAll(modelToClear->getWorldObjectID());
               }
           }
           //remove its children
           if (removeChildren) {
               std::vector<Attachable*> children(objects[modelToClear->getWorldObjectID()]->getChildren());
               for (auto child = children.begin(); child != children.end(); ++child) {
                   Model *model = dynamic_cast<Model *>(*child);
                   if (model != nullptr) {
                       clearWorldRefsBeforeAttachment(model, removeChildren);
                   }
               }
           }
           //clear object itself
           objects.erase(modelToClear->getWorldObjectID());
           unusedIDs.push(modelToClear->getWorldObjectID());
       }
   }

bool World::addPlayerAttachmentUsedIDs(const Attachable *attachment, std::set<uint32_t> &usedIDs,
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

bool World::isIDUsed(uint32_t id) const {
    if (id < 2) return true; // 0 is invalid, 1 is reserved for the physical player
    if (sky != nullptr && sky->getWorldObjectID() == id) return true;
    if (objects.count(id)) return true;
    if (triggers.count(id)) return true;
    if (actors.count(id)) return true;
    if (guiElements.count(id)) return true;
    if (modelGroups.count(id)) return true;
    for (const Light* l : lights) {
        if (l->getWorldObjectID() == id) return true;
    }
    if (sounds.count(id)) return true;
    return false;
}


bool World::verifyIDs() {
    // Rebuild the free pool from the actual world state on every call. verifyIDs() runs once before
    // sounds/lights are loaded (so their IDs would otherwise be seeded as free gaps) and again after
    // the world is complete; without this reset the stale gaps from the first pass survive into the
    // live pool and get handed out by getNextObjectID(), producing duplicate IDs that then get saved.
    std::queue<uint32_t>().swap(unusedIDs);
    std::set<uint32_t > usedIDs;
    uint32_t maxID = 0;
    usedIDs.insert(1);//reserved for physicalPlayer
    /** Places that have IDs:
     * 1) sky
     * 2) objects
     * 3) triggers
     * 4) actors
     * 5) GUI elements
     * 6) model groups
     * 7) player attachments
     * 8) lights
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

    for (Light* light : lights) {
        uint32_t lightID = light->getWorldObjectID();
        auto result = usedIDs.insert(lightID);
        if (!result.second) {
            std::cerr << "world ID repetition on light detected! with id " << lightID << std::endl;
            return false;
        }
        maxID = std::max(maxID, lightID);
    }

    for (auto& kv : sounds) {
        auto result = usedIDs.insert(kv.first);
        if (!result.second) {
            std::cerr << "world ID repetition on sound detected! with id " << kv.first << std::endl;
            return false;
        }
        maxID = std::max(maxID, kv.first);
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
               std::shared_ptr<GraphicsProgram> foundProgram;
               if (geometryShaderNode != nullptr) {
                   foundProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                                    vertexShaderNode->fullPath,
                                                                    geometryShaderNode->fullPath,
                                                                    fragmentShaderNode->fullPath);
               } else {
                   foundProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                                    vertexShaderNode->fullPath,
                                                                    fragmentShaderNode->fullPath);
               }
               programs.emplace_back(foundProgram);
           }
       }

   }

void World::onModelMaterialChanged(uint32_t modelID) {
    for (auto &perCameraVisibility: visibilityManager->getCullingResults()) {
        for (auto perTagVisibilityIt = perCameraVisibility.second->begin(); perTagVisibilityIt != perCameraVisibility.second->end(); ++perTagVisibilityIt) {
            perTagVisibilityIt->second.removeModelFromAll(modelID);
        }
    }
}
