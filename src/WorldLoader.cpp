//
// Created by engin on 10.03.2018.
//

#include <algorithm>

#include "WorldLoader.h"
#include "World.h"
#include "AI/HumanEnemy.h"
#include "GameObjects/SkyBox.h"
#include "GameObjects/Light.h"
#include "GameObjects/TriggerObject.h"

#include "GamePlay/LimonAPI.h"
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

WorldLoader::WorldLoader(AssetManager *assetManager, InputHandler *inputHandler, Options *options) :
        options(options),
        glHelper(assetManager->getGlHelper()),
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
    limonAPI->worldUpdateGuiText = std::bind(&World::updateGuiText, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGenerateEditorElementsForParameters = std::bind(&World::generateEditorElementsForParameters, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldGetResultOfTrigger = std::bind(&World::getResultOfTrigger, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveGuiText = std::bind(&World::removeGuiText, world, std::placeholders::_1);
    limonAPI->worldRemoveObject = std::bind(&World::removeObject, world, std::placeholders::_1);
    limonAPI->worldAttachObjectToObject = std::bind(&World::attachObjectToObject, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldRemoveTriggerObject = std::bind(&World::removeTriggerObject, world, std::placeholders::_1);
    limonAPI->worldDisconnectObjectFromPhysics = std::bind(&World::disconnectObjectFromPhysics, world, std::placeholders::_1);
    limonAPI->worldReconnectObjectToPhysics= std::bind(&World::reconnectObjectToPhysics, world, std::placeholders::_1);
    limonAPI->worldAttachSoundToObjectAndPlay = std::bind(&World::attachSoundToObjectAndPlay, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldDetachSoundFromObject = std::bind(&World::detachSoundFromObject, world, std::placeholders::_1);
    limonAPI->worldPlaySound = std::bind(&World::playSound, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldRayCastToCursor = std::bind(&World::rayCastToCursorAPI, world);
    limonAPI->worldGetObjectTransformation = std::bind(&World::getObjectTransformationAPI, world, std::placeholders::_1);
    limonAPI->worldGetObjectTransformationMatrix = std::bind(&World::getObjectTransformationMatrixAPI, world, std::placeholders::_1);
    limonAPI->worldInteractWithAI = std::bind(&World::interactWithAIAPI, world, std::placeholders::_1, std::placeholders::_2);
    limonAPI->worldInteractWithPlayer = std::bind(&World::interactWithPlayerAPI, world, std::placeholders::_1);
    limonAPI->worldaddTimedEvent = std::bind(&World::addTimedEventAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldGetPlayerAttachedModel = std::bind(&World::getPlayerAttachedModelAPI, world);
    limonAPI->worldGetModelAnimationName = std::bind(&World::getModelAnimationNameAPI, world, std::placeholders::_1);
    limonAPI->worldGetModelAnimationFinished = std::bind(&World::getModelAnimationFinishedAPI, world, std::placeholders::_1);
    limonAPI->worldSetAnimationOfModel = std::bind(&World::setModelAnimationAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    limonAPI->worldSetAnimationOfModelWithBlend = std::bind(&World::setModelAnimationWithBlendAPI, world, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    limonAPI->worldGetPlayerAttachmentOffset = std::bind(&World::getPlayerModelOffsetAPI, world);
    world->apiInstance = limonAPI;
}


World * WorldLoader::loadMapFromXML(const std::string &worldFileName, LimonAPI *limonAPI) const {
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

        tinyxml2::XMLElement* playerAttachementModel =  worldStartPlayer->FirstChildElement("Attachement");
        if(playerAttachementModel != nullptr) {
            tinyxml2::XMLElement* objectNode =  playerAttachementModel->FirstChildElement("Object");
            if (objectNode != nullptr) {
                std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds; //required. Should not be used normally.

                std::unique_ptr<ObjectInformation> objectInfo = loadObject(objectNode,
                                                                           requiredSounds, limonAPI);//this map is used to load all the sounds, while sharing same objects.
                if (objectInfo) { //if not null
                    if (objectInfo->modelActor != nullptr) {
                        std::cerr << "There was an AI attached to player model, this shouldn't happen. Ignoring" << std::endl;
                        delete objectInfo->modelActor;
                    }
                    startingPlayer.attachedModel = objectInfo->model;
                }
            }
        }
    }

    World* world = new World(std::string(worldName->GetText()), startingPlayer, inputHandler, assetManager, options);

    attachedAPIMethodsToWorld(world, limonAPI);

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

    //load triggers
    loadTriggers(worldNode, world);

    //load onloadActions
    loadOnLoadActions(worldNode, world);

    loadOnLoadAnimations(worldNode, world);

    loadGUILayersAndElements(worldNode, world);

    return world;
}

bool WorldLoader::loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World *world, LimonAPI *limonAPI) const {
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
    std::vector<Model*> notStaticObjects;

    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);
    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;

    while(objectNode != nullptr) {

        std::unique_ptr<ObjectInformation> objectInfo = loadObject(objectNode,
                                                                   requiredSounds, limonAPI);//this map is used to load all the sounds, while sharing same objects.

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
        objectNode = objectNode->NextSiblingElement("Object");
    } // end of while (objects)

    world->createGridFrom(aiGridStartPoint);

    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        world->addModelToWorld(notStaticObjects[i]);
    }
    return true;
}

std::unique_ptr<WorldLoader::ObjectInformation> WorldLoader::loadObject(tinyxml2::XMLElement *objectNode,
                                                                        std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                                                        LimonAPI *limonAPI) const {
    std::unique_ptr<ObjectInformation> loadedObjectInformation = std::make_unique<ObjectInformation>();


    tinyxml2::XMLElement *objectAttribute =  objectNode->FirstChildElement("File");
    if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
        return std::unique_ptr<ObjectInformation>(nullptr);
        }
    std::string modelFile = objectAttribute->GetText();
    objectAttribute =  objectNode->FirstChildElement("Mass");
    float modelMass;
    if (objectAttribute == nullptr) {
            std::cout << "Object does not have mass, assume 0." << std::endl;
            modelMass = 0;
        } else {
            modelMass = std::stof(objectAttribute->GetText());
        }
    int id;
    objectAttribute =  objectNode->FirstChildElement("ID");
    if (objectAttribute == nullptr) {
            std::cerr << "Object does not have ID. Can't be loaded" << std::endl;
            return std::unique_ptr<ObjectInformation>(nullptr);
        } else {
            id = std::stoi(objectAttribute->GetText());
        }

    bool disconnected = false;
    objectAttribute =  objectNode->FirstChildElement("Disconnected");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
            std::cout << "Object disconnect status is not set. defaulting to False" << std::endl;
#endif
        } else {
            std::string disConnectedText = objectAttribute->GetText();
            if(disConnectedText == "True") {
                disconnected = true;
            } else if(disConnectedText == "False") {
                disconnected = false;
            } else {
                std::cout << "Object disconnect status is unknown. defaulting to False" << std::endl;
            }
        }

    loadedObjectInformation->model = new Model(id, assetManager, modelMass, modelFile, disconnected);

    objectAttribute =  objectNode->FirstChildElement("StepOnSound");

    if (objectAttribute == nullptr) {
            std::cerr << "Object does not have step on sound." << std::endl;
        } else {
            std::string stepOnSound = objectAttribute->GetText();
            if(requiredSounds.find(stepOnSound) == requiredSounds.end()) {
                requiredSounds[stepOnSound] = std::make_shared<Sound>(0, assetManager, stepOnSound);//since the step on is not managed by world, not feed world object ID
            }
            loadedObjectInformation->model->setPlayerStepOnSound(requiredSounds[stepOnSound]);
        }

    objectAttribute =  objectNode->FirstChildElement("Transformation");
    if(objectAttribute == nullptr) {
            std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
            delete loadedObjectInformation->model;
        return std::unique_ptr<ObjectInformation>(nullptr);
    }
    loadedObjectInformation->model->getTransformation()->deserialize(objectAttribute);

    //Since we are not loading objects recursively, these can be set here safely
    objectAttribute =  objectNode->FirstChildElement("AI");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
            std::cout << "Object does not have AI." << std::endl;
#endif
        } else {
            int ai_id;
            objectAttribute =  objectNode->FirstChildElement("AI_ID");
            if (objectAttribute == nullptr) {
                std::cerr << "Object AI does not have ID. Can't be loaded" << std::endl;
                delete loadedObjectInformation->model;
                return std::unique_ptr<ObjectInformation>(nullptr);
            } else {
                ai_id = std::stoi(objectAttribute->GetText());
            }
            loadedObjectInformation->aiGridStartPoint = GLMConverter::BltToGLM(loadedObjectInformation->model->getRigidBody()->getCenterOfMassPosition()) +
                               glm::vec3(0, 2.0f, 0);
            loadedObjectInformation->isAIGridStartPointSet = true;
            std::cout << "Object has AI." << std::endl;

            loadedObjectInformation->modelActor = new HumanEnemy(ai_id, limonAPI);
            loadedObjectInformation->modelActor->setModel(loadedObjectInformation->model);
        }

    objectAttribute =  objectNode->FirstChildElement("Animation");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
            std::cout << "Object does not have default animation." << std::endl;
#endif
        } else {
        loadedObjectInformation->model->setAnimation(objectAttribute->GetText());
        }

    return loadedObjectInformation;
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
    while(lightNode != nullptr) {
        lightAttribute = lightNode->FirstChildElement("Type");
        if (lightAttribute == nullptr) {
            std::cerr << "Light must have a type." << std::endl;
            return false;
        }

        std::string typeString = lightAttribute->GetText();
        if (typeString == "POINT") {
            type = Light::POINT;
        } else if (typeString == "DIRECTIONAL") {
            type = Light::DIRECTIONAL;
        } else {
            std::cerr << "Light type is not POINT or DIRECTIONAL. it is " << lightAttribute->GetText() << std::endl;
            return false;
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

        xmlLight = new Light(glHelper, world->lights.size(), type, position, color);
        world->addLight(xmlLight);
        lightNode =  lightNode->NextSiblingElement("Light");
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
        TriggerObject* triggerObject = triggerObject->deserialize(triggerNode, world->apiInstance);
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

                LimonAPI::ParameterRequest request;
                if(!request.deserialize(parameterNode, index)) {
                    return false;
                }
                actionForOnload->parameters.insert(actionForOnload->parameters.begin() + index, request);

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

        GUILayer* layer = new GUILayer(glHelper, world->debugDrawer, level);
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
                    element = GUIText::deserialize(GUIElementNode, glHelper, &world->fontManager, options);
                    name = static_cast<GUIText*>(element)->getName();
                } else if(typeName == "GUIImage") {
                    element = GUIImage::deserialize(GUIElementNode, assetManager, options);
                    name = static_cast<GUIImage*>(element)->getName();
                } else if(typeName == "GUIButton") {
                    element = GUIButton::deserialize(GUIElementNode, assetManager, options, world->apiInstance);
                    name = static_cast<GUIButton*>(element)->getName();
                } else if(typeName == "GUIAnimation") {
                    element = GUIAnimation::deserialize(GUIElementNode, assetManager, options);
                    name = static_cast<GUIAnimation*>(element)->getName();
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

bool WorldLoader::loadVec3(tinyxml2::XMLNode *vectorNode, glm::vec3& vector) {
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
