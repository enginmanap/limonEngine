//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"
#include "../GameObjects/Model.h"
#include "../World.h"
#include "AnimationAssimp.h"

World* LimonAPI::world;

void LimonAPI::setWorld(World *inputWorld) {
        world = inputWorld;
}

void LimonAPI::animateModel(Model *model,const AnimationCustom *animation, bool looped) {
    world->addAnimationToObject(model, animation, looped);
}

const std::map<uint32_t, PhysicalRenderable *> & LimonAPI::getObjects() {
    return world->objects;
}

const std::vector<AnimationCustom> & LimonAPI::getAnimations() {
    return world->loadedAnimations;
}
