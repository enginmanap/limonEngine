//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"

World::World(GLHelper *glHelper) {
    this->glHelper = glHelper;

    Model *star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, -3.0f)));
    objects.push_back(star);

    star = new Model(glHelper);
    star->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, -3.0f)));
    objects.push_back(star);
}

void World::play(Uint32 ticks, InputHandler& inputHandler) {
    float rotation = ticks / 5000.0f * 3.14f * 2;
    glm::mat4 transform;
    transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -3.0f));
    transform *= glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    objects[0]->setWorldTransform(transform);

    transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, -3.0f));
    transform *= glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, -1.0f, 0.0f));
    objects[1]->setWorldTransform(transform);

    float sinTic = fabs(sin(ticks / 1000.0f * 3.14));

    transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, -3.0f));
    transform *= glm::scale(glm::mat4(1.0f), glm::vec3(sinTic, sinTic, sinTic));
    objects[2]->setWorldTransform(transform);

    float cosTic = fabs(cos(ticks / 1000.0f * 3.14));

    transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, -3.0f));
    transform *= glm::scale(glm::mat4(1.0f), glm::vec3(cosTic, cosTic, cosTic));
    objects[3]->setWorldTransform(transform);

    float xLook, yLook;
    if(inputHandler.getMouseChange(xLook, yLook)){
        camera.rotate(xLook,yLook);
    }
    Camera::moveDirections direction = Camera::moveDirections::NONE;
    //ignore if both are pressed.
    if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) !=  inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD)) {
            direction = camera.FORWARD;
        } else {
            direction = camera.BACKWARD;
        }
    }
    if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT) != inputHandler.getInputStatus(inputHandler.MOVE_RIGHT)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT)) {
            if (direction == camera.FORWARD) {
                direction = camera.LEFT_FORWARD;
            } else if (direction == camera.BACKWARD){
                direction = camera.LEFT_BACKWARD;
            } else {
                direction = camera.LEFT;
            }
        } else
        if (direction == camera.FORWARD) {
            direction = camera.RIGHT_FORWARD;
        } else if (direction == camera.BACKWARD){
            direction = camera.RIGHT_BACKWARD;
        } else {
            direction = camera.RIGHT;
        }
    }
    if (direction!= camera.NONE){
        camera.move(direction);
    }

}

void World::render() {
    glHelper->setCamera(camera.getCameraMatrix());
    for (std::vector<Model *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it)->render(glHelper);
    }
}