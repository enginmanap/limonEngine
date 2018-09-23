//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "AI/HumanEnemy.h"

#include "Camera.h"
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



const std::map<World::PlayerTypes::Types, std::string> World::PlayerTypes::typeNames =
        {
                { Types::PHYSICAL_PLAYER, "Physical"},
                { Types::DEBUG_PLAYER, "Debug"},
                { Types::EDITOR_PLAYER, "Editor"},
                { Types::MENU_PLAYER, "Menu" }
        };

World::World(const std::string &name, PlayerTypes startingPlayerType, InputHandler *inputHandler,
             AssetManager *assetManager, Options *options)
        : assetManager(assetManager),options(options), glHelper(assetManager->getGlHelper()), alHelper(assetManager->getAlHelper()), name(name), fontManager(glHelper), startingPlayer(startingPlayerType) {

    strncpy(worldSaveNameBuffer, name.c_str(), sizeof(worldSaveNameBuffer) -1 );

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
    debugDrawer = new BulletDebugDrawer(glHelper, options);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);


    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Engine/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Engine/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Engine/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Engine/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Engine/Shaders/ShadowMap/fragmentPoint.glsl", false);


    apiGUILayer = new GUILayer(glHelper, debugDrawer, 1);
    apiGUILayer->setDebug(false);

    renderCounts = new GUIText(glHelper, getNextObjectID(), "Render Counts",
                               fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0", glm::vec3(204, 204, 0));
    renderCounts->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 170, options->getScreenHeight() - 36), 0);

    cursor = new GUICursor(glHelper, assetManager, "./Data/Textures/crosshair.png");

    cursor->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), 0);

    debugOutputGUI = new GUITextDynamic(glHelper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16),
                                        glm::vec3(0, 0, 0), 640, 380, options);
    debugOutputGUI->set2dWorldTransform(glm::vec2(320, options->getScreenHeight()-200), 0.0f);


    switch(startingPlayer.type) {
        case PlayerTypes::Types::PHYSICAL_PLAYER:
            physicalPlayer = new PhysicalPlayer(options, cursor);
            currentPlayer = physicalPlayer;
            break;
        case PlayerTypes::Types::DEBUG_PLAYER:
            debugPlayer = new FreeMovingPlayer(options, cursor);
            currentPlayer = debugPlayer;
            break;
        case PlayerTypes::Types::EDITOR_PLAYER:
            editorPlayer = new FreeCursorPlayer(options, cursor);
            currentPlayer = editorPlayer;
            break;
        case PlayerTypes::Types::MENU_PLAYER:
            menuPlayer = new MenuPlayer(options, cursor);
            currentPlayer = menuPlayer;
            break;
    }

    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    camera = new Camera(options, currentPlayer->getCameraAttachment());//register is just below
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, worldAABBMin, worldAABBMax);
    switchPlayer(currentPlayer, *inputHandler); //switching to itself, to set the states properly. It uses camera so done after camera creation




    fpsCounter = new GUIFPSCounter(glHelper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                                   glm::vec3(204, 204, 0));
    fpsCounter->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);

    onLoadActions.push_back(new ActionForOnload());//this is here for editor, as if no action is added, editor would fail to allow setting the first one.

    modelIndicesBuffer.reserve(NR_MAX_MODELS);
    modelsInLightFrustum.resize(NR_POINT_LIGHTS);
    animatedModelsInLightFrustum.resize(NR_POINT_LIGHTS);

    /************ ImGui *****************************/
    // Setup ImGui binding
    imgGuiHelper = new ImGuiHelper(glHelper, options);

    //setup request
    request = new GameObject::ImGuiRequest(glHelper->getCameraMatrix(), glHelper->getProjectionMatrix(),
            glHelper->getOrthogonalProjectionMatrix(), options->getScreenHeight(), options->getScreenWidth());
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

     //debugDrawer->flushDraws();
     if (RayCallback.hasHit()) {
         bool hasSeen = false;
         for (int i = 0; i < RayCallback.m_collisionObjects.size(); ++i) {
             GameObject *gameObject = static_cast<GameObject *>(RayCallback.m_collisionObjects[i]->getUserPointer());
             if (gameObject->getTypeID() != GameObject::PLAYER && gameObject->getTypeID() != GameObject::TRIGGER && //trigger is ghost, so it should not block
                 gameObject->getName() != fromName) {
                 return false;
             }
             if(gameObject->getTypeID() == GameObject::PLAYER) {
                 hasSeen = true;
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
 void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {


     // If not in editor mode, dont let imgGuiHelper get input
     // if in editor mode, but player press editor button, dont allow imgui to process input
     // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
     if(!currentPlayersSettings->editorShown || inputHandler.getInputEvents(InputHandler::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
         if(handlePlayerInput(inputHandler)) {
             handleQuitRequest();
             return;
         }
     }

     if(currentPlayersSettings->worldSimulation) {
        //every time we call this method, we increase the time only by simulationTimeframe
        gameTime += simulationTimeFrame;
        dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
        currentPlayer->processPhysicsWorld(dynamicsWorld);

        for(auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
            trigger->second->checkAndTrigger();
        }

        // ATTENTION iterator is not increased in for, it is done manually.
        for(auto animIt = activeAnimations.begin(); animIt != activeAnimations.end();) {
            AnimationStatus* animationStatus = &(animIt->second);
            const AnimationCustom* animationCustom = &loadedAnimations[animationStatus->animationIndex];
            if((animationStatus->loop ) || animationCustom->getDuration() / animationCustom->getTicksPerSecond() * 1000  + animationStatus->startTime > gameTime) {

                float ticksPerSecond;
                if (animationCustom->getTicksPerSecond() != 0) {
                    ticksPerSecond = animationCustom->getTicksPerSecond();
                } else {
                    ticksPerSecond = 60.0f;
                }
                float animationTime = fmod(((gameTime - animationStatus->startTime) / 1000.0f) * ticksPerSecond, animationCustom->getDuration());
                bool isFound;
                Transformation tf = animationCustom->calculateTransform("", animationTime, isFound);

                //FIXME this is not an acceptable animating technique, I need a transform stack, but not implemented it yet.
                (*animationStatus->object->getTransformation()) = animationStatus->originalTransformation;
                animationStatus->object->getTransformation()->addOrientation(tf.getOrientation());
                animationStatus->object->getTransformation()->addScale(tf.getScale());
                animationStatus->object->getTransformation()->addTranslate(tf.getTranslate());
                if(animationStatus->sound) {
                    animationStatus->sound->setWorldPosition(
                            animationStatus->object->getTransformation()->getTranslate());
                }
                animIt++;
            } else {
                if(!animationStatus->wasKinematic) {
                    animationStatus->object->getRigidBody()->setCollisionFlags(animationStatus->object->getRigidBody()->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                    animationStatus->object->getRigidBody()->setActivationState(ACTIVE_TAG);
                }

                if(animationStatus->sound) {
                    animationStatus->sound->stop();
                }
                options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_DEBUG, "Animation " + animationCustom->getName() +" finished, removing. ");
                animIt = activeAnimations.erase(animIt);

            }
        }

        for (auto actorIt = actors.begin(); actorIt != actors.end(); ++actorIt) {
            ActorInformation information = fillActorInformation(actorIt->second);
            actorIt->second->play(gameTime, information, options);
        }
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (!it->second->getRigidBody()->isStaticOrKinematicObject() && it->second->getRigidBody()->isActive()) {
                it->second->updateTransformFromPhysics();
                Model* model = dynamic_cast<Model*>(it->second);
                assert(model!= nullptr);
                updatedModels.push_back(model);
            }
        }

         fillVisibleObjects();

         for (auto modelAssetIterator = modelsInCameraFrustum.begin();
              modelAssetIterator != modelsInCameraFrustum.end(); ++modelAssetIterator) {
             for (auto modelIterator = modelAssetIterator->second.begin();
                  modelIterator != modelAssetIterator->second.end(); ++modelIterator) {
                 (*modelIterator)->setupForTime(gameTime);
             }
         }

         for (auto modelIt = animatedModelsInAnyFrustum.begin(); modelIt != animatedModelsInAnyFrustum.end(); ++modelIt) {
             (*modelIt)->setupForTime(gameTime);
         }

    } else {

         fillVisibleObjects();
    }

     if(camera->isDirty()) {
         glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());//this is required for any render
         alHelper->setListenerPositionAndOrientation(camera->getPosition(), camera->getCenter(), camera->getUp());
     }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }
    debugOutputGUI->setupForTime(gameTime);

    if(currentPlayersSettings->menuInteraction) {
        GUIButton* button = nullptr;

        GameObject* pointed = this->getPointedObject();
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
            if(inputHandler.getInputStatus(InputHandler::MOUSE_BUTTON_LEFT)) {
                if(inputHandler.getInputEvents(InputHandler::MOUSE_BUTTON_LEFT)) {
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

void World::fillVisibleObjects(){
    if(camera->isDirty()) {
        modelsInCameraFrustum.clear();
        animatedModelsInFrustum.clear();
        for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
            setVisibilityAndPutToSets(objectIt->second, false);
        }
    } else {
        //if camera frustum not changed, but object itself changed case
        for (size_t i = 0; i < updatedModels.size(); ++i) {
            setVisibilityAndPutToSets(updatedModels[i], true);
        }
    }

    for (size_t currentLightIndex = 0; currentLightIndex < lights.size(); ++currentLightIndex) {
        if(lights[currentLightIndex]->isFrustumChanged()) {
            modelsInLightFrustum[currentLightIndex].clear();
            animatedModelsInLightFrustum[currentLightIndex].clear();
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
                setLightVisibilityAndPutToSets(currentLightIndex, objectIt->second, false);
            }
            lights[currentLightIndex]->setFrustumChanged(false);
        } else {
            //if camera frustum not changed, but object itself changed case
            for (size_t i = 0; i < updatedModels.size(); ++i) {
                setLightVisibilityAndPutToSets(currentLightIndex, updatedModels[i], true);
            }
        }
    }

    updatedModels.clear();
}

void World::setLightVisibilityAndPutToSets(size_t currentLightIndex, PhysicalRenderable *PhysicalRenderable, bool removePossible) {
    Model* currentModel = dynamic_cast<Model*>(PhysicalRenderable);
    assert(currentModel != nullptr);
    currentModel->setIsInLightFrustum(currentLightIndex,
                                                  lights[currentLightIndex]->isShadowCaster(currentModel->getAabbMin(),
                                                                                            currentModel->getAabbMax(),
                                                                                            currentModel->getTransformation()->getTranslate()));
    if(currentModel->isInLightFrustum(currentLightIndex)) {
        if(currentModel->isAnimated()) {
            animatedModelsInLightFrustum[currentLightIndex].insert(currentModel);
            animatedModelsInAnyFrustum.insert(currentModel);
        } else {
            if (modelsInLightFrustum[currentLightIndex].find(currentModel->getAssetID()) ==
                modelsInLightFrustum[currentLightIndex].end()) {
                modelsInLightFrustum[currentLightIndex][currentModel->getAssetID()] = std::set<Model *>();
            }
            modelsInLightFrustum[currentLightIndex][currentModel->getAssetID()].insert(currentModel);
        }
    } else if(removePossible) {
        //if remove possible, and not in light frustum, search for the model, and remove
        if(currentModel->isAnimated()) {
            bool isInAnyFrustum = false;
            animatedModelsInLightFrustum[currentLightIndex].erase(currentModel);
            //now check if it is in any other frustums
            if(animatedModelsInFrustum.find(currentModel) == animatedModelsInFrustum.end()) {
                for (uint32_t i = 0; i < animatedModelsInLightFrustum.size(); ++i) {
                    if(animatedModelsInLightFrustum[i].find(currentModel) != animatedModelsInLightFrustum[i].end()) {
                        isInAnyFrustum = true;
                        break;
                    }
                }
            }
            if(!isInAnyFrustum) {
                animatedModelsInAnyFrustum.erase(currentModel);
            }
        } else {
            //if not animated
            modelsInLightFrustum[currentLightIndex][currentModel->getAssetID()].erase(currentModel);
        }
    }
}

void World::setVisibilityAndPutToSets(PhysicalRenderable *PhysicalRenderable, bool removePossible) {
    Model* currentModel = dynamic_cast<Model*>(PhysicalRenderable);
    assert(currentModel != nullptr);
    currentModel->setIsInFrustum(glHelper->isInFrustum(currentModel->getAabbMin(), currentModel->getAabbMax()));
    if(currentModel->isIsInFrustum()) {
        if(currentModel->isAnimated()) {
            animatedModelsInFrustum.insert(currentModel);
            animatedModelsInAnyFrustum.insert(currentModel);
        } else {
            if (modelsInCameraFrustum.find(currentModel->getAssetID()) == modelsInCameraFrustum.end()) {
                modelsInCameraFrustum[currentModel->getAssetID()] = std::set<Model *>();
            }
            modelsInCameraFrustum[currentModel->getAssetID()].insert(currentModel);
        }
    } else if(removePossible) {
        //if remove possible, and not in frustum, search for the model, and remove
        if(currentModel->isAnimated()) {
            bool isInAnyFrustum = false;
            animatedModelsInFrustum.erase(currentModel);
            //now check if it is in any other frustums
            for (uint32_t i = 0; i < animatedModelsInLightFrustum.size(); ++i) {
                if(animatedModelsInLightFrustum[i].find(currentModel) != animatedModelsInLightFrustum[i].end()) {
                    isInAnyFrustum = true;
                    break;
                }
            }
            if(!isInAnyFrustum) {
                animatedModelsInAnyFrustum.erase(currentModel);
            }
        } else {
            //if not animated
            modelsInCameraFrustum[currentModel->getAssetID()].erase(currentModel);
        }
    }
}

ActorInformation World::fillActorInformation(Actor *actor) {
    ActorInformation information;
    information.canSeePlayerDirectly = checkPlayerVisibility(actor->getPosition()+ AIMovementGrid::floatingHeight, actor->getModel()->getName());
    glm::vec3 front = actor->getFrontVector();
    glm::vec3 rayDir = currentPlayer->getPosition() - actor->getPosition();
    float cosBetween = glm::dot(normalize(front), normalize(rayDir));
    information.cosineBetweenPlayer = cosBetween;
    information.playerDirection = normalize(rayDir);
    if(cosBetween > 0) {
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
    if(crossBetween.y > 0){
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
    if(crossBetween.x > 0){
            information.isPlayerUp = false;
            information.isPlayerDown = true;
        } else {
            information.isPlayerUp = true;
            information.isPlayerDown = false;
        }
    std::vector<glm::vec3> route;
    glm::vec3 playerPosWithGrid = currentPlayer->getPosition();
    bool isPlayerReachable = grid->setProperHeight(&playerPosWithGrid, AIMovementGrid::floatingHeight, 0.0f, dynamicsWorld);
    if(isPlayerReachable && grid->coursePath(actor->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0), playerPosWithGrid, actor->getWorldID(), &route)) {
        if (route.empty()) {
            information.toPlayerRoute = glm::vec3(0, 0, 0);
            information.canGoToPlayer = false;
        } else {
            //Normally, this information should be used for straightening the path, but not yet.
            information.toPlayerRoute = route[route.size() - 1] - actor->getPosition() - glm::vec3(0, 2.0f, 0);
            information.canGoToPlayer = true;
        }
    }
    return information;
}

bool World::handlePlayerInput(InputHandler &inputHandler) {
    if(inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_LEFT)) {
        if(inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_LEFT)) {
            GameObject *gameObject = getPointedObject();
            if (gameObject != nullptr) {
                pickedObject = gameObject;
            } else {
                pickedObject = nullptr;
            }
        }
    }


    if (inputHandler.getInputEvents(inputHandler.EDITOR) && inputHandler.getInputStatus(inputHandler.EDITOR)) {
        if(editorPlayer == nullptr) {
            editorPlayer = new FreeCursorPlayer(options, cursor);
            editorPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, worldAABBMin, worldAABBMax);

        }
        if(!currentPlayersSettings->editorShown) {
            switchPlayer(editorPlayer, inputHandler);
        } else {
            switchPlayer(beforePlayer, inputHandler);
        }
    }
    //if not in editor mode and press debug
    if (!currentPlayersSettings->editorShown && inputHandler.getInputEvents(inputHandler.DEBUG) && inputHandler.getInputStatus(inputHandler.DEBUG)) {
        if(currentPlayersSettings->debugMode != Player::DEBUG_ENABLED) {
            if(debugPlayer == nullptr) {
                debugPlayer = new FreeMovingPlayer(options, cursor);
                debugPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, worldAABBMin, worldAABBMax);

            }
            switchPlayer(debugPlayer, inputHandler);
        } else {
            if(physicalPlayer == nullptr) {
                physicalPlayer = new PhysicalPlayer(options, cursor);
                physicalPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, worldAABBMin, worldAABBMax);

            }
            switchPlayer(physicalPlayer, inputHandler);
        }
    }

    float xPosition, yPosition, xChange, yChange;
    if (inputHandler.getMouseChange(xPosition, yPosition, xChange, yChange)) {
        currentPlayer->rotate(xPosition, yPosition, xChange, yChange);
    }

    if (inputHandler.getInputEvents(inputHandler.RUN)) {
        if(inputHandler.getInputStatus(inputHandler.RUN)) {
            options->setMoveSpeed(Options::RUN);
        } else {
            options->setMoveSpeed(Options::WALK);
        }
    }

    PhysicalPlayer::moveDirections direction = PhysicalPlayer::NONE;
    //ignore if both are pressed.
    if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) !=
        inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD)) {
            direction = Player::FORWARD;
        } else {
            direction = Player::BACKWARD;
        }
    }
    if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT) != inputHandler.getInputStatus(inputHandler.MOVE_RIGHT)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT)) {
            if (direction == Player::FORWARD) {
                direction = Player::LEFT_FORWARD;
            } else if (direction == Player::BACKWARD) {
                direction = Player::LEFT_BACKWARD;
            } else {
                direction = Player::LEFT;
            }
        } else if (direction == Player::FORWARD) {
            direction = Player::RIGHT_FORWARD;
        } else if (direction == Player::BACKWARD) {
            direction = Player::RIGHT_BACKWARD;
        } else {
            direction = Player::RIGHT;
        }
    }

    if (inputHandler.getInputStatus(inputHandler.JUMP) && inputHandler.getInputEvents(inputHandler.JUMP)) {
        direction = Player::UP;
    }

    //if none, camera should handle how to get slower.
    currentPlayer->move(direction);

    if(inputHandler.getInputEvents(inputHandler.QUIT) &&  inputHandler.getInputStatus(inputHandler.QUIT)) {
        return true;
    } else {
        return false;
    }
}

GameObject * World::getPointedObject() const {
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
        RayCallback.m_collisionFilterGroup = COLLIDE_EVERYTHING;
        RayCallback.m_collisionFilterMask = ~(COLLIDE_NOTHING);

        dynamicsWorld->rayTest(
                GLMConverter::GLMToBlt(from),
                GLMConverter::GLMToBlt(to),
                RayCallback
        );

        //debugDrawer->flushDraws();
        if (RayCallback.hasHit()) {
            return static_cast<GameObject *>(RayCallback.m_collisionObject->getUserPointer());
        } else {
            return nullptr;
        }
    }
}

void World::render() {

    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::DIRECTIONAL) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapDirectional(i);
        //FIXME why are these set here?
        shadowMapProgramDirectional->setUniform("renderLightIndex", (int)i);

        for (auto modelIterator = modelsInLightFrustum[i].begin(); modelIterator != modelsInLightFrustum[i].end(); ++modelIterator) {
            //each iterator has a vector. each vector is a model that can be rendered instanced. They share is animated
            std::set<Model*> modelSet = modelIterator->second;
            modelIndicesBuffer.clear();
            Model* sampleModel = nullptr;
            for (auto model = modelSet.begin(); model != modelSet.end(); ++model) {
                sampleModel = *model;
                //all of these models will be rendered
                modelIndicesBuffer.push_back((*model)->getWorldObjectID());
            }
            if(sampleModel != nullptr) {
                sampleModel->renderWithProgramInstanced(modelIndicesBuffer, *shadowMapProgramDirectional);
            }
        }

        for (auto animatedModelIterator = animatedModelsInLightFrustum[i].begin(); animatedModelIterator != animatedModelsInLightFrustum[i].end(); ++animatedModelIterator) {
            std::vector<uint32_t > temp;
            temp.push_back((*animatedModelIterator)->getWorldObjectID());
            (*animatedModelIterator)->renderWithProgramInstanced(temp,*shadowMapProgramDirectional);
        }
    }

    glHelper->switchRenderToShadowMapPoint();
    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::POINT) {
            continue;
        }
        //FIXME why are these set here?
        shadowMapProgramDirectional->setUniform("renderLightIndex", (int)i);
        for (auto modelIterator = modelsInLightFrustum[i].begin(); modelIterator != modelsInLightFrustum[i].end(); ++modelIterator) {
            //each iterator has a vector. each vector is a model that can be rendered instanced. They share is animated
            std::set<Model*> modelSet = modelIterator->second;
            modelIndicesBuffer.clear();
            Model* sampleModel = nullptr;
            for (auto model = modelSet.begin(); model != modelSet.end(); ++model) {
                //all of these models will be rendered
                modelIndicesBuffer.push_back((*model)->getWorldObjectID());
                sampleModel = *model;
            }
            if(sampleModel != nullptr) {
                sampleModel->renderWithProgramInstanced(modelIndicesBuffer, *shadowMapProgramPoint);
            }
        }

        for (auto animatedModelIterator = animatedModelsInLightFrustum[i].begin(); animatedModelIterator != animatedModelsInLightFrustum[i].end(); ++animatedModelIterator) {
            std::vector<uint32_t > temp;
            temp.push_back((*animatedModelIterator)->getWorldObjectID());
            (*animatedModelIterator)->renderWithProgramInstanced(temp,*shadowMapProgramPoint);
        }
    }

    glHelper->switchRenderToDefault();
    if(sky!=nullptr) {
        sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    }

    for (auto modelIterator = modelsInCameraFrustum.begin(); modelIterator != modelsInCameraFrustum.end(); ++modelIterator) {
        //each iterator has a vector. each vector is a model that can be rendered instanced. They share is animated
        std::set<Model*> modelSet = modelIterator->second;
        modelIndicesBuffer.clear();
        Model* sampleModel = nullptr;
        for (auto model = modelSet.begin(); model != modelSet.end(); ++model) {
            //all of these models will be rendered
            modelIndicesBuffer.push_back((*model)->getWorldObjectID());
            sampleModel = *model;
        }
        if(sampleModel != nullptr) {
            sampleModel->renderInstanced(modelIndicesBuffer);
        }
    }

    for (auto modelIterator = animatedModelsInFrustum.begin(); modelIterator != animatedModelsInFrustum.end(); ++modelIterator) {
        std::vector<uint32_t > temp;
        temp.push_back((*modelIterator)->getWorldObjectID());
        (*modelIterator)->renderInstanced(temp);
    }

    dynamicsWorld->debugDrawWorld();
    if (this->dynamicsWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
        debugDrawer->drawLine(btVector3(0, 0, 0), btVector3(0, 250, 0), btVector3(1, 1, 1));
        //draw the ai-grid
        if(grid != nullptr) {
            grid->debugDraw(debugDrawer);
        }
    }

    if(currentPlayersSettings->editorShown) {
        for (auto it = triggers.begin(); it != triggers.end(); ++it) {
            it->second->render(debugDrawer);
        }
    }

    debugDrawer->flushDraws();


    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();

    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }
    renderCounts->render();
    cursor->render();
    debugOutputGUI->render();
    fpsCounter->render();

    //render API gui layer
    apiGUILayer->render();

    uint32_t triangle, line;
    glHelper->getRenderTriangleAndLineCount(triangle, line);
    renderCounts->updateText("Tris: " + std::to_string(triangle) + ", lines: " + std::to_string(line));
    if(currentPlayersSettings->editorShown) {
        ImGuiFrameSetup();
    }
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

/**
 * This method checks if we are in editor mode, and if we are, enables ImGui windows
 * It also fills the windows with relevant parameters.
 */
void World::ImGuiFrameSetup() {//TODO not const because it removes the object. Should be separated
    if(!availableAssetsLoaded) {
        assetManager->loadAssetList("./Data/AssetList.xml");
        availableAssetsLoaded = true;
    }
    imgGuiHelper->NewFrame();

    /* window definitions */
    {
        ImGui::Begin("Editor");
        if(guiPickMode == false) {
            if (ImGui::Button("Switch to GUI selection mode")) {
                this->guiPickMode = true;
            }
        } else {
            if (ImGui::Button("Switch to World selection mode")) {
                this->guiPickMode = false;
            }
        }

        //list available elements
        static std::string selectedAssetFile = "";
        glm::vec3 newObjectPosition = camera->getPosition() + 10.0f * camera->getCenter();

        if (ImGui::CollapsingHeader("Add New Object")) {
            if (ImGui::BeginCombo("Available objects", selectedAssetFile.c_str())) {
                for (auto it = assetManager->getAvailableAssetsList().begin();
                     it != assetManager->getAvailableAssetsList().end(); it++) {
                    bool selectedElement = selectedAssetFile == it->first;
                    if (ImGui::Selectable(it->first.c_str(), selectedElement)) {
                        selectedAssetFile = it->first;
                    }
                    if(selectedElement) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            static float newObjectWeight;
            ImGui::SliderFloat("Weight", &newObjectWeight, 0.0f, 100.0f);

            ImGui::NewLine();
            if(selectedAssetFile != "") {
                if(ImGui::Button("Add Object")) {
                    Model* newModel = new Model(this->getNextObjectID(), assetManager, newObjectWeight,
                                                selectedAssetFile, false);
                    newModel->getTransformation()->setTranslate(newObjectPosition);
                    this->addModelToWorld(newModel);
                    newModel->getRigidBody()->activate();
                    pickedObject = static_cast<GameObject*>(newModel);
                }
            }
        }
        if(pickedObject != nullptr && pickedObject->getTypeID() == GameObject::MODEL) {
            static float copyOffsets[3] { 0.25f, 0.25f, 0.25f};
            ImGui::DragFloat3("Copy position offsets", copyOffsets, 0.1f);
            if (ImGui::Button("Copy Selected object")) {

                Model* pickedModel = dynamic_cast<Model*>(pickedObject);
                Model* newModel = new Model(*pickedModel, this->getNextObjectID());
                newModel->getTransformation()->addTranslate(glm::vec3(copyOffsets[0], copyOffsets[1], copyOffsets[2]));
                addModelToWorld(newModel);
                //now we should apply the animations

                if(onLoadAnimations.find(pickedModel) != onLoadAnimations.end() &&
                        activeAnimations.find(pickedModel) != activeAnimations.end()) {
                    addAnimationToObject(newModel->getWorldObjectID(), activeAnimations[pickedModel].animationIndex,
                                         true, true);
                }
                pickedObject = static_cast<GameObject*>(newModel);
            }
        }


        if(ImGui::Button("Add Trigger Volume")) {

            TriggerObject* to = new TriggerObject(this->getNextObjectID(), this->apiInstance);
            to->getTransformation()->setTranslate(newObjectPosition);
            this->dynamicsWorld->addCollisionObject(to->getGhostObject(), COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, COLLIDE_PLAYER | COLLIDE_EVERYTHING);
            triggers[to->getWorldObjectID()] = to;

            pickedObject = static_cast<GameObject*>(to);
        }

        if (ImGui::CollapsingHeader("Add GUI Elements##The header")) {
            ImGui::Indent( 16.0f );
            if (ImGui::CollapsingHeader("Add GUI Layer##The header")) {
                addGUILayerControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Text##The header")) {
                addGUITextControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Image##The header")) {
                addGUIImageControls();
            }
            if (ImGui::CollapsingHeader("Add GUI Button##The header")) {
                addGUIButtonControls();
            }
            ImGui::Unindent( 16.0f );
        }


        if (ImGui::CollapsingHeader("Custom Animations")) {
            //list loaded animations
            int listbox_item_current = -1;//not static because I don't want user to select a item.
            ImGui::ListBox("Loaded animations", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&loadedAnimations), loadedAnimations.size(), 10);
            ImGui::Separator();


            ImGui::NewLine();
            static char loadAnimationNameBuffer[32];
            ImGui::Text("Load animation from file:");
            //double # because I don't want to show it
            ImGui::InputText("##LoadAnimationNameField", loadAnimationNameBuffer, sizeof(loadAnimationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);
            if (ImGui::Button("load animation")) {
                AnimationCustom *animation = AnimationLoader::loadAnimation("./Data/Animations/" + std::string(loadAnimationNameBuffer) + ".xml");
                if (animation == nullptr) {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation load failed");
                } else {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation loaded");
                    loadedAnimations.push_back(*animation);
                }
            }
        }
        ImGui::Separator();
        if(ImGui::CollapsingHeader("World properties")) {
            ImGui::Indent( 16.0f );

            if (ImGui::CollapsingHeader("Add on load trigger")) {
                static size_t onLoadTriggerIndex = 0;//maximum size
                std::string selectedID = std::to_string(onLoadTriggerIndex);
                if (ImGui::BeginCombo("Current Triggers", selectedID.c_str())) {
                    for (size_t i = 0; i < onLoadActions.size(); i++) {
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
                TriggerObject::PutTriggerInGui(apiInstance, onLoadActions[onLoadTriggerIndex]->action,
                                               onLoadActions[onLoadTriggerIndex]->parameters,
                                               onLoadActions[onLoadTriggerIndex]->enabled, 100 + onLoadTriggerIndex);
                if (onLoadActions[onLoadActions.size() - 1]->enabled) {
                    //when user presses the enable button, add another and select it
                    onLoadTriggerIndex = onLoadActions.size();
                    onLoadActions.push_back(new ActionForOnload());
                }
            }
            static char musicNameBuffer[128] = {};
            static bool musicRefresh = true;
            if (musicRefresh) {
                if (this->music != nullptr) {
                    if (this->music->getName().length() < sizeof(musicNameBuffer)) {
                        strcpy(musicNameBuffer, this->music->getName().c_str());
                    } else {
                        strncpy(musicNameBuffer, this->music->getName().c_str(), sizeof(musicNameBuffer) - 1);
                    }
                }
                musicRefresh = false;
            }
            ImGui::InputText("OnLoad Music", musicNameBuffer, 128);
            if (ImGui::Button("Change Music")) {
                musicRefresh = true;
                this->music->stop();
                delete this->music;
                this->music = new Sound(getNextObjectID(), assetManager, std::string(musicNameBuffer));
                this->music->setLoop(true);
                this->music->setWorldPosition(glm::vec3(0, 0, 0), true);
                this->music->play();
            }


            if (ImGui::BeginCombo("Starting Player Type", startingPlayer.toString().c_str())) {
                for (auto iterator = PlayerTypes::typeNames.begin();
                     iterator != PlayerTypes::typeNames.end(); ++iterator) {
                    bool isThisTypeSelected = iterator->second == startingPlayer.toString();
                    if (ImGui::Selectable(iterator->second.c_str(), isThisTypeSelected)) {
                        this->startingPlayer.setType(iterator->second);
                    }
                    if (isThisTypeSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Text("By default, esc quits the game");

            if (ImGui::RadioButton("Quit Game", currentQuitResponse == QuitResponse::QUIT_GAME)) {
                currentQuitResponse = QuitResponse::QUIT_GAME;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Return Previous", currentQuitResponse == QuitResponse::RETURN_PREVIOUS)) {
                currentQuitResponse = QuitResponse::RETURN_PREVIOUS;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Load World", currentQuitResponse == QuitResponse::LOAD_WORLD)) {
                currentQuitResponse = QuitResponse::LOAD_WORLD;
            }
            if(currentQuitResponse == QuitResponse::LOAD_WORLD) {
                ImGui::InputText("Custom World file ", quitWorldNameBuffer, sizeof(quitWorldNameBuffer));
                if(ImGui::Button("Apply##custom world file setting")) {
                    quitWorldName = quitWorldNameBuffer;
                }
            }

            ImGui::Unindent( 16.0f );

        }

        ImGui::Separator();

        ImGui::InputText("##save world name", worldSaveNameBuffer, sizeof(worldSaveNameBuffer));
        ImGui::SameLine();
        if(ImGui::Button("Save World")) {
            for(auto animIt = loadedAnimations.begin(); animIt != loadedAnimations.end(); animIt++) {
                if(animIt->serializeAnimation("./Data/Animations/")) {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation saved");
                } else {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation save failed");
                }

            }

            if(WorldSaver::saveWorld(worldSaveNameBuffer, this)) {
                options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "World save successful");
            } else {
                options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "World save Failed");
            }
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(0,0), true);//true means set it only once

        ImGui::Begin("Selected Object Properties");
        std::string selectedName;
        if(pickedObject == nullptr) {
            selectedName = "No object selected";
        } else {
            selectedName = pickedObject->getName().c_str();
        }
        if (ImGui::BeginCombo("PickedGameObject", selectedName.c_str())) {
            for (auto it = objects.begin(); it != objects.end(); it++) {
                GameObject* gameObject = dynamic_cast<GameObject *>(it->second);
                bool selectedElement = gameObject->getName() == selectedName;
                if (ImGui::Selectable(gameObject->getName().c_str(), selectedElement)) {
                    pickedObject = gameObject;
                }
                if(selectedElement) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            for (auto it = guiElements.begin(); it != guiElements.end(); it++) {
                GameObject* gameObject = dynamic_cast<GameObject *>(it->second);
                bool selectedElement = gameObject->getName() == selectedName;
                if (ImGui::Selectable(gameObject->getName().c_str(), selectedElement)) {
                    pickedObject = gameObject;
                }
                if(selectedElement) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            for (auto it = lights.begin(); it != lights.end(); it++) {
                GameObject* gameObject = dynamic_cast<GameObject *>(*it);
                bool selectedElement = gameObject->getName() == selectedName;
                if (ImGui::Selectable(gameObject->getName().c_str(), selectedElement)) {
                    pickedObject = (*it);
                }
                if(selectedElement) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            for (auto it = triggers.begin(); it != triggers.end(); it++) {
                GameObject* gameObject = dynamic_cast<GameObject *>(it->second);
                bool selectedElement = gameObject->getName() == selectedName;
                if (ImGui::Selectable(gameObject->getName().c_str(), selectedElement)) {
                    pickedObject = it->second;
                }
                if(selectedElement) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if(pickedObject != nullptr) {
            GameObject::ImGuiResult objectEditorResult = pickedObject->addImGuiEditorElements(*request);
            if(pickedObject->getTypeID() == GameObject::MODEL) {
                Model* selectedObject = dynamic_cast<Model*>(pickedObject);
                if(objectEditorResult.updated) {
                    updatedModels.push_back(selectedObject);
                }
                if(activeAnimations.find(selectedObject) != activeAnimations.end()) {
                    if(objectEditorResult.updated) {
                        activeAnimations[selectedObject].originalTransformation = *selectedObject->getTransformation();
                    }


                    if(ImGui::Button(("Remove custom animation: " + loadedAnimations[activeAnimations[selectedObject].animationIndex].getName()).c_str())) {
                        (*selectedObject->getTransformation()) = activeAnimations[selectedObject].originalTransformation;
                        activeAnimations.erase(selectedObject);
                        if(onLoadAnimations.find(selectedObject) != onLoadAnimations.end()) {
                            onLoadAnimations.erase(selectedObject);
                        }
                     }
                } else {
                    addAnimationDefinitionToEditor();
                }

                if (objectEditorResult.removeAI) {
                    //remove AI requested
                    if (dynamic_cast<Model *>(pickedObject)->getAIID() != 0) {
                        actors.erase(dynamic_cast<Model *>(pickedObject)->getAIID());
                        dynamic_cast<Model *>(pickedObject)->detachAI();
                    }
                }

                if (objectEditorResult.addAI) {
                    std::cout << "adding AI to model " << std::endl;
                    HumanEnemy *newEnemy = new HumanEnemy(getNextObjectID());
                    newEnemy->setModel(dynamic_cast<Model *>(pickedObject));

                    addActor(newEnemy);
                }

            }
            ImGui::NewLine();
            switch (pickedObject->getTypeID()) {
                case GameObject::MODEL: {
                    if (static_cast<Model *>(pickedObject)->isDisconnected()) {
                        if (ImGui::Button("reconnect to physics")) {
                            reconnectObjectToPhysics(static_cast<Model *>(pickedObject)->getWorldObjectID());
                        }
                    } else {
                        if (ImGui::Button("Disconnect from physics")) {
                            disconnectObjectFromPhysics(static_cast<Model *>(pickedObject)->getWorldObjectID());
                        }
                        ImGui::Text(
                                "If object is placed in trigger volume, \ndisconnecting drastically improve performance.");
                    }

                    if (ImGui::Button("Remove This Object")) {
                        removeObject(pickedObject->getWorldObjectID());
                        pickedObject = nullptr;
                    }
                }
                    break;
                case GameObject::TRIGGER: {
                    if (ImGui::Button("Remove This Trigger")) {
                        removeTriggerObject(pickedObject->getWorldObjectID());
                        pickedObject = nullptr;
                    }
                }
                break;
                case GameObject::GUI_TEXT:
                case GameObject::GUI_IMAGE:
                case GameObject::GUI_BUTTON: {
                    if(objectEditorResult.remove) {
                        this->guiElements.erase(pickedObject->getWorldObjectID());
                        delete pickedObject;
                        pickedObject = nullptr;
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
    imgGuiHelper->RenderDrawLists();

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
        guiLayers.push_back(new GUILayer(glHelper, debugDrawer, 10));
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
        GUIText *guiText = new GUIText(glHelper, getNextObjectID(), GUITextName,
                                       fontManager.getFont(selectedFontName, fontSize), "New Text", glm::vec3(0, 0, 0));
        guiText->set2dWorldTransform(
                glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
        guiElements[guiText->getWorldObjectID()] = guiText;
        guiLayers[selectedLayerIndex]->addGuiElement(guiText);
        pickedObject = guiText;
    }
}

void World::addAnimationDefinitionToEditor() {
    if (ImGui::CollapsingHeader("Custom animation properties")) {
        //If there is no animation setup ongoing, or there is one, but not for this model,
        //put start animation button.
        //else put time input, add and finalize buttons.
        if (animationInProgress == nullptr || animationInProgress->getAnimatingObject() != dynamic_cast<Model *>(pickedObject)) {

            static int listbox_item_current = 0;
            ImGui::Text("Loaded animations list");
            ImGui::ListBox("##Loaded animations listbox", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&loadedAnimations), loadedAnimations.size(), 10);

            if (ImGui::Button("Apply selected")) {
                addAnimationToObject(dynamic_cast<Model *>(pickedObject)->getWorldObjectID(), listbox_item_current,
                                     true, true);
            }

            ImGui::SameLine();
            if (ImGui::Button("Create new")) {
                if (animationInProgress == nullptr) {
                    animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<PhysicalRenderable *>(pickedObject));
                } else {
                    //ask for removal of the old work
                    delete animationInProgress;
                    animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<PhysicalRenderable *>(pickedObject));
                }
                // At this point we should know the animationInProgress is for current object
            }
        } else {
            ImGui::Text("Please use animation definition window.");
            bool finished, cancelled;
            animationInProgress->addAnimationSequencerToEditor(finished, cancelled);
            if (finished) {
                loadedAnimations.push_back(AnimationCustom(*animationInProgress->buildAnimationFromCurrentItems()));

                addAnimationToObject(dynamic_cast<Model *>(pickedObject)->getWorldObjectID(),
                                     loadedAnimations.size() - 1, true, true);
                delete animationInProgress;
                animationInProgress = nullptr;
            }
            if (cancelled) {
                delete animationInProgress;
                animationInProgress = nullptr;
            }
        }
    }
}

World::~World() {
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
    delete camera;
    delete physicalPlayer;
    delete debugPlayer;
    delete editorPlayer;
    delete menuPlayer;

    delete imgGuiHelper;
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
        dynamicsWorld->removeRigidBody(xmlModel->getRigidBody());
    } else {
        dynamicsWorld->addRigidBody(xmlModel->getRigidBody(), COLLIDE_MODELS, COLLIDE_MODELS | COLLIDE_PLAYER | COLLIDE_EVERYTHING);
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

void World::addActor(Actor *actor) {
    this->actors[actor->getWorldID()] = actor;
}

void World::createGridFrom(const glm::vec3 &aiGridStartPoint) {
    if(grid != nullptr) {
        delete grid;
    }
    grid = new AIMovementGrid(aiGridStartPoint, dynamicsWorld, worldAABBMin, worldAABBMax, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING);
}

void World::setSky(SkyBox *skyBox) {
    if(sky!= nullptr) {
        delete sky;
    }
    sky = skyBox;
}

void World::addLight(Light *light) {
    glHelper->setLight(*(light), lights.size());//since size start from 0, this should be before adding it to vector
    this->lights.push_back(light);
}

uint32_t World::addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                              const std::string *soundToPlay) {
    AnimationStatus as;
    as.object = objects[modelID];

    as.animationIndex = animationID;
    as.loop = looped;
    as.wasKinematic = as.object->getRigidBody()->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;
    as.startTime = gameTime;
    if(activeAnimations.count(as.object) != 0) {
        options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_WARN, "Model had custom animation, overriding.");
        as.originalTransformation = activeAnimations[as.object].originalTransformation;
    } else {
        as.originalTransformation = *(as.object->getTransformation());
    }

    as.object->getRigidBody()->setCollisionFlags(as.object->getRigidBody()->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    as.object->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);

    if(startOnLoad) {
        onLoadAnimations.insert(as.object);
        as.startTime = 0;
    }

    if(soundToPlay != nullptr) {
        as.sound = std::make_unique<Sound>(this->getNextObjectID(), assetManager, *soundToPlay);
        as.sound->setLoop(looped);
        as.sound->setWorldPosition(as.object->getTransformation()->getTranslate());
        as.sound->play();
    }
    activeAnimations[as.object] = std::move(as);
    return modelID;
}


bool
World::generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters, uint32_t index) {
    bool isAllSet = true;
    for (size_t i = 0; i < runParameters.size(); ++i) {
        LimonAPI::ParameterRequest& parameter = runParameters[i];

        switch(parameter.requestType) {
            case LimonAPI::ParameterRequest::RequestParameterTypes::MODEL: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
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
            case LimonAPI::ParameterRequest::RequestParameterTypes::ANIMATION: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
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
            case LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
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
            case LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
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
            case LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
                if (!parameter.isSet) {
                    isAllSet = false;
                }
                if (ImGui::InputText((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                     parameter.value.stringValue, sizeof(parameter.value.stringValue))) {
                    parameter.isSet = true;
                };
            }
                break;
            case LimonAPI::ParameterRequest::RequestParameterTypes::FREE_NUMBER: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
                if (!parameter.isSet) {
                    isAllSet = false;
                }
                int value = parameter.value.longValue;
                if (ImGui::DragInt((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(),
                                   &value, sizeof(parameter.value.longValue))) {
                    parameter.value.longValue = value;
                    parameter.isSet = true;
                };
            }
                break;
            case LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG_ARRAY;
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
        }
    }
    return isAllSet;
}

uint32_t World::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &name, const std::string &text,
                           const glm::vec3 &color,
                           const glm::vec2 &position, float rotation) {
    GUIText* tr = new GUIText(glHelper, getNextObjectID(), name, fontManager.getFont(fontFilePath, fontSize),
                              text, color);
    glm::vec2 screenPosition;
    screenPosition.x = position.x * this->options->getScreenWidth();
    screenPosition.y = position.y * this->options->getScreenHeight();

    tr->set2dWorldTransform(screenPosition, rotation);
    guiElements[tr->getWorldObjectID()] = tr;
    apiGUILayer->addGuiElement(tr);
    return tr->getWorldObjectID();

}

uint32_t World::removeGuiText(uint32_t guiElementID) {
    if(guiElements.find(guiElementID) != guiElements.end()) {
        delete guiElements[guiElementID];
        guiElements.erase(guiElementID);
        return 0;
    }
    return 1;
}

std::vector<LimonAPI::ParameterRequest> World::getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID) {
    std::vector<LimonAPI::ParameterRequest> result;
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
        return true;
    }
    return false;//not successful
}

bool World::removeObject(uint32_t objectID) {
    if(objects.find(objectID) != objects.end()) {
        PhysicalRenderable* objectToRemove = objects[objectID];
        dynamicsWorld->removeRigidBody(objectToRemove->getRigidBody());
        //disconnect AI
        if (dynamic_cast<Model *>(objectToRemove)->getAIID() != 0) {
            actors.erase(dynamic_cast<Model *>(objectToRemove)->getAIID());
        }
        //remove any active animations
        activeAnimations.erase(objectToRemove);
        onLoadAnimations.erase(objectToRemove);


        Model* modelToRemove = dynamic_cast<Model*>(objectToRemove);
        if(modelToRemove != nullptr) {
            //we need to remove from ligth frustum lists, and camera frustum lists
            if(modelToRemove->isAnimated()) {
                animatedModelsInFrustum.erase(modelToRemove);
                for (size_t i = 0; i < lights.size(); ++i) {
                    animatedModelsInLightFrustum[i].erase(modelToRemove);
                }

                animatedModelsInAnyFrustum.erase(modelToRemove);
            } else {
                modelsInCameraFrustum[modelToRemove->getAssetID()].erase(modelToRemove);
                for (size_t i = 0; i < lights.size(); ++i) {
                    modelsInLightFrustum[i][modelToRemove->getAssetID()].erase(modelToRemove);
                }
            }
        }

        //delete object itself
        delete objects[objectID];
        objects.erase(objectID);
        return true;
    }
    return false;//not successful
}

void World::afterLoadFinished() {
    for (size_t i = 0; i < onLoadActions.size(); ++i) {
        if(onLoadActions[i]->enabled) {
            std::cout << "running trigger " << onLoadActions[i]->action->getName() << std::endl;
            onLoadActions[i]->action->run(onLoadActions[i]->parameters);
        }
    }

    if(music != nullptr) {
        music->play();
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

bool World::attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    objects[objectWorldID]->setSoundAttachementAndPlay(std::move(std::make_unique<Sound>(getNextObjectID(), assetManager, soundPath)));
    return true;
}

bool World::detachSoundFromObject(uint32_t objectWorldID) {
    if(objects.find(objectWorldID) == objects.end()) {
        return false;//fail
    }
    objects[objectWorldID]->detachSound();
    return true;
}

uint32_t World::playSound(const std::string &soundPath, const glm::vec3 &position, bool looped) {
    std::unique_ptr<Sound> sound = std::make_unique<Sound>(getNextObjectID(), assetManager, soundPath);
    sound->setLoop(looped);
    sound->setWorldPosition(position, true);
    sound->play();
    uint32_t soundID = sound->getWorldObjectID();
    sounds[soundID] = std::move(sound);
    return soundID;
}

void World::addGUIImageControls() {
    /**
     * For a new GUI Image we need only name and filename
     */
    static char GUIImageName[32];
    ImGui::InputText("GUI Image Name", GUIImageName, sizeof(GUIImageName), ImGuiInputTextFlags_CharsNoBlank);

    static char GUIImageFileName[256];
    ImGui::InputText("Image path", GUIImageFileName, sizeof(GUIImageFileName));

    static size_t selectedLayerIndex = 0;
    if (guiLayers.size() == 0) {
        guiLayers.push_back(new GUILayer(glHelper, debugDrawer, 10));
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
    if (ImGui::Button("Add GUI Image")) {
        GUIImage *guiImage = new GUIImage(this->getNextObjectID(), options, assetManager, std::string(GUIImageName),
                                          std::string(GUIImageFileName));
        guiImage->set2dWorldTransform(
                glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
        guiElements[guiImage->getWorldObjectID()] = guiImage;
        guiLayers[selectedLayerIndex]->addGuiElement(guiImage);
        pickedObject = guiImage;
    }
}

void World::switchPlayer(Player *targetPlayer, InputHandler &inputHandler) {
    currentPlayersSettings = &(targetPlayer->getWorldSettings());

    setupForPlay(inputHandler);

    //now all settings done, switch player
    beforePlayer = currentPlayer;
    currentPlayer = targetPlayer;
    targetPlayer->ownControl(beforePlayer->getPosition(), beforePlayer->getLookDirection());

    dynamicsWorld->updateAabbs();
    camera->setCameraAttachment(currentPlayer->getCameraAttachment());

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
                (*(*it)->getTransformation()) = activeAnimations[*it].originalTransformation;
            }
        }
    }

    if(currentPlayersSettings->menuInteraction) {
        guiPickMode = true;
    } else {
        guiPickMode = false;
    }
}

void World::addGUIButtonControls() {
    /**
     * For a new GUI Image we need only name and filename
     */
    static char GUIButtonName[32];
    ImGui::InputText("GUI Button Name", GUIButtonName, sizeof(GUIButtonName), ImGuiInputTextFlags_CharsNoBlank);

    static char GUIButtonNormalFileName[256] = "./Data/Textures/Menu/Buttons/";
    ImGui::InputText("Normal image path", GUIButtonNormalFileName, sizeof(GUIButtonNormalFileName));

    static char GUIButtonOnHoverFileName[256] = "./Data/Textures/Menu/Buttons/";
    ImGui::InputText("On hover image path", GUIButtonOnHoverFileName, sizeof(GUIButtonOnHoverFileName));

    static char GUIButtonOnClicklFileName[256] = "./Data/Textures/Menu/Buttons/";
    ImGui::InputText("On click image path", GUIButtonOnClicklFileName, sizeof(GUIButtonOnClicklFileName));

    static char GUIButtonDisabledFileName[256] = "./Data/Textures/Menu/Buttons/";
    ImGui::InputText("Disabled image path", GUIButtonDisabledFileName, sizeof(GUIButtonDisabledFileName));

    static size_t selectedLayerIndex = 0;
    if (guiLayers.size() == 0) {
        guiLayers.push_back(new GUILayer(glHelper, debugDrawer, 10));
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
    if (ImGui::Button("Add GUI Button")) {
        std::vector<std::string> fileNames;
        fileNames.push_back(std::string(GUIButtonNormalFileName));
        if(strlen(GUIButtonOnHoverFileName) > 0) {
            fileNames.push_back(std::string(GUIButtonOnHoverFileName));
            if(strlen(GUIButtonOnClicklFileName) > 0) {
                fileNames.push_back(std::string(GUIButtonOnClicklFileName));
                if(strlen(GUIButtonDisabledFileName) > 0) {
                    fileNames.push_back(std::string(GUIButtonDisabledFileName));
                }
            }
        }

        GUIButton *guiButton = new GUIButton(this->getNextObjectID(), assetManager, apiInstance, std::string(GUIButtonName),
                                             fileNames);
        guiButton->set2dWorldTransform(
                glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f), 0.0f);
        guiElements[guiButton->getWorldObjectID()] = guiButton;
        guiLayers[selectedLayerIndex]->addGuiElement(guiButton);
        pickedObject = guiButton;
    }
}

void World::addGUILayerControls() {
    /**
     * we need these set:
     * 1) font
     * 2) font size
     * 3) name
     *
     */

    static  int32_t levelSlider = 0;
    ImGui::DragInt("Layer level", &levelSlider, 1, 1, 128);
    if (ImGui::Button("Add GUI Layer")) {
        this->guiLayers.push_back(new GUILayer(glHelper, debugDrawer, (uint32_t)levelSlider));
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
