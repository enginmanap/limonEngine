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
#include "GUI/GUIText.h"
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


World::World(AssetManager *assetManager, GLHelper *glHelper, Options *options)
        : assetManager(assetManager),options(options), glHelper(glHelper), fontManager(glHelper){
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
    shadowMapProgramPoint->setUniform("farPlanePoint", options->getLightPerspectiveProjectionValues().z);


    ApiLayer = new GUILayer(glHelper, debugDrawer, 1);
    ApiLayer->setDebug(false);
    guiLayers.push_back(ApiLayer);

    GUILayer *layer1 = new GUILayer(glHelper, debugDrawer, 2);
    layer1->setDebug(false);

    GUIText *tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf", 64), "Limon Engine",
                              glm::vec3(0, 0, 0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2, options->getScreenHeight()-20), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "Version 0.4",
                     glm::vec3(255, 255, 255));
    //tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, 100), -1 * options->PI / 2);
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, 100), 0);
    layer1->addGuiElement(tr);

    cursor = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "+",
                     glm::vec3(255, 255, 255));
    cursor->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), -1 * options->PI / 4);
    layer1->addGuiElement(cursor);

    trd = new GUITextDynamic(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), glm::vec3(0, 0, 0), 640, 380, options);
    trd->set2dWorldTransform(glm::vec2(320, options->getScreenHeight()-200), 0.0f);
    layer1->addGuiElement(trd);



    physicalPlayer = new PhysicalPlayer(options, cursor);
    currentPlayer = physicalPlayer;
    camera = new Camera(options, physicalPlayer);

    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);


    tr = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                           glm::vec3(204, 204, 0));
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);
    layer1->addGuiElement(tr);

    guiLayers.push_back(layer1);

    /************ ImGui *****************************/
    // Setup ImGui binding
    imgGuiHelper = new ImGuiHelper(glHelper, options);
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
            if((animationStatus->loop ) || animationStatus->animation->getDuration() / animationStatus->animation->getTicksPerSecond() * 1000  + animationStatus->startTime > gameTime) {

                float ticksPerSecond;
                if (animationStatus->animation->getTicksPerSecond() != 0) {
                    ticksPerSecond = animationStatus->animation->getTicksPerSecond();
                } else {
                    ticksPerSecond = 60.0f;
                }
                float animationTime = fmod(((gameTime - animationStatus->startTime) / 1000.0f) * ticksPerSecond, animationStatus->animation->getDuration());

                bool isFound;
                glm::mat4 tf = animationStatus->animation->calculateTransform(animationTime, isFound);

                glm::vec3 translate, scale;
                glm::quat orientation;
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(tf, scale, orientation, translate, skew, perspective);
                //FIXME this is not an acceptable animating technique, I need a transform stack, but not implemented it yet.
                (*animationStatus->object->getTransformation()) = animationStatus->originalTransformation;
                animationStatus->object->getTransformation()->addOrientation(orientation);
                animationStatus->object->getTransformation()->addScale(scale);
                animationStatus->object->getTransformation()->addTranslate(translate);
                animIt++;
            } else {
                if(!animationStatus->wasKinematic) {
                    animationStatus->object->getRigidBody()->setCollisionFlags(animationStatus->object->getRigidBody()->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                    animationStatus->object->getRigidBody()->setActivationState(ACTIVE_TAG);
                }
                options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_DEBUG, "Animation " + animIt->second.animation->getName() +" finished, removing. ");
                animIt = activeAnimations.erase(animIt);

            }
        }

        for (auto actorIt = actors.begin(); actorIt != actors.end(); ++actorIt) {
            ActorInformation information = fillActorInformation(actorIt->second);
            actorIt->second->play(gameTime, information, options);
        }
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (!it->second->getRigidBody()->isStaticOrKinematicObject()) {
                it->second->updateTransformFromPhysics();
            }
            it->second->setIsInFrustum(glHelper->isInFrustum(it->second->getAabbMin(), it->second->getAabbMax()));
            if(it->second->isIsInFrustum()) {
                it->second->setupForTime(gameTime);
            }

        }
    } else {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            it->second->setIsInFrustum(glHelper->isInFrustum(it->second->getAabbMin(), it->second->getAabbMax()));
        }
            dynamicsWorld->updateAabbs();
    }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }



    //end of physics step

    // If not in editor mode, dont let imgGuiHelper get input
    // if in editor mode, but player press editor button, dont allow imgui to process input
    // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
    if(currentMode != EDITOR_MODE || inputHandler.getInputEvents(InputHandler::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
            if(handlePlayerInput(inputHandler)) {
                isQuitRequest = !isQuitRequest;
            }
    }
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
            switch (beforeMode) {
                case DEBUG_MODE: {
                    switchToDebugMode(inputHandler);
                    break;
                }
                case PHYSICAL_MODE:
                default: {
                    switchToPhysicalPlayer(inputHandler);//if double editor, return to physical. This can happen when try to quit
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
    guiLayers[0]->setDebug(true);
    //switch control to debug player
    if(debugPlayer == nullptr) {
                debugPlayer = new FreeMovingPlayer(options, cursor);
                debugPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);
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
    this->guiLayers[0]->setDebug(false);
    beforeMode = currentMode;
    currentMode = PHYSICAL_MODE;
}

void World::switchToEditorMode(InputHandler &inputHandler) {//switch control to free cursor player
    if(editorPlayer == nullptr) {
                editorPlayer = new FreeCursorPlayer(options, cursor);
                editorPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);
            }
    editorPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
    currentPlayer = editorPlayer;
    camera->setCameraAttachment(editorPlayer);
    inputHandler.setMouseModeFree();
    beforeMode = currentMode;
    currentMode = EDITOR_MODE;
}

GameObject * World::getPointedObject() const {
    glm::vec3 from, lookDirection;
    currentPlayer->getWhereCameraLooks(from, lookDirection);
    //we want to extend to vector to world AABB limit
    float maxFactor = 0;

    if(lookDirection.x > 0 ) {
        //so we are looking at positive x. determine how many times the ray x we need
        maxFactor = (worldAABBMax.x - from.x) / lookDirection.x;
    } else {
        maxFactor = (worldAABBMin.x - from.x) / lookDirection.x; //Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }

    if(lookDirection.y > 0 ) {
        std::max(maxFactor, (worldAABBMax.y - from.y) / lookDirection.y);
    } else {
        std::max(maxFactor, (worldAABBMin.y - from.y) / lookDirection.y);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }

    if(lookDirection.z > 0 ) {
        std::max(maxFactor, (worldAABBMax.z - from.z) / lookDirection.z);
    } else {
        std::max(maxFactor, (worldAABBMin.z - from.z) / lookDirection.z);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }
    lookDirection = lookDirection * maxFactor;
    glm::vec3 to = lookDirection + from;
    btCollisionWorld::ClosestRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), GLMConverter::GLMToBlt(to));

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

void World::render() {
    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::DIRECTIONAL) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapDirectional(i);
        shadowMapProgramDirectional->setUniform("renderLightIndex", (int)i);
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            (*it).second->renderWithProgram(*shadowMapProgramDirectional);
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
        //FIXME this is suppose to be an option //FarPlanePoint is set at declaration, since it is a constant
        shadowMapProgramPoint->setUniform("farPlanePoint", 100.0f);
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            (*it).second->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    if(camera->isDirty()) {
        glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());//this is required for any render
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
    if(currentMode == EDITOR_MODE) {
        ImGuiFrameSetup();
    }
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
            //list available elements
            static std::string selectedAssetFile = "";
            glm::vec3 newObjectPosition = camera->getPosition() + 10.0f * camera->getCenter();

            if (ImGui::CollapsingHeader("Add New Object")) {
                if (ImGui::BeginCombo("Available objects", selectedAssetFile.c_str())) {
                    for (auto it = assetManager->getAvailableAssetsList().begin();
                         it != assetManager->getAvailableAssetsList().end(); it++) {
                        if (ImGui::Selectable(it->first.c_str())) {
                            selectedAssetFile = it->first;
                        }
                    }
                    ImGui::EndCombo();
                }
                static float newObjectWeight;
                ImGui::SliderFloat("Weight", &newObjectWeight, 0.0f, 100.0f);

                ImGui::NewLine();
                if(selectedAssetFile != "") {
                    if(ImGui::Button("Add Object")) {
                        Model* newModel = new Model(this->getNextObjectID(), assetManager, newObjectWeight, selectedAssetFile);
                        newModel->getTransformation()->setTranslate(newObjectPosition);
                        this->addModelToWorld(newModel);
                        newModel->getRigidBody()->activate();
                        pickedObject = static_cast<GameObject*>(newModel);
                    }
                }
            }

            if(ImGui::Button("Add Trigger")) {

                TriggerObject* to = new TriggerObject(this->getNextObjectID());
                to->getTransformation()->setTranslate(newObjectPosition);
                this->dynamicsWorld->addCollisionObject(to->getGhostObject(), btBroadphaseProxy::SensorTrigger,
                                                        btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
                triggers[to->getWorldObjectID()] = to;

                pickedObject = static_cast<GameObject*>(to);
            }

            ImGui::SetNextWindowSize(ImVec2(0,0), true);//true means set it only once

            ImGui::Begin("Selected Object Properties");
            bool isObjectSelectorOpen;
            if(pickedObject == nullptr) {
                isObjectSelectorOpen = ImGui::BeginCombo("Picked object", "No object selected");
            } else {
                isObjectSelectorOpen =ImGui::BeginCombo("Picked object", (pickedObject->getName().c_str()));
            }
            if (isObjectSelectorOpen) {
                for (auto it = objects.begin(); it != objects.end(); it++) {
                    GameObject* gameObject = dynamic_cast<GameObject *>(it->second);
                    if (ImGui::Selectable(gameObject->getName().c_str())) {
                        pickedObject = gameObject;
                    }
                }
                for (auto it = lights.begin(); it != lights.end(); it++) {
                    GameObject* gameObject = dynamic_cast<GameObject *>(*it);
                    if (ImGui::Selectable(gameObject->getName().c_str())) {
                        pickedObject = (*it);
                    }
                }

                for (auto it = triggers.begin(); it != triggers.end(); it++) {
                    GameObject* gameObject = dynamic_cast<GameObject *>(it->second);
                    if (ImGui::Selectable(gameObject->getName().c_str())) {
                        pickedObject = it->second;
                    }
                }
                ImGui::EndCombo();
            }
            if(pickedObject != nullptr) {
                GameObject::ImGuiResult request = pickedObject->addImGuiEditorElements(camera->getCameraMatrix(), glHelper->getProjectionMatrix());
                if(pickedObject->getTypeID() == GameObject::MODEL) {
                    static char animationNameBuffer[32];

                    ImGui::Text("New animation name:");
                    //double # because I don't want to show it
                    ImGui::InputText("##newAnimationNameField", animationNameBuffer, sizeof(animationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);

                    //If there is no animation setup ongoing, or there is one, but not for this model,
                    //put start animation button.
                    //else put time input, add and finalize buttons.
                    if(animationInProgress == nullptr || animationInProgress->object != dynamic_cast<Model*>(pickedObject)) {
                        if (ImGui::Button("Start definition")) {
                            if (animationInProgress == nullptr) {
                                animationInProgress = new AnimationStatus();
                            } else {
                                //ask for removal of the old work
                                delete animationInProgress->animationNode;
                                delete animationInProgress;
                                animationInProgress = new AnimationStatus();
                            }
                            animationInProgress->object = dynamic_cast<Model*>(pickedObject);
                            // At this point we should know the animationInProgress is for current object
                            animationInProgress->originalTransformation = *dynamic_cast<Model *>(pickedObject)->getTransformation();
                            animationInProgress->animationNode = new AnimationNode();
                            /* The animation should start with the current position, so save empty transforms */
                            animationInProgress->animationNode->translates.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
                            animationInProgress->animationNode->translateTimes.push_back(0);
                            animationInProgress->animationNode->scales.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
                            animationInProgress->animationNode->scaleTimes.push_back(0);
                            animationInProgress->animationNode->rotations.push_back(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                            animationInProgress->animationNode->rotationTimes.push_back(0.0f);
                        }

                    } else {
                        //this means the original transform is saved, with others(possibly) stacked on top.
                        static int time = 60;
                        ImGui::InputInt("Time of position:", &time);
                        if(ImGui::Button("Add Animation key frame")) {
                            glm::vec3 translate, scale;
                            glm::quat rotation;
                            animationInProgress->originalTransformation.getDifference(*dynamic_cast<Model *>(pickedObject)->getTransformation(), translate, scale, rotation);
                            animationInProgress->animationNode->translates.push_back(translate);
                            animationInProgress->animationNode->translateTimes.push_back(time);
                            animationInProgress->animationNode->scales.push_back(scale);
                            animationInProgress->animationNode->scaleTimes.push_back(time);
                            animationInProgress->animationNode->rotations.push_back(rotation);
                            animationInProgress->animationNode->rotationTimes.push_back(time);
                        }

                        if(ImGui::Button("Finish Animation")) {
                            loadedAnimations.push_back(AnimationCustom(std::string(animationNameBuffer),
                                                                                 animationInProgress->animationNode,
                                                                                 time));

                            (*dynamic_cast<Model *>(pickedObject)->getTransformation()) = animationInProgress->originalTransformation;
                            //Calling addAnimation here is odd, but I am planning to add animations using an external API call in next revisions.

                            addAnimationToObject(dynamic_cast<Model *>(pickedObject)->getWorldObjectID(), loadedAnimations.size()-1, true);
                            delete animationInProgress;
                            animationInProgress = nullptr;
                            //TODO, who deletes what is not clear, I should use smart pointers.
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("Cancel ")) {
                            delete animationInProgress->animationNode;
                            delete animationInProgress->animation;
                            delete animationInProgress;
                            animationInProgress = nullptr;
                        }
                    }
                }

                if (request.removeAI) {
                    //remove AI requested
                    if (dynamic_cast<Model *>(pickedObject)->getAIID() != 0) {
                        actors.erase(dynamic_cast<Model *>(pickedObject)->getAIID());
                        dynamic_cast<Model *>(pickedObject)->detachAI();
                    }
                }

                if (request.addAI) {
                    std::cout << "adding AI to model " << std::endl;
                    HumanEnemy *newEnemy = new HumanEnemy(getNextObjectID());
                    newEnemy->setModel(dynamic_cast<Model *>(pickedObject));

                    addActor(newEnemy);
                }
                    ImGui::NewLine();
                    if (pickedObject->getTypeID() == GameObject::MODEL) {
                        if (ImGui::Button("Remove This Object")) {
                            //remove animation
                            activeAnimations.erase(dynamic_cast<Model *>(pickedObject));

                            //remove the object.
                            PhysicalRenderable *removeObject = objects[pickedObject->getWorldObjectID()];
                            //disconnect from physics
                            dynamicsWorld->removeRigidBody(removeObject->getRigidBody());
                            //disconnect AI
                            if (dynamic_cast<Model *>(removeObject)->getAIID() != 0) {
                                actors.erase(dynamic_cast<Model *>(removeObject)->getAIID());
                            }
                            objects.erase(pickedObject->getWorldObjectID());
                            pickedObject = nullptr;
                            delete removeObject;
                        }
                    }

            }

            ImGui::End();
            ImGui::NewLine();
            static char loadAnimationNameBuffer[32];
            ImGui::Text("Load animation:");
            //double # because I don't want to show it
            ImGui::InputText("##LoadAnimationNameField", loadAnimationNameBuffer, sizeof(loadAnimationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);
            if(ImGui::Button("load animation")) {

                AnimationCustom* animation = AnimationLoader::loadAnimation("./Data/Animations/"+ std::string(loadAnimationNameBuffer) +".xml");
                if(animation == nullptr) {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "Animation load failed");
                } else {
                    options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "Animation loaded");
                    loadedAnimations.push_back(*animation);
                }
            }

            if(ImGui::Button("Save Map")) {
                for(auto animIt = activeAnimations.begin(); animIt != activeAnimations.end(); animIt++) {
                    if(animIt->second.animation->serializeAnimation("./Data/Animations/")) {
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
        }

        /* window definitions */
        imgGuiHelper->RenderDrawLists();
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
    dynamicsWorld->addRigidBody(xmlModel->getRigidBody());
    btVector3 aabbMin, aabbMax;
    xmlModel->getRigidBody()->getAabb(aabbMin, aabbMax);
    std::cout << "bounding box of model " << xmlModel->getName() << " is "
              << GLMUtils::vectorToString(GLMConverter::BltToGLM(aabbMin)) << ", "
              << GLMUtils::vectorToString(GLMConverter::BltToGLM(aabbMax)) << std::endl;
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

void World::addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped) {
    AnimationStatus as;
    as.object = objects[modelID];
    as.originalTransformation = *(as.object->getTransformation());
    as.animation = &(loadedAnimations[animationID]);
    as.loop = looped;
    as.wasKinematic = as.object->getRigidBody()->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;
    as.startTime = gameTime;
    if(activeAnimations.count(as.object) != 0) {
        options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_WARN, "Model had custom animation, overriding.");
    }
    activeAnimations[as.object] = as;
    as.object->getRigidBody()->setCollisionFlags(as.object->getRigidBody()->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    as.object->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);

}


bool World::generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters) {
    bool isAllSet = true;
    for (size_t i = 0; i < runParameters.size(); ++i) {
        LimonAPI::ParameterRequest& parameter = runParameters[i];

        switch(parameter.requestType) {
            case LimonAPI::ParameterRequest::RequestParameterTypes::MODEL: {
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
                std::string currentObject;
                if (parameter.isSet) {
                    currentObject = dynamic_cast<Model *>(objects[(uint32_t) (parameter.value.longValue)])->getName();
                } else {
                    currentObject = "Not selected";
                    isAllSet = false;
                }
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i)).c_str(),
                                      currentObject.c_str())) {
                    for (auto it = objects.begin();
                         it != objects.end(); it++) {
                        if (ImGui::Selectable(dynamic_cast<Model *>((it->second))->getName().c_str())) {
                            parameter.value.longValue = static_cast<long>(dynamic_cast<Model *>((it->second))->getWorldObjectID());
                            parameter.isSet = true;
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
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i)).c_str(),
                                      currentAnimation.c_str())) {
                    for (uint32_t j = 0; j < loadedAnimations.size(); ++j) {
                        if (ImGui::Selectable(loadedAnimations[j].getName().c_str())) {
                            parameter.value.longValue = static_cast<long>(j);
                            parameter.isSet = true;
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
                if (ImGui::Checkbox((parameter.description + "##triggerParam" + std::to_string(i)).c_str(),
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
                if (ImGui::InputText((parameter.description + "##triggerParam" + std::to_string(i)).c_str(),
                                     parameter.value.stringValue, sizeof(parameter.value.stringValue))) {
                    parameter.isSet = true;
                };
            }
                break;
        }
    }
    return isAllSet;
}

void World::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &text, const glm::vec3 &color,
                       const glm::vec2 &position, float rotation) {
    GUIText* tr = new GUIText(glHelper, fontManager.getFont(fontFilePath, fontSize), text,
                                 color);
    tr->set2dWorldTransform(position, rotation);
    ApiLayer->addGuiElement(tr);

}
