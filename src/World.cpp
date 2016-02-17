//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"

World::World(GLHelper* glHelper){
    this->glHelper = glHelper;

    Model *star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f),glm::vec3(1.0f, 1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f),glm::vec3(-1.0f, -1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f),glm::vec3(1.0f, -1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f),glm::vec3(-1.0f, 1.0f, -3.0f)));
    objects.push_back(star);
}

void World::play(Uint32 ticks, InputHandler inputHandler){
    float rotation = ticks / 5000.0f * 3.14f * 2;
    glm::mat4 transform;
    transform = glm::translate(glm::mat4(1.0f),glm::vec3(1.0f, 1.0f, -3.0f));
    transform *= glm::rotate(glm::mat4(1.0f),rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    objects[0]->setWorldTransform(transform);

    transform = glm::translate(glm::mat4(1.0f),glm::vec3(-1.0f, -1.0f, -3.0f));
    transform *= glm::rotate(glm::mat4(1.0f),rotation, glm::vec3(0.0f, -1.0f, 0.0f));
    objects[1]->setWorldTransform(transform);

    float sinTic = fabs(sin(ticks/1000.0f * 3.14));

    transform = glm::translate(glm::mat4(1.0f),glm::vec3(1.0f, -1.0f, -3.0f));
    transform *= glm::scale(glm::mat4(1.0f),glm::vec3(sinTic, sinTic, sinTic));
    objects[2]->setWorldTransform(transform);

    float cosTic = fabs(cos(ticks/1000.0f * 3.14));

    transform = glm::translate(glm::mat4(1.0f),glm::vec3(-1.0f, 1.0f, -3.0f));
    transform *= glm::scale(glm::mat4(1.0f),glm::vec3(cosTic, cosTic, cosTic));
    objects[3]->setWorldTransform(transform);

    float xLook,yLook;
    inputHandler.getMousePosition(xLook, yLook);
    float zLook = sqrt(1 - (xLook * xLook + yLook * yLook));
    camera.setCenter(glm::vec3(xLook, yLook, -1 * zLook));
}

void World::render(){
    glHelper->setCamera(camera.getCameraMatrix());
    for(std::vector<Model*>::iterator it = objects.begin(); it != objects.end(); ++it){
        (*it)->render(glHelper);
    }
}