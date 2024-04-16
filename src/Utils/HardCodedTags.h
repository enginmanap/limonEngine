//
// Created by engin on 01/11/2023.
//

#ifndef LIMONENGINE_HARDCODEDTAGS_H
#define LIMONENGINE_HARDCODEDTAGS_H

#include "string"

/**
 * This class is here because we don't allow adding tags yet.
 * After that is introduced, it might be removed.
 */

class HardCodedTags {
public:
    static const std::string OBJECT_MODEL_STATIC;
    static const std::string OBJECT_MODEL_PHYSICAL;//kinematics are also physical

    static const std::string OBJECT_MODEL_BASIC; //Non animated, non transparent
    static const std::string OBJECT_MODEL_ANIMATED;
    static const std::string OBJECT_MODEL_TRANSPARENT;

    static const std::string CAMERA_LIGHT_DIRECTIONAL;
    static const std::string CAMERA_LIGHT_POINT;
    static const std::string CAMERA_PLAYER;
};

#endif //LIMONENGINE_HARDCODEDTAGS_H
