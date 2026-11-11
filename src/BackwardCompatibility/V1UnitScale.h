//
// Created by Engin Manap on 10/11/2026.
//

#ifndef LIMONENGINE_V1UNITSCALE_H
#define LIMONENGINE_V1UNITSCALE_H

#include <tinyxml2.h>

//save version 1 worlds are 2.54 times real size. Trigger, actor, rig and extension parameters are not converted, nothing says which are distances
class V1UnitScale {
    static void toMeters(tinyxml2::XMLElement *element);//the X, Y and Z children, or the element's own text for a single number
    static void childToMeters(tinyxml2::XMLElement *owner, const char *childName);
    //Translate and Scale of the owner's Transformation, unless it is local to a parent
    static void rootTransformationToMeters(tinyxml2::XMLElement *owner, const char *kind);
    static bool isParented(tinyxml2::XMLElement *owner, const char *kind);
    static void attenuationToMeters(tinyxml2::XMLElement *lightNode);

    static void convertObjects(tinyxml2::XMLNode *worldNode);
    static void convertObjectGroups(tinyxml2::XMLNode *worldNode);
    static void convertPlayer(tinyxml2::XMLNode *worldNode);
    static void convertLights(tinyxml2::XMLNode *worldNode);
    static void convertSounds(tinyxml2::XMLNode *worldNode);
    static void convertEmitters(tinyxml2::XMLNode *worldNode, const char *listName, const char *emitterName);
    static void convertTriggers(tinyxml2::XMLNode *worldNode);
    static void convertCameraRigs(tinyxml2::XMLNode *worldNode);

public:
    //custom animations stay as they are, they play in the object's local space and its converted scale already shrinks them
    static void convertWorld(tinyxml2::XMLNode *worldNode);
};

#endif //LIMONENGINE_V1UNITSCALE_H
