//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"

World::World(GLHelper *glHelper, Options *options) : glHelper(glHelper), fontManager(glHelper), camera(options) {
    assetManager = new AssetManager(glHelper);
    // physics init
    broadphase = new btDbvtBroadphase();

    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    solver = new btSequentialImpulseConstraintSolver;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));
    debugDrawer = new BulletDebugDrawer(glHelper, options);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);

    GUILayer *layer1 = new GUILayer(glHelper, debugDrawer, 1);
    layer1->setDebug(false);
    // end of physics init

    rigidBodies.push_back(camera.getRigidBody());
    dynamicsWorld->addRigidBody(camera.getRigidBody());

    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Data/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Data/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Data/Shaders/ShadowMap/fragmentPoint.glsl", false);
    shadowMapProgramPoint->setUniform("farPlanePoint", options->getLightPerspectiveProjectionValues().z);

    Model *crate = new Model(assetManager, "./Data/Models/Box/Box.obj");
    crate->addScale(glm::vec3(250.0f, 1.0f, 250.0f));
    crate->addTranslate(glm::vec3(-125.0f, 0.0f, 125.0f));
    crate->addOrientation(glm::quat(0.0f, 0.0f, 1.0f, 0.2f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());


    crate = new Model(assetManager, 1, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(2.0f, 25.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());


    crate = new Model(assetManager, 1, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(-2.0f, 23.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(assetManager, 1, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(2.0f, 23.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(assetManager, 1, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(3.25f, 2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

/*
    Model *mario = new Model(assetManager, 100, "./Data/Models/Mario/Mario_obj.obj");
    mario->addTranslate(glm::vec3(5.0f, 23.0f, -3.0f));
    mario->addScale(glm::vec3(0.25f, 0.25f, 0.25f));
    mario->getWorldTransform();
    objects.push_back(mario);
    rigidBodies.push_back(mario->getRigidBody());
    dynamicsWorld->addRigidBody(mario->getRigidBody());
*/

    Model *armyPilot = new Model(assetManager, 25, "./Data/Models/ArmyPilot/ArmyPilot.dae");
    armyPilot->addTranslate(glm::vec3(10.0f, 10.0f, -3.0f));
    armyPilot->addScale(glm::vec3(2.0f,2.0f,2.0f));
    armyPilot->getWorldTransform();
    objects.push_back(armyPilot);
    rigidBodies.push_back(armyPilot->getRigidBody());
    dynamicsWorld->addRigidBody(armyPilot->getRigidBody());


    crate = new Model(assetManager, 50, "./Data/Models/testAnim/animatedBoxes.dae");
    crate->addTranslate(glm::vec3(10.0f, 10.5f, 15.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());

    crate = new Model(assetManager, 5, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(-3.0f, 2.5f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());


    crate = new Model(assetManager, 0, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(1.0f, 6.0f, 0.0f));
    crate->addScale(glm::vec3(0.2f,0.2f,0.2f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());


    Model *wall= new Model(assetManager, 0, "./Data/Models/Wall/archandwalls.FBX");
    wall->addTranslate(glm::vec3(10.0f, 0.0f, 10.0f));
    wall->addScale(glm::vec3(0.05f, 0.05f, 0.05f));
    wall->addOrientation(glm::quat(0,0.7f,0,0.7f));
    wall->getWorldTransform();
    objects.push_back(wall);
    rigidBodies.push_back(wall->getRigidBody());
    dynamicsWorld->addRigidBody(wall->getRigidBody());

/*
    Model *shanghai= new Model(assetManager, 0, "./Data/Models/shanghai/shanghai.obj");
    shanghai->addTranslate(glm::vec3(10.0f, 0.0f, 10.0f));
    shanghai->addScale(glm::vec3(0.02f, 0.02f, 0.02f));
    shanghai->getWorldTransform();
    objects.push_back(shanghai);
    rigidBodies.push_back(shanghai->getRigidBody());
    dynamicsWorld->addRigidBody(shanghai->getRigidBody());
*/

/*
    Model *militaryZone= new Model(assetManager, 0, "./Data/Models/MilitaryZone/MilitaryZone.obj");
    militaryZone->addTranslate(glm::vec3(-40.0f, 0.0f, 10.0f));
    militaryZone->addScale(glm::vec3(1.0f, 1.0f, 1.0f));
    //tavern->addOrientation(glm::quat(0,0.7f,0,0.7f));
    militaryZone->getWorldTransform();
    objects.push_back(militaryZone);
    rigidBodies.push_back(militaryZone->getRigidBody());
    dynamicsWorld->addRigidBody(militaryZone->getRigidBody());
*/

    Model *dwarf = new Model(assetManager, 10, "./Data/Models/Dwarf/dwarf.x");
    dwarf->addTranslate(glm::vec3(-3.0f, 6.5f, -3.0f));
    dwarf->addScale(glm::vec3(0.04f, 0.04f, 0.04f));
    dwarf->getWorldTransform();
    objects.push_back(dwarf);
    rigidBodies.push_back(dwarf->getRigidBody());
    dynamicsWorld->addRigidBody(dwarf->getRigidBody());

    sky = new SkyBox(assetManager,
                     std::string("./Data/Textures/Skyboxes/ThickCloudsWater"),
                     std::string("right.jpg"),
                     std::string("left.jpg"),
                     std::string("top.jpg"),
                     std::string("bottom.jpg"),
                     std::string("back.jpg"),
                     std::string("front.jpg")
    );

    GUIText *tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf", 64), "Uber Game",
                              glm::vec3(0, 0, 0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(512.0f, 740.0f), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "Version 0.2",
                     glm::vec3(255, 255, 255));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(1024 - 50, 100), 0.0f);
    layer1->addGuiElement(tr);


    tr = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                           glm::vec3(204, 204, 0));
    tr->set2dWorldTransform(glm::vec2(1024 - 50, 768 - 18), 0);
    layer1->addGuiElement(tr);

    guiLayers.push_back(layer1);

    Light *light;

    light = new Light(Light::POINT, glm::vec3(1.0f, 6.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    lights.push_back(light);
/*
    light = new Light(Light::POINT, glm::vec3(8.0f, 6.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    lights.push_back(light);
*/

    light = new Light(Light::DIRECTIONAL, glm::vec3(-25.0f, 50.0f, -25.0f), glm::vec3(0.7f, 0.7f, 0.7f));
    lights.push_back(light);

}


void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {
    //everytime we call this method, we increase the time only by simulationTimeframe
    gameTime += simulationTimeFrame;
    dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
    camera.updateTransfromFromPhysics(dynamicsWorld);

    for (int i = 0; i < objects.size(); ++i) {
        objects[i]->setupForTime(gameTime);
        objects[i]->updateTransformFromPhysics();
    }

    //end of physics step

    //FIXME this ray code looks like leftover from camera code seperation
    btCollisionWorld::ClosestRayResultCallback RayCallback(btVector3(0, 0, 0), btVector3(0, 25, -3));

// Perform raycast
    dynamicsWorld->rayTest(btVector3(0, 20, 0), btVector3(0, 0, -3), RayCallback);

    if (RayCallback.hasHit()) {
        /*
        End = RayCallback.m_hitPointWorld;
        Normal = RayCallback.m_hitNormalWorld;
*/
        // Do some clever stuff here
    }

    float xLook, yLook;
    if (inputHandler.getMouseChange(xLook, yLook)) {
        camera.rotate(xLook, yLook);
    }
    Camera::moveDirections direction = Camera::moveDirections::NONE;
    //ignore if both are pressed.
    if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) !=
        inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD)) {
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
            } else if (direction == camera.BACKWARD) {
                direction = camera.LEFT_BACKWARD;
            } else {
                direction = camera.LEFT;
            }
        } else if (direction == camera.FORWARD) {
            direction = camera.RIGHT_FORWARD;
        } else if (direction == camera.BACKWARD) {
            direction = camera.RIGHT_BACKWARD;
        } else {
            direction = camera.RIGHT;
        }
    }
    if (inputHandler.getInputStatus(inputHandler.JUMP)) {
        direction = camera.UP;
    }
    if (inputHandler.getInputStatus(inputHandler.DEBUG)) {
        if(dynamicsWorld->getDebugDrawer()->getDebugMode() == btIDebugDraw::DBG_NoDebug) {
            dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);
            guiLayers[0]->setDebug(true);
        } else {
            dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
            guiLayers[0]->setDebug(false);
        }
    }
    if (direction != camera.NONE) {
        camera.move(direction);
    }

}

void World::render() {
    for (int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::DIRECTIONAL) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapDirectional(i);
        shadowMapProgramDirectional->setUniform("renderLightIndex", i);
        for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            (*it)->renderWithProgram(*shadowMapProgramDirectional);
        }
    }

    for (int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::POINT) {
            continue;
        }
        //generate shadow map
        std::vector<glm::mat4> shadowTransforms = glHelper->switchRenderToShadowMapPoint(lights[i]->getPosition(), i);
        shadowMapProgramPoint->setUniformArray("shadowMatrices[0]", shadowTransforms);
        shadowMapProgramPoint->setUniform("renderLightIndex", i);
        //FarPlanePoint is set at declaration, since it is a constant
        shadowMapProgramPoint->setUniform("farPlanePoint", 100.0f);
        for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            (*it)->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    glHelper->switchrenderToDefault();
    sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    for (int i = 0; i < lights.size(); ++i) {
        glHelper->setLight(*(lights[i]), i);
    }


    if(camera.isDirty()) {
        glHelper->setPlayerMatrices(camera.getPosition(), camera.getCameraMatrix());
    }


    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it)->render();
    }

    dynamicsWorld->debugDrawWorld();
    //debugDrawer->drawLine(btVector3(0,0,-3),btVector3(0,250,-3),btVector3(1,1,1));

    debugDrawer->flushDraws();



    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();

    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }


}

World::~World() {

    //FIXME clear GUIlayer elements
    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        delete (*it);
    }
    delete sky;
    for (std::vector<btRigidBody *>::iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it) {
        dynamicsWorld->removeRigidBody((*it));
    }

    for (std::vector<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        delete (*it);
    }

    delete debugDrawer;
    delete dynamicsWorld;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;

}