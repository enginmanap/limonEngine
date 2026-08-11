//
// Created by engin on 24.03.2018.
//


#include <string>
#include <unordered_set>

#include "WorldSaver.h"
#include "XMLHelper.h"
#include "World.h"
#include "GameObjects/Model.h"
#include "GameObjects/Light.h"
#include "GameObjects/SkyBox.h"
#include "Assets/Animations/AnimationCustom.h"
#include "GameObjects/TriggerObject.h"
#include "GameObjects/ModelGroup.h"
#include "GUI/GUILayer.h"
#include "GameObjects/Sound.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "limonAPI/CameraExtensionInterface.h"
#include "GameObjects/CameraRig.h"
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

    XMLHelper::writeElement(mapDocument, rootNode, "Name", mapName);
    XMLHelper::writeElement(mapDocument, rootNode, "SaveVersion", 2);
    XMLHelper::writeElement(mapDocument, rootNode, "LoadingImage", world->loadingImage);

    tinyxml2::XMLElement * currentElement = mapDocument.NewElement("Player");
    tinyxml2::XMLElement *playerType = mapDocument.NewElement("Type");
    playerType->SetText(world->startingPlayer.typeToString().c_str());
    currentElement->InsertEndChild(playerType);

    tinyxml2::XMLElement *playerPosition = mapDocument.NewElement("Position");
    XMLHelper::writeVec3(mapDocument, playerPosition, world->startingPlayer.position);
    currentElement->InsertEndChild(playerPosition);

    tinyxml2::XMLElement *playerOrientation = mapDocument.NewElement("Orientation");
    XMLHelper::writeVec3(mapDocument, playerOrientation, world->startingPlayer.orientation);
    currentElement->InsertEndChild(playerOrientation);

    tinyxml2::XMLElement *playerExtension = mapDocument.NewElement("ExtensionName");
    playerExtension->SetText(world->startingPlayer.extensionName.c_str());
    currentElement->InsertEndChild(playerExtension);

    tinyxml2::XMLElement *playerExtensionParameters = mapDocument.NewElement("ExtensionParameters");
    for (size_t i = 0; i < world->startingPlayer.parameters.size(); ++i) {
        APISerializer::serializeParameterRequest(world->startingPlayer.parameters[i], mapDocument, playerExtensionParameters, i);
    }
    currentElement->InsertEndChild(playerExtensionParameters);

    tinyxml2::XMLElement *playerAttachment = mapDocument.NewElement("Attachment");
    if(world->startingPlayer.attachedModel != nullptr) {
        if(world->physicalPlayer != nullptr) {
            glm::vec3 attachmentPositionBackup = world->startingPlayer.attachedModel->getTransformation()->getTranslate();
            world->startingPlayer.attachedModel->getTransformation()->setTranslate(
                    world->physicalPlayer->getAttachedModelOffset());
            world->startingPlayer.attachedModel->fillObjects(mapDocument, playerAttachment);
            world->startingPlayer.attachedModel->getTransformation()->setTranslate(attachmentPositionBackup);
        } else {
            //if physical player doesn't exists, but attached model does. This should not happen now, but this line is here as future proofing.
            world->startingPlayer.attachedModel->fillObjects(mapDocument, playerAttachment);
        }
        // Save children as flat V2 sibling <Object> elements (each with <ParentID>).
        saveAttachmentChildrenFlat(mapDocument, playerAttachment, world->startingPlayer.attachedModel);
    }
    currentElement->InsertEndChild(playerAttachment);
    rootNode->InsertEndChild(currentElement);

    // The world's camera rigs (each a CameraRig GameObject owning a registered CameraExtensionInterface).
    if(!world->cameraRigs.empty()) {
        tinyxml2::XMLElement *cameraRigsNode = mapDocument.NewElement("CameraRigs");

        XMLHelper::writeElement(mapDocument, cameraRigsNode, "ActiveID",
                                 world->activeCameraRig != nullptr ? world->activeCameraRig->getWorldObjectID() : 0u);

        for(const std::unique_ptr<CameraRig>& cameraRig : world->cameraRigs) {
            cameraRig->serialize(mapDocument, cameraRigsNode);
        }
        rootNode->InsertEndChild(cameraRigsNode);
    }

    if(world->music != nullptr) {
        XMLHelper::writeElement(mapDocument, rootNode, "Music", world->music->getName());
    }

    const char* quitResponseStr = "QuitGame";
    switch (world->currentQuitResponse) {
        case World::QuitResponse::QUIT_GAME:       quitResponseStr = "QuitGame";       break;
        case World::QuitResponse::RETURN_PREVIOUS: quitResponseStr = "ReturnPrevious"; break;
        case World::QuitResponse::LOAD_WORLD:      quitResponseStr = "LoadWorld";      break;
    }
    XMLHelper::writeElement(mapDocument, rootNode, "QuitResponse", quitResponseStr);
    XMLHelper::writeElement(mapDocument, rootNode, "QuitWorldName", world->quitWorldName);

    const char* distanceModelStr = "LinearClamped";
    switch(world->soundDistanceModel) {
        case ALHelper::DistanceModel::INVERSE_CLAMPED:  distanceModelStr = "InverseClamped";  break;
        case ALHelper::DistanceModel::EXPONENT_CLAMPED: distanceModelStr = "ExponentClamped"; break;
        case ALHelper::DistanceModel::LINEAR_CLAMPED:   distanceModelStr = "LinearClamped";   break;
        default:                                        distanceModelStr = "LinearClamped";   break;
    }
    XMLHelper::writeElement(mapDocument, rootNode, "SoundDistanceModel", distanceModelStr);

    currentElement = mapDocument.NewElement("Materials");
    if(!fillMaterials(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add objects

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

    currentElement = mapDocument.NewElement("Sounds");
    if(!fillSounds(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add sounds

    currentElement = mapDocument.NewElement("Emitters");
    if(!fillEmitters(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add emitters

    currentElement = mapDocument.NewElement("Sky");
    if(world->sky != nullptr) {
        world->sky->serialize(mapDocument, currentElement);
    }
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

void WorldSaver::saveAttachmentChildrenFlat(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* attachmentNode, const Model* model) {
    for(Attachable* child : model->getChildren()) {
        const Model* childModel = dynamic_cast<const Model*>(child);
        if(childModel == nullptr) continue;
        // fillObjects already writes <ParentID> and <ParentBoneID> when parentObject != nullptr
        childModel->fillObjects(document, attachmentNode);
        saveAttachmentChildrenFlat(document, attachmentNode, childModel);
    }
}

bool WorldSaver::fillObjectGroups(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectGroupsNode, const World *world){
    for(auto it=world->modelGroups.begin(); it != world->modelGroups.end(); it++) {//object ids are not constant, so they can be removed.
        // Every group is written flat; nested groups carry a <ParentID> and are reattached on load.
        (it->second)->fillObjects(document, objectGroupsNode);
    }
    return true;
}

bool WorldSaver::fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode, const World* world ) {
    for(auto it=world->objects.begin(); it != world->objects.end(); it++) {
        (it->second)->fillObjects(document, objectsNode);
    }
    return true;
}

bool WorldSaver::fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world) {
    for(auto it=world->lights.begin(); it != world->lights.end(); it++) {//object ids are not constant, so they can be removed.
        (*it)->serialize(document, lightsNode);
    }
    return true;
}

bool WorldSaver::fillSounds(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *soundsNode, const World *world) {
    for(auto& kv : world->sounds) {
        kv.second->serialize(document, soundsNode);
    }
    return true;
}


bool WorldSaver::fillEmitters(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *EmittersNode, const World *world) {
    for(auto it=world->emitters.begin(); it != world->emitters.end(); it++) {//object ids are not constant, so they can be removed.
        it->second->serialize(document, EmittersNode);
    }
    return true;
}

bool WorldSaver::fillLoadedAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *loadedAnimationsNode, const World *world) {
    for(size_t index = 0; index < world->loadedAnimations.size(); index++) {
        tinyxml2::XMLElement *animationElement = document.NewElement("LoadedAnimation");
        loadedAnimationsNode->InsertEndChild(animationElement);
        XMLHelper::writeElement(document, animationElement, "Name", world->loadedAnimations[index].getName());
        XMLHelper::writeElement(document, animationElement, "Index", std::to_string(index));
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

        XMLHelper::writeElement(document, onloadActionNode, "ActionName", (*it)->action->getName());


        //now serialize the parameters, owned by the trigger instance
        const std::vector<LimonTypes::GenericParameter> parameters = (*it)->action->getParameters();
        tinyxml2::XMLElement* parametersNode = document.NewElement("Parameters");
        for (size_t i = 0; i < parameters.size(); ++i) {
            APISerializer::serializeParameterRequest(parameters[i], document, parametersNode, i);
        }
        onloadActionNode->InsertEndChild(parametersNode);

        XMLHelper::writeElement(document, onloadActionNode, "Enabled", (*it)->enabled ? "True" : "False");
        XMLHelper::writeElement(document, onloadActionNode, "Index", std::to_string(it - world->onLoadActions.begin()));
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

        XMLHelper::writeElement(document, onloadActionNode, "ModelID", objectID);

        // Name is the key we save and load by now. We used to save ID, but that is depending on the load order, so it was not safe.
        // Loader still supports ID based loading, but as backward compatibility, we don't wanna keep that around so we will not save it.
        if(loadedAnimationID < world->loadedAnimations.size()) {
            XMLHelper::writeElement(document, onloadActionNode, "Name", world->loadedAnimations[loadedAnimationID].getName());
        } else {
            std::cerr << "OnLoad animation index " << loadedAnimationID
                      << " is out of range, animation can't be saved by name, skipping." << std::endl;
            isSuccess = false;
        }
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

bool WorldSaver::fillMaterials(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode, const World *world) {
    //this world's overrides, each identified on load by the originalHash of the base it replaces
    const std::unordered_map<uint32_t, std::shared_ptr<Material>>& materialOverrides = world->getMaterialOverrides();

    for (std::unordered_map<uint32_t, std::shared_ptr<Material>>::const_iterator it = materialOverrides.begin(); it != materialOverrides.end(); ++it) {
        it->second->serialize(document, materialsNode);
    }
    return true;
}


