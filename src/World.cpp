//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "SkyBox.h"

World::World(GLHelper *glHelper): glHelper(glHelper), fontManager(glHelper) {

    // physics init
    broadphase = new btDbvtBroadphase();

    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    solver = new btSequentialImpulseConstraintSolver;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));
    debugDrawer = new BulletDebugDrawer(glHelper);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);

    // end of physics init

    rigidBodies.push_back(camera.getRigidBody());
    dynamicsWorld->addRigidBody(camera.getRigidBody());

    Model *crate = new Model(glHelper);
    crate->addScale(glm::vec3(250.0f,1.0f,250.0f));
    crate->addTranslate(glm::vec3(-125.0f, 0.0f, 125.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(glHelper,1);
    crate->addTranslate(glm::vec3(2.0f, 25.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(glHelper,1);
    crate->addTranslate(glm::vec3(-2.0f, 23.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(glHelper,1);
    crate->addTranslate(glm::vec3(2.0f, 23.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(glHelper,1);
    crate->addTranslate(glm::vec3(3.25f, 2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    sky = new SkyBox(glHelper,
                            std::string("D:user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterUp2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterDown2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterRight2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterLeft2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterBack2048.png"),
                            std::string("D:/user_files/engin/Documents/engin/UberGame/Data/Textures/Skyboxes/ThickCloudsWater/ThickCloudsWaterFront2048.png")
    );

    GUILayer* layer1 = new GUILayer(glHelper, 1);
    layer1->setDebug(true);
    GUIText* tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf",128), "Uber Game", glm::vec3(0,0,0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(512.0f,700.0f), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 32), "Version 0.1", glm::vec3(255,255,255));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(1024 - 50,100), -3.14f / 2);
    layer1->addGuiElement(tr);


    tr = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 32), "0", glm::vec3(204,204,0));
    tr->set2dWorldTransform(glm::vec2(1024 - 51,768 -38), 0);
    layer1->addGuiElement(tr);

    guiLayers.push_back(layer1);
}

void World::play(Uint32 simulationTimeFrame, InputHandler& inputHandler) {
    // Step simulation
    dynamicsWorld->stepSimulation(simulationTimeFrame/1000.0f);
    camera.updateTransfromFromPhysics(dynamicsWorld);
    objects[0]->updateTransformFromPhysics();
    objects[1]->updateTransformFromPhysics();
    objects[2]->updateTransformFromPhysics();
    objects[3]->updateTransformFromPhysics();
    objects[4]->updateTransformFromPhysics();
    //end of physics step

    btCollisionWorld::ClosestRayResultCallback RayCallback(btVector3(0,0,0), btVector3(0,25,-3));

// Perform raycast
    dynamicsWorld->rayTest(btVector3(0,20,0), btVector3(0,0,-3), RayCallback);

    if(RayCallback.hasHit()) {
        /*
        End = RayCallback.m_hitPointWorld;
        Normal = RayCallback.m_hitNormalWorld;
*/
        // Do some clever stuff here
    }

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
    if(inputHandler.getInputStatus(inputHandler.JUMP)){
        direction = camera.UP;
    }
    if (direction!= camera.NONE){
        camera.move(direction);
    }

}

void World::render() {
    glHelper->setCamera(camera.getCameraMatrix());
    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it)->render();
    }
    dynamicsWorld->debugDrawWorld();
    sky->render();

    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();
    for (std::vector<GUILayer*>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }
    //debugDrawer->drawLine(btVector3(0,0,-3),btVector3(0,250,-3),btVector3(1,1,1));
}

World::~World() {

    //FIXME clear GUIlayer elements
    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        delete (*it);
    }
    delete sky;
    for(std::vector<btRigidBody*>::iterator it = rigidBodies.begin(); it!= rigidBodies.end(); ++it){
        dynamicsWorld->removeRigidBody((*it));
    }

    delete dynamicsWorld;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;

}