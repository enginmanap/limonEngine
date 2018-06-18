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


WorldLoader::WorldLoader(AssetManager* assetManager, GLHelper* glHelper, Options* options):
        options(options),
        glHelper(glHelper),
        assetManager(assetManager)
{}

World* WorldLoader::loadWorld(const std::string& worldFile) const {
    World* newWorld = new World(assetManager, glHelper, options);


    // Set api endpoints accordingly
    LimonAPI* api = new LimonAPI();
    api->worldAddAnimationToObject = std::bind(&World::addAnimationToObject, newWorld, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false);
    api->worldAddGuiText = std::bind(&World::addGuiText, newWorld, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    api->worldUpdateGuiText = std::bind(&World::updateGuiText, newWorld, std::placeholders::_1, std::placeholders::_2);
    api->worldGenerateEditorElementsForParameters = std::bind(&World::generateEditorElementsForParameters, newWorld, std::placeholders::_1, std::placeholders::_2);
    api->worldGetResultOfTrigger = std::bind(&World::getResultOfTrigger, newWorld, std::placeholders::_1, std::placeholders::_2);
    api->worldRemoveGuiText = std::bind(&World::removeGuiText, newWorld, std::placeholders::_1);
    api->worldRemoveObject = std::bind(&World::removeObject, newWorld, std::placeholders::_1);

    newWorld->apiInstance = api;

    if(!loadMapFromXML(worldFile, newWorld)) {
        std::cerr << "world load failed" << std::endl;
        delete newWorld;
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


bool WorldLoader::loadMapFromXML(const std::string& worldFileName, World* world) const {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(worldFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML "<< worldFileName << ": " <<  xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * worldNode = xmlDoc.FirstChild();
    if (worldNode == nullptr) {
        std::cerr << "World xml is not a valid XML." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* worldName =  worldNode->FirstChildElement("Name");
    if (worldName == nullptr) {
        std::cerr << "World must have a name." << std::endl;
        return false;
    }
    std::cout << "read name as " << worldName->GetText() << std::endl;

    //load objects
    if(!loadObjectsFromXML(worldNode, world)) {
        return false;
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

    return true;
}

bool WorldLoader::loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World* world) const {
    tinyxml2::XMLElement* objectsListNode =  objectsNode->FirstChildElement("Objects");
    if (objectsListNode == nullptr) {
        std::cerr << "World Must have and Objects clause." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* objectNode =  objectsListNode->FirstChildElement("Object");
    if (objectNode == nullptr) {
        std::cerr << "World Must have at least one object." << std::endl;
        return false;
    }
    Model *xmlModel;
    tinyxml2::XMLElement* objectAttribute;
    std::string modelFile;
    float modelMass;
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint;
    while(objectNode != nullptr) {
        objectAttribute =  objectNode->FirstChildElement("File");
        if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
            return false;
        }
        modelFile = objectAttribute->GetText();
        objectAttribute =  objectNode->FirstChildElement("Mass");
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
            return false;
        } else {
            id = std::stoi(objectAttribute->GetText());
        }

        bool disconnected = false;
        objectAttribute =  objectNode->FirstChildElement("Disconnected");
        if (objectAttribute == nullptr) {
            std::cout << "Object disconnect status is not set. defaulting to False" << std::endl;
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

        xmlModel = new Model(id, assetManager, modelMass, modelFile, disconnected);

        objectAttribute =  objectNode->FirstChildElement("Transformation");
        if(objectAttribute == nullptr) {
            std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
            return false;
        }
        xmlModel->getTransformation()->deserialize(objectAttribute);

        //Since we are not loading objects recursively, these can be set here safely
        objectAttribute =  objectNode->FirstChildElement("AI");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have AI." << std::endl;
        } else {
            int ai_id;
            objectAttribute =  objectNode->FirstChildElement("AI_ID");
            if (objectAttribute == nullptr) {
                std::cerr << "Object AI does not have ID. Can't be loaded" << std::endl;
                return false;
            } else {
                ai_id = std::stoi(objectAttribute->GetText());
            }
            if (!isAIGridStartPointSet) {
                aiGridStartPoint = GLMConverter::BltToGLM(xmlModel->getRigidBody()->getCenterOfMassPosition()) +
                                   glm::vec3(0, 2.0f, 0);
                isAIGridStartPointSet = true;
            }
            std::cout << "Object has AI." << std::endl;
            HumanEnemy* newEnemy = new HumanEnemy(ai_id);
            newEnemy->setModel(xmlModel);
            world->addActor(newEnemy);
        }

        objectAttribute =  objectNode->FirstChildElement("Animation");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have default animation." << std::endl;
        } else {
            xmlModel->setAnimation(objectAttribute->GetText());
        }

        //ADD NEW ATTRIBUTES GOES UP FROM HERE
        // We will add static objects first, build AI grid, then add other objects
        if(xmlModel->getMass() == 0 && !xmlModel->isAnimated()) {
            world->addModelToWorld(xmlModel);
        } else {
            notStaticObjects.push_back(xmlModel);
        }
        objectNode = objectNode->NextSiblingElement("Object");
    } // end of while (objects)

    world->createGridFrom(aiGridStartPoint);

    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        world->addModelToWorld(notStaticObjects[i]);
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

WorldLoader::~WorldLoader() {
    for (unsigned int i = 0; i < loadedWorlds.size(); ++i) {
        delete loadedWorlds[i];
    }
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
        TriggerObject* triggerObject = new TriggerObject(0, world->apiInstance);//0 is place holder, deserialize sets real value;
        if(!triggerObject->deserialize(triggerNode)) {
            //this trigger is now headless
            delete triggerObject;
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

                    world->addAnimationToObject(modelID,animationID,true,true);
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
            GUIText* element = GUIText::deserialize(GUIElementNode, glHelper, &world->fontManager);
            world->guiElements[element->getWorldObjectID()] = element;
            layer->addGuiElement(element);

            GUIElementNode = GUIElementNode->NextSiblingElement("GUIElement");
        }// end of while (GUIElementNode)

        GUILayerNode = GUILayerNode->NextSiblingElement("GUILayer");
    } // end of while (GUILayer)
    return true;
}