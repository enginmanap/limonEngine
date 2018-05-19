//
// Created by engin on 13.05.2018.
//

#ifndef LIMONENGINE_LIMONAPI_H
#define LIMONENGINE_LIMONAPI_H

#include <vector>
#include <map>
#include <cstdint>


class Model;
class AnimationCustom;
class World;
class WorldLoader;
class PhysicalRenderable;



class LimonAPI {
    friend class WorldLoader;
    static World* world;

    static void setWorld(World* inputWorld);
public:
    static void animateModel(Model *model, const AnimationCustom *, bool looped);
    static const std::map<uint32_t, PhysicalRenderable *> & getObjects();
    static const std::vector<AnimationCustom> & getAnimations();
};


#endif //LIMONENGINE_LIMONAPI_H
