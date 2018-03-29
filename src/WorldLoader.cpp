//
// Created by engin on 10.03.2018.
//

#include "WorldLoader.h"
#include "World.h"
#include "AI/HumanEnemy.h"
#include "GameObjects/SkyBox.h"
#include "GameObjects/Light.h"

WorldLoader::WorldLoader(AssetManager* assetManager, GLHelper* glHelper, Options* options):
        options(options),
        glHelper(glHelper),
        assetManager(assetManager)
{}

World* WorldLoader::loadWorld(const std::string& worldFile) const {
    World* newWorld = new World(assetManager, glHelper, options);
    if(!loadMapFromXML(worldFile, newWorld)) {
        std::cerr << "world load failed" << std::endl;
        return nullptr;
    }
    return newWorld;
}


bool WorldLoader::loadMapFromXML(const std::string& worldFileName, World* world) const {

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(worldFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML: " <<  xmlDoc.ErrorName() << std::endl;
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
    //load Skymap
    loadSkymap(worldNode, world);

    //load lights
    loadLights(worldNode, world);

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
    tinyxml2::XMLElement* objectAttributeAttribute;
    std::string modelFile;
    float modelMass;
    float x,y,z,w;
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

        xmlModel = new Model(world->getNextObjectID(), assetManager, modelMass, modelFile);

        objectAttribute =  objectNode->FirstChildElement("Scale");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have scale." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 1.0;
            }
            xmlModel->addScale(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Translate");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            xmlModel->addTranslate(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Rotate");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("W");
            if(objectAttributeAttribute != nullptr) {
                w = std::stof(objectAttributeAttribute->GetText());
            } else {
                w = 0.0;
            }
            xmlModel->addOrientation(glm::quat(w, x, y, z));
        }
        //Since we are not loading objects recursively, these can be set here safely
        objectAttribute =  objectNode->FirstChildElement("AI");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have AI." << std::endl;
        } else {
            if (!isAIGridStartPointSet) {
                aiGridStartPoint = GLMConverter::BltToGLM(xmlModel->getRigidBody()->getCenterOfMassPosition()) +
                                   glm::vec3(0, 2.0f, 0);
                isAIGridStartPointSet = true;
            }
            std::cout << "Object has AI." << std::endl;
            HumanEnemy* newEnemy = new HumanEnemy(world->getNextObjectID());
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

    world->setSky(
            new SkyBox(world->getNextObjectID(), assetManager, std::string(path), std::string(right), std::string(left),
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