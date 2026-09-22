//
// Created by engin on 01/11/2023.
//

#ifndef LIMONENGINE_HARDCODEDTAGS_H
#define LIMONENGINE_HARDCODEDTAGS_H

#include <string>

/**
 * The tags the engine sets and the render pipeline knows by name. Games are free to add their own,
 * and to remove these from an object, so nothing may assume an object carries them.
 */

class HardCodedTags {
public:
    static const std::string OBJECT_MODEL_STATIC;
    static const std::string OBJECT_MODEL_PHYSICAL;//kinematics are also physical

    static const std::string OBJECT_MODEL_BASIC; //Non animated, non transparent
    static const std::string OBJECT_MODEL_ANIMATED;
    static const std::string OBJECT_MODEL_TRANSPARENT;
    static const std::string OBJECT_MODEL_AMBIENT;

    static const std::string OBJECT_PLAYER_BASIC;
    static const std::string OBJECT_PLAYER_ANIMATED;
    static const std::string OBJECT_PLAYER_TRANSPARENT;

    static const std::string CAMERA_LIGHT_DIRECTIONAL;
    static const std::string CAMERA_LIGHT_POINT;
    static const std::string CAMERA_PLAYER;

    static const std::string PICKED_OBJECT;

    static const std::vector<std::string> ALL_INTERNAL_TAGS;//all tags
};

#endif //LIMONENGINE_HARDCODEDTAGS_H
