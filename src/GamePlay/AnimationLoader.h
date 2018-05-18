//
// Created by engin on 17.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONLOADER_H
#define LIMONENGINE_ANIMATIONLOADER_H


#include "Animation.h"

class Animation;

class AnimationLoader {
    static bool loadAnimationFromXML(const std::string &fileName, Animation *loadingAnimation);
    static bool loadNodesFromXML(tinyxml2::XMLNode *animationNode, Animation *loadingAnimation);

    static bool readTranslateAndTimes(tinyxml2::XMLElement *nodeNode, AnimationNode *animationForNode);
    static bool readScaleAndTimes(tinyxml2::XMLElement *nodeNode, AnimationNode *animationForNode);
    static bool readRotationAndTimes(tinyxml2::XMLElement *nodeNode, AnimationNode *animationForNode);
public:
    static Animation* loadAnimation(const std::string& fileName);


};


#endif //LIMONENGINE_ANIMATIONLOADER_H
