//
// Created by Engin Manap on 10/11/2026.
//

#include "V1UnitScale.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "../Assets/Animations/AnimationCustom.h"

static const float CENTIMETERS_PER_INCH = 2.54f;

void V1UnitScale::toMeters(tinyxml2::XMLElement *element) {
    if (element == nullptr) {
        return;
    }
    if (element->FirstChildElement() == nullptr) {
        if (element->GetText() != nullptr) {
            element->SetText(std::strtof(element->GetText(), nullptr) / CENTIMETERS_PER_INCH);
        }
        return;
    }
    const char *axisNames[] = {"X", "Y", "Z"};
    for (const char *axisName : axisNames) {
        tinyxml2::XMLElement *axisNode = element->FirstChildElement(axisName);
        if (axisNode != nullptr && axisNode->GetText() != nullptr) {
            axisNode->SetText(std::strtof(axisNode->GetText(), nullptr) / CENTIMETERS_PER_INCH);
        }
    }
}

void V1UnitScale::childToMeters(tinyxml2::XMLElement *owner, const char *childName) {
    toMeters(owner->FirstChildElement(childName));
}

//version 1 nests children with local transforms, so a parent link here is rare. A local transform shrinks with its parent
bool V1UnitScale::isParented(tinyxml2::XMLElement *owner, const char *kind) {
    tinyxml2::XMLElement *parentNode = owner->FirstChildElement("ParentID");
    if (parentNode == nullptr || parentNode->GetText() == nullptr) {
        return false;
    }
    std::cerr << "Legacy unit conversion left the transform of a " << kind << " attached to " << parentNode->GetText()
              << " as it is, check its offset" << std::endl;
    return true;
}

void V1UnitScale::rootTransformationToMeters(tinyxml2::XMLElement *owner, const char *kind) {
    tinyxml2::XMLElement *transformationNode = owner->FirstChildElement("Transformation");
    if (transformationNode == nullptr || isParented(owner, kind)) {
        return;
    }
    childToMeters(transformationNode, "Translate");
    childToMeters(transformationNode, "Scale");
}

//1 / (constant + linear * d + quadratic * d²) keeps its falloff over the same ground only like this
void V1UnitScale::attenuationToMeters(tinyxml2::XMLElement *lightNode) {
    tinyxml2::XMLElement *attenuationNode = lightNode->FirstChildElement("Attenuation");
    if (attenuationNode == nullptr) {
        return;
    }
    tinyxml2::XMLElement *linearNode = attenuationNode->FirstChildElement("Y");
    if (linearNode != nullptr && linearNode->GetText() != nullptr) {
        linearNode->SetText(std::strtof(linearNode->GetText(), nullptr) * CENTIMETERS_PER_INCH);
    }
    tinyxml2::XMLElement *quadraticNode = attenuationNode->FirstChildElement("Z");
    if (quadraticNode != nullptr && quadraticNode->GetText() != nullptr) {
        quadraticNode->SetText(std::strtof(quadraticNode->GetText(), nullptr) * CENTIMETERS_PER_INCH * CENTIMETERS_PER_INCH);
    }
}

//only the top level, an object's Children are local to it
void V1UnitScale::convertObjects(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("Objects");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *objectNode = listNode->FirstChildElement("Object"); objectNode != nullptr;
         objectNode = objectNode->NextSiblingElement("Object")) {
        rootTransformationToMeters(objectNode, "object");
    }
}

void V1UnitScale::convertObjectGroups(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("ObjectGroups");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *groupNode = listNode->FirstChildElement("ObjectGroup"); groupNode != nullptr;
         groupNode = groupNode->NextSiblingElement("ObjectGroup")) {
        rootTransformationToMeters(groupNode, "object group");
    }
}

void V1UnitScale::convertPlayer(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *playerNode = worldNode->FirstChildElement("Player");
    if (playerNode == nullptr) {
        return;
    }
    childToMeters(playerNode, "Position");
    tinyxml2::XMLElement *attachmentNode = playerNode->FirstChildElement("Attachment");
    if (attachmentNode == nullptr) {
        attachmentNode = playerNode->FirstChildElement("Attachement");//maps saved before 5806295d
    }
    if (attachmentNode == nullptr) {
        return;
    }
    //local to the player, but the player shrinks in code rather than in the map, so the offset has to shrink here
    for (tinyxml2::XMLElement *objectNode = attachmentNode->FirstChildElement("Object"); objectNode != nullptr;
         objectNode = objectNode->NextSiblingElement("Object")) {
        tinyxml2::XMLElement *transformationNode = objectNode->FirstChildElement("Transformation");
        if (transformationNode != nullptr) {
            childToMeters(transformationNode, "Translate");
            childToMeters(transformationNode, "Scale");
        }
    }
}

void V1UnitScale::convertLights(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("Lights");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *lightNode = listNode->FirstChildElement("Light"); lightNode != nullptr;
         lightNode = lightNode->NextSiblingElement("Light")) {
        tinyxml2::XMLElement *typeNode = lightNode->FirstChildElement("Type");
        if (typeNode != nullptr && typeNode->GetText() != nullptr && std::strcmp(typeNode->GetText(), "DIRECTIONAL") == 0) {
            continue;//position is a direction, radius and attenuation are not used
        }
        if (lightNode->FirstChildElement("Transformation") != nullptr) {
            rootTransformationToMeters(lightNode, "light");
        } else if (!isParented(lightNode, "light")) {
            childToMeters(lightNode, "Position");
        }
        //load rebalances attenuation against the radius, both have to move or the falloff changes. No radius takes the default, already in meters
        childToMeters(lightNode, "Radius");
        attenuationToMeters(lightNode);
    }
}

void V1UnitScale::convertSounds(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("Sounds");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *soundNode = listNode->FirstChildElement("Sound"); soundNode != nullptr;
         soundNode = soundNode->NextSiblingElement("Sound")) {
        rootTransformationToMeters(soundNode, "sound");
        childToMeters(soundNode, "ReferenceDistance");
        childToMeters(soundNode, "MaxDistance");
    }
}

void V1UnitScale::convertEmitters(tinyxml2::XMLNode *worldNode, const char *listName, const char *emitterName) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement(listName);
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *emitterNode = listNode->FirstChildElement(emitterName); emitterNode != nullptr;
         emitterNode = emitterNode->NextSiblingElement(emitterName)) {
        if (emitterNode->FirstChildElement("Transformation") != nullptr) {
            rootTransformationToMeters(emitterNode, emitterName);
        } else if (!isParented(emitterNode, emitterName)) {
            childToMeters(emitterNode, "StartPosition");
        }
        childToMeters(emitterNode, "MaximumStartDistances");
        childToMeters(emitterNode, "Size");
        childToMeters(emitterNode, "SpeedMultiplier");
        childToMeters(emitterNode, "SpeedOffset");
        childToMeters(emitterNode, "Gravity");
    }
}

void V1UnitScale::convertTriggers(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("Triggers");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *triggerNode = listNode->FirstChildElement("Trigger"); triggerNode != nullptr;
         triggerNode = triggerNode->NextSiblingElement("Trigger")) {
        rootTransformationToMeters(triggerNode, "trigger");//its scale is the volume's size
    }
}

void V1UnitScale::convertCameraRigs(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement *listNode = worldNode->FirstChildElement("CameraRigs");
    if (listNode == nullptr) {
        return;
    }
    for (tinyxml2::XMLElement *rigNode = listNode->FirstChildElement("CameraRig"); rigNode != nullptr;
         rigNode = rigNode->NextSiblingElement("CameraRig")) {
        rootTransformationToMeters(rigNode, "camera rig");
    }
}

void V1UnitScale::convertWorld(tinyxml2::XMLNode *worldNode) {
    convertObjects(worldNode);
    convertObjectGroups(worldNode);
    convertPlayer(worldNode);
    convertLights(worldNode);
    convertSounds(worldNode);
    convertEmitters(worldNode, "Emitters", "Emitter");
    convertEmitters(worldNode, "GPUEmitters", "GPUEmitter");
    convertTriggers(worldNode);
    convertCameraRigs(worldNode);
}

//translations are offsets from where the object stands, scales multiply and rotations have no unit
void V1UnitScale::convertAnimation(AnimationCustom &animation) {
    for (glm::vec3 &translate : animation.animationNode->translates) {
        translate /= CENTIMETERS_PER_INCH;
    }
}
