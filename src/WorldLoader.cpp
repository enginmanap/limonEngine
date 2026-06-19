//
// Created by engin on 10.03.2018.
//

#include <algorithm>

#include "WorldLoader.h"
#include "GameObjects/Model.h"
#include "World.h"
#include "GameObjects/SkyBox.h"
#include "GameObjects/Light.h"
#include "GameObjects/TriggerObject.h"

#include "limonAPI/LimonAPI.h"

#ifdef TRACY_ENABLE
#include <cstring>
#include "tracy/TracyC.h"
#endif
#include "Assets/Animations/AnimationLoader.h"
#include "Assets/Animations/AnimationCustom.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUILayer.h"
#include "GameObjects/GUIText.h"
#include "ALHelper.h"
#include "GameObjects/Sound.h"
#include "GameObjects/CameraRig.h"
#include "GameObjects/GUIImage.h"
#include "GameObjects/GUIButton.h"


#include "main.h"
#include "Editor/Editor.h"
#include "WorldAPIAccessor.h"
#include "GameObjects/GUIAnimation.h"
#include "GameObjects/ModelGroup.h"
#include "GamePlay/APISerializer.h"
#include "limonAPI/CameraExtensionInterface.h"

WorldLoader::WorldLoader(std::shared_ptr<AssetManager> assetManager, InputHandler *inputHandler, OptionsUtil::Options *options, ProfilerSystem* profilerSystem) :
        options(options),
        graphicsWrapper(assetManager->getGraphicsWrapper()),
        alHelper(assetManager->getAlHelper()),
        assetManager(assetManager),
        inputHandler(inputHandler),
        profilerSystem(profilerSystem)
{}

World * WorldLoader::loadWorld(const std::string &worldFile, LimonAPI *limonAPI) const {
    World* newWorld = loadMapFromXML(worldFile, limonAPI);
    if(newWorld == nullptr) {
        std::cerr << "world load failed" << std::endl;
        return nullptr;
    }

    if(!(newWorld->verifyIDs())) {
        std::cerr << "world ID verification failed" << std::endl;
        delete newWorld;
        return nullptr;
    };

    newWorld->afterLoadFinished();
    return newWorld;
}

void WorldLoader::attachedAPIMethodsToWorld(World *world, LimonAPI *limonAPI) const {// Set api endpoints accordingly
    world->apiAccessor = new WorldAPIAccessor(world);

    limonAPI->worldAddAnimationToObject = std::bind(&WorldAPIAccessor::addAnimationToObjectWithSound, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false, std::placeholders::_4);
    limonAPI->worldAddAnimationToObjectByName = std::bind(&WorldAPIAccessor::addAnimationToObjectByNameWithSound, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false, std::placeholders::_4);
    limonAPI->worldListLoadedAnimations = std::bind(&WorldAPIAccessor::listLoadedAnimationsAPI, world->apiAccessor);
    limonAPI->worldAddGuiText = std::bind(&WorldAPIAccessor::addGuiText, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    limonAPI->worldAddGuiImage = std::bind(&WorldAPIAccessor::addGuiImageAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);

    limonAPI->worldAddModel = std::bind(&WorldAPIAccessor::addModelApi, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    limonAPI->worldSetModelTemporary = std::bind(&WorldAPIAccessor::setModelTemporaryAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldUpdateGuiText = std::bind(&WorldAPIAccessor::updateGuiText, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetResultOfTrigger = std::bind(&WorldAPIAccessor::getResultOfTrigger, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldIsInsideTrigger = std::bind(&WorldAPIAccessor::isInsideTrigger, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetObjectByName = std::bind(&WorldAPIAccessor::getObjectByName, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetObjectParent = std::bind(&WorldAPIAccessor::getObjectParent, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldIsObjectPhysicsConnected = std::bind(&WorldAPIAccessor::isObjectPhysicsConnected, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldRemoveGuiElement = std::bind(&WorldAPIAccessor::removeGuiElement, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetGuiElementPosition = std::bind(&WorldAPIAccessor::getGuiElementPositionAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetGuiElementPosition = std::bind(&WorldAPIAccessor::setGuiElementPositionAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetGuiElementVisible = std::bind(&WorldAPIAccessor::setGuiElementVisibleAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveObject = std::bind(&WorldAPIAccessor::removeObject, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAttachObjectToObject = std::bind(&WorldAPIAccessor::attachObjectToObject, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAttachObjectToObjectAtWorldPosition = std::bind(&WorldAPIAccessor::attachObjectToObjectAtWorldPosition, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveTriggerObject = std::bind(&WorldAPIAccessor::removeTriggerObject, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetObjectLinearVelocity = std::bind(&WorldAPIAccessor::getObjectLinearVelocity, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetObjectLinearVelocity = std::bind(&WorldAPIAccessor::setObjectLinearVelocity, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetObjectMass = std::bind(&WorldAPIAccessor::getObjectMass, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldDisconnectObjectFromPhysics = std::bind(&WorldAPIAccessor::disconnectObjectFromPhysics, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldReconnectObjectToPhysics = std::bind(&WorldAPIAccessor::reconnectObjectToPhysics, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldCreateCameraRig = std::bind(&WorldAPIAccessor::createCameraRig, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldActivateCameraRig = std::bind(&WorldAPIAccessor::activateCameraRig, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldDeactivateCameraRig = std::bind(&WorldAPIAccessor::deactivateCameraRig, world->apiAccessor);
    limonAPI->worldApplyForce = std::bind(&WorldAPIAccessor::applyForceAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldApplyForceToPlayer = std::bind(&WorldAPIAccessor::applyForceToPlayerAPI, world->apiAccessor, std::placeholders::_1);

    limonAPI->worldPlaySound = std::bind(&WorldAPIAccessor::playSound, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    limonAPI->worldStopSound = std::bind(&WorldAPIAccessor::stopSound, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldPauseSound = std::bind(&WorldAPIAccessor::pauseSound, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldResumeSound = std::bind(&WorldAPIAccessor::resumeSound, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetSoundVolume = std::bind(&WorldAPIAccessor::setSoundVolume, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetSoundLooped = std::bind(&WorldAPIAccessor::setSoundLooped, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldIsSoundPlaying = std::bind(&WorldAPIAccessor::isSoundPlaying, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetSoundTemporary = std::bind(&WorldAPIAccessor::setSoundTemporaryAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetMusic = std::bind(&WorldAPIAccessor::setMusic, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldStopMusic = std::bind(&WorldAPIAccessor::stopMusic, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetMusicName = std::bind(&WorldAPIAccessor::getMusicName, world->apiAccessor);
    limonAPI->worldIsMusicPlaying = std::bind(&WorldAPIAccessor::isMusicPlaying, world->apiAccessor);
    limonAPI->worldRayCastToCursor = std::bind(&WorldAPIAccessor::rayCastToCursorAPI, world->apiAccessor);
    limonAPI->worldRayCast = std::bind(&WorldAPIAccessor::rayCastAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetObjectTransformation = std::bind(&WorldAPIAccessor::getObjectTransformationAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetObjectTransformationMatrix = std::bind(&WorldAPIAccessor::getObjectTransformationMatrixAPI, world->apiAccessor, std::placeholders::_1);

    limonAPI->worldSetObjectTranslate =   std::bind(&WorldAPIAccessor::setObjectTranslateAPI,   world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetObjectMass =        std::bind(&WorldAPIAccessor::setObjectMassAPI,        world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetObjectScale =       std::bind(&WorldAPIAccessor::setObjectScaleAPI,       world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetObjectOrientation = std::bind(&WorldAPIAccessor::setObjectOrientationAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectTranslate =   std::bind(&WorldAPIAccessor::addObjectTranslateAPI,   world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectScale =       std::bind(&WorldAPIAccessor::addObjectScaleAPI,       world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectOrientation = std::bind(&WorldAPIAccessor::addObjectOrientationAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);

    limonAPI->worldInteractWithAI = std::bind(&WorldAPIAccessor::interactWithAIAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldInteractWithPlayer = std::bind(&WorldAPIAccessor::interactWithPlayerAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSimulateInput = std::bind(&WorldAPIAccessor::simulateInputAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldAddTimedEvent = std::bind(&WorldAPIAccessor::addTimedEventAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldCancelTimedEvent = std::bind(&WorldAPIAccessor::cancelTimedEventAPI, world->apiAccessor, std::placeholders::_1);

    limonAPI->worldKillPlayer = std::bind(&WorldAPIAccessor::killPlayerAPI, world->apiAccessor);
    limonAPI->worldGetPlayerAttachedModel = std::bind(&WorldAPIAccessor::getPlayerAttachedModelAPI, world->apiAccessor);
    limonAPI->worldGetModelChildren = std::bind(&WorldAPIAccessor::getModelChildrenAPI, world->apiAccessor, std::placeholders::_1);

    limonAPI->worldGetModelAnimationName = std::bind(&WorldAPIAccessor::getModelAnimationNameAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetModelAnimationFinished = std::bind(&WorldAPIAccessor::getModelAnimationFinishedAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetModelAnimationProgress = std::bind(&WorldAPIAccessor::getModelAnimationProgressAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldListModelAnimations = std::bind(&WorldAPIAccessor::listModelAnimationsAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetAnimationOfModel = std::bind(&WorldAPIAccessor::setModelAnimationAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldSetAnimationOfModelWithBlend = std::bind(&WorldAPIAccessor::setModelAnimationWithBlendAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldSetModelAnimationSpeed = std::bind(&WorldAPIAccessor::setModelAnimationSpeedAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetPlayerPosition = std::bind(&WorldAPIAccessor::getPlayerPositionAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldGetPlayerPositionVec4 = std::bind(&WorldAPIAccessor::getPlayerPositionVec4API, world->apiAccessor);
    limonAPI->worldGetPlayerLookDirection = std::bind(&WorldAPIAccessor::getPlayerLookDirectionAPI, world->apiAccessor);
    limonAPI->worldGetCameraPosition = std::bind(&WorldAPIAccessor::getCameraPositionAPI, world->apiAccessor);
    limonAPI->worldGetCameraLookDirection = std::bind(&WorldAPIAccessor::getCameraLookDirectionAPI, world->apiAccessor);
    limonAPI->worldGetPlayerAttachmentOffset = std::bind(&WorldAPIAccessor::getPlayerModelOffsetAPI, world->apiAccessor);
    limonAPI->worldSetPlayerAttachmentOffset = std::bind(&WorldAPIAccessor::setPlayerModelOffsetAPI, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldEnableParticleEmitter = std::bind(&WorldAPIAccessor::enableParticleEmitter, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldDisableParticleEmitter = std::bind(&WorldAPIAccessor::disableParticleEmitter, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldAddParticleEmitter = std::bind(&WorldAPIAccessor::addParticleEmitter, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                                                  std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9);
    limonAPI->worldRemoveParticleEmitter = std::bind(&WorldAPIAccessor::removeParticleEmitter, world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetEmitterParticleSpeed = std::bind(&WorldAPIAccessor::setEmitterParticleSpeed, world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldSetEmitterParticleGravity = std::bind(&WorldAPIAccessor::setEmitterParticleGravity, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldChangeRenderPipeline = std::bind(&WorldAPIAccessor::changeRenderPipeline, world->apiAccessor, std::placeholders::_1);

    limonAPI->worldAddLight          = std::bind(&WorldAPIAccessor::addLightAPI,          world->apiAccessor, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldRemoveLight       = std::bind(&WorldAPIAccessor::removeLightAPI,       world->apiAccessor, std::placeholders::_1);
    limonAPI->worldAddLightTranslate = std::bind(&WorldAPIAccessor::addLightTranslateAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetLightColor     = std::bind(&WorldAPIAccessor::setLightColorAPI,     world->apiAccessor, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetLightPosition  = std::bind(&WorldAPIAccessor::getLightPositionAPI,  world->apiAccessor, std::placeholders::_1);
    limonAPI->worldGetLightColor     = std::bind(&WorldAPIAccessor::getLightColorAPI,     world->apiAccessor, std::placeholders::_1);
    limonAPI->worldSetLightTranslate = std::bind(&WorldAPIAccessor::setLightTranslateAPI, world->apiAccessor, std::placeholders::_1, std::placeholders::_2);

    limonAPI->worldLog = [world](Logger::Subsystem subsystem, Logger::Level level, const std::string& text) {
        world->options->getLogger()->log(subsystem, level, text);
    };
    limonAPI->worldDrawDebugLine = [world](const LimonTypes::Vec4& from, const LimonTypes::Vec4& to,
                                           const LimonTypes::Vec4& fromColor, const LimonTypes::Vec4& toColor,
                                           bool requireCameraTransform) -> uint32_t {
        return world->options->getLogger()->drawLine(
            glm::vec3(from.x, from.y, from.z), glm::vec3(to.x, to.y, to.z),
            glm::vec3(fromColor.x, fromColor.y, fromColor.z), glm::vec3(toColor.x, toColor.y, toColor.z),
            requireCameraTransform);
    };
    limonAPI->worldAddToDebugLine = [world](uint32_t bufferIndex,
                                            const LimonTypes::Vec4& from, const LimonTypes::Vec4& to,
                                            const LimonTypes::Vec4& fromColor, const LimonTypes::Vec4& toColor,
                                            bool requireCameraTransform) -> bool {
        return world->options->getLogger()->drawLine(bufferIndex,
            glm::vec3(from.x, from.y, from.z), glm::vec3(to.x, to.y, to.z),
            glm::vec3(fromColor.x, fromColor.y, fromColor.z), glm::vec3(toColor.x, toColor.y, toColor.z),
            requireCameraTransform);
    };
    limonAPI->worldClearDebugLines = [world](uint32_t bufferIndex) -> bool {
        return world->options->getLogger()->clearLineBuffer(bufferIndex);
    };

    world->apiInstance = limonAPI;

#ifdef TRACY_ENABLE
    limonAPI->worldBeginProfileZone = [](const char* name, size_t nameLen) noexcept -> uint64_t {
        uint64_t srcloc = ___tracy_alloc_srcloc_name(0, "", 0, "", 0, name, nameLen, 0);
        ___tracy_c_zone_context ctx = ___tracy_emit_zone_begin_alloc(srcloc, 1);
        uint64_t result = 0;
        static_assert(sizeof(ctx) <= sizeof(uint64_t));
        std::memcpy(&result, &ctx, sizeof(ctx));
        return result;
    };
    limonAPI->worldEndProfileZone = [](uint64_t zoneContext) noexcept {
        ___tracy_c_zone_context ctx;
        std::memcpy(&ctx, &zoneContext, sizeof(ctx));
        ___tracy_emit_zone_end(ctx);
    };
#else
    limonAPI->worldBeginProfileZone = [](const char*, size_t) -> uint64_t { return 0; };
    limonAPI->worldEndProfileZone   = [](uint64_t) {};
#endif
}


World * WorldLoader::loadMapFromXML(const std::string &worldFileName, LimonAPI *limonAPI) const {
    uint32_t currentTime = SDL_GetTicks();

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(worldFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML "<< worldFileName << ": " <<  xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * worldNode = xmlDoc.FirstChild();
    if (worldNode == nullptr) {
        std::cerr << "World xml is not a valid XML." << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* worldName =  worldNode->FirstChildElement("Name");
    if (worldName == nullptr) {
        std::cerr << "World must have a name." << std::endl;
        return nullptr;
    }
    std::cout << "read name as " << worldName->GetText() << std::endl;

    tinyxml2::XMLElement* saveVersionElement = worldNode->FirstChildElement("SaveVersion");
    int saveVersion = 1;
    if(saveVersionElement != nullptr && saveVersionElement->GetText() != nullptr) {
        saveVersion = std::stoi(saveVersionElement->GetText());
    }

    std::string loadingImageStr;
    tinyxml2::XMLElement* worldLoadingImage =  worldNode->FirstChildElement("LoadingImage");
    if (worldLoadingImage != nullptr && worldLoadingImage->GetText() != nullptr) {
        loadingImageStr = worldLoadingImage->GetText();
    }

    tinyxml2::XMLElement* worldStartPlayer =  worldNode->FirstChildElement("Player");
    World::PlayerInfo startingPlayer;
    if (worldStartPlayer == nullptr) {
        std::cerr << "World starting player not found, will start with Physical player at 0,0,0." << std::endl;
    } else {
        //load player
        tinyxml2::XMLElement* playerType =  worldStartPlayer->FirstChildElement("Type");
        if (playerType == nullptr) {
            std::cerr << "World starting player type not found, will start with Physical player." << std::endl;
        } else {
            std::string playerTypeName = playerType->GetText();
            startingPlayer.setType(playerTypeName);
        }

        tinyxml2::XMLElement* playerStartPosition =  worldStartPlayer->FirstChildElement("Position");
        if(playerStartPosition != nullptr) {
            loadVec3(playerStartPosition, startingPlayer.position);
        }
        tinyxml2::XMLElement* playerStartOrientation =  worldStartPlayer->FirstChildElement("Orientation");
        if(playerStartPosition != nullptr) {
            loadVec3(playerStartOrientation, startingPlayer.orientation);
        }

        tinyxml2::XMLElement* playerExtension =  worldStartPlayer->FirstChildElement("ExtensionName");
        if(playerExtension != nullptr) {
            if(playerExtension->GetText() != nullptr) {
                startingPlayer.extensionName = playerExtension->GetText();
            }
        }

        tinyxml2::XMLElement* playerExtensionParameters =  worldStartPlayer->FirstChildElement("ExtensionParameters");
        if(playerExtensionParameters != nullptr) {
            tinyxml2::XMLElement* parameterNode = playerExtensionParameters->FirstChildElement("Parameter");
            uint32_t index;
            while(parameterNode != nullptr) {
                std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterNode, index);
                if(request != nullptr && index <= startingPlayer.parameters.size()) {
                    startingPlayer.parameters.insert(startingPlayer.parameters.begin() + index, *request);
                }
                parameterNode = parameterNode->NextSiblingElement("Parameter");
            }
        }

        tinyxml2::XMLElement* playerAttachmentModel =  worldStartPlayer->FirstChildElement("Attachment");
        if(playerAttachmentModel != nullptr) {
            if(saveVersion >= 2) {
                loadPlayerAttachmentV2(playerAttachmentModel, startingPlayer.attachedModel, limonAPI);
            } else {
                loadPlayerAttachmentV1(playerAttachmentModel, startingPlayer.attachedModel, limonAPI);
            }
        }
    }

    World* world = new World(std::string(worldName->GetText()), startingPlayer, inputHandler, assetManager, options, profilerSystem);

    attachedAPIMethodsToWorld(world, limonAPI);
    world->loadingImage = loadingImageStr;

    // Camera rigs are loaded later (loadCameraRigs), after objects exist, because a rig may attach to one.

    tinyxml2::XMLElement* musicNameNode =  worldNode->FirstChildElement("Music");
    if (musicNameNode == nullptr) {
        std::cout << "No music found." << std::endl;
    } else {
        std::string musicName = musicNameNode->GetText();
        std::cout << "reading music as as " << musicName << std::endl;
        //configure here but defer play() to World::loadAndChangeWorld (the "switch to this world" moment)
        world->music = std::make_unique<Sound>(world->getNextObjectID(), assetManager, musicName);
        world->music->setChannel(LimonTypes::AudioChannel::MUSIC);
        world->music->setLoop(true);
        world->music->setWorldPosition(glm::vec3(0,0,0), true);
    }

    tinyxml2::XMLElement* returnCustomWorld =  worldNode->FirstChildElement("QuitResponse");
    if (returnCustomWorld == nullptr) {
        std::cout << "Return custom world flag can't be read, assuming Quit." << std::endl;
    } else {
        if(!strcmp(returnCustomWorld->GetText(),"LoadWorld")) {
            world->currentQuitResponse = World::QuitResponse::LOAD_WORLD;
        } else if(!strcmp(returnCustomWorld->GetText(),"ReturnPrevious")) {
            world->currentQuitResponse = World::QuitResponse::RETURN_PREVIOUS;
        } else if(!strcmp(returnCustomWorld->GetText(),"QuitGame")) {
            world->currentQuitResponse = World::QuitResponse::QUIT_GAME;
        } else {
            std::cerr << "Return custom world flag found but value was unknown. Assuming Quit" << std::endl;
        }
    }

    tinyxml2::XMLElement* quitWorldName =  worldNode->FirstChildElement("QuitWorldName");
    if (quitWorldName != nullptr) {
        if(quitWorldName->GetText() != nullptr) {
            world->quitWorldName =quitWorldName->GetText();
            strncpy(world->editor->quitWorldNameBuffer, world->quitWorldName.c_str(), sizeof(world->editor->quitWorldNameBuffer) - 1);
        } else {
            world->quitWorldName = "";
            strncpy(world->editor->quitWorldNameBuffer, world->quitWorldName.c_str(), sizeof(world->editor->quitWorldNameBuffer) - 1);
        }
    }

    tinyxml2::XMLElement* soundDistanceModelEl = worldNode->FirstChildElement("SoundDistanceModel");
    if(soundDistanceModelEl != nullptr && soundDistanceModelEl->GetText() != nullptr) {
        std::string modelStr = soundDistanceModelEl->GetText();
        if(modelStr == "InverseClamped") {
            world->soundDistanceModel = ALHelper::DistanceModel::INVERSE_CLAMPED;
        } else if(modelStr == "ExponentClamped") {
            world->soundDistanceModel = ALHelper::DistanceModel::EXPONENT_CLAMPED;
        } else {
            world->soundDistanceModel = ALHelper::DistanceModel::LINEAR_CLAMPED;
        }
    }

    if (!loadMaterials(worldNode, world)) {
        delete world;
        return nullptr;
    }


    //load objects
    if(!loadObjectsFromXML(worldNode, world, limonAPI, saveVersion)) {
        delete world;
        return nullptr;
    }

    loadAnimations(worldNode, world);
    //load Skymap
    loadSkymap(worldNode, world);

    //load emitters
    loadParticleEmitters(worldNode, world);
    //load GPU emitters
    loadGPUParticleEmitters(worldNode, world);

    loadGUILayersAndElements(worldNode, world);

    //load triggers
    loadTriggers(worldNode, world);

    //We are changing the light ID generation, so we need to make sure:
    // 1) All objects are loaded before lights
    // 2) The World did finish ID verification
    // After some time, this can be dropped, it is here only for backwards compatibility
    if (!world->verifyIDs()) {
        std::cerr << "World ID verification failed before light loading." << std::endl;
        delete world;
        return nullptr;
    }
    loadLights(worldNode, world);
    loadSounds(worldNode, world);
    loadCameraRigs(worldNode, world);

    //load onloadActions
    loadOnLoadActions(worldNode, world);

    loadOnLoadAnimations(worldNode, world);
    uint32_t endTime = SDL_GetTicks();
    std::cout << "World " << worldName->GetText() << " loaded in " << endTime - currentTime << "ms." << std::endl;
    return world;
}

// Old V1 path: groups save children nested in <Children>. ModelGroup::deserializeV1 loads
// them and returns inner models here so they can be added to the world. V2 uses the flat loader below.
bool WorldLoader::loadObjectGroupsFromXMLV1(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI,
        std::vector<Model*> &notStaticObjects, bool &isAIGridStartPointSet, glm::vec3 &aiGridStartPoint) const {
    tinyxml2::XMLElement* objectGroupsListNode =  worldNode->FirstChildElement("ObjectGroups");
    if (objectGroupsListNode == nullptr) {
        std::cout << "World doesn't have Object Groups clause." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectGroupNode =  objectGroupsListNode->FirstChildElement("ObjectGroup");
    if (objectGroupNode == nullptr) {
        std::cout << "World doesn't have any object Groups." << std::endl;
        return true;
    }

    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;
    std::map<uint32_t , ModelGroup*> modelGroups;
    std::vector<std::unique_ptr<ObjectInformation>> innerModels;

    while(objectGroupNode != nullptr) {
        ModelGroup* modelGroup = ModelGroup::deserializeV1(graphicsWrapper, assetManager, objectGroupNode, requiredSounds,
                                                         modelGroups, innerModels, limonAPI, nullptr);
        world->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;
        objectGroupNode = objectGroupNode->NextSiblingElement("ObjectGroup");
    } // end of while (objects)

    for (auto iterator = modelGroups.begin(); iterator != modelGroups.end(); ++iterator) {
        world->modelGroups[iterator->first] = iterator->second;
    }

    for (auto modelIterator = innerModels.begin(); modelIterator != innerModels.end(); ++modelIterator) {
        ObjectInformation* objectInfo = modelIterator->get();
        if(objectInfo) { //if not null
            if(objectInfo->modelActor != nullptr) {
                world->addActor(objectInfo->modelActor);
            }
            if(!isAIGridStartPointSet && objectInfo->isAIGridStartPointSet ) { //if this is the first actor to set AI grid start point
                aiGridStartPoint = objectInfo->aiGridStartPoint;
            }
        }
        // We will add static objects first, build AI grid, then add other objects
        if(objectInfo->model->getMass() == 0 && !objectInfo->model->isAnimated()) {
            world->addModelToWorld(objectInfo->model);
        } else {
            notStaticObjects.push_back(objectInfo->model);
        }
    }

    return true;
}

bool WorldLoader::loadObjectGroupsFromXMLV2(tinyxml2::XMLNode *worldNode, World *world) const {

    tinyxml2::XMLElement* objectGroupsListNode =  worldNode->FirstChildElement("ObjectGroups");
    if (objectGroupsListNode == nullptr) {
        std::cout << "World doesn't have Object Groups clause." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectGroupNode =  objectGroupsListNode->FirstChildElement("ObjectGroup");
    if (objectGroupNode == nullptr) {
        std::cout << "World doesn't have any object Groups." << std::endl;
        return true;
    }

    // Group children (models and nested groups) are loaded flat: model children come from <Objects>
    // and nested groups from sibling <ObjectGroup> entries, each reattached via its own <ParentID>.
    // (notStaticObjects / AI grid start point for those children are handled by the flat <Objects> pass.)
    struct PendingGroupAttachment { ModelGroup* group; uint32_t parentID; };
    std::vector<PendingGroupAttachment> pendingGroupAttachments;

    // First pass: create every group (as a root for now).
    while(objectGroupNode != nullptr) {
        uint32_t parentID = 0;
        ModelGroup* modelGroup = ModelGroup::deserialize(graphicsWrapper, objectGroupNode, parentID);
        if(modelGroup != nullptr) {
            world->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;
            if(parentID != 0) {
                pendingGroupAttachments.push_back({modelGroup, parentID});
            }
        }
        objectGroupNode = objectGroupNode->NextSiblingElement("ObjectGroup");
    } // end of while (objects)

    // Second pass: wire nested group -> parent. Done here (before flat objects load) so a nested group's
    // world transform is settled before its own model children attach in the <Objects> second pass.
    for(auto& pendingGroupAttachment : pendingGroupAttachments) {
        Attachable* parent = world->findAttachableByID(pendingGroupAttachment.parentID);
        if(parent == nullptr) {
            std::cerr << "Object group " << pendingGroupAttachment.group->getWorldObjectID() << " parent "
                      << pendingGroupAttachment.parentID << " not found, attachment skipped." << std::endl;
            continue;
        }
        ModelGroup* parentGroup = dynamic_cast<ModelGroup*>(parent);
        if(parentGroup != nullptr) {
            parentGroup->addChild(pendingGroupAttachment.group);//averaging re-centers the group gizmo on its children's centroid; child world positions are preserved
        } else {
            pendingGroupAttachment.group->attachTo(parent);
        }
    }

    return true;
}

void WorldLoader::loadPlayerAttachmentV1(tinyxml2::XMLElement* attachmentNode, Model*& attachedModel, LimonAPI* limonAPI) const {
    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;
    tinyxml2::XMLElement* objectNode = attachmentNode->FirstChildElement("Object");
    if(objectNode == nullptr) {
        return;
    }
    std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(assetManager, objectNode, requiredSounds, limonAPI, nullptr);
    for(auto& info : objectInfos) {
        if(info->modelActor != nullptr) {
            std::cerr << "There was an AI attached to player model, this shouldn't happen. Ignoring" << std::endl;
            delete info->modelActor;
        }
        attachedModel = info->model;
        if(attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_BASIC))) {
            attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_BASIC);
        } else if(attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_ANIMATED))) {
            attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_ANIMATED);
        } else if(attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_TRANSPARENT))) {
            attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_TRANSPARENT);
        }
    }
}

void WorldLoader::loadPlayerAttachmentV2(tinyxml2::XMLElement* attachmentNode, Model*& attachedModel, LimonAPI* limonAPI) const {
    struct PendingAttachmentLocal { Model* child; uint32_t parentID; int32_t boneID; };
    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;
    std::unordered_map<uint32_t, Model*> attachmentModels;
    std::vector<PendingAttachmentLocal> pending;

    tinyxml2::XMLElement* objectNode = attachmentNode->FirstChildElement("Object");
    while(objectNode != nullptr) {
        auto objectInfos = loadObjectV2(assetManager, objectNode, requiredSounds, limonAPI);
        for(auto& info : objectInfos) {
            if(info->modelActor != nullptr) {
                std::cerr << "There was an AI attached to player model, this shouldn't happen. Ignoring" << std::endl;
                delete info->modelActor;
            }
            Model* m = info->model;
            attachmentModels[m->getWorldObjectID()] = m;
            if(m->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_BASIC))) {
                m->addTag(HardCodedTags::OBJECT_PLAYER_BASIC);
            } else if(m->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_ANIMATED))) {
                m->addTag(HardCodedTags::OBJECT_PLAYER_ANIMATED);
            } else if(m->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_TRANSPARENT))) {
                m->addTag(HardCodedTags::OBJECT_PLAYER_TRANSPARENT);
            }
            tinyxml2::XMLElement* parentIDElement = objectNode->FirstChildElement("ParentID");
            if(parentIDElement != nullptr && parentIDElement->GetText() != nullptr) {
                uint32_t parentID = std::stoul(parentIDElement->GetText());
                int32_t boneID = -1;
                tinyxml2::XMLElement* boneIDElement = objectNode->FirstChildElement("ParentBoneID");
                if(boneIDElement != nullptr && boneIDElement->GetText() != nullptr) {
                    boneID = std::stoi(boneIDElement->GetText());
                }
                pending.push_back({m, parentID, boneID});
            } else {
                attachedModel = m;
            }
        }
        objectNode = objectNode->NextSiblingElement("Object");
    }

    for(auto& pa : pending) {
        auto it = attachmentModels.find(pa.parentID);
        if(it != attachmentModels.end()) {
            Model* parentModel = it->second;
            if(pa.boneID != -1) {
                pa.child->setParentObject(parentModel, pa.boneID);
                parentModel->addChild(pa.child);
                pa.child->getTransformation()->setParentTransform(
                    parentModel->getAttachmentTransformForKnownBone(pa.boneID));
            } else {
                pa.child->attachTo(parentModel, pa.boneID);
            }
        } else {
            std::cerr << "Attachment child " << pa.child->getWorldObjectID() << " parent " << pa.parentID << " not found, skipped." << std::endl;
        }
    }
}

bool WorldLoader::loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World *world, LimonAPI *limonAPI, int saveVersion) const {
    if(saveVersion == 2) {
        return loadObjectsFromXMLV2(objectsNode, world, limonAPI);
    }

    // TODO: Remove this v1 backwards-compatible path once all world files have been resaved as v2.
    std::vector<std::vector<std::string>> preloadAssetFiles; //used to load the assets in parallel instead of serial
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);

    //first load the groups
    loadObjectGroupsFromXMLV1(objectsNode, world, limonAPI, notStaticObjects, isAIGridStartPointSet, aiGridStartPoint);

    tinyxml2::XMLElement* objectsListNode =  objectsNode->FirstChildElement("Objects");
    if (objectsListNode == nullptr) {
        std::cerr << "World doesn't have Objects clause, this might be a mistake." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectNode =  objectsListNode->FirstChildElement("Object");
    if (objectNode == nullptr) {
        std::cout << "World doesn't have any objects, this might be a mistake." << std::endl;
        return true;
    }


    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;


    tinyxml2::XMLElement* objectNodeForPreload =  objectNode;
    while(objectNodeForPreload != nullptr) {
        tinyxml2::XMLElement *objectAttribute =  objectNodeForPreload->FirstChildElement("File");
        if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
            objectNodeForPreload = objectNodeForPreload->NextSiblingElement("Object");
            continue;
        }
        std::string modelFile = objectAttribute->GetText();
        tinyxml2::XMLElement *flipAttributeForPreload = objectNodeForPreload->FirstChildElement("Flip");
        std::string flipAxesForPreload;
        if (flipAttributeForPreload != nullptr && flipAttributeForPreload->GetText() != nullptr) {
            flipAxesForPreload = flipAttributeForPreload->GetText();
        }
        std::string assetKeyForPreload = flipAxesForPreload.empty() ? modelFile : modelFile + "?flip" + flipAxesForPreload;
        std::vector<std::string> temp;
        temp.emplace_back(assetKeyForPreload);
        preloadAssetFiles.emplace_back(temp);
        objectNodeForPreload = objectNodeForPreload->NextSiblingElement("Object");
    }

    std::vector<std::shared_ptr<ModelAsset>> preloadAssets = assetManager->parallelLoadAssetList<ModelAsset>(preloadAssetFiles); //loads the assets in parallel()

    while(objectNode != nullptr) {

        std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(assetManager, objectNode,
                                                                                 requiredSounds, limonAPI, nullptr);//this map is used to load all the sounds, while sharing same objects.

        for (auto objectIterator = objectInfos.begin(); objectIterator != objectInfos.end(); ++objectIterator) {
            if((*objectIterator)->modelActor != nullptr) {
                world->addActor((*objectIterator)->modelActor);
            }
            if(!isAIGridStartPointSet && (*objectIterator)->isAIGridStartPointSet ) { //if this is the first actor to set AI grid start point
                aiGridStartPoint = (*objectIterator)->aiGridStartPoint;
            }

            // We will add static objects first, build AI grid, then add other objects
            if((*objectIterator)->model->getMass() == 0 && !(*objectIterator)->model->isAnimated()) {
                world->addModelToWorld((*objectIterator)->model);
            } else {
                notStaticObjects.push_back((*objectIterator)->model);
            }
        }

        //DON'T ADD NEW ATTRIBUTES HERE STATIC AND OTHER OBJECTS ARE HANDLED DIFFERENTLY, ADD ATTRIBUTES BEFORE THAT

        objectNode = objectNode->NextSiblingElement("Object");
    } // end of while (objects)

    world->createGridFrom(aiGridStartPoint);

    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        world->addModelToWorld(notStaticObjects[i]);
    }

    //clear up the preloaded asset counts.
    for(const auto& assetFile:preloadAssetFiles) {
        assetManager->freeAsset({assetFile});
    }
    return true;
}

/**
 * Last element in the vector is the parent of all
 * @param assetManager
 * @param objectNode
 * @param requiredSounds
 * @param limonAPI
 * @param parentObject
 * @return
 */
std::vector<std::unique_ptr<WorldLoader::ObjectInformation>>
WorldLoader::loadObject( std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                        std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI,
                        PhysicalRenderable *parentObject) {
    std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> loadedObjects;

    tinyxml2::XMLElement *objectAttribute =  objectNode->FirstChildElement("File");
    if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
        return loadedObjects;
        }
    std::string modelFile = objectAttribute->GetText();
    objectAttribute =  objectNode->FirstChildElement("Mass");
    float modelMass;
    if (objectAttribute == nullptr) {
            //std::cout << "Object does not have mass, assume 0." << std::endl;
            modelMass = 0;
        } else {
            modelMass = std::stof(objectAttribute->GetText());
        }
    int id;
    objectAttribute =  objectNode->FirstChildElement("ID");
    if (objectAttribute == nullptr) {
            std::cerr << "Object does not have ID. Can't be loaded" << std::endl;
            return loadedObjects;
        } else {
            id = std::stoi(objectAttribute->GetText());
        }

    bool disconnected = false;
    objectAttribute =  objectNode->FirstChildElement("Disconnected");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
            //std::cout << "Object disconnect status is not set. defaulting to False" << std::endl;
#endif
        } else {
            std::string disConnectedText = objectAttribute->GetText();
            if(disConnectedText == "True") {
                disconnected = true;
            } else if(disConnectedText == "False") {
                disconnected = false;
            } else {
                //std::cout << "Object disconnect status is unknown. defaulting to False" << std::endl;
            }
        }

    std::string flipAxes;
    tinyxml2::XMLElement *flipAttribute = objectNode->FirstChildElement("Flip");
    if (flipAttribute != nullptr && flipAttribute->GetText() != nullptr) {
        flipAxes = flipAttribute->GetText();
    }

    std::unique_ptr<ObjectInformation> loadedObjectInformation = std::make_unique<ObjectInformation>();
    loadedObjectInformation->model = new Model(id, assetManager, modelMass, modelFile, disconnected, flipAxes);

    int32_t parentBoneID = -1;
    objectAttribute =  objectNode->FirstChildElement("ParentBoneID");

    if (objectAttribute != nullptr) {
        parentBoneID = std::stoi(objectAttribute->GetText());
    }

    loadedObjectInformation->model->setParentObject(parentObject, parentBoneID);

    objectAttribute =  objectNode->FirstChildElement("StepOnSound");

    if (objectAttribute == nullptr) {
        //std::cerr << "Object does not have step on sound." << std::endl;
    } else {
        std::string stepOnSound = objectAttribute->GetText();
        if(requiredSounds.find(stepOnSound) == requiredSounds.end()) {
            requiredSounds[stepOnSound] = std::make_shared<Sound>(0, assetManager, stepOnSound);//since the step on is not managed by world, not feed world object ID
            requiredSounds[stepOnSound]->changeGain(0.125f);
        }
        loadedObjectInformation->model->setPlayerStepOnSound(requiredSounds[stepOnSound]);
    }

    objectAttribute =  objectNode->FirstChildElement("Transformation");
    if(objectAttribute == nullptr) {
            std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
            delete loadedObjectInformation->model;
        return loadedObjects;
    }
    loadedObjectInformation->model->getTransformation()->deserialize(objectAttribute);
    if(parentObject != nullptr) {
        Model* parentModel = dynamic_cast<Model*>(parentObject);
        if(parentModel != nullptr) {
            loadedObjectInformation->model->getTransformation()->setParentTransform(
                    parentModel->getAttachmentTransformForKnownBone(parentBoneID));
        } else {
            loadedObjectInformation->model->getTransformation()->setParentTransform(
                    parentObject->getTransformation());
        }
    }
    //Since we are not loading objects recursively, these can be set here safely
    objectAttribute =  objectNode->FirstChildElement("Actor");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
        //std::cout << "Object does not have AI." << std::endl;
#endif
    } else {
        ActorInterface* actor = APISerializer::deserializeActorInterface(objectAttribute, limonAPI);

        loadedObjectInformation->aiGridStartPoint = GLMConverter::BltToGLM(loadedObjectInformation->model->getRigidBody()->getCenterOfMassPosition()) +
                           glm::vec3(0, 2.0f, 0);
        loadedObjectInformation->isAIGridStartPointSet = true;
        if(actor != nullptr) {//most likely shared library not found, but in general possible.
            loadedObjectInformation->modelActor = actor;
            loadedObjectInformation->modelActor->setModel(loadedObjectInformation->model->getWorldObjectID());
            loadedObjectInformation->model->attachAI(loadedObjectInformation->modelActor);
        }
    }

    objectAttribute =  objectNode->FirstChildElement("Animation");
    if (objectAttribute == nullptr || objectAttribute->GetText() == nullptr) {
#ifndef NDEBUG
        //std::cout << "Object does not have default animation." << std::endl;
#endif
    } else {
        loadedObjectInformation->model->setAnimation(objectAttribute->GetText());
    }

    //now load children

    tinyxml2::XMLElement* childrenNode =  objectNode->FirstChildElement("Children");

    if(childrenNode != nullptr) {
        //means there are children

        tinyxml2::XMLElement* childrenCountNode =  childrenNode->FirstChildElement("Count");
        if(childrenCountNode == nullptr || childrenCountNode->GetText() == nullptr) {
            std::cerr << "Object has children node, but count it unknown. Children can't be loaded! " << std::endl;
                    } else {
            //loadedObjectInformation->model->children.resize(childCount);
            tinyxml2::XMLElement *childNode = childrenNode->FirstChildElement("Child");
            while (childNode != nullptr) {
                tinyxml2::XMLElement *childObjectNode = childNode->FirstChildElement("Object");
                std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> objectInfos = loadObject(assetManager,
                                                                                                      childObjectNode,
                                                                                                      requiredSounds,
                                                                                                      limonAPI,
                                                                                                      loadedObjectInformation->model);

                loadedObjectInformation->model->addChild(objectInfos[objectInfos.size()-1]->model);//we know the root of the list is the last element

                std::move(std::begin(objectInfos), std::end(objectInfos), std::back_inserter(loadedObjects));
                childNode = childNode->NextSiblingElement("Child");
            }
        }
    }
    //Now Load material changes
    std::vector<std::pair<std::string, std::shared_ptr<Material>>> custumizedMeshMaterialList;
    tinyxml2::XMLElement* meshMaterialListNode = objectNode->FirstChildElement("MeshMaterialList");
    if (meshMaterialListNode != nullptr) {
        tinyxml2::XMLElement *meshMaterialNode = meshMaterialListNode->FirstChildElement("MeshMaterial");
        while (meshMaterialNode != nullptr) {
            if (meshMaterialNode->Attribute("MeshName") == nullptr) {
                std::cerr << "A mesh material definition with no MeshName found. Can't be processed. Skipping." << std::endl;
            } else {
                std::string meshName = meshMaterialNode->Attribute("MeshName");
                tinyxml2::XMLElement *materialNode = meshMaterialNode->FirstChildElement("Material");
                if (materialNode == nullptr) {
                    std::cerr << "A mesh material definition with no Material found. Can't be processed. Skipping." << std::endl;
                } else {
                    std::shared_ptr<Material> newMaterial = Material::deserialize(assetManager.get(), materialNode);
                    custumizedMeshMaterialList.emplace_back(meshName, newMaterial);
                }
            }
            meshMaterialNode = meshMaterialNode->NextSiblingElement("MeshMaterial");
        }
    }
    if (!custumizedMeshMaterialList.empty()) {
        loadedObjectInformation->model->loadOverriddenMeshMaterial(custumizedMeshMaterialList);
    }

    tinyxml2::XMLElement* customTagsNode = objectNode->FirstChildElement("CustomTags");
    if (customTagsNode != nullptr) {
        tinyxml2::XMLElement *customTagNode = customTagsNode->FirstChildElement("CustomTag");
        while (customTagNode != nullptr) {
            if (customTagNode->GetText() != nullptr) {
                loadedObjectInformation->model->addTag(customTagNode->GetText());
            }
            customTagNode = customTagNode->NextSiblingElement("CustomTag");
        }
    }


    loadedObjects.push_back(std::move(loadedObjectInformation));


    return loadedObjects;
}

// V2 object loader: loads a single object with no parent/children handling.
// Attachment is deferred to loadObjectsFromXMLV2 which calls attachTo() after all objects are in the world.
std::vector<std::unique_ptr<WorldLoader::ObjectInformation>>
WorldLoader::loadObjectV2(std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                          std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI) {
    std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> loadedObjects;

    tinyxml2::XMLElement *objectAttribute = objectNode->FirstChildElement("File");
    if (objectAttribute == nullptr) {
        std::cerr << "Object must have a source file." << std::endl;
        return loadedObjects;
    }
    std::string modelFile = objectAttribute->GetText();

    objectAttribute = objectNode->FirstChildElement("Mass");
    float modelMass = 0;
    if (objectAttribute != nullptr) {
        modelMass = std::stof(objectAttribute->GetText());
    }

    int id;
    objectAttribute = objectNode->FirstChildElement("ID");
    if (objectAttribute == nullptr) {
        std::cerr << "Object does not have ID. Can't be loaded" << std::endl;
        return loadedObjects;
    }
    id = std::stoi(objectAttribute->GetText());

    bool disconnected = false;
    objectAttribute = objectNode->FirstChildElement("Disconnected");
    if (objectAttribute != nullptr && objectAttribute->GetText() != nullptr) {
        disconnected = (std::string(objectAttribute->GetText()) == "True");
    }

    std::string flipAxes;
    tinyxml2::XMLElement *flipAttribute = objectNode->FirstChildElement("Flip");
    if (flipAttribute != nullptr && flipAttribute->GetText() != nullptr) {
        flipAxes = flipAttribute->GetText();
    }

    std::unique_ptr<ObjectInformation> loadedObjectInformation = std::make_unique<ObjectInformation>();
    loadedObjectInformation->model = new Model(id, assetManager, modelMass, modelFile, disconnected, flipAxes);

    objectAttribute = objectNode->FirstChildElement("StepOnSound");
    if (objectAttribute != nullptr) {
        std::string stepOnSound = objectAttribute->GetText();
        if(requiredSounds.find(stepOnSound) == requiredSounds.end()) {
            requiredSounds[stepOnSound] = std::make_shared<Sound>(0, assetManager, stepOnSound);
            requiredSounds[stepOnSound]->changeGain(0.125f);
        }
        loadedObjectInformation->model->setPlayerStepOnSound(requiredSounds[stepOnSound]);
    }

    objectAttribute = objectNode->FirstChildElement("Transformation");
    if(objectAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
        delete loadedObjectInformation->model;
        return loadedObjects;
    }
    loadedObjectInformation->model->getTransformation()->deserialize(objectAttribute);

    objectAttribute = objectNode->FirstChildElement("Actor");
    if (objectAttribute != nullptr) {
        ActorInterface* actor = APISerializer::deserializeActorInterface(objectAttribute, limonAPI);
        loadedObjectInformation->aiGridStartPoint = GLMConverter::BltToGLM(loadedObjectInformation->model->getRigidBody()->getCenterOfMassPosition()) +
                                                    glm::vec3(0, 2.0f, 0);
        loadedObjectInformation->isAIGridStartPointSet = true;
        if(actor != nullptr) {
            loadedObjectInformation->modelActor = actor;
            loadedObjectInformation->modelActor->setModel(loadedObjectInformation->model->getWorldObjectID());
            loadedObjectInformation->model->attachAI(loadedObjectInformation->modelActor);
        }
    }

    objectAttribute = objectNode->FirstChildElement("Animation");
    if (objectAttribute != nullptr && objectAttribute->GetText() != nullptr) {
        loadedObjectInformation->model->setAnimation(objectAttribute->GetText());
    }

    std::vector<std::pair<std::string, std::shared_ptr<Material>>> custumizedMeshMaterialList;
    tinyxml2::XMLElement* meshMaterialListNode = objectNode->FirstChildElement("MeshMaterialList");
    if (meshMaterialListNode != nullptr) {
        tinyxml2::XMLElement *meshMaterialNode = meshMaterialListNode->FirstChildElement("MeshMaterial");
        while (meshMaterialNode != nullptr) {
            if (meshMaterialNode->Attribute("MeshName") != nullptr) {
                std::string meshName = meshMaterialNode->Attribute("MeshName");
                tinyxml2::XMLElement *materialNode = meshMaterialNode->FirstChildElement("Material");
                if (materialNode != nullptr) {
                    std::shared_ptr<Material> newMaterial = Material::deserialize(assetManager.get(), materialNode);
                    custumizedMeshMaterialList.emplace_back(meshName, newMaterial);
                } else {
                    std::cerr << "A mesh material definition with no Material found. Can't be processed. Skipping." << std::endl;
                }
            } else {
                std::cerr << "A mesh material definition with no MeshName found. Can't be processed. Skipping." << std::endl;
            }
            meshMaterialNode = meshMaterialNode->NextSiblingElement("MeshMaterial");
        }
    }
    if (!custumizedMeshMaterialList.empty()) {
        loadedObjectInformation->model->loadOverriddenMeshMaterial(custumizedMeshMaterialList);
    }

    tinyxml2::XMLElement* customTagsNode = objectNode->FirstChildElement("CustomTags");
    if (customTagsNode != nullptr) {
        tinyxml2::XMLElement *customTagNode = customTagsNode->FirstChildElement("CustomTag");
        while (customTagNode != nullptr) {
            if (customTagNode->GetText() != nullptr) {
                loadedObjectInformation->model->addTag(customTagNode->GetText());
            }
            customTagNode = customTagNode->NextSiblingElement("CustomTag");
        }
    }

    loadedObjects.push_back(std::move(loadedObjectInformation));
    return loadedObjects;
}

bool WorldLoader::loadObjectsFromXMLV2(tinyxml2::XMLNode *objectsNode, World *world, LimonAPI *limonAPI) const {
    struct PendingAttachment {
        Model* child;
        uint32_t parentID;
        int32_t parentBoneID;
    };

    std::vector<std::vector<std::string>> preloadAssetFiles;
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);
    std::vector<PendingAttachment> pendingAttachments;

    loadObjectGroupsFromXMLV2(objectsNode, world);

    tinyxml2::XMLElement* objectsListNode = objectsNode->FirstChildElement("Objects");
    if (objectsListNode == nullptr) {
        std::cerr << "World doesn't have Objects clause, this might be a mistake." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectNode = objectsListNode->FirstChildElement("Object");
    if (objectNode == nullptr) {
        std::cout << "World doesn't have any objects, this might be a mistake." << std::endl;
        return true;
    }

    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;

    tinyxml2::XMLElement* objectNodeForPreload = objectNode;
    while(objectNodeForPreload != nullptr) {
        tinyxml2::XMLElement *objectAttribute = objectNodeForPreload->FirstChildElement("File");
        if (objectAttribute != nullptr) {
            std::string modelFile = objectAttribute->GetText();
            tinyxml2::XMLElement *flipAttributeForPreload = objectNodeForPreload->FirstChildElement("Flip");
            std::string flipAxesForPreload;
            if (flipAttributeForPreload != nullptr && flipAttributeForPreload->GetText() != nullptr) {
                flipAxesForPreload = flipAttributeForPreload->GetText();
            }
            std::string assetKeyForPreload = flipAxesForPreload.empty() ? modelFile : modelFile + "?flip" + flipAxesForPreload;
            std::vector<std::string> temp;
            temp.emplace_back(assetKeyForPreload);
            preloadAssetFiles.emplace_back(temp);
        } else {
            std::cerr << "Object must have a source file." << std::endl;
        }
        objectNodeForPreload = objectNodeForPreload->NextSiblingElement("Object");
    }
    std::vector<std::shared_ptr<ModelAsset>> preloadAssets = assetManager->parallelLoadAssetList<ModelAsset>(preloadAssetFiles);

    // First pass: load all objects as roots (no attachment yet)
    while(objectNode != nullptr) {
        std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObjectV2(assetManager, objectNode, requiredSounds, limonAPI);

        Model* loadedModel = nullptr;
        for (auto objectIterator = objectInfos.begin(); objectIterator != objectInfos.end(); ++objectIterator) {
            if((*objectIterator)->modelActor != nullptr) {
                world->addActor((*objectIterator)->modelActor);
            }
            if(!isAIGridStartPointSet && (*objectIterator)->isAIGridStartPointSet) {
                aiGridStartPoint = (*objectIterator)->aiGridStartPoint;
            }
            loadedModel = (*objectIterator)->model;
            if((*objectIterator)->model->getMass() == 0 && !(*objectIterator)->model->isAnimated()) {
                world->addModelToWorld((*objectIterator)->model);
            } else {
                notStaticObjects.push_back((*objectIterator)->model);
            }
        }

        // Collect attachment info to process after all objects are in the world
        if(loadedModel != nullptr) {
            tinyxml2::XMLElement* parentIDElement = objectNode->FirstChildElement("ParentID");
            if(parentIDElement != nullptr && parentIDElement->GetText() != nullptr) {
                uint32_t parentID = std::stoul(parentIDElement->GetText());
                int32_t boneID = -1;
                tinyxml2::XMLElement* boneIDElement = objectNode->FirstChildElement("ParentBoneID");
                if(boneIDElement != nullptr && boneIDElement->GetText() != nullptr) {
                    boneID = std::stoi(boneIDElement->GetText());
                }
                pendingAttachments.push_back({loadedModel, parentID, boneID});
            }
        }

        objectNode = objectNode->NextSiblingElement("Object");
    }

    world->createGridFrom(aiGridStartPoint);
    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        world->addModelToWorld(notStaticObjects[i]);
    }

    // Second pass: wire up all parent-child relationships.
    // Non-bone: saved values are world coords, attachTo converts world->local correctly.
    // Bone: boneTransforms are identity at load time so attachTo would give wrong local;
    //       saved values are local (serializeLocal), preserved via setParentTransform.
    for(auto& pa : pendingAttachments) {
        Attachable* parent = world->findAttachableByID(pa.parentID);
        if(parent != nullptr) {
            ModelGroup* parentGroup = dynamic_cast<ModelGroup*>(parent);
            if(parentGroup != nullptr) {
                parentGroup->addChild(pa.child);//averaging re-centers the group gizmo on its children's centroid; child world positions are preserved
            } else if(pa.parentBoneID != -1) {
                Model* parentModel = dynamic_cast<Model*>(parent);
                if(parentModel != nullptr) {
                    pa.child->setParentObject(parentModel, pa.parentBoneID);
                    parentModel->addChild(pa.child);
                    pa.child->getTransformation()->setParentTransform(
                        parentModel->getAttachmentTransformForKnownBone(pa.parentBoneID));
                } else {
                    pa.child->attachTo(parent, pa.parentBoneID);
                }
            } else {
                pa.child->attachTo(parent, pa.parentBoneID);
            }
        } else {
            std::cerr << "Object " << pa.child->getWorldObjectID() << " parent " << pa.parentID << " not found, attachment skipped." << std::endl;
        }
    }

    for(const auto& assetFile : preloadAssetFiles) {
        assetManager->freeAsset({assetFile});
    }
    return true;
}

bool WorldLoader::loadSkymap(tinyxml2::XMLNode *skymapNode, World* world) const {
    tinyxml2::XMLElement* skyNode =  skymapNode->FirstChildElement("Sky");
    if (skyNode == nullptr) {
        std::cerr << "Sky clause not found." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* imagesPath =  skyNode->FirstChildElement("ImagesPath");
    if (imagesPath == nullptr) {
        std::cerr << "Sky map must have the root path." << std::endl;
        return false;
    }

    std::string path, left, right, top, bottom, front, back;
    path = imagesPath->GetText();

    tinyxml2::XMLElement* leftNode =  skyNode->FirstChildElement("Left");
    if (leftNode == nullptr) {
        std::cerr << "Sky map must have left image name." << std::endl;
        return false;
    }
    left = leftNode->GetText();

    tinyxml2::XMLElement* rightNode =  skyNode->FirstChildElement("Right");
    if (rightNode == nullptr) {
        std::cerr << "Sky map must have right image name." << std::endl;
        return false;
    }
    right = rightNode->GetText();

    tinyxml2::XMLElement* topNode =  skyNode->FirstChildElement("Top");
    if (topNode == nullptr) {
        std::cerr << "Sky map must have top image name." << std::endl;
        return false;
    }
    top = topNode->GetText();

    tinyxml2::XMLElement* bottomNode =  skyNode->FirstChildElement("Bottom");
    if (bottomNode == nullptr) {
        std::cerr << "Sky map must have bottom image name." << std::endl;
        return false;
    }
    bottom = bottomNode->GetText();

    tinyxml2::XMLElement* backNode =  skyNode->FirstChildElement("Back");
    if (backNode == nullptr) {
        std::cerr << "Sky map must have back image name." << std::endl;
        return false;
    }
    back = backNode->GetText();

    tinyxml2::XMLElement* frontNode =  skyNode->FirstChildElement("Front");
    if (frontNode == nullptr) {
        std::cerr << "Sky map must have front image name." << std::endl;
        return false;
    }
    front = frontNode->GetText();

    int id;
    tinyxml2::XMLElement* idNode =  skyNode->FirstChildElement("ID");
    if (idNode == nullptr) {
        std::cerr << "Sky map must have ID. Can't be loaded." << std::endl;
        return false;
    }
    id = std::stoi(idNode->GetText());

    world->setSky(
            new SkyBox(id, assetManager, std::string(path), std::string(right), std::string(left),
                       std::string(top),
                       std::string(bottom), std::string(back), std::string(front)));
    return true;
}

bool WorldLoader::loadLights(tinyxml2::XMLNode *lightsNode, World* world) const {
    tinyxml2::XMLElement* lightsListNode =  lightsNode->FirstChildElement("Lights");
    if (lightsListNode == nullptr) {
        std::cerr << "Lights clause not found." << std::endl;
        return false;
    }


    tinyxml2::XMLElement* lightNode =  lightsListNode->FirstChildElement("Light");
    if (lightNode == nullptr) {
        std::cerr << "Lights did not have at least one light." << std::endl;
        return false;
    }

    Light::LightTypes type;
    glm::vec3 position;
    glm::vec3 color;
    Light *xmlLight;
    tinyxml2::XMLElement* lightAttribute;
    tinyxml2::XMLElement* lightAttributeAttribute;
    float x,y,z;
    uint32_t lightID;
    while(lightNode != nullptr) {
        lightAttribute = lightNode->FirstChildElement("Type");
        if (lightAttribute == nullptr) {
            std::cerr << "Light must have a type." << std::endl;
            return false;
        }

        std::string typeString = lightAttribute->GetText();
        if (typeString == "POINT") {
            type = Light::LightTypes::POINT;
        } else if (typeString == "DIRECTIONAL") {
            type = Light::LightTypes::DIRECTIONAL;
        } else {
            std::cerr << "Light type is not POINT or DIRECTIONAL. it is " << lightAttribute->GetText() << std::endl;
            return false;
        }

        lightAttribute =  lightNode->FirstChildElement("ID");
        if (lightAttribute == nullptr) {
            lightID = world->getNextObjectID();
            std::cerr << "Light does not have ID. This is deprecated. Assigning " << lightID << ". Re-save the world to make this permanent." << std::endl;
        } else {
            lightID = std::stoul(lightAttribute->GetText());
            if (world->isIDUsed(lightID)) {
                uint32_t newID = world->getNextObjectID();
                std::cerr << "Light ID " << lightID << " is already in use. Assigning " << newID << ". Re-save the world to make this permanent." << std::endl;
                lightID = newID;
            }
        }

        tinyxml2::XMLElement* lightTransformationElement = lightNode->FirstChildElement("Transformation");
        if(lightTransformationElement != nullptr) {
            tinyxml2::XMLElement* translateEl = lightTransformationElement->FirstChildElement("Translate");
            if(translateEl == nullptr || !loadVec3(translateEl, position)) {
                std::cerr << "Light Transformation is missing Translate." << std::endl;
                return false;
            }
        } else {
            lightAttribute = lightNode->FirstChildElement("Position");
            if (lightAttribute == nullptr) {
                std::cerr << "Light must have a position/direction." << std::endl;
                return false;
            } else {
                lightAttributeAttribute = lightAttribute->FirstChildElement("X");
                if (lightAttributeAttribute != nullptr) {
                    x = std::stof(lightAttributeAttribute->GetText());
                } else {
                    std::cerr << "Light position/direction missing x." << std::endl;
                    return false;
                }
                lightAttributeAttribute = lightAttribute->FirstChildElement("Y");
                if (lightAttributeAttribute != nullptr) {
                    y = std::stof(lightAttributeAttribute->GetText());
                } else {
                    std::cerr << "Light position/direction missing y." << std::endl;
                    return false;
                }
                lightAttributeAttribute = lightAttribute->FirstChildElement("Z");
                if (lightAttributeAttribute != nullptr) {
                    z = std::stof(lightAttributeAttribute->GetText());
                } else {
                    std::cerr << "Light position/direction missing z." << std::endl;
                    return false;
                }
            }
            position.x = x;
            position.y = y;
            position.z = z;
        }

        lightAttribute = lightNode->FirstChildElement("Color");
        if (lightAttribute == nullptr) {
            x = y = z = 1.0f;
        } else {
            lightAttributeAttribute = lightAttribute->FirstChildElement("R");
            if (lightAttributeAttribute != nullptr) {
                x = std::stof(lightAttributeAttribute->GetText());
            } else {
                x = 1.0f;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("G");
            if (lightAttributeAttribute != nullptr) {
                y = std::stof(lightAttributeAttribute->GetText());
            } else {
                y = 1.0f;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("B");
            if (lightAttributeAttribute != nullptr) {
                z = std::stof(lightAttributeAttribute->GetText());
            } else {
                z = 1.0f;
            }
        }
        color.x = x;
        color.y = y;
        color.z = z;

        xmlLight = new Light(graphicsWrapper, lightID, type, position, color);

        if(lightTransformationElement != nullptr) {
            xmlLight->getTransformation()->deserialize(lightTransformationElement);
        }

        glm::vec3 attenuation(1, 0.1f, 0.01f);
        tinyxml2::XMLElement* lightAttenuation =  lightNode->FirstChildElement("Attenuation");
        if(lightAttenuation != nullptr) {
            if(loadVec3(lightAttenuation, attenuation)) {
                xmlLight->setAttenuation(attenuation);
            }
        }

        glm::vec3 ambientColor(1, 0.1f, 0.01f);
        tinyxml2::XMLElement* lightAmbient =  lightNode->FirstChildElement("Ambient");
        if(lightAmbient != nullptr) {
            if(loadVec3(lightAmbient, ambientColor)) {
                xmlLight->setAmbientColor(ambientColor);
            }
        }

        tinyxml2::XMLElement* lightParentIDElement = lightNode->FirstChildElement("ParentID");
        if(lightParentIDElement != nullptr && lightParentIDElement->GetText() != nullptr) {
            uint32_t parentID = std::stoul(lightParentIDElement->GetText());
            Attachable* parent = world->findAttachableByID(parentID);
            if(parent != nullptr) {
                xmlLight->attachTo(parent);
            } else {
                std::cerr << "Light parent ID " << parentID << " not found, attachment skipped." << std::endl;
            }
        }

        world->addLight(xmlLight);

        lightNode =  lightNode->NextSiblingElement("Light");
    }
    return true;
}

bool WorldLoader::loadSounds(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* soundsListNode = worldNode->FirstChildElement("Sounds");
    if(soundsListNode == nullptr) {
        return true; // No sounds section — valid for all pre-feature world files.
    }

    struct PendingSound { Sound* sound; uint32_t parentID; };
    std::vector<PendingSound> pendingAttachments;

    tinyxml2::XMLElement* soundNode = soundsListNode->FirstChildElement("Sound");
    while(soundNode != nullptr) {
        tinyxml2::XMLElement* fileEl = soundNode->FirstChildElement("File");
        if(fileEl == nullptr || fileEl->GetText() == nullptr) {
            std::cerr << "Sound entry missing File element, skipping." << std::endl;
            soundNode = soundNode->NextSiblingElement("Sound");
            continue;
        }
        std::string filePath = fileEl->GetText();

        tinyxml2::XMLElement* idEl = soundNode->FirstChildElement("ID");
        if(idEl == nullptr || idEl->GetText() == nullptr) {
            std::cerr << "Sound entry missing ID element, skipping." << std::endl;
            soundNode = soundNode->NextSiblingElement("Sound");
            continue;
        }
        uint32_t soundID = std::stoul(idEl->GetText());

        Sound* sound = new Sound(soundID, assetManager, filePath);

        tinyxml2::XMLElement* gainEl = soundNode->FirstChildElement("Gain");
        if(gainEl != nullptr && gainEl->GetText() != nullptr) {
            //Gain is now normalized 0..1. Levels saved before normalization stored it on a 0..1000+ scale;
            //any value above the normalized max must be legacy, so migrate it on load.
            float storedGain = std::stof(gainEl->GetText());
            if(storedGain > 1.0f) {
                storedGain = storedGain / 1000.0f;
            }
            sound->changeGain(storedGain);
        }

        tinyxml2::XMLElement* refDistEl = soundNode->FirstChildElement("ReferenceDistance");
        if(refDistEl != nullptr && refDistEl->GetText() != nullptr) {
            sound->setReferenceDistance(std::stof(refDistEl->GetText()));
        }

        tinyxml2::XMLElement* maxDistEl = soundNode->FirstChildElement("MaxDistance");
        if(maxDistEl != nullptr && maxDistEl->GetText() != nullptr) {
            sound->setMaxDistance(std::stof(maxDistEl->GetText()));
        }

        tinyxml2::XMLElement* loopedEl = soundNode->FirstChildElement("Looped");
        if(loopedEl != nullptr && loopedEl->GetText() != nullptr) {
            sound->setLoop(std::string(loopedEl->GetText()) == "true");
        }

        tinyxml2::XMLElement* autoPlayEl = soundNode->FirstChildElement("AutoPlay");
        if(autoPlayEl != nullptr && autoPlayEl->GetText() != nullptr) {
            sound->setAutoPlay(std::string(autoPlayEl->GetText()) == "true");
        }

        tinyxml2::XMLElement* listenerRelEl = soundNode->FirstChildElement("ListenerRelative");
        bool listenerRelative = false;
        if(listenerRelEl != nullptr && listenerRelEl->GetText() != nullptr) {
            listenerRelative = std::string(listenerRelEl->GetText()) == "true";
        }

        tinyxml2::XMLElement* transformEl = soundNode->FirstChildElement("Transformation");
        if(transformEl != nullptr) {
            sound->getTransformation()->deserialize(transformEl);
        }

        glm::vec3 worldPos = glm::vec3(sound->getTransformation()->getWorldTransform()[3]);
        sound->setWorldPosition(worldPos, listenerRelative);

        tinyxml2::XMLElement* parentEl = soundNode->FirstChildElement("ParentID");
        if(parentEl != nullptr && parentEl->GetText() != nullptr) {
            pendingAttachments.push_back({sound, std::stoul(parentEl->GetText())});
        }

        world->addSound(sound);
        soundNode = soundNode->NextSiblingElement("Sound");
    }

    for(auto& ps : pendingAttachments) {
        Attachable* parent = world->findAttachableByID(ps.parentID);
        if(parent != nullptr) {
            ps.sound->attachTo(parent);
        } else {
            std::cerr << "Sound parent ID " << ps.parentID << " not found, attachment skipped." << std::endl;
        }
    }

    return true;
}

bool WorldLoader::loadCameraRigs(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* cameraRigsListNode = worldNode->FirstChildElement("CameraRigs");
    if(cameraRigsListNode == nullptr) {
        return true; // No camera rigs section — valid for all pre-feature world files.
    }

    uint32_t activeCameraRigID = 0; // 0 == none
    tinyxml2::XMLElement* activeIDNode = cameraRigsListNode->FirstChildElement("ActiveID");
    if(activeIDNode != nullptr && activeIDNode->GetText() != nullptr) {
        activeCameraRigID = std::stoul(activeIDNode->GetText());
    }

    struct PendingCameraRig { CameraRig* rig; uint32_t parentID; };
    std::vector<PendingCameraRig> pendingAttachments;

    tinyxml2::XMLElement* cameraRigNode = cameraRigsListNode->FirstChildElement("CameraRig");
    while(cameraRigNode != nullptr) {
        tinyxml2::XMLElement* typeEl = cameraRigNode->FirstChildElement("Type");
        tinyxml2::XMLElement* idEl   = cameraRigNode->FirstChildElement("ID");
        if(typeEl == nullptr || typeEl->GetText() == nullptr || idEl == nullptr || idEl->GetText() == nullptr) {
            std::cerr << "CameraRig entry missing Type or ID element, skipping." << std::endl;
            cameraRigNode = cameraRigNode->NextSiblingElement("CameraRig");
            continue;
        }
        std::string rigTypeName = typeEl->GetText();
        uint32_t rigID = std::stoul(idEl->GetText());

        CameraExtensionInterface* heldAttachment = CameraExtensionInterface::createExtension(rigTypeName, world->apiInstance);
        if(heldAttachment == nullptr) {
            std::cerr << "Camera rig type '" << rigTypeName << "' not found. Is the correct plugin loaded? Skipping." << std::endl;
            cameraRigNode = cameraRigNode->NextSiblingElement("CameraRig");
            continue;
        }

        std::vector<LimonTypes::GenericParameter> rigParameters;
        tinyxml2::XMLElement* parametersNode = cameraRigNode->FirstChildElement("Parameters");
        if(parametersNode != nullptr) {
            tinyxml2::XMLElement* parameterNode = parametersNode->FirstChildElement("Parameter");
            uint32_t index;
            while(parameterNode != nullptr) {
                std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterNode, index);
                if(request != nullptr && index <= rigParameters.size()) {
                    rigParameters.insert(rigParameters.begin() + index, *request);
                }
                parameterNode = parameterNode->NextSiblingElement("Parameter");
            }
        }
        heldAttachment->setParameters(rigParameters);

        std::string rigName = rigTypeName + "_" + std::to_string(rigID);
        tinyxml2::XMLElement* nameEl = cameraRigNode->FirstChildElement("Name");
        if(nameEl != nullptr && nameEl->GetText() != nullptr) {
            rigName = nameEl->GetText();
        }

        std::unique_ptr<CameraRig> cameraRig(new CameraRig(rigID, rigName, heldAttachment));

        tinyxml2::XMLElement* transformEl = cameraRigNode->FirstChildElement("Transformation");
        if(transformEl != nullptr) {
            cameraRig->getTransformation()->deserialize(transformEl);
        }

        tinyxml2::XMLElement* parentEl = cameraRigNode->FirstChildElement("ParentID");
        if(parentEl != nullptr && parentEl->GetText() != nullptr) {
            pendingAttachments.push_back({cameraRig.get(), std::stoul(parentEl->GetText())});
        }

        if(rigID == activeCameraRigID) {
            world->activeCameraRig = cameraRig.get(); // afterLoadFinished activates it once players exist
        }
        world->addCameraRig(std::move(cameraRig));
        cameraRigNode = cameraRigNode->NextSiblingElement("CameraRig");
    }

    for(auto& pca : pendingAttachments) {
        Attachable* parent = world->findAttachableByID(pca.parentID);
        if(parent != nullptr) {
            pca.rig->attachTo(parent);
        } else {
            std::cerr << "CameraRig parent ID " << pca.parentID << " not found, attachment skipped." << std::endl;
        }
    }

    return true;
}


bool WorldLoader::loadParticleEmitters(tinyxml2::XMLNode *EmittersNode, World* world) const {
    tinyxml2::XMLElement* EmittersListNode =  EmittersNode->FirstChildElement("Emitters");
    if (EmittersListNode == nullptr) {
        std::cerr << "Emitters clause not found." << std::endl;
        return false;
    }


    tinyxml2::XMLElement* EmitterNode =  EmittersListNode->FirstChildElement("Emitter");
    if (EmitterNode == nullptr) {
        std::cerr << "Emitters did not have at least one Emitter." << std::endl;
        return false;
    }

    long id;
    std::string name;
    glm::vec2 size;
    long maxCount;
    long lifeTime;
    glm::vec3 startPosition;
    glm::vec3 maxStartDistances;
    std::string textureFile;

    tinyxml2::XMLElement* emitterAttributeElement;
    tinyxml2::XMLElement* emitterAttributeAttributeElement;

    while(EmitterNode != nullptr) {
        emitterAttributeElement = EmitterNode->FirstChildElement("MaxCount");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter must have a maximum particle count." << std::endl;
            return false;
        }
        std::string maxCountString = emitterAttributeElement->GetText();
        maxCount = std::stoul(maxCountString);

        emitterAttributeElement = EmitterNode->FirstChildElement("LifeTime");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter must have a life time." << std::endl;
            return false;
        }
        std::string lifeTimeString = emitterAttributeElement->GetText();
        lifeTime = std::stoul(lifeTimeString);


        emitterAttributeElement = EmitterNode->FirstChildElement("Texture");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter must have a Texture." << std::endl;
            return false;
        }
        textureFile = emitterAttributeElement->GetText();


        emitterAttributeElement =  EmitterNode->FirstChildElement("ID");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter does not have ID. This is invalid!" << std::endl;
            return false;
        } else {
            id = std::stoul(emitterAttributeElement->GetText());
        }

        bool continuousEmit = true;
        emitterAttributeElement =  EmitterNode->FirstChildElement("ContinuousEmitting");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter does not have Continuous emitting set. Assuming true" << std::endl;
        } else {
            if(std::string(emitterAttributeElement->GetText()) == "False") {
                continuousEmit = false;
            } else if(std::string(emitterAttributeElement->GetText()) != "True") {
                std::cerr << "Continuous emit setting unknown, assuming true " << std::endl;
            }
        }

        bool enabled = true;
        emitterAttributeElement =  EmitterNode->FirstChildElement("Enabled");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter does not have Enabled set. Assuming true" << std::endl;
        } else {
            if(std::string(emitterAttributeElement->GetText()) == "False") {
                enabled = false;
            } else if(std::string(emitterAttributeElement->GetText()) != "True") {
                std::cerr << "Enabled setting unknown, assuming true " << std::endl;
            }
        }

        emitterAttributeElement =  EmitterNode->FirstChildElement("Name");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "Particle emitter does not have Name. This is invalid!" << std::endl;
            return false;
        } else {
            name = emitterAttributeElement->GetText();
        }

        tinyxml2::XMLElement* emitterTransformationElement = EmitterNode->FirstChildElement("Transformation");
        if(emitterTransformationElement != nullptr) {
            tinyxml2::XMLElement* translateEl = emitterTransformationElement->FirstChildElement("Translate");
            if(translateEl == nullptr || !loadVec3(translateEl, startPosition)) {
                std::cerr << "Emitter Transformation is missing Translate." << std::endl;
                return false;
            }
        } else {
            emitterAttributeElement = EmitterNode->FirstChildElement("StartPosition");
            if (emitterAttributeElement == nullptr) {
                std::cerr << "Particle Emitter must have a position/direction." << std::endl;
                return false;
            } else {
                emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
                if (emitterAttributeAttributeElement != nullptr) {
                    startPosition.x = std::stof(emitterAttributeAttributeElement->GetText());
                } else {
                    std::cerr << "Particle Emitter position/direction missing x." << std::endl;
                    return false;
                }
                emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
                if (emitterAttributeAttributeElement != nullptr) {
                    startPosition.y = std::stof(emitterAttributeAttributeElement->GetText());
                } else {
                    std::cerr << "Particle Emitter position/direction missing y." << std::endl;
                    return false;
                }
                emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
                if (emitterAttributeAttributeElement != nullptr) {
                    startPosition.z = std::stof(emitterAttributeAttributeElement->GetText());
                } else {
                    std::cerr << "Particle Emitter position/direction missing z." << std::endl;
                    return false;
                }
            }
        }

        emitterAttributeElement = EmitterNode->FirstChildElement("MaximumStartDistances");
        if (emitterAttributeElement == nullptr) {
            std::cerr << "Particle Emitter must have a Maximum Start distance." << std::endl;
            return false;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter Maximum Start distance missing x." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter Maximum Start distance missing y." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter Maximum Start distance missing z." << std::endl;
                return false;
            }
        }

        emitterAttributeElement = EmitterNode->FirstChildElement("Size");
        if (emitterAttributeElement == nullptr) {
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {
                size.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                size.x = 1.0f;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                size.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                size.y = 1.0f;
            }
        }

        glm::vec3 gravity = glm::vec3(0,0,0);
        emitterAttributeElement = EmitterNode->FirstChildElement("Gravity");
        if (emitterAttributeElement == nullptr) {
            std::cout << "Particle Emitter has no gravity." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                gravity.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter gravity missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                gravity.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter gravity missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                gravity.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter gravity missing z." << std::endl;
            }
        }

        glm::vec3 speedMultiplier = glm::vec3(1,1,1);
        emitterAttributeElement = EmitterNode->FirstChildElement("SpeedMultiplier");
        if (emitterAttributeElement == nullptr) {
            std::cout << "Particle Emitter has no speedMultiplier." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                speedMultiplier.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedMultiplier missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                speedMultiplier.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedMultiplier missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                speedMultiplier.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedMultiplier missing z." << std::endl;
            }
        }

        glm::vec3 speedOffset = glm::vec3(0,0,0);
        emitterAttributeElement = EmitterNode->FirstChildElement("SpeedOffset");
        if (emitterAttributeElement == nullptr) {
            std::cout << "Particle Emitter has no gravity." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                speedOffset.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedOffset missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                speedOffset.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedOffset missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                speedOffset.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "Particle Emitter speedOffset missing z." << std::endl;
            }
        }

        std::vector<Emitter::TimedColorMultiplier> multipliers;
        emitterAttributeElement = EmitterNode->FirstChildElement("TimedColorMultipliers");
        if (emitterAttributeElement == nullptr) {
            std::cout << "Particle Emitter has no Timed color shift." << std::endl;
        } else {
            tinyxml2::XMLElement* timedColorMultiplierElement = emitterAttributeElement->FirstChildElement("TimedColorMultiplier");
            while(timedColorMultiplierElement != nullptr) {
                Emitter::TimedColorMultiplier timedColorMultiplier;

                tinyxml2::XMLElement* colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("Time");
                if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                    std::cerr << "time can't be found for timed color multiplier, skipping!" << std::endl;
                } else {
                    timedColorMultiplier.time = std::atol(colorMultiplierComponentElement->GetText());

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("R");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color R can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.r = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("G");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color G can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.g = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("B");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color B can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.b = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("A");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color A can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.a = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    multipliers.emplace_back(timedColorMultiplier);
                }

                timedColorMultiplierElement = timedColorMultiplierElement->NextSiblingElement("TimedColorMultiplier");
            }

        }

        std::shared_ptr<Emitter> emitter = std::make_shared<Emitter>(id, name, this->assetManager, textureFile,
                                                                     startPosition, maxStartDistances, size, maxCount,
                                                                     lifeTime);
        if(emitterTransformationElement != nullptr) {
            emitter->getTransformation()->deserialize(emitterTransformationElement);
        }
        emitter->setGravity(gravity);
        emitter->setSpeedMultiplier(speedMultiplier);
        emitter->setSpeedOffset(speedOffset);
        emitter->setTimedColorMultipliers(multipliers);
        emitter->setContinuousEmit(continuousEmit);
        emitter->setEnabled(enabled);
        world->emitters[emitter->getWorldObjectID()] = emitter;

        emitterAttributeElement = EmitterNode->FirstChildElement("ParentID");
        if(emitterAttributeElement != nullptr && emitterAttributeElement->GetText() != nullptr) {
            uint32_t parentID = std::stoul(emitterAttributeElement->GetText());
            Attachable* parent = world->findAttachableByID(parentID);
            if(parent != nullptr) {
                emitter->attachTo(parent);
            } else {
                std::cerr << "Emitter parent ID " << parentID << " not found, attachment skipped." << std::endl;
            }
        }

        EmitterNode =  EmitterNode->NextSiblingElement("Emitter");
    }
    return true;
}

bool WorldLoader::loadGPUParticleEmitters(tinyxml2::XMLNode *GPUEmittersNode, World* world) const {
    tinyxml2::XMLElement* gpuEmittersListNode =  GPUEmittersNode->FirstChildElement("GPUEmitters");
    if (gpuEmittersListNode == nullptr) {
        std::cerr << "GPUEmitters clause not found." << std::endl;
        return false;
    }


    tinyxml2::XMLElement* gpuEmitterNode =  gpuEmittersListNode->FirstChildElement("GPUEmitter");
    if (gpuEmitterNode == nullptr) {
        std::cerr << "GPU Emitters did not have at least one GPUEmitter." << std::endl;
        return false;
    }

    long id;
    std::string name;
    glm::vec2 size;
    long maxCount;
    long lifeTime;
    glm::vec3 startPosition;
    glm::vec3 maxStartDistances;
    std::string textureFile;

    tinyxml2::XMLElement* emitterAttributeElement;
    tinyxml2::XMLElement* emitterAttributeAttributeElement;

    while(gpuEmitterNode != nullptr) {
        emitterAttributeElement = gpuEmitterNode->FirstChildElement("MaxCount");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter must have a maximum particle count." << std::endl;
            return false;
        }
        std::string maxCountString = emitterAttributeElement->GetText();
        maxCount = std::stoul(maxCountString);

        emitterAttributeElement = gpuEmitterNode->FirstChildElement("LifeTime");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter must have a life time." << std::endl;
            return false;
        }
        std::string lifeTimeString = emitterAttributeElement->GetText();
        lifeTime = std::stoul(lifeTimeString);


        emitterAttributeElement = gpuEmitterNode->FirstChildElement("Texture");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter must have a Texture." << std::endl;
            return false;
        }
        textureFile = emitterAttributeElement->GetText();


        emitterAttributeElement =  gpuEmitterNode->FirstChildElement("ID");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter does not have ID. This is invalid!" << std::endl;
            return false;
        } else {
            id = std::stoul(emitterAttributeElement->GetText());
        }

        bool continuousEmit = true;
        emitterAttributeElement =  gpuEmitterNode->FirstChildElement("ContinuousEmitting");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter does not have Continuous emitting set. Assuming true" << std::endl;
        } else {
            if(std::string(emitterAttributeElement->GetText()) == "False") {
                continuousEmit = false;
            } else if(std::string(emitterAttributeElement->GetText()) != "True") {
                std::cerr << "Continuous emit setting unknown, assuming true " << std::endl;
            }
        }

        bool enabled = true;
        emitterAttributeElement =  gpuEmitterNode->FirstChildElement("Enabled");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter does not have Enabled set. Assuming true" << std::endl;
        } else {
            if(std::string(emitterAttributeElement->GetText()) == "False") {
                enabled = false;
            } else if(std::string(emitterAttributeElement->GetText()) != "True") {
                std::cerr << "Enabled setting unknown, assuming true " << std::endl;
            }
        }

        emitterAttributeElement =  gpuEmitterNode->FirstChildElement("Name");
        if (emitterAttributeElement == nullptr || emitterAttributeElement->GetText() == nullptr) {
            std::cerr << "GPU Particle emitter does not have Name. This is invalid!" << std::endl;
            return false;
        } else {
            name = emitterAttributeElement->GetText();
        }

        emitterAttributeElement = gpuEmitterNode->FirstChildElement("StartPosition");
        if (emitterAttributeElement == nullptr) {
            std::cerr << "GPU Particle Emitter must have a position/direction." << std::endl;
            return false;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {
                startPosition.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter position/direction missing x." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                startPosition.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter position/direction missing y." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                startPosition.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter position/direction missing z." << std::endl;
                return false;
            }
        }

        emitterAttributeElement = gpuEmitterNode->FirstChildElement("MaximumStartDistances");
        if (emitterAttributeElement == nullptr) {
            std::cerr << "GPU Particle Emitter must have a Maximum Start distance." << std::endl;
            return false;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter Maximum Start distance missing x." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter Maximum Start distance missing y." << std::endl;
                return false;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                maxStartDistances.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter Maximum Start distance missing z." << std::endl;
                return false;
            }
        }

        emitterAttributeElement = gpuEmitterNode->FirstChildElement("Size");
        if (emitterAttributeElement == nullptr) {
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {
                size.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                size.x = 1.0f;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                size.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                size.y = 1.0f;
            }
        }

        glm::vec3 gravity = glm::vec3(0,0,0);
        emitterAttributeElement = gpuEmitterNode->FirstChildElement("Gravity");
        if (emitterAttributeElement == nullptr) {
            std::cout << "GPU Particle Emitter has no gravity." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                gravity.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter gravity missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                gravity.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter gravity missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                gravity.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter gravity missing z." << std::endl;
            }
        }

        glm::vec3 speedMultiplier = glm::vec3(1,1,1);
        emitterAttributeElement = gpuEmitterNode->FirstChildElement("SpeedMultiplier");
        if (emitterAttributeElement == nullptr) {
            std::cout << "GPU Particle Emitter has no speedMultiplier." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                speedMultiplier.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter speedMultiplier missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                speedMultiplier.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter speedMultiplier missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                speedMultiplier.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter speedMultiplier missing z." << std::endl;
            }
        }

        glm::vec3 speedOffset = glm::vec3(0,0,0);
        emitterAttributeElement = gpuEmitterNode->FirstChildElement("SpeedOffset");
        if (emitterAttributeElement == nullptr) {
            std::cout << "GPU Particle Emitter has no SpeedOffset." << std::endl;
        } else {
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("X");
            if (emitterAttributeAttributeElement != nullptr) {

                speedOffset.x = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter SpeedOffset missing x." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Y");
            if (emitterAttributeAttributeElement != nullptr) {
                speedOffset.y = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter SpeedOffset missing y." << std::endl;
            }
            emitterAttributeAttributeElement = emitterAttributeAttributeElement->FirstChildElement("Z");
            if (emitterAttributeAttributeElement != nullptr) {
                speedOffset.z = std::stof(emitterAttributeAttributeElement->GetText());
            } else {
                std::cerr << "GPU Particle Emitter SpeedOffset missing z." << std::endl;
            }
        }

        std::vector<GPUParticleEmitter::TimedColorMultiplier> multipliers;
        emitterAttributeElement = gpuEmitterNode->FirstChildElement("TimedColorMultipliers");
        if (emitterAttributeElement == nullptr) {
            std::cout << "GPU Particle Emitter has no Timed color shift." << std::endl;
        } else {
            tinyxml2::XMLElement* timedColorMultiplierElement = emitterAttributeElement->FirstChildElement("TimedColorMultiplier");
            while(timedColorMultiplierElement != nullptr) {
                GPUParticleEmitter::TimedColorMultiplier timedColorMultiplier;

                tinyxml2::XMLElement* colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("Time");
                if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                    std::cerr << "time can't be found for timed color multiplier, skipping!" << std::endl;
                } else {
                    timedColorMultiplier.time = std::atol(colorMultiplierComponentElement->GetText());

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("R");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color R can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.r = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("G");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color G can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.g = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("B");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color B can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.b = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    colorMultiplierComponentElement = timedColorMultiplierElement->FirstChildElement("A");
                    if(colorMultiplierComponentElement == nullptr || colorMultiplierComponentElement->GetText() == nullptr) {
                        std::cerr << "color A can't be found for timed color multiplier, assuming 255" << std::endl;
                    } else {
                        timedColorMultiplier.colorMultiplier.a = std::atoi(colorMultiplierComponentElement->GetText());
                    }

                    multipliers.emplace_back(timedColorMultiplier);
                }

                timedColorMultiplierElement = timedColorMultiplierElement->NextSiblingElement("TimedColorMultiplier");
            }
        }

        std::shared_ptr<GPUParticleEmitter> emitter = std::make_shared<GPUParticleEmitter>(id, name, this->assetManager, textureFile,
                                                                     startPosition, maxStartDistances, size, maxCount,
                                                                     lifeTime, 0);
        emitter->setGravity(gravity);
        emitter->setSpeedMultiplier(speedMultiplier);
        emitter->setSpeedOffset(speedOffset);
        emitter->setTimedColorMultipliers(multipliers);
        emitter->setContinuousEmit(continuousEmit);
        emitter->setEnabled(enabled);
        world->gpuParticleEmitters[emitter->getWorldObjectID()] = emitter;
        gpuEmitterNode =  gpuEmitterNode->NextSiblingElement("GPUEmitter");
    }
    return true;
}

bool WorldLoader::loadAnimations(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* loadedAnimationsListNode =  worldNode->FirstChildElement("LoadedAnimations");
    if (loadedAnimationsListNode == nullptr) {
        std::cerr << "LoadedAnimations clause not found." << std::endl;
        return false;
    }


    tinyxml2::XMLElement* loadedAnimationNode =  loadedAnimationsListNode->FirstChildElement("LoadedAnimation");
    if (loadedAnimationNode == nullptr) {
        std::cerr << "Loaded animations did not have at least one animation." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* animationAttribute;
    while(loadedAnimationNode != nullptr) {
        animationAttribute = loadedAnimationNode->FirstChildElement("Name");
        if (animationAttribute == nullptr) {
            std::cerr << "Animation must have a name." << std::endl;
            return false;
        }
        std::string name = animationAttribute->GetText();

        animationAttribute = loadedAnimationNode->FirstChildElement("Index");
        if (animationAttribute == nullptr) {
            std::cerr << "Animation must have a name." << std::endl;
            return false;
        }
        uint32_t index = std::stoi(animationAttribute->GetText());

        AnimationCustom* animation = AnimationLoader::loadAnimation("./Data/Animations/"+ std::string(name) +".xml");
        if(animation == nullptr) {
            std::cout << "Animation " << name << " load failed" << std::endl;
            return false;
        } else {
            std::cout << "Animation " << name << " loaded" << std::endl;
            world->loadedAnimations.insert(world->loadedAnimations.begin() + index, *animation);
            delete animation;
        }
        loadedAnimationNode =  loadedAnimationNode->NextSiblingElement("LoadedAnimation");
    }
    return true;

}

bool WorldLoader::loadTriggers(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* triggerListNode =  worldNode->FirstChildElement("Triggers");
    if (triggerListNode == nullptr) {
        std::cout << "World doesn't have any triggers." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* triggerNode =  triggerListNode->FirstChildElement("Trigger");

    while(triggerNode != nullptr) {
        TriggerObject* triggerObject = TriggerObject::deserialize(triggerNode, world->apiInstance);
        if(triggerObject == nullptr) {
            //this trigger is now headless
            return false;
        }
        world->triggers[triggerObject->getWorldObjectID()] = triggerObject;
        //FIXME adding the collision object should not be here
        world->dynamicsWorld->addCollisionObject(triggerObject->getGhostObject(), btBroadphaseProxy::SensorTrigger,
                                                btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);

        tinyxml2::XMLElement* triggerParentIDElement = triggerNode->FirstChildElement("ParentID");
        if(triggerParentIDElement != nullptr && triggerParentIDElement->GetText() != nullptr) {
            uint32_t parentID = std::stoul(triggerParentIDElement->GetText());
            Attachable* parent = world->findAttachableByID(parentID);
            if(parent != nullptr) {
                triggerObject->attachTo(parent);
            } else {
                std::cerr << "Trigger parent ID " << parentID << " not found, attachment skipped." << std::endl;
            }
        }

        triggerNode = triggerNode->NextSiblingElement("Trigger");
    } // end of while (Triggers)
    return true;
}

bool WorldLoader::loadOnLoadActions(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* onloadActionListNode =  worldNode->FirstChildElement("OnloadActions");
    if (onloadActionListNode == nullptr) {
        std::cout << "World doesn't have any On load actions." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* onloadActionNode =  onloadActionListNode->FirstChildElement("OnloadAction");

    while(onloadActionNode != nullptr) {
        tinyxml2::XMLElement* actionCodeNameNode = onloadActionNode->FirstChildElement("ActionName");
        if(actionCodeNameNode == nullptr) {
            std::cerr << "Action Code name can't be read, skipping on load action load" << std::endl;
        } else {
            //now we know the action name, create it
            World::ActionForOnload* actionForOnload = new World::ActionForOnload();
            actionForOnload->action = TriggerInterface::createTrigger(actionCodeNameNode->GetText(), world->apiInstance);

            //now we have the Trigger Action, load parameters
            tinyxml2::XMLElement* parametersListNode = onloadActionNode->FirstChildElement("Parameters");

            tinyxml2::XMLElement* parameterNode = parametersListNode->FirstChildElement("Parameter");
            std::vector<LimonTypes::GenericParameter> parameters;
            uint32_t index;
            while(parameterNode != nullptr) {
                std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterNode, index);
                if(request == nullptr) {
                    delete actionForOnload;
                    return false;
                }
                parameters.insert(parameters.begin() + index, *request);

                parameterNode = parameterNode->NextSiblingElement("Parameter");
            }
            //values are owned by the trigger instance
            actionForOnload->action->setParameters(parameters);

            //now load enabled state
            tinyxml2::XMLElement* enabledNode = onloadActionNode->FirstChildElement("Enabled");
            if (enabledNode == nullptr) {
                std::cerr << "Onload Action Didn't have enabled set, defaulting to True." << std::endl;
                actionForOnload->enabled = true;
            } else {
                if(strcmp(enabledNode->GetText(), "True") == 0) {
                    actionForOnload->enabled = true;
                } else if(strcmp(enabledNode->GetText(), "False") == 0) {
                    actionForOnload->enabled = false;
                } else {
                    std::cerr << "Onload action enabled setting is unknown value [" << enabledNode->GetText() << "], can't be loaded " << std::endl;
                    delete actionForOnload;
                    return false;
                }
            }

            tinyxml2::XMLElement* indexNode = onloadActionNode->FirstChildElement("Index");
            if(indexNode == nullptr) {
                std::cerr << "Onload Action  Didn't have index, action can't be loaded." << std::endl;
                delete actionForOnload;
                return false;
            }

            size_t actionIndex = std::stoi(indexNode->GetText());
            if(world->onLoadActions.size() < actionIndex + 1) {
                world->onLoadActions.resize(actionIndex+1);
            }
            world->onLoadActions[actionIndex] = actionForOnload;
        }
        onloadActionNode = onloadActionNode->NextSiblingElement("OnloadAction");
    } // end of while (OnloadAction)
    return true;
}

bool WorldLoader::loadOnLoadAnimations(tinyxml2::XMLNode *worldNode, World *world) const {
        tinyxml2::XMLElement* onloadAnimationsListNode =  worldNode->FirstChildElement("OnLoadAnimations");
        if (onloadAnimationsListNode == nullptr) {
            std::cout << "World doesn't have any On load animations." << std::endl;
            return true;
        }

        tinyxml2::XMLElement* onloadAnimationNode =  onloadAnimationsListNode->FirstChildElement("OnLoadAnimation");

        while(onloadAnimationNode != nullptr) {
            tinyxml2::XMLElement* modelIDNode = onloadAnimationNode->FirstChildElement("ModelID");
            if(modelIDNode == nullptr) {
                std::cerr << "Animation model ID can't be read. Animation loading not possible, skipping" << std::endl;
            } else {
                uint32_t modelID = std::stoi(modelIDNode->GetText());

                //We wanna use name to load, as ID depends on loading order and not safe.
                bool applied = false;
                tinyxml2::XMLElement *animationNameNode = onloadAnimationNode->FirstChildElement("Name");
                if (animationNameNode != nullptr && animationNameNode->GetText() != nullptr) {
                    std::string animationName = animationNameNode->GetText();
                    if (world->apiAccessor->addAnimationToObjectByNameWithSound(modelID, animationName, true, true, "") != 0) {
                        applied = true;
                    } else {
                        std::cerr << "OnLoad animation name \"" << animationName
                                  << "\" could not be resolved, falling back to AnimationID." << std::endl;
                    }
                }

                //AnimationID is only a fallback, for maps saved before names were stored.
                if (!applied) {
                    tinyxml2::XMLElement *animationIDNode = onloadAnimationNode->FirstChildElement("AnimationID");
                    if (animationIDNode == nullptr) {
                        std::cerr << "Animation has neither resolvable Name nor AnimationID. Animation loading not possible, skipping"  << std::endl;
                    } else {
                        uint32_t animationID = std::stoi(animationIDNode->GetText());

                        world->apiAccessor->addAnimationToObject(modelID, animationID, true, true);
                    }
                }
            }
            onloadAnimationNode = onloadAnimationNode->NextSiblingElement("OnLoadAnimation");
        } // end of while (OnLoadAnimation)
        return true;
}

bool WorldLoader::loadGUILayersAndElements(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement* GuiLayersListNode =  worldNode->FirstChildElement("GUILayers");
    if (GuiLayersListNode == nullptr) {
        std::cout << "World doesn't have any GUI elements." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* GUILayerNode =  GuiLayersListNode->FirstChildElement("GUILayer");

    while(GUILayerNode != nullptr) {
        tinyxml2::XMLElement* levelNode = GUILayerNode->FirstChildElement("Level");
        if(levelNode == nullptr) {
            std::cerr << "GUI layer level can't be read. GUI loading not possible" << std::endl;
            return false;
        }
        uint32_t level = std::stoi(levelNode->GetText());

        GUILayer* layer = new GUILayer(graphicsWrapper, world->debugDrawer, level);
        layer->setDebug(false);
        world->guiLayers.push_back(layer);
        //now we should deserialize each element
        tinyxml2::XMLElement* GUIElementNode =  GUILayerNode->FirstChildElement("GUIElement");
        while(GUIElementNode != nullptr) {
            // TODO we should have a factory to create objects from parameters we collect, currently single type, GUITEXT
            tinyxml2::XMLElement* typeNode =  GUIElementNode->FirstChildElement("Type");
            if(typeNode != nullptr) {
                std::string typeName = typeNode->GetText();
                GUIRenderable *element = nullptr;
                std::string name;
                if(typeName == "GUIText") {
                    element = GUIText::deserialize(GUIElementNode, graphicsWrapper, &world->fontManager, options);
                    if(element != nullptr) {
                        name = static_cast<GUIText*>(element)->getName();
                    }
                } else if(typeName == "GUIImage") {
                    element = GUIImage::deserialize(GUIElementNode, assetManager, options);
                    if(element != nullptr) {
                        name = static_cast<GUIImage *>(element)->getName();
                    }
                } else if(typeName == "GUIButton") {
                    element = GUIButton::deserialize(GUIElementNode, assetManager, options, world->apiInstance);
                    if(element != nullptr) {
                        name = static_cast<GUIButton *>(element)->getName();
                    }
                } else if(typeName == "GUIAnimation") {
                    element = GUIAnimation::deserialize(GUIElementNode, assetManager, options);
                    if(element != nullptr) {
                        name = static_cast<GUIAnimation *>(element)->getName();
                    }
                }

                if(element != nullptr) {
                    if(!world->addGUIElementToWorld(element, layer)) {
                        std::cerr << "failed to add gui element [" << name << "] to world!" << std::endl;
                    }
                }


            }

            GUIElementNode = GUIElementNode->NextSiblingElement("GUIElement");
        }// end of while (GUIElementNode)

        GUILayerNode = GUILayerNode->NextSiblingElement("GUILayer");
    } // end of while (GUILayer)
    return true;
}


bool WorldLoader::loadMaterials(tinyxml2::XMLNode *worldNode, World *world) const {
    tinyxml2::XMLElement *materialsNode = worldNode->FirstChildElement("Materials");
    if (!materialsNode) {
        std::cerr << "No materials found in XML." << std::endl;
        return true;
    }

    for (tinyxml2::XMLElement *materialNode = materialsNode->FirstChildElement("Material");
         materialNode != nullptr;
         materialNode = materialNode->NextSiblingElement("Material")) {
         std::shared_ptr<Material> material = Material::deserialize(world->assetManager.get(), materialNode);
    }
    return true;
}

bool WorldLoader::loadVec3(tinyxml2::XMLNode *vectorNode, glm::vec3& vector) {
    if(vectorNode == nullptr) {
        return false;
    }
    tinyxml2::XMLElement *vectorAttributeNode = vectorNode->FirstChildElement("X");
    if (vectorAttributeNode != nullptr) {
        vector.x = std::stof(vectorAttributeNode->GetText());
    } else {
        std::cerr << "Vector is missing x." << std::endl;
        return false;
    }
    vectorAttributeNode = vectorNode->FirstChildElement("Y");
    if (vectorAttributeNode != nullptr) {
        vector.y = std::stof(vectorAttributeNode->GetText());
    } else {
        std::cerr << "Vector is missing y." << std::endl;
        return false;
    }
    vectorAttributeNode = vectorNode->FirstChildElement("Z");
    if (vectorAttributeNode != nullptr) {
        vector.z = std::stof(vectorAttributeNode->GetText());
    } else {
        std::cerr << "Vector is missing z." << std::endl;
        return false;
    }
    return true;
}

std::unique_ptr<std::string> WorldLoader::getLoadingImage(const std::string &worldFile) const {
    std::unique_ptr<std::string> imageFilePath;
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(worldFile.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML "<< worldFile << ": " <<  xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * worldNode = xmlDoc.FirstChild();
    if (worldNode == nullptr) {
        std::cerr << "World xml is not a valid XML." << std::endl;
        return imageFilePath;
    }

    tinyxml2::XMLElement* loadImage =  worldNode->FirstChildElement("LoadingImage");
    if (loadImage == nullptr || loadImage->GetText() == nullptr) {
        return imageFilePath;
    } else {
        imageFilePath = std::make_unique<std::string>(loadImage->GetText());
        return imageFilePath;
    }
}
