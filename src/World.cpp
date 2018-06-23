//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "AI/HumanEnemy.h"

#include "Camera.h"
#include "GameObjects/SkyBox.h"
#include "BulletDebugDrawer.h"
#include "AI/AIMovementGrid.h"


#include "GameObjects/Players/FreeCursorPlayer.h"
#include "GameObjects/Players/FreeMovingPlayer.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Light.h"
#include "GameObjects/GameObject.h"
#include "GUI/GUILayer.h"
#include "GUI/GUIRenderable.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUIFPSCounter.h"
#include "GUI/GUITextDynamic.h"
#include "ImGuiHelper.h"
#include "WorldSaver.h"
#include "../libs/ImGuizmo/ImGuizmo.h"
#include "GameObjects/TriggerObject.h"
#include "Assets/Animations/AnimationAssimp.h"
#include "Assets/Animations/AnimationLoader.h"
#include "Assets/Animations/AnimationNode.h"
#include "Assets/Animations/AnimationCustom.h"
#include "GamePlay/AnimateOnTrigger.h"
#include "GamePlay/AddGuiTextOnTrigger.h"
#include "AnimationSequencer.h"
#include "GUI/Cursor.h"
#include "GUI/GUILayer.h"
#include "GameObjects/GUIText.h"


World::World(AssetManager *assetManager, GLHelper *glHelper, Options *options)
        : assetManager(assetManager),options(options), glHelper(glHelper), fontManager(glHelper) {
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


    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Data/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Data/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Data/Shaders/ShadowMap/fragmentPoint.glsl", false);


    apiGUILayer = new GUILayer(glHelper, debugDrawer, 1);
    apiGUILayer->setDebug(false);

    renderCounts = new GUIText(glHelper, getNextObjectID(), "Render Counts",
                               fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "0", glm::vec3(204, 204, 0));
    renderCounts->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 170, options->getScreenHeight() - 36), 0);

    cursor = new Cursor(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "+",
                        glm::vec3(255, 255, 255));
    cursor->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), -1 * options->PI / 4);

    debugOutputGUI = new GUITextDynamic(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16),
                                        glm::vec3(0, 0, 0), 640, 380, options);
    debugOutputGUI->set2dWorldTransform(glm::vec2(320, options->getScreenHeight()-200), 0.0f);

    physicalPlayer = new PhysicalPlayer(options, cursor);
    currentPlayer = physicalPlayer;
    camera = new Camera(options, physicalPlayer);//register is just below

    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER, COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, worldAABBMin, worldAABBMax);


    fpsCounter = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                                   glm::vec3(204, 204, 0));
    fpsCounter->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);

    onLoadActions.push_back(new ActionForOnload());//this is here for editor, as if no action is added, editor would fail to allow setting the first one.

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
bool World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {


     // If not in editor mode, dont let imgGuiHelper get input
     // if in editor mode, but player press editor button, dont allow imgui to process input
     // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
     if(currentMode != EDITOR_MODE || inputHandler.getInputEvents(InputHandler::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
         if(handlePlayerInput(inputHandler)) {
             isQuitRequest = !isQuitRequest;
         }
     }

     if(camera->isDirty()) {
         glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());//this is required for any render
     }



     if(currentMode != EDITOR_MODE && currentMode != PAUSED_MODE) {
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

                Transformation tf = animationCustom->calculateTransform(animationTime);

                //FIXME this is not an acceptable animating technique, I need a transform stack, but not implemented it yet.
                (*animationStatus->object->getTransformation()) = animationStatus->originalTransformation;
                animationStatus->object->getTransformation()->addOrientation(tf.getOrientation());
                animationStatus->object->getTransformation()->addScale(tf.getScale());
                animationStatus->object->getTransformation()->addTranslate(tf.getTranslate());
                animIt++;
            } else {
                if(!animationStatus->wasKinematic) {
                    animationStatus->object->getRigidBody()->setCollisionFlags(animationStatus->object->getRigidBody()->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                    animationStatus->object->getRigidBody()->setActivationState(ACTIVE_TAG);
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
            }
            it->second->setIsInFrustum(glHelper->isInFrustum(it->second->getAabbMin(), it->second->getAabbMax()));
            if(it->second->isIsInFrustum()) {
                it->second->setupForTime(gameTime);
            }
            for(size_t i = 0; i < lights.size(); i++) {
                if(it->second->isDirtyForFrustum() ||lights[i]->isFrustumChanged()) {
                    it->second->setIsInLightFrustum(i, lights[i]->isShadowCaster(it->second->getAabbMin(),
                                                                                 it->second->getAabbMax(),
                                                                                 it->second->getTransformation()->getTranslate()));
                    it->second->setCleanForFrustum();
                }
            }
        }
        for(size_t i = 0; i < lights.size(); i++) {
            lights[i]->setFrustumChanged(false);
        }

    } else {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            it->second->setIsInFrustum(glHelper->isInFrustum(it->second->getAabbMin(), it->second->getAabbMax()));
            for(size_t i = 0; i < lights.size(); i++) {
                it->second->setIsInLightFrustum(i, lights[i]->isShadowCaster(it->second->getAabbMin(),
                                                                             it->second->getAabbMax(),
                                                                             it->second->getTransformation()->getTranslate()));
            }
        }
            dynamicsWorld->updateAabbs();
    }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }
    debugOutputGUI->setupForTime(gameTime);

    return isQuitRequest && isQuitVerified;
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
    crossBetween = cross(normalize(front), normalize(rayDir));
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
    if(!isQuitRequest && isQuitVerified) {
        isQuitVerified = false;
        //means player selected stay, we should revert to last player type
        switch (beforeMode) {
            case DEBUG_MODE:
                switchToDebugMode(inputHandler);
                break;
            case EDITOR_MODE:
                switchToEditorMode(inputHandler);
                break;
            case PHYSICAL_MODE:
            default:
                switchToPhysicalPlayer(inputHandler);
                break;
        }
    }
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
        if(currentMode != EDITOR_MODE ) {
            switchToEditorMode(inputHandler);
        } else {
            //if user is shown quit dialog, don't allow switching modes, user should say no
            if(!isQuitRequest == true) {
                switch (beforeMode) {
                    case DEBUG_MODE: {
                        switchToDebugMode(inputHandler);
                        break;
                    }
                    case PHYSICAL_MODE:
                    default: {
                        switchToPhysicalPlayer(
                                inputHandler);//if double editor, return to physical. This can happen when try to quit
                    }
                }
            }
        }
    }
    //if not in editor mode and press debug
    if (currentMode != EDITOR_MODE && inputHandler.getInputEvents(inputHandler.DEBUG) && inputHandler.getInputStatus(inputHandler.DEBUG)) {
        if(currentMode != DEBUG_MODE) {
            switchToDebugMode(inputHandler);
        } else {
            //no matter what was the before, just return to player
            switchToPhysicalPlayer(inputHandler);
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
        if(currentMode != EDITOR_MODE) {
            switchToEditorMode(inputHandler);

        } else {
            beforeMode = EDITOR_MODE;//you should return to editor mode after quitting
        }
        return true;
    } else {
        return false;
    }
}

void World::switchToDebugMode(InputHandler &inputHandler) {
    dynamicsWorld->getDebugDrawer()->setDebugMode(
            dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE | dynamicsWorld->getDebugDrawer()->DBG_DrawAabb | dynamicsWorld->getDebugDrawer()->DBG_DrawConstraints | dynamicsWorld->getDebugDrawer()->DBG_DrawConstraintLimits);
    options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_INFO, "Debug enabled");
    for (size_t i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setDebug(true);
    }

    //switch control to debug player
    if(debugPlayer == nullptr) {
                debugPlayer = new FreeMovingPlayer(options, cursor);
        debugPlayer->registerToPhysicalWorld(dynamicsWorld, 0, 0, worldAABBMin, worldAABBMax);
            }
    debugPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
    currentPlayer = debugPlayer;
    camera->setCameraAttachment(debugPlayer);
    inputHandler.setMouseModeRelative();
    beforeMode = currentMode;
    currentMode = DEBUG_MODE;
}

void World::switchToPhysicalPlayer(InputHandler &inputHandler) {
    physicalPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
    currentPlayer = physicalPlayer;
    camera->setCameraAttachment(physicalPlayer);
    dynamicsWorld->updateAabbs();
    inputHandler.setMouseModeRelative();
    this->dynamicsWorld->getDebugDrawer()->setDebugMode(this->dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    for (size_t i = 0; i < guiLayers.size(); ++i) {
        this->guiLayers[i]->setDebug(false);
    }

    beforeMode = currentMode;
    currentMode = PHYSICAL_MODE;
}

void World::switchToEditorMode(InputHandler &inputHandler) {//switch control to free cursor player
    if(editorPlayer == nullptr) {
                editorPlayer = new FreeCursorPlayer(options, cursor);
        editorPlayer->registerToPhysicalWorld(dynamicsWorld, 0, 0, worldAABBMin, worldAABBMax);
            }
    editorPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
    currentPlayer = editorPlayer;
    camera->setCameraAttachment(editorPlayer);
    inputHandler.setMouseModeFree();
    beforeMode = currentMode;
    currentMode = EDITOR_MODE;

    //when switching to editor mode, return all objects that are custom animated without triggers
    //to original position
    for(auto it = onLoadAnimations.begin(); it != onLoadAnimations.end(); it++) {
         if(activeAnimations.find(*it) != activeAnimations.end()) {
             (*(*it)->getTransformation()) = activeAnimations[*it].originalTransformation;
         }
    }

}

GameObject * World::getPointedObject() const {
    glm::vec3 from, lookDirection;
    currentPlayer->getWhereCameraLooks(from, lookDirection);

    if(guiPickMode) {
        GUIText* pickedGuiElement = nullptr;
        // TODO this should filter by level
        //then we don't need to rayTest. We can get the picked object directly by coordinate.
        for (size_t i = 0; i < guiLayers.size(); ++i) {
            GUIText* pickedGuiTemp = dynamic_cast<GUIText*>(guiLayers[i]->getRenderableFromCoordinate(cursor->getTranslate()));
            if(pickedGuiTemp != nullptr) {
                pickedGuiElement = pickedGuiTemp;
            }
        }
        return pickedGuiElement;
    } else {
        //we want to extend to vector to world AABB limit
        float maxFactor = 0;

        if (lookDirection.x > 0) {
            //so we are looking at positive x. determine how many times the ray x we need
            maxFactor = (worldAABBMax.x - from.x) / lookDirection.x;
        } else {
            maxFactor = (worldAABBMin.x - from.x) /
                        lookDirection.x; //Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
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
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if(it->second->isInLightFrustum(i)) { // FIXME this should have " && it->second->isIsInFrustum()" but we are calculating the frustum planes without shadows
                (*it).second->renderWithProgram(*shadowMapProgramDirectional);
            }
        }
    }

    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::POINT) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapPoint();
        //FIXME why are these set here?
        shadowMapProgramPoint->setUniform("renderLightIndex", (int)i);
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            (*it).second->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    glHelper->switchRenderToDefault();
    if(sky!=nullptr) {
        sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    }

    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if(it->second->isIsInFrustum()) {
            (*it).second->render();
        }
    }

    dynamicsWorld->debugDrawWorld();
    if (this->dynamicsWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
        debugDrawer->drawLine(btVector3(0, 0, 0), btVector3(0, 250, 0), btVector3(1, 1, 1));
        //draw the ai-grid
        grid->debugDraw(debugDrawer);
    }

    if(currentMode == PlayerModes::EDITOR_MODE  && !isQuitRequest) {
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
    if(currentMode == EDITOR_MODE) {
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
void World::ImGuiFrameSetup() {//TODO not const because it removes the object. Should be seperated
    if(this->isQuitRequest) {
        if(isQuitRequest) {
            //ask if wants to save
            imgGuiHelper->NewFrame();
            ImGui::Begin("Quitting, Are you sure?");
            if(ImGui::Button("Yes, quit")) {
                isQuitVerified = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("No, stay")) {
                isQuitRequest = false;
                isQuitVerified = true;
                currentMode = PAUSED_MODE;//FIXME we are rendering more than we allow input. That causes a bit delay before we exit editor mode, which in turn creates a bit shutter.
                //this line should have 0 effect because it will be overriden, but it will remove the shutter.
                //return to the player we left off in next frame
            }
            ImGui::End();
            imgGuiHelper->RenderDrawLists();
        }
    } else {
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
                if (ImGui::Button("Copy Selected object")) {

                    Model* pickedModel = dynamic_cast<Model*>(pickedObject);
                    Model* newModel = new Model(*pickedModel, this->getNextObjectID());
                    newModel->getTransformation()->addTranslate(glm::vec3(5.08f,0.0f,5.08f));
                    addModelToWorld(newModel);
                    //now we should apply the animations

                    if(onLoadAnimations.find(pickedModel) != onLoadAnimations.end() &&
                            activeAnimations.find(pickedModel) != activeAnimations.end()) {
                        addAnimationToObject(newModel->getWorldObjectID(), activeAnimations[pickedModel].animationIndex, true, true);
                    }
                    pickedObject = static_cast<GameObject*>(newModel);
                }
            }


            if(ImGui::Button("Add Trigger Volume")) {

                TriggerObject* to = new TriggerObject(this->getNextObjectID(), this->apiInstance);
                to->getTransformation()->setTranslate(newObjectPosition);
                this->dynamicsWorld->addCollisionObject(to->getGhostObject(), COLLIDE_TRIGGER_VOLUME, COLLIDE_PLAYER | COLLIDE_EVERYTHING);
                triggers[to->getWorldObjectID()] = to;

                pickedObject = static_cast<GameObject*>(to);
            }

            if (ImGui::CollapsingHeader("Add on load trigger")) {
                static size_t onLoadTriggerIndex = 0;//maximum size
                std::string selectedID = std::to_string(onLoadTriggerIndex);
                if (ImGui::BeginCombo("Current Triggers", selectedID.c_str())) {
                    for(size_t i = 0; i < onLoadActions.size(); i++) {
                        bool isTriggerSelected = selectedID == std::to_string(i);

                        if (ImGui::Selectable(std::to_string(i).c_str(), isTriggerSelected)) {
                            onLoadTriggerIndex = i;
                        }
                        if(isTriggerSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                //currently any trigger object can have 3 elements, so this should be >2 to avoid collision on imgui tags. I am assigning 100 just to be safe
                TriggerObject::PutTriggerInGui(apiInstance, onLoadActions[onLoadTriggerIndex]->action, onLoadActions[onLoadTriggerIndex]->parameters,
                                               onLoadActions[onLoadTriggerIndex]->enabled, 100+onLoadTriggerIndex);
                if(onLoadActions[onLoadActions.size()-1]->enabled) {
                    //when user presses the enable button, add another and select it
                    onLoadTriggerIndex=onLoadActions.size();
                    onLoadActions.push_back(new ActionForOnload());
                }
            }

            if (ImGui::CollapsingHeader("Add GUI Elements")) {
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
                if (ImGui::BeginCombo("Layer To add", std::to_string(selectedLayerIndex).c_str())) {
                    for (size_t i = 0; i < this->guiLayers.size(); ++i) {
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
                    GUIText* guiText = new GUIText(glHelper, getNextObjectID(), GUITextName, fontManager.getFont(selectedFontName, fontSize), "New Text", glm::vec3(0,0,0));
                    guiText->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), 0.0f);
                    this->guiElements[guiText->getWorldObjectID()] = guiText;
                    this->guiLayers[selectedLayerIndex]->addGuiElement(guiText);
                    pickedObject = guiText;
                }



            }


            if (ImGui::CollapsingHeader("Custom Animations ")) {
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
            if(ImGui::Button("Save Map")) {
                for(auto animIt = loadedAnimations.begin(); animIt != loadedAnimations.end(); animIt++) {
                    if(animIt->serializeAnimation("./Data/Animations/")) {
                        options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation saved");
                    } else {
                        options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation save failed");
                    }

                }

                if(WorldSaver::saveWorld("./Data/Maps/CustomWorld001.xml", this)) {
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
                addAnimationToObject(dynamic_cast<Model *>(pickedObject)->getWorldObjectID(), listbox_item_current, true, true);
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
    if(debugPlayer!= nullptr) {
        delete debugPlayer;
    }
    delete imgGuiHelper;
}

void World::addModelToWorld(Model *xmlModel) {
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



    /*
    std::cout << "bounding box of model " << xmlModel->getName() << " is "
              << GLMUtils::vectorToString(GLMConverter::BltToGLM(aabbMin)) << ", "
              << GLMUtils::vectorToString(GLMConverter::BltToGLM(aabbMax)) << std::endl;
    */
    updateWorldAABB(GLMConverter::BltToGLM(aabbMin), GLMConverter::BltToGLM(aabbMax));
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
    grid = new AIMovementGrid(aiGridStartPoint, dynamicsWorld, worldAABBMin, worldAABBMax);
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

uint32_t World::addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad) {
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
    activeAnimations[as.object] = as;
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
                        currentGUIText = guiElements[static_cast<uint32_t >(parameter.value.longValue)]->getName();
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
                        bool isThisGUITextSelected = currentGUIText == it->second->getName();
                        if (ImGui::Selectable(it->second->getName().c_str(), isThisGUITextSelected)) {
                            parameter.value.longValue = static_cast<long>(it->first);
                            parameter.isSet = true;
                        }

                        if(isThisGUITextSelected) {
                            ImGui::SetItemDefaultFocus();
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

uint32_t World::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    if(guiElements.find(guiTextID) != guiElements.end()) {
        dynamic_cast<GUITextBase*>(guiElements[guiTextID])->updateText(newText);
    }
    return 0;
}


uint32_t World::removeTriggerObject(uint32_t triggerobjectID) {
    if(triggers.find(triggerobjectID) != triggers.end()) {
        TriggerObject* objectToRemove = triggers[triggerobjectID];
        dynamicsWorld->removeCollisionObject(objectToRemove->getGhostObject());
        //delete object itself
        delete triggers[triggerobjectID];
        triggers.erase(triggerobjectID);
        return 0;
    }
    return 1;//not successful
}

uint32_t World::removeObject(uint32_t objectID) {
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
        //delete object itself
        delete objects[objectID];
        objects.erase(objectID);
        return 0;
    }
    return 1;//not successful
}

void World::afterLoadFinished() {
    for (size_t i = 0; i < onLoadActions.size(); ++i) {
        if(onLoadActions[i]->enabled) {
            std::cout << "running trigger " << onLoadActions[i]->action->getName() << std::endl;
            onLoadActions[i]->action->run(onLoadActions[i]->parameters);
        }
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
