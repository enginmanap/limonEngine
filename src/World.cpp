   //
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"

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
#include "GameObjects/GUIAnimation.h"
#include "GameObjects/ModelGroup.h"
#include "PostProcess/QuadRenderBase.h"
#include "PostProcess/CombinePostProcess.h"
#include "PostProcess/SSAOPostProcess.h"
#include "PostProcess/SSAOBlurPostProcess.h"
#include "SDL2Helper.h"
#include "GraphicsPipelineStage.h"

   const std::map<World::PlayerInfo::Types, std::string> World::PlayerInfo::typeNames =
    {
            { Types::PHYSICAL_PLAYER, "Physical"},
            { Types::DEBUG_PLAYER, "Debug"},
            { Types::EDITOR_PLAYER, "Editor"},
            { Types::MENU_PLAYER, "Menu" }
    };

World::World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
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

    depthBufferProgram = new GLSLProgram(glHelper, "./Engine/Shaders/depthPrePass/vertex.glsl",
                                  "./Engine/Shaders/depthPrePass/fragment.glsl", false);


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


    ssaoPostProcess = new SSAOPostProcess(glHelper, options->getSSAOSampleCount());
    ssaoPostProcess->setSourceTexture("depthMapSampler", 1);
    ssaoPostProcess->setSourceTexture("normalMapSampler", 2);
    ssaoPostProcess->setSourceTexture("ssaoNoiseSampler", 3);

    ssaoBlurPostProcess = new SSAOBlurPostProcess(glHelper);
    ssaoBlurPostProcess->setSourceTexture("ssaoResultSampler", 1);

    if(options->isSsaoEnabled()) {
        combiningObject = new CombinePostProcess(glHelper,true);
        combiningObject->setSourceTexture("diffuseSpecularLighted", 1);
        combiningObject->setSourceTexture("ambient", 2);
        combiningObject->setSourceTexture("ssao", 3);
        combiningObject->setSourceTexture("depthMap", 4);
    } else {
        combiningObject = new CombinePostProcess(glHelper,false);
        combiningObject->setSourceTexture("diffuseSpecularLighted", 1);
        combiningObject->setSourceTexture("depthMap", 4);
    }

    //FIXME adding camera after dynamic world because static only world is needed for ai movement grid generation
    camera = new Camera(options, currentPlayer->getCameraAttachment());//register is just below
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, COLLIDE_PLAYER,
                                           COLLIDE_MODELS | COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING,
                                           COLLIDE_MODELS | COLLIDE_EVERYTHING, worldAABBMin,
                                           worldAABBMax);
    switchPlayer(currentPlayer, *inputHandler); //switching to itself, to set the states properly. It uses camera so done after camera creation



    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};

    directionalShadowStage = new GraphicsPipelineStage(glHelper, options->getShadowMapDirectionalWidth(), options->getShadowMapDirectionalHeight(), false);
    directionalShadowStage->setOutput(GLHelper::FrameBufferAttachPoints::DEPTH, glHelper->depthMapDirectional);
    directionalShadowStage->setCullMode(GLHelper::CullModes::FRONT);

    pointShadowStage = new GraphicsPipelineStage(glHelper, options->getShadowMapPointWidth(), options->getShadowMapPointHeight(), false);
    pointShadowStage->setOutput(GLHelper::FrameBufferAttachPoints::DEPTH, glHelper->depthMapPoint);
    pointShadowStage->setCullMode(GLHelper::CullModes::FRONT);

    coloringStage = new GraphicsPipelineStage(glHelper, options->getScreenWidth(), options->getScreenHeight(), false);
    coloringStage->setOutput(GLHelper::FrameBufferAttachPoints::COLOR0, glHelper->diffuseAndSpecularLightedMap);
    coloringStage->setOutput(GLHelper::FrameBufferAttachPoints::COLOR1, glHelper->ambientMap);
    coloringStage->setOutput(GLHelper::FrameBufferAttachPoints::COLOR2, glHelper->normalMap);
    coloringStage->setOutput(GLHelper::FrameBufferAttachPoints::DEPTH, glHelper->depthMap);
    coloringStage->setInput(glHelper->getMaxTextureImageUnits() - 1, glHelper->depthMapDirectional);
    coloringStage->setInput(glHelper->getMaxTextureImageUnits() - 2, glHelper->depthMapPoint);
    coloringStage->setInput(glHelper->getMaxTextureImageUnits() - 3, glHelper->depthMap);
    coloringStage->setInput(glHelper->getMaxTextureImageUnits() - 4, glHelper->ssaoNoiseTexture);
    coloringStage->setCullMode(GLHelper::CullModes::BACK);

    std::shared_ptr<GLHelper::Texture> ssaoTexture = std::make_shared<GLHelper::Texture>(glHelper, GLHelper::TextureTypes::T2D, GLHelper::InternalFormatTypes::RED, GLHelper::FormatTypes::RGB, GLHelper::DataTypes::FLOAT, options->getScreenWidth(), options->getScreenHeight());
    ssaoTexture->setBorderColor(borderColor[0], borderColor[1], borderColor[2], borderColor[3]);
    ssaoGenerationStage = new GraphicsPipelineStage(glHelper, options->getScreenWidth(), options->getScreenHeight(), false);
    ssaoGenerationStage->setOutput(GLHelper::FrameBufferAttachPoints::COLOR1, ssaoTexture);
    ssaoGenerationStage->setInput(1, glHelper->depthMap);
    ssaoGenerationStage->setInput(2, glHelper->normalMap);
    ssaoGenerationStage->setInput(3, glHelper->ssaoNoiseTexture);

    std::shared_ptr<GLHelper::Texture> ssaoBlurTexture = std::make_shared<GLHelper::Texture>(glHelper, GLHelper::TextureTypes::T2D, GLHelper::InternalFormatTypes::RED, GLHelper::FormatTypes::RGB, GLHelper::DataTypes::FLOAT, options->getScreenWidth(), options->getScreenHeight());
    ssaoBlurStage = new GraphicsPipelineStage(glHelper, options->getScreenWidth(), options->getScreenHeight(), false);
    ssaoBlurStage->setOutput(GLHelper::FrameBufferAttachPoints::COLOR1, ssaoBlurTexture);
    ssaoBlurStage->setInput(1, ssaoTexture);

    glHelper->ssaoBlurredMap = ssaoBlurTexture;

    combiningStage = new GraphicsPipelineStage(glHelper, options->getScreenWidth(), options->getScreenHeight(), true, true);
    combiningStage->setInput(1, glHelper->diffuseAndSpecularLightedMap);
    combiningStage->setInput(2, glHelper->ambientMap);
    combiningStage->setInput(3, glHelper->ssaoBlurredMap);
    combiningStage->setInput(4, glHelper->depthMap);

    fpsCounter = new GUIFPSCounter(glHelper, fontManager.getFont("./Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                                   glm::vec3(204, 204, 0));
    fpsCounter->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);

    onLoadActions.push_back(new ActionForOnload());//this is here for editor, as if no action is added, editor would fail to allow setting the first one.

    modelIndicesBuffer.reserve(NR_MAX_MODELS);
    modelsInLightFrustum.resize(NR_TOTAL_LIGHTS);
    animatedModelsInLightFrustum.resize(NR_TOTAL_LIGHTS);
    activeLights.reserve(NR_TOTAL_LIGHTS);

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
 void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {

     // If not in editor mode, dont let imgGuiHelper get input
     // if in editor mode, but player press editor button, dont allow imgui to process input
     // if in editor mode, player did not press editor button, then check if imgui processed, if not use the input
     if(!currentPlayersSettings->editorShown || inputHandler.getInputStates().getInputEvents(InputStates::Inputs ::EDITOR) || !imgGuiHelper->ProcessEvent(inputHandler)) {
         if(handlePlayerInput(inputHandler)) {
             handleQuitRequest();
             return;
         }
     }

     //Seperating physics step and visibility, because physics is used by camera, and camera is used by visibility
     if(currentPlayersSettings->worldSimulation) {
         //every time we call this method, we increase the time only by simulationTimeframe
         gameTime += simulationTimeFrame;
         dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
         currentPlayer->processPhysicsWorld(dynamicsWorld);
     }
     if(camera->isDirty()) {
         glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());//this is required for any render
         alHelper->setListenerPositionAndOrientation(camera->getPosition(), camera->getCenter(), camera->getUp());
     }
     checkAndRunTimedEvents();
     if(currentPlayersSettings->worldSimulation) {
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
         for (auto modelAssetIterator = modelsInCameraFrustum.begin(); modelAssetIterator != modelsInCameraFrustum.end(); ++modelAssetIterator) {
             for (auto modelIterator = modelAssetIterator->second.begin(); modelIterator != modelAssetIterator->second.end(); ++modelIterator) {
                 (*modelIterator)->setupForTime(gameTime);
             }
         }
         for (auto modelIt = animatedModelsInAnyFrustum.begin(); modelIt != animatedModelsInAnyFrustum.end(); ++modelIt) {
             (*modelIt)->setupForTime(gameTime);
         }
     }

     for (size_t j = 0; j < activeLights.size(); ++j) {
         activeLights[j]->step(gameTime);
     }
     updateActiveLights(false);

     fillVisibleObjects();

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
                ticksPerSecond = 60.0f;
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

    for (size_t currentLightIndex = 0; currentLightIndex < activeLights.size(); ++currentLightIndex) {
        if(activeLights[currentLightIndex]->isFrustumChanged()) {
            modelsInLightFrustum[currentLightIndex].clear();
            animatedModelsInLightFrustum[currentLightIndex].clear();
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
                setLightVisibilityAndPutToSets(currentLightIndex, objectIt->second, false);
            }
            activeLights[currentLightIndex]->setFrustumChanged(false);
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
                                      activeLights[currentLightIndex]->isShadowCaster(currentModel->getAabbMin(),
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
    if(currentModel->isTransparent()) {
        if(currentModel->isIsInFrustum()) {
            if (transparentModelsInCameraFrustum.find(currentModel->getAssetID()) == transparentModelsInCameraFrustum.end()) {
                transparentModelsInCameraFrustum[currentModel->getAssetID()] = std::set<Model *>();
            }
            transparentModelsInCameraFrustum[currentModel->getAssetID()].insert(currentModel);
        } else {
            if(removePossible) {
                if(transparentModelsInCameraFrustum[currentModel->getAssetID()].find(currentModel) != transparentModelsInCameraFrustum[currentModel->getAssetID()].end()) {
                    transparentModelsInCameraFrustum[currentModel->getAssetID()].erase(currentModel);
                }
            }
        }
    } else {
        if (currentModel->isIsInFrustum()) {
            if (currentModel->isAnimated()) {
                animatedModelsInFrustum.insert(currentModel);
                animatedModelsInAnyFrustum.insert(currentModel);
            } else {
                if (modelsInCameraFrustum.find(currentModel->getAssetID()) == modelsInCameraFrustum.end()) {
                    modelsInCameraFrustum[currentModel->getAssetID()] = std::set<Model *>();
                }
                modelsInCameraFrustum[currentModel->getAssetID()].insert(currentModel);
            }
        } else if (removePossible) {
            //if remove possible, and not in frustum, search for the model, and remove
            if (currentModel->isAnimated()) {
                bool isInAnyFrustum = false;
                animatedModelsInFrustum.erase(currentModel);
                //now check if it is in any other frustums
                for (uint32_t i = 0; i < animatedModelsInLightFrustum.size(); ++i) {
                    if (animatedModelsInLightFrustum[i].find(currentModel) != animatedModelsInLightFrustum[i].end()) {
                        isInAnyFrustum = true;
                        break;
                    }
                }
                if (!isInAnyFrustum) {
                    animatedModelsInAnyFrustum.erase(currentModel);
                }
            } else {
                //if not animated
                modelsInCameraFrustum[currentModel->getAssetID()].erase(currentModel);
            }
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
            std::vector<LimonAPI::ParameterRequest> parameters;
            parameters.push_back(LimonAPI::ParameterRequest());

            parameters[0].value.vectorValue = GLMConverter::GLMToLimon(
                    actor->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0));
            parameters.push_back(LimonAPI::ParameterRequest());
            parameters[1].value.longValues[0] = 2;
            parameters[1].value.longValues[1] = actor->getWorldID();
            parameters[1].value.longValues[2] = information.maximumRouteDistance;

            std::function<std::vector<LimonAPI::ParameterRequest>(
                    std::vector<LimonAPI::ParameterRequest>)> functionToRun =
                    std::bind(&World::fillRouteInformation, this, std::placeholders::_1);
            routeThreads[actor->getWorldID()] = new SDL2Helper::Thread("FillRouteForActor", functionToRun, parameters);
            routeThreads[actor->getWorldID()]->run();
        }

        if (routeThreads.find(actor->getWorldID()) != routeThreads.end() && routeThreads[actor->getWorldID()]->isThreadDone()) {
            const std::vector<LimonAPI::ParameterRequest>* route = routeThreads[actor->getWorldID()]->getResult();
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


   //std::function<std::vector<LimonAPI::ParameterRequest>(std::vector<LimonAPI::ParameterRequest>)> functionToRun

std::vector<LimonAPI::ParameterRequest>
World::fillRouteInformation(std::vector<LimonAPI::ParameterRequest> parameters) const {
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
    std::vector<LimonAPI::ParameterRequest> routeList;
    for (size_t i = 0; i < route.size(); ++i) {
        LimonAPI::ParameterRequest pr;
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
                pickedObject = gameObject;
            } else {
                pickedObject = nullptr;
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

void World::render() {
    std::map<std::shared_ptr<GLHelper::Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> shadowAttachmentTextureLayers;
    for (unsigned int i = 0; i < activeLights.size(); ++i) {
        shadowAttachmentTextureLayers.clear();
        if(activeLights[i]->getLightType() != Light::DIRECTIONAL) {
            continue;
        }
        //generate shadow map
        shadowAttachmentTextureLayers[glHelper->depthMapDirectional] = std::make_pair(GLHelper::FrameBufferAttachPoints::DEPTH, i);
        directionalShadowStage->activate(shadowAttachmentTextureLayers, true);
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

    pointShadowStage->activate(true);
    for (unsigned int i = 0; i < activeLights.size(); ++i) {
        if(activeLights[i]->getLightType() != Light::POINT) {
            continue;
        }
        //FIXME why are these set here?
        shadowMapProgramPoint->setUniform("renderLightIndex", (int)i);
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

    coloringStage->activate(true);
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

    if(currentPlayersSettings->editorShown) { //if editor is shown, render wireframe of the triggers
        for (auto it = triggers.begin(); it != triggers.end(); ++it) {
            it->second->render(debugDrawer);
        }
        if(physicalPlayer != nullptr) {
            if(playerPlaceHolder == nullptr) {
                std::string assetFile;
                glm::vec3 scale;
                physicalPlayer->getRenderProperties(assetFile, scale);
                playerPlaceHolder = new Model(this->getNextObjectID(), assetManager, 0, assetFile, true);
                playerPlaceHolder->getTransformation()->setScale(scale);
            }

            startingPlayer.orientation = physicalPlayer->getLookDirection();
            startingPlayer.position = physicalPlayer->getPosition();

            playerPlaceHolder->getTransformation()->setTranslate(physicalPlayer->getPosition());
            playerPlaceHolder->getTransformation()->setOrientation(physicalPlayer->getLookDirectionQuaternion());
            std::vector<uint32_t > temp;
            temp.push_back(playerPlaceHolder->getWorldObjectID());
            playerPlaceHolder->renderInstanced(temp);
        }
    }

    if(!currentPlayer->isDead() && startingPlayer.attachedModel != nullptr) {//don't render attched model if dead
        Model* attachedModel = startingPlayer.attachedModel;
        renderPlayerAttachments(attachedModel);
    }

    debugDrawer->flushDraws();

    //at this point, we should combine all of the coloring
    if(options->isSsaoEnabled()) {
        //glHelper->switchRenderToSSAOGeneration();
        ssaoGenerationStage->activate();
        ssaoPostProcess->render();

        //glHelper->switchRenderToSSAOBlur();
        ssaoBlurStage->activate();
        ssaoBlurPostProcess->render();
    }
    combiningStage->activate(true);
    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    combiningObject->render();

    //now render transparent objects
    for (auto modelIterator = transparentModelsInCameraFrustum.begin(); modelIterator != transparentModelsInCameraFrustum.end(); ++modelIterator) {
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

    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }
    cursor->render();
    if(options->getRenderInformations()) {
        renderCounts->render();
        debugOutputGUI->render();
        fpsCounter->render();
    }

    //render API gui layer
    apiGUILayer->render();

    uint32_t triangle, line;
    glHelper->getRenderTriangleAndLineCount(triangle, line);
    renderCounts->updateText("Tris: " + std::to_string(triangle) + ", lines: " + std::to_string(line));
    if(currentPlayersSettings->editorShown) {
        ImGuiFrameSetup();
    }
}

   void World::renderPlayerAttachments(GameObject *attachment) const {
     if(attachment->getTypeID() == GameObject::MODEL) {
         Model* attachedModel = static_cast<Model*>(attachment);
         attachedModel->setupForTime(gameTime);
         std::vector<uint32_t> temp;
         temp.push_back(attachedModel->getWorldObjectID());
         attachedModel->renderInstanced(temp);
         if (attachedModel->hasChildren()) {
             const std::vector<PhysicalRenderable *> &children = attachedModel->getChildren();
             for (auto iterator = children.begin(); iterator != children.end(); ++iterator) {
                 GameObject* gameObject = dynamic_cast<GameObject*>(*iterator);
                 if(gameObject != nullptr) {
                     renderPlayerAttachments(gameObject);
                 }
             }
         }
     } else if(attachment->getTypeID() == GameObject::MODEL_GROUP) {
         ModelGroup* attachedModelGroup = static_cast<ModelGroup*>(attachment);
         attachedModelGroup->setupForTime(gameTime);
         std::vector<uint32_t> temp;
         temp.push_back(attachedModelGroup->getWorldObjectID());
         if (attachedModelGroup->hasChildren()) {
             const std::vector<PhysicalRenderable *> &children = attachedModelGroup->getChildren();
             for (auto iterator = children.begin(); iterator != children.end(); ++iterator) {
                 GameObject* gameObject = dynamic_cast<GameObject*>(*iterator);
                 if(gameObject != nullptr) {
                     renderPlayerAttachments(gameObject);
                 }
             }
         }
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
        static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;
        glm::vec3 newObjectPosition = camera->getPosition() + 10.0f * camera->getCenter();


        if (ImGui::CollapsingHeader("Add New Object")) {
            static char modelAssetFilter[32] = {0};
            ImGui::InputText("Filter Assets ##ModelsAssetTreeFilter", modelAssetFilter, sizeof(modelAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
            std::string modelAssetFilterStr = modelAssetFilter;
            std::transform(modelAssetFilterStr.begin(), modelAssetFilterStr.end(), modelAssetFilterStr.begin(), ::tolower);
            const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_MODEL, modelAssetFilterStr);
            imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_MODEL,
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
                    Model* newModel = new Model(this->getNextObjectID(), assetManager, newObjectWeight,
                                                selectedAsset->fullPath, false);
                    newModel->getTransformation()->setTranslate(newObjectPosition);
                    this->addModelToWorld(newModel);
                    newModel->getRigidBody()->activate();
                    pickedObject = static_cast<GameObject*>(newModel);
                }
            }
        }
        if(pickedObject != nullptr && pickedObject->getTypeID() == GameObject::MODEL) {
            ImGui::Separator();
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
                    addAnimationToObject(newModel->getWorldObjectID(), activeAnimations[pickedModel]->animationIndex,
                                         true, true);
                }
                pickedObject = static_cast<GameObject*>(newModel);
            }

            if(ImGui::Button("Attach this object to another")) {
                this->objectToAttach = dynamic_cast<Model*>(pickedObject);
            }
            if(this->objectToAttach != nullptr) {
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + this->objectToAttach->getName());
            }
            if(objectToAttach!= nullptr && objectToAttach->getWorldObjectID() != pickedObject->getWorldObjectID()) {
                std::string savedObjectName = this->objectToAttach->getName();
                if (ImGui::Button("Attach saved object to current")) {
                    Model *pickedModel = dynamic_cast<Model *>(pickedObject);
                    int32_t attachedBoneID;
                    Transformation* pickedModelTransformation = pickedModel->getAttachmentTransform(attachedBoneID);

                    glm::vec3 translate, scale;
                    glm::quat orientation;
                    pickedModelTransformation->getDifferenceAddition(*objectToAttach->getTransformation(), translate,
                                                                     scale, orientation);
                    translate = translate * (1.0f / pickedModelTransformation->getScale());
                    translate = translate * pickedModelTransformation->getOrientation();
                    objectToAttach->getTransformation()->setTranslate(translate);
                    objectToAttach->getTransformation()->setScale(scale);
                    objectToAttach->getTransformation()->setOrientation(orientation);
                    objectToAttach->getTransformation()->setParentTransform(pickedModelTransformation);
                    objectToAttach->setParentObject(pickedModel, attachedBoneID);
                    pickedModel->addChild(objectToAttach);
                    this->objectToAttach = nullptr;
                }
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Saved Object: " + savedObjectName);
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Current Object: " + pickedObject->getName());
            }
        }
        if(pickedObject != nullptr && pickedObject->getTypeID() == GameObject::PLAYER) {
            if(objectToAttach!= nullptr && objectToAttach->getWorldObjectID() != pickedObject->getWorldObjectID()) {
                if (ImGui::Button("Attach saved object to Player")) {
                    clearWorldRefsBeforeAttachment(objectToAttach);
                    physicalPlayer->setAttachedModel(objectToAttach);
                    startingPlayer.attachedModel = objectToAttach;
                    this->objectToAttach = nullptr;
                }
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Model Groups")) {
            static uint32_t selectedModelGroup = 0;

            if (ImGui::BeginCombo("Model Group##combobox", (selectedModelGroup == 0? "No Group Selected." : modelGroups[selectedModelGroup]->getName().c_str()))) {
                for (auto iterator = modelGroups.begin();
                     iterator != modelGroups.end(); ++iterator) {
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
            PhysicalRenderable* pickedPhysicalRenderable = dynamic_cast<PhysicalRenderable*>(pickedObject);
            if(pickedPhysicalRenderable != nullptr && selectedModelGroup != 0 && pickedObject->getWorldObjectID() != selectedModelGroup) {
                //now prevent adding to self

                if(ImGui::Button("Add model to group")) {
                    modelGroups[selectedModelGroup]->addChild(pickedPhysicalRenderable);
                }
            } else {
                ImGui::Button("Add model to group");
                if(pickedObject == nullptr) {
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No object Selected");
                }else {
                    if (pickedObject->getWorldObjectID() == selectedModelGroup) {
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
                    ModelGroup* modelGroup = new ModelGroup(glHelper, this->getNextObjectID(), std::string(modelGroupNameBuffer));
                    this->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;

                }
            } else {
                ImGui::Button("Create Group");
                ImGui::SameLine();
                ImGuiHelper::ShowHelpMarker("Name is mandatory!");

            }
        }
        if(ImGui::Button("Add Trigger Volume")) {

            TriggerObject* to = new TriggerObject(this->getNextObjectID(), this->apiInstance);
            to->getTransformation()->setTranslate(newObjectPosition);
            this->dynamicsWorld->addCollisionObject(to->getGhostObject(), COLLIDE_TRIGGER_VOLUME | COLLIDE_EVERYTHING, COLLIDE_PLAYER | COLLIDE_EVERYTHING);
            triggers[to->getWorldObjectID()] = to;

            pickedObject = static_cast<GameObject*>(to);
        }

        if (ImGui::CollapsingHeader("Add New Light")) {

            if(ImGui::Button("Add Point Light")) {
                Light* newLight = new Light(glHelper, this->getNextObjectID(), Light::LightTypes::POINT, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                //this->lights.push_back(newLight);
                this->addLight(newLight);
                pickedObject = newLight;
            }
            if(directionalLightIndex == -1) {//Allow single directional light
                if(ImGui::Button("Add Directional Light")) {
                    Light* newLight = new Light(glHelper, this->getNextObjectID(), Light::LightTypes::DIRECTIONAL, newObjectPosition, glm::vec3(0.5f, 0.5f, 0.5f));
                    this->addLight(newLight);
                }
            }

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
            if (ImGui::CollapsingHeader("Add GUI Animation##The header")) {
                addGUIAnimationControls();
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
        if(ImGui::CollapsingHeader("Player properties")) {
            if (ImGui::BeginCombo("Starting Type", startingPlayer.typeToString().c_str())) {
                for (auto iterator = PlayerInfo::typeNames.begin();
                     iterator != PlayerInfo::typeNames.end(); ++iterator) {
                    bool isThisTypeSelected = iterator->second == startingPlayer.typeToString();
                    if (ImGui::Selectable(iterator->second.c_str(), isThisTypeSelected)) {
                        this->startingPlayer.setType(iterator->second);
                    }
                    if (isThisTypeSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            static bool showError = false;
            if(ImGui::InputText("Custom Extension name", extensionNameBuffer, 31, ImGuiInputTextFlags_CharsNoBlank)) {
                showError = false;
            }

            if(startingPlayer.attachedModel != nullptr) {
                if(ImGui::Button("Disconnect Attachment##player attachment")) {
                    Model* attachedModel = startingPlayer.attachedModel;
                    startingPlayer.attachedModel = nullptr;
                    this->physicalPlayer->setAttachedModel(nullptr);
                    this->addModelToWorld(attachedModel);
                }
            }

            if(ImGui::Button("Apply##PlayerExtensionUpdate")) {
                std::string tempName = extensionNameBuffer;
                //find the starting player, and apply this change to it:
                Player* playerToUpdate = nullptr;
                switch (startingPlayer.type) {
                    case PlayerInfo::Types::DEBUG_PLAYER: playerToUpdate = debugPlayer; break;
                    case PlayerInfo::Types::EDITOR_PLAYER: playerToUpdate = editorPlayer; break;
                    case PlayerInfo::Types::PHYSICAL_PLAYER: playerToUpdate = physicalPlayer; break;
                    case PlayerInfo::Types::MENU_PLAYER: playerToUpdate = menuPlayer; break;
                }
                if(playerToUpdate != nullptr && tempName != "") {
                    PlayerExtensionInterface* extension = PlayerExtensionInterface::createExtension(tempName, apiInstance);
                    if(extension != nullptr) {
                        this->startingPlayer.extensionName = tempName;
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

            if(ImGui::CollapsingHeader("Music")) {
                ImGui::Indent(16.0f);
                static const AssetManager::AvailableAssetsNode *selectedSoundAsset = nullptr;
                static char musicAssetFilter[32] = {0};
                ImGui::InputText("Filter Assets ##MusicAssetTreeFilter", musicAssetFilter, sizeof(musicAssetFilter),
                                 ImGuiInputTextFlags_CharsNoBlank);
                std::string musicAssetFilterStr = musicAssetFilter;
                std::transform(musicAssetFilterStr.begin(), musicAssetFilterStr.end(), musicAssetFilterStr.begin(),
                               ::tolower);
                const AssetManager::AvailableAssetsNode *filteredAssets = assetManager->getAvailableAssetsTreeFiltered(
                        AssetManager::Asset_type_SOUND, musicAssetFilterStr);
                imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_SOUND,
                                                  "Music",
                                                  &selectedSoundAsset);

                if (this->music != nullptr) {
                    ImGui::Text("Current Music: %s", this->music->getName().c_str());
                } else {
                    ImGui::Text("No music set for level. ");
                }

                if (selectedSoundAsset != nullptr) {
                    if (ImGui::Button("Change Music")) {
                        if (this->music != nullptr) {
                            this->music->stop();
                            delete this->music;
                        }
                        this->music = new Sound(getNextObjectID(), assetManager, selectedSoundAsset->fullPath);
                        this->music->setLoop(true);
                        this->music->setWorldPosition(glm::vec3(0, 0, 0), true);
                        this->music->play();
                    }
                } else {
                    ImGui::Button("Change Music");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No sound asset selected");
                }
            }
            if(ImGui::CollapsingHeader("SkyBox")) {
                ImGui::Indent(16.0f);
                addSkyBoxControls();
            }
            if(ImGui::CollapsingHeader("LoadingImage")) {
                ImGui::Indent(16.0f);
                static const AssetManager::AvailableAssetsNode* selectedLoadingImageAsset = nullptr;

                static char loadingImageAssetFilter[32] = {0};
                ImGui::InputText("Filter Assets ##TextureAssetTreeFilter", loadingImageAssetFilter, sizeof(loadingImageAssetFilter), ImGuiInputTextFlags_CharsNoBlank);
                std::string loadingImageAssetFilterStr = loadingImageAssetFilter;
                std::transform(loadingImageAssetFilterStr.begin(), loadingImageAssetFilterStr.end(), loadingImageAssetFilterStr.begin(), ::tolower);
                const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, loadingImageAssetFilterStr);
                imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE, "LoadingImage", &selectedLoadingImageAsset);

                if(selectedLoadingImageAsset == nullptr) {
                    ImGui::Button("Set Loading Image");
                    ImGui::SameLine();
                    ImGuiHelper::ShowHelpMarker("No Asset Selected!");
                } else {
                    if (ImGui::Button("Set Loading Image")) {
                        this->loadingImage = selectedLoadingImageAsset->fullPath;
                    }
                }
                if(this->loadingImage != "" ) {
                    ImGui::Text("Current Loading Image: %s", this->loadingImage.c_str() );
                } else {
                    ImGui::Text("No loading image set");
                }
            }


            if(ImGui::CollapsingHeader("ESC handling")) {
                ImGui::Indent(16.0f);
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
                if (currentQuitResponse == QuitResponse::LOAD_WORLD) {
                    ImGui::InputText("Custom World file ", quitWorldNameBuffer, sizeof(quitWorldNameBuffer));
                    if (ImGui::Button("Apply##custom world file setting")) {
                        quitWorldName = quitWorldNameBuffer;
                    }
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
            //before saving, set the connection state
            for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
                disconnectObjectFromPhysics(*objectIt);
            }

            if(WorldSaver::saveWorld(worldSaveNameBuffer, this)) {
                options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_INFO, "World save successful");
            } else {
                options->getLogger()->log(Logger::log_Subsystem_LOAD_SAVE, Logger::log_level_ERROR, "World save Failed");
            }
            //after save, set the states back
            for (auto objectIt = disconnectedModels.begin(); objectIt != disconnectedModels.end(); ++objectIt) {
                reconnectObjectToPhysics(*objectIt);
            }

        }
        if(ImGui::Button("Save AI walk Grid")) {
            if(this->grid != nullptr) {
                std::string AIWalkName = this->name.substr(0, this->name.find_last_of(".")) + ".aiwalk";
                this->grid->serializeXML(AIWalkName);
            }
        }
#ifdef CEREAL_SUPPORT
   if(ImGui::Button("Save AI walk Grid Binary")) {
            if(this->grid != nullptr) {
                std::string AIWalkName = this->name.substr(0, this->name.find_last_of(".")) + ".aiwalkb";
                std::ofstream os(AIWalkName, std::ios::binary);
                cereal::BinaryOutputArchive archive( os );

                archive(*grid);
            }
        }
#endif
        if(ImGui::Button("Convert models to binary")) {
            std::set<std::vector<std::string>> convertedAssets;
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {

                Model* model = dynamic_cast<Model*>(objectIt->second);
                if(model!= nullptr) {
                    model->convertAssetToLimon(convertedAssets);
                }
            }
            if(startingPlayer.attachedModel != nullptr) {
                startingPlayer.attachedModel->convertAssetToLimon(convertedAssets);
            }
        }

        ImGui::End();

        //ImGui::SetNextWindowSize(ImVec2(0,0), false);//true means set it only once

        ImGui::Begin("Selected Object Properties");
        std::string selectedName;
        if(pickedObject == nullptr) {
            selectedName = "No object selected";
        } else {
            selectedName = pickedObject->getName().c_str();
        }

        buildTreeFromAllGameObjects();

        if(pickedObject != nullptr) {
            GameObject::ImGuiResult objectEditorResult = pickedObject->addImGuiEditorElements(*request);

            switch(pickedObject->getTypeID()) {
                case GameObject::MODEL: {
                    Model* selectedObject = static_cast<Model*>(pickedObject);
                    if(objectEditorResult.updated) {
                        if(!selectedObject->isDisconnected()) {
                            dynamicsWorld->updateSingleAabb(selectedObject->getRigidBody());
                        }
                        updatedModels.push_back(selectedObject);
                    }
                    uint32_t removedActorID = 0;
                    if (objectEditorResult.removeAI) {
                        //remove AI requested
                        if (dynamic_cast<Model *>(pickedObject)->getAIID() != 0) {
                            removedActorID = dynamic_cast<Model *>(pickedObject)->getAIID();
                            actors.erase(dynamic_cast<Model *>(pickedObject)->getAIID());
                            dynamic_cast<Model *>(pickedObject)->detachAI();
                        }
                    }

                    if (objectEditorResult.addAI) {
                        std::cout << "adding AI to model " << std::endl;
                        if(removedActorID == 0) {
                            removedActorID = getNextObjectID();
                        }
                        //if remove and add is called in same frame, it means the type is changed, reuse the ID
                        ActorInterface *newEnemy = ActorInterface::createActor(objectEditorResult.actorTypeName, removedActorID, apiInstance);
                        Model* model = dynamic_cast<Model *>(pickedObject);
                        if(model != nullptr) {
                            newEnemy->setModel(model->getWorldObjectID());
                            model->attachAI(newEnemy);
                        } else {
                            std::cerr << "ActorInterface Model setting failed, because picked object is not a model." << std::endl;

                        }
                        addActor(newEnemy);
                    } else {
                        if(removedActorID != 0) {
                            unusedIDs.push(removedActorID);
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
                    Renderable* selectedObject = dynamic_cast<Renderable*>(pickedObject);
                    if(selectedObject != nullptr) {
                        //Now we are looking for animations
                        if (activeAnimations.find(selectedObject) != activeAnimations.end()) {
                            if (objectEditorResult.updated) {
                                activeAnimations[selectedObject]->originChange = true;
                            }

                            if (ImGui::Button(("Remove custom animation: " +
                                               loadedAnimations[activeAnimations[selectedObject]->animationIndex].getName()).c_str())) {
                                ImGui::OpenPopup("How To Remove Custom Animation");
                            }
                            if (ImGui::BeginPopupModal("How To Remove Custom Animation")) {
                                AnimationCustom &animationToRemove = loadedAnimations[activeAnimations[selectedObject]->animationIndex];
                                AnimationStatus *animationStatusToRemove = activeAnimations[selectedObject];
                                if (ImGui::Button("Set to Start##CustomAnimationRemoval")) {
                                    removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, 0);
                                    ImGui::CloseCurrentPopup();
                                }

                                if (ImGui::Button("Set to End##CustomAnimationRemoval")) {
                                    removeActiveCustomAnimation(animationToRemove, animationStatusToRemove,
                                                                animationToRemove.getDuration());
                                    ImGui::CloseCurrentPopup();
                                }
                                static float customTime = 0.0f;
                                ImGui::DragFloat("##CustomAnimationRemovalCustomTime", &customTime, 0.1, 0.0,
                                                 animationToRemove.getDuration());
                                ImGui::SameLine();
                                if (ImGui::Button("Set to custom time##CustomAnimationRemoval")) {
                                    removeActiveCustomAnimation(animationToRemove, animationStatusToRemove, customTime);
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
            switch (pickedObject->getTypeID()) {
                case GameObject::MODEL: {
                    if (disconnectedModels.find(pickedObject->getWorldObjectID()) != disconnectedModels.end()) {
                        if (ImGui::Button("reconnect to physics")) {
                            reconnectObjectToPhysicsRequest(static_cast<Model *>(pickedObject)->getWorldObjectID());//Request because that action will not be carried out on editor mode
                        }
                    } else {
                        if (ImGui::Button("Disconnect from physics")) {
                            disconnectObjectFromPhysicsRequest(static_cast<Model *>(pickedObject)->getWorldObjectID());
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
                case GameObject::GUI_BUTTON:
                case GameObject::GUI_ANIMATION: {
                    if(objectEditorResult.remove) {
                        this->guiElements.erase(pickedObject->getWorldObjectID());
                        unusedIDs.push(pickedObject->getWorldObjectID());
                        delete pickedObject;
                        pickedObject = nullptr;
                    }
                    }
                    break;
                case GameObject::LIGHT: {
                    if(objectEditorResult.remove) {
                        for (auto iterator = lights.begin(); iterator != lights.end(); ++iterator) {
                            if((*iterator)->getWorldObjectID() == pickedObject->getWorldObjectID()) {
                                unusedIDs.push(pickedObject->getWorldObjectID());
                                lights.erase(iterator);
                                break;
                            }
                        }
                        if(static_cast<Light*>(pickedObject)->getLightType() == Light::LightTypes::DIRECTIONAL) {
                            directionalLightIndex = -1;
                        }
                        delete pickedObject;
                        updateActiveLights(true);
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
        if (animationInProgress == nullptr || animationInProgress->getAnimatingObject() != dynamic_cast<Renderable *>(pickedObject)) {

            static int listbox_item_current = 0;
            ImGui::Text("Loaded animations list");
            ImGui::ListBox("##Loaded animations listbox", &listbox_item_current, getNameOfLoadedAnimation,
                           static_cast<void *>(&loadedAnimations), loadedAnimations.size(), 10);

            if (ImGui::Button("Apply selected")) {
                addAnimationToObject(pickedObject->getWorldObjectID(), listbox_item_current,
                                     true, true);
            }

            ImGui::SameLine();
            if (ImGui::Button("Create new")) {
                if (animationInProgress == nullptr) {
                    animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(pickedObject));
                } else {
                    //ask for removal of the old work
                    delete animationInProgress;
                    animationInProgress = new AnimationSequenceInterface(
                            dynamic_cast<Renderable *>(pickedObject));
                }
                // At this point we should know the animationInProgress is for current object
            }
        } else {
            ImGui::Text("Please use animation definition window.");
            bool finished, cancelled;
            animationInProgress->addAnimationSequencerToEditor(finished, cancelled);
            if (finished) {
                loadedAnimations.push_back(AnimationCustom(*animationInProgress->buildAnimationFromCurrentItems()));

                addAnimationToObject(pickedObject->getWorldObjectID(),
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
    if(light->getLightType() == Light::DIRECTIONAL) {
        directionalLightIndex = (uint32_t)lights.size()-1;
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
            as->originalTransformation.copy(activeAnimations[as->object]->originalTransformation);//if looped animation, start new one from origin
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
            as->originalTransformation.copy(*as->object->getTransformation());

        }
        delete activeAnimations[as->object];
    } else {
        as->originalTransformation.copy(*as->object->getTransformation());
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
World::generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters, uint32_t index) {
    bool isAllSet = true;
    std::set<std::string> passDescriptions;//multi select is build on first occurance, so pass other times
    for (size_t i = 0; i < runParameters.size(); ++i) {
        LimonAPI::ParameterRequest& parameter = runParameters[i];
        if(passDescriptions.find(parameter.description) != passDescriptions.end()) {
            continue;
        }
        switch(parameter.requestType) {

            case LimonAPI::ParameterRequest::RequestParameterTypes::MULTI_SELECT: {
                //we get a multi select, we should build the multiselect. first one is the selected element
                parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
                if (ImGui::BeginCombo((parameter.description + "##triggerParam" + std::to_string(i) + "##" + std::to_string(index)).c_str(), parameter.value.stringValue)) {
                    for (size_t j = i+1; j < runParameters.size(); ++j) {//passing i because it should be repeated in the list
                        if(runParameters[j].requestType == LimonAPI::ParameterRequest::RequestParameterTypes::MULTI_SELECT && runParameters[j].description == runParameters[i].description) {
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
                switch (parameter.valueType) {
                    case LimonAPI::ParameterRequest::ValueTypes::DOUBLE: {
                        parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::DOUBLE;
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
                    case LimonAPI::ParameterRequest::ValueTypes::LONG:
                    default: {
                        parameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
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
            break;

            case LimonAPI::ParameterRequest::RequestParameterTypes::COORDINATE:
            case LimonAPI::ParameterRequest::RequestParameterTypes::TRANSFORM:
                std::cerr << "These parameter types are not handled!" << std::endl;

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

uint32_t World::addGuiImageAPI(const std::string &imageFilePath, const std::string &name,
                            const LimonAPI::Vec2 &position, const LimonAPI::Vec2 &scale, float rotation) {
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
        unusedIDs.push(triggerobjectID);
        return true;
    }
    return false;//not successful
}

bool World::removeObject(uint32_t objectID) {
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

    if(modelToRemove->isTransparent()) {
        if(transparentModelsInCameraFrustum.find(modelToRemove->getAssetID()) != transparentModelsInCameraFrustum.end()) {
            if(transparentModelsInCameraFrustum[modelToRemove->getAssetID()].find(modelToRemove) !=transparentModelsInCameraFrustum[modelToRemove->getAssetID()].end())
            transparentModelsInCameraFrustum[modelToRemove->getAssetID()].erase(modelToRemove);
        }
    }

    //we need to remove from ligth frustum lists, and camera frustum lists
    if(modelToRemove->isAnimated()) {
        animatedModelsInFrustum.erase(modelToRemove);
        for (size_t i = 0; i < activeLights.size(); ++i) {
            animatedModelsInLightFrustum[i].erase(modelToRemove);
        }

        animatedModelsInAnyFrustum.erase(modelToRemove);
    } else {
        modelsInCameraFrustum[modelToRemove->getAssetID()].erase(modelToRemove);
        for (size_t i = 0; i < activeLights.size(); ++i) {
            modelsInLightFrustum[i][modelToRemove->getAssetID()].erase(modelToRemove);
        }
    }


    //delete object itself
    delete modelToRemove;
    objects.erase(objectID);
    unusedIDs.push(objectID);


    return true;

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

    //setup request
    request = new GameObject::ImGuiRequest(glHelper->getCameraMatrix(), glHelper->getProjectionMatrix(),
                                           glHelper->getOrthogonalProjectionMatrix(), options->getScreenHeight(), options->getScreenWidth(), apiInstance);

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
        guiLayers.push_back(new GUILayer(glHelper, debugDrawer, 10));
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
            pickedObject = guiImage;
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
            pickedObject = guiButton;
        }
    }
}

   void World::addGUIAnimationControls() {

       /**
        * For a new GUI Image we need only name and filename
        */

       static char GUIAnimationName[32];
       ImGui::InputText("GUI Animation Name", GUIAnimationName, sizeof(GUIAnimationName), ImGuiInputTextFlags_CharsNoBlank);

       static int32_t newAnimationFrameSpeed = 60;
       ImGui::DragInt("FrameSpeed", &newAnimationFrameSpeed, 1.0f, 60.0f);

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
                pickedObject = guiAnimation;
            }
        }
   }

void World::addGUILayerControls() {
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

bool World::applyForceAPI(uint32_t objectID, const LimonAPI::Vec4 &forcePosition, const LimonAPI::Vec4 &forceAmount) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getRigidBody()->activate(true);
    btVector3 forcePositionRelative = GLMConverter::LimonToBlt(forcePosition) - model->getRigidBody()->getCenterOfMassTransform().getOrigin();

    model->getRigidBody()->applyForce(GLMConverter::LimonToBlt(forceAmount), forcePositionRelative);
    return true;
}

bool World::applyForceToPlayerAPI(const LimonAPI::Vec4 &forceAmount) {
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

std::vector<LimonAPI::ParameterRequest> World::rayCastToCursorAPI() {
   /**
    * * If nothing is hit, returns empty vector
    * returns these values:
    * 1) objectID for what is under the cursor
    * 2) hit coordinates
    * 3) hit normal
    * 4) If object has AI, id of that AI
    */
   std::vector<LimonAPI::ParameterRequest>result;
   glm::vec3 position, normal;
   GameObject* gameObject = this->getPointedObject(COLLIDE_EVERYTHING, COLLIDE_MODELS |COLLIDE_EVERYTHING, &position, &normal);

   if(gameObject == nullptr) {
       return result;
   }
   LimonAPI::ParameterRequest objectIDParam;
   objectIDParam.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
   objectIDParam.value.longValue = gameObject->getWorldObjectID();
   result.push_back(objectIDParam);

   LimonAPI::ParameterRequest positionParam;
   positionParam.valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
   positionParam.value.vectorValue.x = position.x;
   positionParam.value.vectorValue.y = position.y;
   positionParam.value.vectorValue.z = position.z;
   result.push_back(positionParam);

   LimonAPI::ParameterRequest normalParam;
   normalParam.valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
   normalParam.value.vectorValue.x = normal.x;
   normalParam.value.vectorValue.y = normal.y;
   normalParam.value.vectorValue.z = normal.z;
   result.push_back(normalParam);

   if(gameObject->getTypeID() == GameObject::MODEL) {
       Model * foundModel = dynamic_cast<Model *>(gameObject);
       if (foundModel != nullptr && foundModel->getAIID() != 0) {
           LimonAPI::ParameterRequest aiIDParam;
           aiIDParam.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
           aiIDParam.value.longValue = foundModel->getAIID();
           result.push_back(aiIDParam);
       }
   }

   return result;
}

std::vector<LimonAPI::ParameterRequest> World::getObjectTransformationAPI(uint32_t objectID) const {
    std::vector<LimonAPI::ParameterRequest> result;
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return result;
    }
    const Transformation* transformation = model->getTransformation();

    LimonAPI::ParameterRequest translate;
    translate.valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
    translate.value.vectorValue = GLMConverter::GLMToLimon(transformation->getTranslate());
    result.push_back(translate);

    LimonAPI::ParameterRequest scale;
    scale.valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
    scale.value.vectorValue = GLMConverter::GLMToLimon(transformation->getScale());
    result.push_back(scale);

    LimonAPI::ParameterRequest orientation;
    orientation.valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
    orientation.value.vectorValue.x = transformation->getOrientation().x;
    orientation.value.vectorValue.y = transformation->getOrientation().y;
    orientation.value.vectorValue.z = transformation->getOrientation().z;
    orientation.value.vectorValue.w = transformation->getOrientation().w;
    result.push_back(orientation);

    return result;
}

std::vector<LimonAPI::ParameterRequest> World::getObjectTransformationMatrixAPI(uint32_t objectID) const {
   std::vector<LimonAPI::ParameterRequest> result;
   if(objects.find(objectID) == objects.end()) {
       return result;
   }

   LimonAPI::ParameterRequest transform;
   transform.valueType = LimonAPI::ParameterRequest::ValueTypes::MAT4;
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

bool World::interactWithAIAPI(uint32_t AIID, std::vector<LimonAPI::ParameterRequest> &interactionInformation) const {
   if(actors.find(AIID) == actors.end()) {
       return false;
   }
   return actors.at(AIID)->interaction(interactionInformation);
}

void World::interactWithPlayerAPI(std::vector<LimonAPI::ParameterRequest> &interactionInformation) const {
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

void World::addTimedEventAPI(long waitTime, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall,
                              std::vector<LimonAPI::ParameterRequest> parameters) {
    timedEvents.push(TimedEvent(waitTime + gameTime, methodToCall, parameters));
}

void World::checkAndRunTimedEvents() {
    while (timedEvents.size() > 0 && timedEvents.top().callTime <= gameTime) {
        timedEvents.top().run();
        timedEvents.pop();
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

LimonAPI::Vec4 World::getPlayerModelOffsetAPI() {
   if(this->startingPlayer.attachedModel != nullptr) {
       if(physicalPlayer != nullptr) {
           return GLMConverter::GLMToLimon(physicalPlayer->getAttachedModelOffset());
       }
   }
   return LimonAPI::Vec4(0,0,0);
}

bool World::setPlayerModelOffsetAPI(LimonAPI::Vec4 newOffset) {
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
    if(objectToAttach == 0) {
        return false;
    }
    transform1 = objectToAttach->getTransformation();

    objectToAttachTo = findModelByID(objectToAttachToID);
    if(objectToAttachTo == 0) {
        return false;
    }
    transform2 = objectToAttachTo->getTransformation();

    //The offset removal of the parent is not exposed via API. We should manually remove that from the object
    transform1->addTranslate(-1 * objectToAttachTo->getCenterOffset());

    transform1->setParentTransform(transform2);
    objectToAttach->setParentObject(objectToAttachTo);
    objectToAttachTo->addChild(objectToAttach);

    return true;

}

bool World::setObjectTranslateAPI(uint32_t objectID, const LimonAPI::Vec4 &position) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }

    model->getTransformation()->setTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
    return true;
}

bool World::setObjectScaleAPI(uint32_t objectID, const LimonAPI::Vec4 &scale) {
    Model* model = findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }

    model->getTransformation()->setScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
    return true;
}

bool World::setObjectOrientationAPI(uint32_t objectID, const LimonAPI::Vec4 &orientation) {
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

bool World::addObjectTranslateAPI(uint32_t objectID, const LimonAPI::Vec4 &position) {
   Model* model = findModelByID(objectID);
   if(model == nullptr) {
       return false;
   }

   model->getTransformation()->addTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
   return true;
}

bool World::addObjectScaleAPI(uint32_t objectID, const LimonAPI::Vec4 &scale) {
   Model* model = findModelByID(objectID);
   if(model == nullptr) {
       return false;
   }

   model->getTransformation()->addScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
   return true;
}

bool World::addObjectOrientationAPI(uint32_t objectID, const LimonAPI::Vec4 &orientation) {
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

void World::buildTreeFromAllGameObjects() {

    std::vector<uint32_t> parentageList;
    if(pickedObject != nullptr) {
        if(pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL || pickedObject->getTypeID() == GameObject::ObjectTypes::MODEL_GROUP) {
            PhysicalRenderable *physicalRenderable = dynamic_cast<PhysicalRenderable *>(pickedObject);
            if(physicalRenderable != nullptr) {
                if (ImGui::Button("Find selected") || this->pickedObjectID != pickedObject->getWorldObjectID()) {//trigger find if selected object changes
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
        pickedObjectID = pickedObject->getWorldObjectID();
    }

    ImGui::BeginChild("Game Object Selector##treeMode", ImVec2(400, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    ImGuiTreeNodeFlags leafFlags = nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;//no recursion after this point

    if(!parentageList.empty()) {
        ImGui::SetNextTreeNodeOpen(true);
    }
    //objects
    if (ImGui::TreeNode("Objects##ObjectsTreeRoot")) {
        //ModelGroups
        for (auto iterator = modelGroups.begin(); iterator != modelGroups.end(); ++iterator) {
            if(iterator->second->getParentObject() != nullptr) {
                continue; //the parent will show this group
            }
            createObjectTreeRecursive(iterator->second, pickedObjectID, nodeFlags, leafFlags, parentageList);
        }
        //ModelGroups end

        //Objects recursive
        for (auto iterator = objects.begin(); iterator != objects.end(); ++iterator) {
            if(iterator->second->getParentObject() != nullptr) {
                continue; //the parent will show this group
            }
            if(iterator->second->hasChildren()) {
                createObjectTreeRecursive(iterator->second, pickedObjectID, nodeFlags, leafFlags, parentageList);
            } else {
                GameObject* currentObject = dynamic_cast<GameObject*>(iterator->second);
                if(currentObject != nullptr) {
                    bool isSelected = currentObject->getWorldObjectID() == pickedObjectID;
                    ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | (isSelected ? ImGuiTreeNodeFlags_Selected : 0));
                    if(isSelected && !parentageList.empty()) {
                        ImGui::SetScrollHereY();
                    }

                    if (ImGui::IsItemClicked()) {
                        pickedObject = currentObject;
                    }
                }
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextTreeNodeOpen(false);
    }
    //GUI elements
    if (ImGui::TreeNode("GUI Elements##guiElementsTreeRoot")) {
        for (auto iterator = guiLayers.begin(); iterator != guiLayers.end(); ++iterator) {
            if (ImGui::TreeNode((std::to_string((*iterator)->getLevel()) + "##guiLayerLevelTreeNode").c_str())) {
                std::vector<GameObject*> thisLayersElements = (*iterator)->getGuiElements();
                for (auto guiElement = thisLayersElements.begin(); guiElement != thisLayersElements.end(); ++guiElement) {
                    ImGui::TreeNodeEx((*guiElement)->getName().c_str(), leafFlags | (((*guiElement)->getWorldObjectID() == pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                    if (ImGui::IsItemClicked()) {
                        pickedObject = *guiElement;
                    }

                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextTreeNodeOpen(false);
    }
    //Lights
    if (ImGui::TreeNode("Lights##LightsTreeRoot")) {
        for (auto iterator = lights.begin(); iterator != lights.end(); ++iterator) {
            GameObject* currentObject = dynamic_cast<GameObject*>(*iterator);
            if(currentObject != nullptr) {
                ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | ((currentObject->getWorldObjectID() == pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked()) {
                    pickedObject = currentObject;
                }
            }
        }
        ImGui::TreePop();
    }

    if(!parentageList.empty()) {
        ImGui::SetNextTreeNodeOpen(false);
    }
    //Triggers
    if (ImGui::TreeNode("Trigger Volumes##TriggersTreeRoot")) {
        for (auto iterator = triggers.begin(); iterator != triggers.end(); ++iterator) {
            GameObject* currentObject = dynamic_cast<GameObject*>(iterator->second);
            if(currentObject != nullptr) {
                ImGui::TreeNodeEx(currentObject->getName().c_str(), leafFlags | ((currentObject->getWorldObjectID() == pickedObjectID) ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked()) {
                    pickedObject = currentObject;
                }
            }
        }
        ImGui::TreePop();
    }

    //player
    if(physicalPlayer == nullptr) {
        physicalPlayer = new PhysicalPlayer(1, options, cursor, startingPlayer.position, startingPlayer.orientation, startingPlayer.attachedModel);// 1 is reserved for physical player
    }
    bool isOpen = false;
    if(startingPlayer.attachedModel == nullptr) {
        ImGui::TreeNodeEx(this->physicalPlayer->getName().c_str(), leafFlags |
                                                                   ((this->physicalPlayer->getWorldObjectID() ==
                                                                     pickedObjectID) ? ImGuiTreeNodeFlags_Selected
                                                                                     : 0));
    } else {
        isOpen = ImGui::TreeNodeEx(this->physicalPlayer->getName().c_str(), nodeFlags |
                                                                   ((this->physicalPlayer->getWorldObjectID() ==
                                                                     pickedObjectID) ? ImGuiTreeNodeFlags_Selected
                                                                                     : 0));
    }
    if (ImGui::IsItemClicked()) {
        pickedObject = this->physicalPlayer;
    }
    if(isOpen) {
        createObjectTreeRecursive(startingPlayer.attachedModel, pickedObjectID, nodeFlags, leafFlags, parentageList);
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void World::createObjectTreeRecursive(PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID,
                                      ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                      std::vector<uint32_t> parentage) {
    GameObject* gameObjectOfSame = dynamic_cast<GameObject*>(physicalRenderable);
    if(physicalRenderable == nullptr || gameObjectOfSame == nullptr) {
        return;
    }
    if(!parentage.empty()) {
        if(gameObjectOfSame->getWorldObjectID() == parentage[0]) {
            ImGui::SetNextTreeNodeOpen(true);
        } else {
            ImGui::SetNextTreeNodeOpen(false);
        }

    }
    bool isSelected = gameObjectOfSame->getWorldObjectID() == pickedObjectID;
    bool isNodeOpen = ImGui::TreeNodeEx((gameObjectOfSame->getName() + "##ModelGroupsTreeElement" + std::to_string(gameObjectOfSame->getWorldObjectID())).c_str(),
           nodeFlags | ( isSelected ? ImGuiTreeNodeFlags_Selected: 0));
    if(isSelected && !parentage.empty()) {
        ImGui::SetScrollHereY();
    }
    if (ImGui::IsItemClicked()) {
        pickedObject = gameObjectOfSame;
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
                       pickedObject = currentObject;
                   }
               }
           }
       }
       ImGui::TreePop();
   }
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
        glHelper->setLight(*activeLights[lightIndex], lightIndex);
    }

    for (uint32_t i = activeLights.size(); i < NR_TOTAL_LIGHTS; ++i) {
        glHelper->removeLight(i);
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

bool World::addLightTranslateAPI(uint32_t lightID, const LimonAPI::Vec4 &position) {
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

    light->setPosition(light->getPosition() + glm::vec3(GLMConverter::LimonToGLM(position)));
    return true;
}

bool World::setLightColorAPI(uint32_t lightID, const LimonAPI::Vec4& color) {
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