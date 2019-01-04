//
// Created by engin on 17.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONLOADER_H
#define LIMONENGINE_ANIMATIONLOADER_H


#include "AnimationAssimp.h"

class AnimationCustom;

class AnimationLoader {
    static bool loadAnimationFromXML(const std::string &fileName, AnimationCustom *loadingAnimation);
    static bool loadNodesFromXML(tinyxml2::XMLNode *animationNode, AnimationCustom *loadingAnimation);

    static bool readTranslateAndTimes(tinyxml2::XMLElement *nodeNode, std::shared_ptr<AnimationNode> animationForNode);
    static bool readScaleAndTimes(tinyxml2::XMLElement *nodeNode, std::shared_ptr<AnimationNode> animationForNode);
    static bool readRotationAndTimes(tinyxml2::XMLElement *nodeNode, std::shared_ptr<AnimationNode> animationForNode);
public:
    static AnimationCustom* loadAnimation(const std::string& fileName);


};


#endif //LIMONENGINE_ANIMATIONLOADER_H
