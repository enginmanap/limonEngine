//
// Created by engin on 24.03.2018.
//


#include <string>

#include "WorldSaver.h"
#include "World.h"
#include "GameObjects/Light.h"
#include "Assets/Animations/AnimationCustom.h"
#include "GameObjects/TriggerObject.h"
#include "GameObjects/ModelGroup.h"
#include "GUI/GUILayer.h"
#include "GameObjects/Sound.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GamePlay/APISerializer.h"


/************************************************************************************
 * Map file spec
 * world
 *      Name
 *      Music
 *      Objects
 *          Object (for each)
 *              File
                Mass
                ID
                Transformation
                    Scale
                        X,Y,Z
                    Translate
                        X,Y,Z
                    Rotate
                        X,Y,Z
                    Animation
                AI (True, False)
                    AI_ID (If AI true)

 *      Sky
 *                  ImagesPath
 *                  ID
                    Right
                    Left
                    Top
                    Bottom
                    Back
                    Front
 *      Lights
 *              Light (For each)
                    Type  (POINT, DIRECTIONAL)
                    Position
                        X,Y,Z
                    Color
                        R,G,B
 *      LoadedAnimations
 *              LoadedAnimation
 *                  Name
 *                  Index
 *
 */

#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); return a_eResult; }

/**
 *
 * @return
 */
bool WorldSaver::saveWorld(const std::string& mapName, const World* world) {
    tinyxml2::XMLDocument mapDocument;
    tinyxml2::XMLNode * rootNode = mapDocument.NewElement("World");
    mapDocument.InsertFirstChild(rootNode);

    tinyxml2::XMLElement * currentElement = mapDocument.NewElement("Name");
    currentElement->SetText(mapName.c_str());
    rootNode->InsertEndChild(currentElement);

    currentElement = mapDocument.NewElement("LoadingImage");
    currentElement->SetText(world->loadingImage.c_str());
    rootNode->InsertEndChild(currentElement);

    currentElement = mapDocument.NewElement("Player");
    tinyxml2::XMLElement *playerType = mapDocument.NewElement("Type");
    playerType->SetText(world->startingPlayer.typeToString().c_str());
    currentElement->InsertEndChild(playerType);

    tinyxml2::XMLElement *playerPosition = mapDocument.NewElement("Position");
    serializeVec3(mapDocument, playerPosition, world->startingPlayer.position);
    currentElement->InsertEndChild(playerPosition);

    tinyxml2::XMLElement *playerOrientation = mapDocument.NewElement("Orientation");
    serializeVec3(mapDocument, playerOrientation, world->startingPlayer.orientation);
    currentElement->InsertEndChild(playerOrientation);

    tinyxml2::XMLElement *playerExtension = mapDocument.NewElement("ExtensionName");
    playerExtension->SetText(world->startingPlayer.extensionName.c_str());
    currentElement->InsertEndChild(playerExtension);

    tinyxml2::XMLElement *playerAttachement = mapDocument.NewElement("Attachement");
    if(world->startingPlayer.attachedModel != nullptr) {
        if(world->physicalPlayer != nullptr) {
            glm::vec3 attachmentPositionBackup = world->startingPlayer.attachedModel->getTransformation()->getTranslate();
            world->startingPlayer.attachedModel->getTransformation()->setTranslate(
                    world->physicalPlayer->getAttachedModelOffset());
            world->startingPlayer.attachedModel->fillObjects(mapDocument, playerAttachement);
            world->startingPlayer.attachedModel->getTransformation()->setTranslate(attachmentPositionBackup);
        } else {
            //if physical player doesn't exists, but attached model does. This should not happen now, but this line is here as future proofing.
            world->startingPlayer.attachedModel->fillObjects(mapDocument, playerAttachement);
        }
    }
    currentElement->InsertEndChild(playerAttachement);
    rootNode->InsertEndChild(currentElement);

    if(world->music != nullptr) {
        currentElement = mapDocument.NewElement("Music");
        currentElement->SetText(world->music->getName().c_str());
    }
    rootNode->InsertEndChild(currentElement);

    currentElement = mapDocument.NewElement("QuitResponse");
    switch (world->currentQuitResponse) {
        case World::QuitResponse::QUIT_GAME:
            currentElement->SetText("QuitGame");
            break;
        case World::QuitResponse::RETURN_PREVIOUS:
            currentElement->SetText("ReturnPrevious");
            break;
        case World::QuitResponse::LOAD_WORLD:
            currentElement->SetText("LoadWorld");
            break;
    }
    rootNode->InsertEndChild(currentElement);

    currentElement = mapDocument.NewElement("QuitWorldName");
    currentElement->SetText(world->quitWorldName.c_str());
    rootNode->InsertEndChild(currentElement);

    //after current element is inserted, we can reuse
    currentElement = mapDocument.NewElement("Objects");
    if(!fillObjects(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add objects

    currentElement = mapDocument.NewElement("ObjectGroups");
    if(!fillObjectGroups(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add objects

    currentElement = mapDocument.NewElement("Lights");
    if(!fillLights(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add lights

    currentElement = mapDocument.NewElement("Emitters");
    if(!fillEmitters(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add emitters

    currentElement = mapDocument.NewElement("Sky");
    if(!addSky(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add Sky

    currentElement = mapDocument.NewElement("LoadedAnimations");
    if(!fillLoadedAnimations(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add animations

    currentElement = mapDocument.NewElement("Triggers");
    if(!fillTriggers(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add Triggers

    currentElement = mapDocument.NewElement("OnloadActions");
    if(!fillOnloadActions(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add OnloadActions

    currentElement = mapDocument.NewElement("OnLoadAnimations");//make sure this is after loading animations
    if(!fillOnloadAnimations(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add OnloadAnimations

    currentElement = mapDocument.NewElement("GUILayers");
    if(!fillGUILayersAndElements(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add GUI layers


    tinyxml2::XMLError eResult = mapDocument.SaveFile(mapName.c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cerr  << "ERROR " << eResult << std::endl;
    }

    return true;
}

bool WorldSaver::fillObjectGroups(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectGroupsNode, const World *world){
    for(auto it=world->modelGroups.begin(); it != world->modelGroups.end(); it++) {//object ids are not constant, so they can be removed.
        if((it->second)->getParentObject() == nullptr) { //if part of group, group object serializes
            (it->second)->fillObjects(document, objectGroupsNode);
        }
    }
    return true;
}

bool WorldSaver::fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode, const World* world ) {
    for(auto it=world->objects.begin(); it != world->objects.end(); it++) {//object ids are not constant, so they can be removed.
        if((it->second)->getParentObject() == nullptr) {//if part of group, group object serializes
            (it->second)->fillObjects(document, objectsNode);
        }
    }
    return true;
}

bool WorldSaver::fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world) {
    for(auto it=world->lights.begin(); it != world->lights.end(); it++) {//object ids are not constant, so they can be removed.

        tinyxml2::XMLElement *lightElement = document.NewElement("Light");
        lightsNode->InsertEndChild(lightElement);

        tinyxml2::XMLElement *currentElement = document.NewElement("Type");
        switch((*it)->getLightType()) {
            case Light::NONE:
                currentElement->SetText("NONE");
                break;
            case Light::DIRECTIONAL:
                currentElement->SetText("DIRECTIONAL");
                break;
            case Light::POINT:
                currentElement->SetText("POINT");
                break;
        }
        lightElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("ID");
        currentElement->SetText(std::to_string((*it)->getWorldObjectID()).c_str());
        lightElement->InsertEndChild(currentElement);

        tinyxml2::XMLElement *parent = document.NewElement("Position");
        glm::vec3 position = (*it)->getPosition();
        currentElement = document.NewElement("X");
        currentElement->SetText(position.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(position.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(position.z);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);

        parent = document.NewElement("Color");
        glm::vec3 color = (*it)->getColor();
        currentElement = document.NewElement("R");
        currentElement->SetText(color.r);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("G");
        currentElement->SetText(color.g);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("B");
        currentElement->SetText(color.b);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);

        parent = document.NewElement("Attenuation");
        glm::vec3 attenuation = (*it)->getAttenuation();
        currentElement = document.NewElement("X");
        currentElement->SetText(attenuation.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(attenuation.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(attenuation.z);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);

        parent = document.NewElement("Ambient");
        glm::vec3 ambientColor = (*it)->getAmbientColor();
        currentElement = document.NewElement("X");//these are xyz, because they deserialize in standard vec3 way.
        currentElement->SetText(ambientColor.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(ambientColor.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(ambientColor.z);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);
    }
    return true;
}


bool WorldSaver::fillEmitters(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *EmittersNode, const World *world) {
    for(auto it=world->emitters.begin(); it != world->emitters.end(); it++) {//object ids are not constant, so they can be removed.

        tinyxml2::XMLElement *emitterElement = document.NewElement("Emitter");
        EmittersNode->InsertEndChild(emitterElement);

        tinyxml2::XMLElement *currentElement = document.NewElement("ID");
        currentElement->SetText(std::to_string((*it)->getWorldObjectID()).c_str());
        emitterElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("Name");
        currentElement->SetText((*it)->getName().c_str());
        emitterElement->InsertEndChild(currentElement);

        tinyxml2::XMLElement *parent = document.NewElement("StartPosition");
        glm::vec3 starPosition = (*it)->getStartPosition();
        currentElement = document.NewElement("X");
        currentElement->SetText(starPosition.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(starPosition.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(starPosition.z);
        parent->InsertEndChild(currentElement);
        emitterElement->InsertEndChild(parent);

        parent = document.NewElement("Size");
        glm::vec2 size = (*it)->getSize();
        currentElement = document.NewElement("X");
        currentElement->SetText(size.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(size.y);
        parent->InsertEndChild(currentElement);
        emitterElement->InsertEndChild(parent);

        currentElement = document.NewElement("MaxCount");
        currentElement->SetText(std::to_string((*it)->getMaxCount()).c_str());
        emitterElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("LifeTime");
        currentElement->SetText(std::to_string((*it)->getLifeTime()).c_str());
        emitterElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("StartSphereR");
        currentElement->SetText(std::to_string((*it)->getStartSphereR()).c_str());
        emitterElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("Texture");
        currentElement->SetText((*it)->getTexture()->getName().c_str());
        emitterElement->InsertEndChild(currentElement);
    }
    return true;
}

bool WorldSaver::addSky(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *skyNode, const World *world) {

    if(world->sky == nullptr) {
        return true;
    }
    //ImagesPath, Right, Left, Top, Bottom, Back, Front

    tinyxml2::XMLElement *currentElement = document.NewElement("ImagesPath");
    currentElement->SetText(world->sky->getPath().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(world->sky->getWorldObjectID());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Right");
    currentElement->SetText(world->sky->getRight().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Left");
    currentElement->SetText(world->sky->getLeft().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Top");
    currentElement->SetText(world->sky->getTop().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Bottom");
    currentElement->SetText(world->sky->getDown().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Back");
    currentElement->SetText(world->sky->getBack().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Front");
    currentElement->SetText(world->sky->getFront().c_str());
    skyNode->InsertEndChild(currentElement);
    return true;
}

bool WorldSaver::fillLoadedAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *loadedAnimationsNode, const World *world) {
    for(size_t index = 0; index < world->loadedAnimations.size(); index++) {
        tinyxml2::XMLElement *animationElement = document.NewElement("LoadedAnimation");
        loadedAnimationsNode->InsertEndChild(animationElement);
        tinyxml2::XMLElement *currentElement = document.NewElement("Name");
        currentElement->SetText(world->loadedAnimations[index].getName().c_str());
        animationElement->InsertEndChild(currentElement);
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(index).c_str());
        animationElement->InsertEndChild(currentElement);
    }
    return true;

}

bool WorldSaver::fillTriggers(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggersNode, const World *world) {
    for(auto it= world->triggers.begin(); it != world->triggers.end(); it++) {
        it->second->serialize(document, triggersNode);
    }
    return true;
}

bool WorldSaver::fillOnloadActions(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadActionsNode,
                                   const World *world) {
    for(auto it= world->onLoadActions.begin(); it != world->onLoadActions.end(); it++) {
        if(!(*it)->enabled) {
            continue;//Don't save disabled elements
        }
        //we need to save parameters, and trigger code
        tinyxml2::XMLElement *onloadActionNode= document.NewElement("OnloadAction");
        onloadActionsNode->InsertEndChild(onloadActionNode);

        tinyxml2::XMLElement *actionNameNode = document.NewElement("ActionName");
        actionNameNode->SetText((*it)->action->getName().c_str());
        onloadActionNode->InsertEndChild(actionNameNode);


        //now serialize the parameters
        tinyxml2::XMLElement* parametersNode = document.NewElement("Parameters");
        for (size_t i = 0; i < (*it)->parameters.size(); ++i) {
            APISerializer::serializeParameterRequest((*it)->parameters[i], document, parametersNode, i);
        }
        onloadActionNode->InsertEndChild(parametersNode);

        tinyxml2::XMLElement* enabledNode = document.NewElement("Enabled");
        if((*it)->enabled) {
            enabledNode->SetText("True");
        } else {
            enabledNode->SetText("False");
        }
        onloadActionNode->InsertEndChild(enabledNode);

        tinyxml2::XMLElement* indexNode = document.NewElement("Index");
        indexNode->SetText(std::to_string(it - world->onLoadActions.begin()).c_str());
        onloadActionNode->InsertEndChild(indexNode);
    }
    return true;
}

bool WorldSaver::fillOnloadAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadAnimationsNode,
                                      const World *world) {
    bool isSuccess = true;
    for(auto it= world->onLoadAnimations.begin(); it != world->onLoadAnimations.end(); it++) {
        /**
         * we need only 2 information, model ID and loaded animation ID.
         */

        GameObject* animatedObject = dynamic_cast<GameObject*>(world->activeAnimations.at(*it)->object);

        if(animatedObject == nullptr) {
            std::cerr << "Animation save failed because animated object is not Game Object!" << std::endl;
            isSuccess = false;
            continue;
        }

        uint32_t objectID = animatedObject->getWorldObjectID();
        uint32_t loadedAnimationID = world->activeAnimations.at(*it)->animationIndex;

        //we need to save parameters, and trigger code
        tinyxml2::XMLElement *onloadActionNode= document.NewElement("OnLoadAnimation");
        onloadAnimationsNode->InsertEndChild(onloadActionNode);

        tinyxml2::XMLElement *modelIDNode = document.NewElement("ModelID");
        modelIDNode->SetText(std::to_string(objectID).c_str());
        onloadActionNode->InsertEndChild(modelIDNode);

        tinyxml2::XMLElement *animationIDNode = document.NewElement("AnimationID");
        animationIDNode->SetText(std::to_string(loadedAnimationID).c_str());
        onloadActionNode->InsertEndChild(animationIDNode);
    }
    return isSuccess;
}

bool WorldSaver::fillGUILayersAndElements(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *GUILayersListNode,
                                          const World *world) {
    for (size_t i = 0; i < world->guiLayers.size(); ++i) {
        if (!world->guiLayers[i]->serialize(document, GUILayersListNode, world->options)) {
            return false;
        }
    }
    return true;
}

void WorldSaver::serializeVec3(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, const glm::vec3& vector){
    tinyxml2::XMLElement *currentElement = document.NewElement("X");
    currentElement->SetText(vector.x);
    parentNode->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(vector.y);
    parentNode->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(vector.z);
    parentNode->InsertEndChild(currentElement);
}

