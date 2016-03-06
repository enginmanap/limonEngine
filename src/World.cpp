//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "SkyBox.h"

World::World(GLHelper *glHelper) {
    this->glHelper = glHelper;

    Model *crate = new Model(glHelper);
    //crate->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, -3.0f)));
    crate->addTranslate(glm::vec3(2.0f, 2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);

    crate = new Model(glHelper);
    //crate->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, -2.0f, -3.0f)));
    crate->addTranslate(glm::vec3(-2.0f, -2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);

    crate = new Model(glHelper);
    //crate->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -2.0f, -3.0f)));
    crate->addTranslate(glm::vec3(2.0f, -2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);

    crate = new Model(glHelper);
    //crate->setWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 2.0f, -3.0f)));
    crate->addTranslate(glm::vec3(-2.0f, 2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);

    sky= new SkyBox(glHelper,
                            std::string("D:user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterUp2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterDown2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterRight2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterLeft2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterBack2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterFront2048.png")
    );

}

void World::play(Uint32 simulationTimeFrame, InputHandler& inputHandler) {
    float rotationAngle = (simulationTimeFrame / 1000.0f) * (3.14 / 10);
    glm::quat rotationQuat(cos(rotationAngle/2), 0.0f, sin(rotationAngle/2), 0.0f);
    objects[0]->addOrientation(rotationQuat);

    objects[1]->addOrientation(glm::conjugate(rotationQuat));

    float sinTic = fabs(sin(simulationTimeFrame / 1000.0f * 3.14)) + 1.0f;
    objects[2]->addScale(glm::vec3(sinTic, sinTic, sinTic));

    float cosTic = fabs(cos(simulationTimeFrame / 1000.0f * 3.14)) + 1.0f;
    objects[3]->addScale(glm::vec3(cosTic, cosTic, cosTic));

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
    for (std::vector<Renderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it)->render();
    }
    sky->render();

}