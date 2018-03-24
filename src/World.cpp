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
#include "GUI/GUILayer.h"
#include "GUI/GUIText.h"
#include "GUI/GUIFPSCounter.h"
#include "GUI/GUITextDynamic.h"
#include "ImGuiHelper.h"
#include "WorldSaver.h"


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

    GUILayer *layer1 = new GUILayer(glHelper, debugDrawer, 1);
    layer1->setDebug(false);

    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Data/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Data/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Data/Shaders/ShadowMap/fragmentPoint.glsl", false);
    shadowMapProgramPoint->setUniform("farPlanePoint", options->getLightPerspectiveProjectionValues().z);

    GUIText *tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf", 64), "Limon Engine",
                              glm::vec3(0, 0, 0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2, options->getScreenHeight()-20), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "Version 0.2",
                     glm::vec3(255, 255, 255));
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, 100), -1 * options->PI / 2);
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
             if (gameObject->getTypeID() != GameObject::PLAYER &&
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

void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {
    if(!inEditorMode) {
        //every time we call this method, we increase the time only by simulationTimeframe
        gameTime += simulationTimeFrame;
        dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
        currentPlayer->processPhysicsWorld(dynamicsWorld);

        for (unsigned int j = 0; j < actors.size(); ++j) {
            ActorInformation information = fillActorInformation(j);
            actors[j]->play(gameTime, information, options);
        }
        for (std::unordered_map<uint32_t, PhysicalRenderable *>::iterator it = objects.begin();
             it != objects.end(); ++it) {
            it->second->setupForTime(gameTime);
            it->second->updateTransformFromPhysics();
        }
    } else {
        dynamicsWorld->updateAabbs();
    }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }

    //end of physics step

    // If not in editor mode, dont let imgGuiHelper get input
    // if in editor mode, but player press editor button, dont allow imgui to process input
    // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
    if(!inEditorMode || inputHandler.getInputEvents(InputHandler::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
        handlePlayerInput(inputHandler);
    }
}

ActorInformation World::fillActorInformation(int j) {
    ActorInformation information;
    //FIXME this is just for test
    information.canSeePlayerDirectly = checkPlayerVisibility(actors[j]->getPosition(), "./Data/Models/ArmyPilot/ArmyPilot.mesh.xml");
    glm::vec3 front = actors[j]->getFrontVector();
    glm::vec3 rayDir = currentPlayer->getPosition() - actors[j]->getPosition();
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
    if(isPlayerReachable && grid->coursePath(actors[j]->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0), playerPosWithGrid, j, &route)) {
        if (route.empty()) {
            information.toPlayerRoute = glm::vec3(0, 0, 0);
            information.canGoToPlayer = false;
        } else {
            //Normally, this information should be used for straightening the path, but not yet.
            information.toPlayerRoute = route[route.size() - 1] - actors[j]->getPosition() - glm::vec3(0, 2.0f, 0);
            information.canGoToPlayer = true;
        }
    }
    return information;
}

void World::handlePlayerInput(InputHandler &inputHandler) {
    if(inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_LEFT)) {
        if(inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_LEFT)) {
            GameObject *gameObject = getPointedObject();
            std::string logLine;
            if (gameObject != nullptr) {
                pickedObject = gameObject;
                logLine = "object to pick is " + gameObject->getName();
            } else {
                pickedObject = nullptr;
                logLine = "no object to pick.";
            }
            options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_DEBUG, logLine);
        }
    }


    if (inputHandler.getInputEvents(inputHandler.EDITOR) && inputHandler.getInputStatus(inputHandler.EDITOR)) {
        if(!this->inEditorMode) {
            //switch control to free cursor player
            if(editorPlayer == nullptr) {
                editorPlayer = new FreeCursorPlayer(options, cursor);
                editorPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);
            }
            editorPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = editorPlayer;
            camera->setCameraAttachment(editorPlayer);
            inputHandler.setMouseModeFree();
            inEditorMode = true;
        } else {
            this->dynamicsWorld->getDebugDrawer()->setDebugMode(this->dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
            this->guiLayers[0]->setDebug(false);
            physicalPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = physicalPlayer;
            camera->setCameraAttachment(physicalPlayer);
            dynamicsWorld->updateAabbs();
            inputHandler.setMouseModeRelative();
            inEditorMode = false;
        }
    }

    if (!inEditorMode && inputHandler.getInputEvents(inputHandler.DEBUG) && inputHandler.getInputStatus(inputHandler.DEBUG)) {
        if(this->dynamicsWorld->getDebugDrawer()->getDebugMode() == btIDebugDraw::DBG_NoDebug) {
            this->dynamicsWorld->getDebugDrawer()->setDebugMode(
                    this->dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE | this->dynamicsWorld->getDebugDrawer()->DBG_DrawAabb | this->dynamicsWorld->getDebugDrawer()->DBG_DrawConstraints | this->dynamicsWorld->getDebugDrawer()->DBG_DrawConstraintLimits);
            this->options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_INFO, "Debug enabled");
            this->guiLayers[0]->setDebug(true);
            //switch control to debug player
            if(debugPlayer == nullptr) {
                debugPlayer = new FreeMovingPlayer(options, cursor);
                debugPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);
            }
            debugPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = debugPlayer;
            camera->setCameraAttachment(debugPlayer);
        } else {
            this->dynamicsWorld->getDebugDrawer()->setDebugMode(this->dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
            this->options->getLogger()->log(Logger::log_Subsystem_INPUT, Logger::log_level_INFO, "Debug disabled");
            this->guiLayers[0]->setDebug(false);
            physicalPlayer->ownControl(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = physicalPlayer;
            camera->setCameraAttachment(physicalPlayer);
            dynamicsWorld->updateAabbs();//FIXME So this is wrong, and I am not sure if it is required.
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
        for (std::unordered_map<uint32_t, PhysicalRenderable *>::iterator it = objects.begin();
             it != objects.end(); ++it) {
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
        for (std::unordered_map<uint32_t, PhysicalRenderable *>::iterator it = objects.begin();
             it != objects.end(); ++it) {
            (*it).second->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    glHelper->switchRenderToDefault();
    if(sky!=nullptr) {
        sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    }

    if(camera->isDirty()) {
        glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());
    }


    for (std::unordered_map<uint32_t, PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it).second->render();
    }

    dynamicsWorld->debugDrawWorld();
    if (this->dynamicsWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
        debugDrawer->drawLine(btVector3(0, 0, 0), btVector3(0, 250, 0), btVector3(1, 1, 1));
        //draw the ai-grid
        grid->debugDraw(debugDrawer);
    }

    debugDrawer->flushDraws();


    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();

    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }

    ImGuiFrameSetup();

}

/**
 * This method checks if we are in editor mode, and if we are, enables ImGui windows
 * It also fills the windows with relevant parameters.
 */
void World::ImGuiFrameSetup() {//TODO not const because it removes the object. Should be seperated
    if (this->inEditorMode) {

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
            ImGui::Text("Add new Object");
            ImGui::NewLine();
            if (ImGui::BeginCombo("Available objects", selectedAssetFile.c_str())) {
                for (auto it = assetManager->getAvailableAssetsList().begin();
                     it != assetManager->getAvailableAssetsList().end(); it++) {
                    if(ImGui::Selectable(it->first.c_str())) {
                        selectedAssetFile = it->first;
                    }
                }
                ImGui::EndCombo();
            }
            static float newObjectWeight;
            ImGui::SliderFloat("Weight", &newObjectWeight, 0.0f, 100.0f);
            ImGui::NewLine();
            static float newObjectX=0, newObjectY=15, newObjectZ=0;
            ImGui::SliderFloat("Position X", &newObjectX, -10.0f, 10.0f);
            ImGui::SliderFloat("Position Y", &newObjectY, -10.0f, 10.0f);
            ImGui::SliderFloat("Position Z", &newObjectZ, -10.0f, 10.0f);
            ImGui::NewLine();

            if(selectedAssetFile != "") {
                if(ImGui::Button("Add Object")) {
                    Model* newModel = new Model(this->getNextObjectID(), assetManager, newObjectWeight, selectedAssetFile);
                    newModel->setTranslate(glm::vec3(newObjectX, newObjectY, newObjectZ));
                    this->addModelToWorld(newModel);
                    newModel->getRigidBody()->activate();
                    pickedObject = static_cast<GameObject*>(newModel);
                }
            }

            ImGui::SetNextWindowSize(ImVec2(0,0), true);//true means set it only once
            if(pickedObject != nullptr) {
                ImGui::Begin("Selected Object Properties");

                pickedObject->addImGuiEditorElements();
                ImGui::NewLine();
                if (pickedObject->getTypeID() == GameObject::MODEL) {
                    if (ImGui::Button("Remove This Object")) {
                        //remove the object.
                        PhysicalRenderable *removeObject = objects[pickedObject->getWorldObjectID()];
                        //disconnect from physics
                        dynamicsWorld->removeRigidBody(removeObject->getRigidBody());
                        objects.erase(pickedObject->getWorldObjectID());
                        pickedObject = nullptr;
                        delete removeObject;
                    }
                }
                ImGui::End();
            } else {
                ImGui::Text("No object picked");                           // Some text (you can use a format string too)
            }
            if(ImGui::Button("Save Map")) {
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

    //FIXME clear GUIlayer elements
    for (std::unordered_map<uint32_t, PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
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
    xmlModel->getWorldTransform();
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
    this->actors.push_back(actor);
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
