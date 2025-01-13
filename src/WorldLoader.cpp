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

#include "API/LimonAPI.h"
#include "Assets/Animations/AnimationLoader.h"
#include "Assets/Animations/AnimationCustom.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUILayer.h"
#include "GameObjects/GUIText.h"
#include "ALHelper.h"
#include "GameObjects/Sound.h"
#include "GameObjects/GUIImage.h"
#include "GameObjects/GUIButton.h"


#include "main.h"
#include "GameObjects/GUIAnimation.h"
#include "GameObjects/ModelGroup.h"
#include "GamePlay/APISerializer.h"

WorldLoader::WorldLoader(std::shared_ptr<AssetManager> assetManager, InputHandler *inputHandler, OptionsUtil::Options *options) :
        options(options),
        graphicsWrapper(assetManager->getGraphicsWrapper()),
        alHelper(assetManager->getAlHelper()),
        assetManager(assetManager),
        inputHandler(inputHandler)
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

    limonAPI->worldAddAnimationToObject = std::bind(&World::addAnimationToObjectWithSound, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false, std::placeholders::_4);
    limonAPI->worldAddGuiText = std::bind(&World::addGuiText, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    limonAPI->worldAddGuiImage = std::bind(&World::addGuiImageAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);

    limonAPI->worldAddModel = std::bind(&World::addModelApi, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    limonAPI->worldSetModelTemporary = std::bind(&World::setModelTemporaryAPI, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldUpdateGuiText = std::bind(&World::updateGuiText, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGenerateEditorElementsForParameters = std::bind(&World::generateEditorElementsForParameters, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetResultOfTrigger = std::bind(&World::getResultOfTrigger, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveGuiElement = std::bind(&World::removeGuiElement, world, std::placeholders::_1);
    limonAPI->worldRemoveObject = std::bind(&World::removeObject, world, std::placeholders::_1,  std::placeholders::_2);
    limonAPI->worldAttachObjectToObject = std::bind(&World::attachObjectToObject, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveTriggerObject = std::bind(&World::removeTriggerObject, world, std::placeholders::_1);
    limonAPI->worldDisconnectObjectFromPhysics = std::bind(&World::disconnectObjectFromPhysics, world, std::placeholders::_1);
    limonAPI->worldReconnectObjectToPhysics= std::bind(&World::reconnectObjectToPhysics, world, std::placeholders::_1);
    limonAPI->worldApplyForce = std::bind(&World::applyForceAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldApplyForceToPlayer = std::bind(&World::applyForceToPlayerAPI, world, std::placeholders::_1);

    limonAPI->worldAttachSoundToObjectAndPlay = std::bind(&World::attachSoundToObjectAndPlay, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldDetachSoundFromObject = std::bind(&World::detachSoundFromObject, world, std::placeholders::_1);
    limonAPI->worldPlaySound = std::bind(&World::playSound, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldRayCastToCursor = std::bind(&World::rayCastToCursorAPI, world);
    limonAPI->worldGetObjectTransformation = std::bind(&World::getObjectTransformationAPI, world, std::placeholders::_1);
    limonAPI->worldGetObjectTransformationMatrix = std::bind(&World::getObjectTransformationMatrixAPI, world, std::placeholders::_1);

    limonAPI->worldSetObjectTranslate =   std::bind(&World::setObjectTranslateAPI,   world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetObjectScale =       std::bind(&World::setObjectScaleAPI,       world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetObjectOrientation = std::bind(&World::setObjectOrientationAPI, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectTranslate =   std::bind(&World::addObjectTranslateAPI,   world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectScale =       std::bind(&World::addObjectScaleAPI,       world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldAddObjectOrientation = std::bind(&World::addObjectOrientationAPI, world, std::placeholders::_1, std::placeholders::_2);

    limonAPI->worldInteractWithAI = std::bind(&World::interactWithAIAPI, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldInteractWithPlayer = std::bind(&World::interactWithPlayerAPI, world, std::placeholders::_1);
    limonAPI->worldSimulateInput = std::bind(&World::simulateInputAPI, world, std::placeholders::_1);
    limonAPI->worldAddTimedEvent = std::bind(&World::addTimedEventAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldCancelTimedEvent = std::bind(&World::cancelTimedEventAPI, world, std::placeholders::_1);

    limonAPI->worldKillPlayer = std::bind(&World::killPlayerAPI, world);
    limonAPI->worldGetPlayerAttachedModel = std::bind(&World::getPlayerAttachedModelAPI, world);
    limonAPI->worldGetModelChildren = std::bind(&World::getModelChildrenAPI, world, std::placeholders::_1);

    limonAPI->worldGetModelAnimationName = std::bind(&World::getModelAnimationNameAPI, world, std::placeholders::_1);
    limonAPI->worldGetModelAnimationFinished = std::bind(&World::getModelAnimationFinishedAPI, world, std::placeholders::_1);
    limonAPI->worldSetAnimationOfModel = std::bind(&World::setModelAnimationAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldSetAnimationOfModelWithBlend = std::bind(&World::setModelAnimationWithBlendAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldSetModelAnimationSpeed = std::bind(&World::setModelAnimationSpeedAPI, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetPlayerAttachmentOffset = std::bind(&World::getPlayerModelOffsetAPI, world);
    limonAPI->worldSetPlayerAttachmentOffset = std::bind(&World::setPlayerModelOffsetAPI, world, std::placeholders::_1);
    limonAPI->worldEnableParticleEmitter = std::bind(&World::enableParticleEmitter, world, std::placeholders::_1);
    limonAPI->worldDisableParticleEmitter = std::bind(&World::disableParticleEmitter, world, std::placeholders::_1);
    limonAPI->worldAddParticleEmitter = std::bind(&World::addParticleEmitter, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                                                  std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9);
    limonAPI->worldRemoveParticleEmitter = std::bind(&World::removeParticleEmitter, world, std::placeholders::_1);
    limonAPI->worldSetEmitterParticleSpeed = std::bind(&World::setEmitterParticleSpeed, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldSetEmitterParticleGravity = std::bind(&World::setEmitterParticleGravity, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldChangeRenderPipeline = std::bind(&World::changeRenderPipeline, world, std::placeholders::_1);


    uint32_t addParticleEmitter(const std::string &name, const std::string& textureFile,
                                const LimonTypes::Vec4& startPosition,
                                const LimonTypes::Vec4& maxStartDistances,
                                const LimonTypes::Vec2& size,
                                uint32_t count,
                                uint32_t lifeTime,
                                float particlePerMs,
                                bool continuouslyEmit);
    bool removeParticleEmitter(uint32_t emitterID);
    bool setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset);
    bool setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4& gravity);

    limonAPI->worldAddLightTranslate =   std::bind(&World::addLightTranslateAPI,   world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldSetLightColor     =   std::bind(&World::setLightColorAPI,       world, std::placeholders::_1, std::placeholders::_2);

    world->apiInstance = limonAPI;
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

        tinyxml2::XMLElement* playerAttachmentModel =  worldStartPlayer->FirstChildElement("Attachment");
        if(playerAttachmentModel != nullptr) {
            tinyxml2::XMLElement* objectNode =  playerAttachmentModel->FirstChildElement("Object");
            if (objectNode != nullptr) {
                std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds; //required. Should not be used normally.

                std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(assetManager, objectNode,
                                                                                         requiredSounds, limonAPI,
                                                                                         nullptr);//this map is used to load all the sounds, while sharing same objects.

                for (auto objectIterator = objectInfos.begin(); objectIterator != objectInfos.end(); ++objectIterator) {
                    if((*objectIterator)->modelActor != nullptr) {
                        std::cerr << "There was an AI attached to player model, this shouldn't happen. Ignoring" << std::endl;
                        delete (*objectIterator)->modelActor;
                    }
                    startingPlayer.attachedModel = (*objectIterator)->model;
                    if(startingPlayer.attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_BASIC))) {
                        startingPlayer.attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_BASIC);
                    } else if(startingPlayer.attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_ANIMATED))) {
                        startingPlayer.attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_ANIMATED);
                    } else if(startingPlayer.attachedModel->hasTag(HashUtil::hashString(HardCodedTags::OBJECT_MODEL_TRANSPARENT))) {
                        startingPlayer.attachedModel->addTag(HardCodedTags::OBJECT_PLAYER_TRANSPARENT);
                    }
                }
            }
        }
    }

    World* world = new World(std::string(worldName->GetText()), startingPlayer, inputHandler, assetManager, options);

    attachedAPIMethodsToWorld(world, limonAPI);
    world->loadingImage = loadingImageStr;

    tinyxml2::XMLElement* musicNameNode =  worldNode->FirstChildElement("Music");
    if (musicNameNode == nullptr) {
        std::cout << "No music found." << std::endl;
    } else {
        std::string musicName = musicNameNode->GetText();
        std::cout << "reading music as as " << musicName << std::endl;
        world->music = new Sound(world->getNextObjectID(), assetManager, musicName);
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
            strncpy(world->quitWorldNameBuffer, world->quitWorldName.c_str(), sizeof(world->quitWorldNameBuffer) - 1);
        } else {
            world->quitWorldName = "";
            strncpy(world->quitWorldNameBuffer, world->quitWorldName.c_str(), sizeof(world->quitWorldNameBuffer) - 1);
        }
    }

    if (!loadMaterials(worldNode, world)) {
        delete world;
        return nullptr;
    }


    //load objects
    if(!loadObjectsFromXML(worldNode, world, limonAPI)) {
        delete world;
        return nullptr;
    }

    loadAnimations(worldNode, world);
    //load Skymap
    loadSkymap(worldNode, world);

    //load lights
    loadLights(worldNode, world);
    //load emitters
    loadParticleEmitters(worldNode, world);
    //load GPU emitters
    loadGPUParticleEmitters(worldNode, world);

    loadGUILayersAndElements(worldNode, world);

    //load triggers
    loadTriggers(worldNode, world);

    //load onloadActions
    loadOnLoadActions(worldNode, world);

    loadOnLoadAnimations(worldNode, world);
    uint32_t endTime = SDL_GetTicks();
    std::cout << "World " << worldName->GetText() << " loaded in " << endTime - currentTime << "ms." << std::endl;
    return world;
}

bool WorldLoader::loadObjectGroupsFromXML(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI,
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
        ModelGroup* modelGroup = ModelGroup::deserialize(graphicsWrapper, assetManager, objectGroupNode, requiredSounds,
                                                         modelGroups, innerModels, limonAPI, nullptr);
        world->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;
        objectGroupNode = objectGroupNode->NextSiblingElement("ObjectGroup");
    } // end of while (objects)

    //now we have 2 more lists to handle

    // 1) Model groups
    for (auto iterator = modelGroups.begin(); iterator != modelGroups.end(); ++iterator) {
        world->modelGroups[iterator->first] = iterator->second;
    }

    //2) ObjectInformations

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

        //ADD NEW ATTRIBUTES GOES UP FROM HERE
        // We will add static objects first, build AI grid, then add other objects
        if(objectInfo->model->getMass() == 0 && !objectInfo->model->isAnimated()) {
            world->addModelToWorld(objectInfo->model);
        } else {
            notStaticObjects.push_back(objectInfo->model);
        }
    }


    return true;
}

bool WorldLoader::loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World *world, LimonAPI *limonAPI) const {
    std::vector<std::vector<std::string>> preloadAssetFiles; //used to load the assets in parallel instead of serial
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);

    //first load the groups
    loadObjectGroupsFromXML(objectsNode, world, limonAPI, notStaticObjects, isAIGridStartPointSet, aiGridStartPoint);

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
        }
        std::string modelFile = objectAttribute->GetText();
        std::vector<std::string> temp;
        temp.emplace_back(modelFile);
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
    std::unique_ptr<ObjectInformation> loadedObjectInformation = std::make_unique<ObjectInformation>();
    loadedObjectInformation->model = new Model(id, assetManager, modelMass, modelFile, disconnected);

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
            std::cerr << "Light does not have ID. This is depricated, and will be removed!" << std::endl;
            lightID = (uint32_t)world->lights.size();
        } else {
            lightID = std::stoul(lightAttribute->GetText());
        }

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

        glm::vec3 attenuation(1, 0.1f, 0.01f);
        tinyxml2::XMLElement* lightAttenuation =  lightNode->FirstChildElement("Attenuation");
        if(lightAttenuation != nullptr) {
            if(loadVec3(lightAttenuation, attenuation)) {
                xmlLight->setAttenuation(attenuation);
            }
        }

        glm::vec3 ambientColor(1, 0.1f, 0.01f);
        tinyxml2::XMLElement* lightAmbient =  lightNode->FirstChildElement("Ambient");
        if(lightAttenuation != nullptr) {
            if(loadVec3(lightAmbient, ambientColor)) {
                xmlLight->setAmbientColor(ambientColor);
            }
        }

        world->addLight(xmlLight);
        lightNode =  lightNode->NextSiblingElement("Light");
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
        emitter->setGravity(gravity);
        emitter->setSpeedMultiplier(speedMultiplier);
        emitter->setSpeedOffset(speedOffset);
        emitter->setTimedColorMultipliers(multipliers);
        emitter->setContinuousEmit(continuousEmit);
        emitter->setEnabled(enabled);
        world->emitters[emitter->getWorldObjectID()] = emitter;
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
            emitterAttributeAttributeElement = emitterAttributeElement->FirstChildElement("Z");
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
            uint32_t index;
            while(parameterNode != nullptr) {
                std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterNode, index);
                if(request == nullptr) {
                    return false;
                }
                actionForOnload->parameters.insert(actionForOnload->parameters.begin() + index, *request);

                parameterNode = parameterNode->NextSiblingElement("Parameter");
            }

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

                tinyxml2::XMLElement *animationIDNode = onloadAnimationNode->FirstChildElement("AnimationID");
                if (animationIDNode == nullptr) {
                    std::cerr << "Animation ID can't be read. Animation loading not possible, skipping"  << std::endl;
                } else {
                    uint32_t animationID = std::stoi(animationIDNode->GetText());

                    world->addAnimationToObject(modelID, animationID, true, true);
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
