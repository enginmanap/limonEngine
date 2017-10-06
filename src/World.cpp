//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"

World::World(GLHelper *glHelper) : glHelper(glHelper), fontManager(glHelper) {
    assetManager = new AssetManager(glHelper);
    // physics init
    broadphase = new btDbvtBroadphase();

    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    solver = new btSequentialImpulseConstraintSolver;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));
    debugDrawer = new BulletDebugDrawer(glHelper);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);

    GUILayer *layer1 = new GUILayer(glHelper, 1);
    layer1->setDebug(false);
    // end of physics init

    rigidBodies.push_back(camera.getRigidBody());
    dynamicsWorld->addRigidBody(camera.getRigidBody());

    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Data/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Data/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Data/Shaders/ShadowMap/fragmentPoint.glsl", false);
    shadowMapProgramPoint->setUniform("farPlanePoint", 100.0f);
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


    Model *mario = new Model(assetManager, 100, "./Data/Models/Mario/Mario_obj.obj");
    mario->addTranslate(glm::vec3(5.0f, 23.0f, -3.0f));
    mario->addScale(glm::vec3(0.25f, 0.25f, 0.25f));

    mario->getWorldTransform();
    objects.push_back(mario);
    rigidBodies.push_back(mario->getRigidBody());
    dynamicsWorld->addRigidBody(mario->getRigidBody());


    crate = new Model(assetManager, 1, "./Data/Models/Box/Box.obj");
    crate->addTranslate(glm::vec3(-3.0f, 2.0f, -3.0f));
    crate->getWorldTransform();
    objects.push_back(crate);
    rigidBodies.push_back(crate->getRigidBody());
    dynamicsWorld->addRigidBody(crate->getRigidBody());


    Model *dwarf = new Model(assetManager, 10, "./Data/Models/Dwarf/dwarf.x");
    dwarf->addTranslate(glm::vec3(-3.0f, 5.0f, -3.0f));
    dwarf->addScale(glm::vec3(0.04f, 0.04f, 0.04f));
    dwarf->getWorldTransform();
    objects.push_back(dwarf);
    rigidBodies.push_back(dwarf->getRigidBody());
    dynamicsWorld->addRigidBody(dwarf->getRigidBody());

/*
    Model *animatedBoxes = new Model(assetManager, 1, "./Data/Models/testAnim/animatedBoxes.dae");
    animatedBoxes->addTranslate(glm::vec3(10.0f, 29.0f, -3.0f));
    animatedBoxes->getWorldTransform();
    objects.push_back(animatedBoxes);
    rigidBodies.push_back(animatedBoxes->getRigidBody());
    dynamicsWorld->addRigidBody(animatedBoxes->getRigidBody());
*/
    sky = new SkyBox(assetManager,
                     std::string("./Data/Textures/Skyboxes/ThickCloudsWater"),
                     std::string("right.jpg"),
                     std::string("left.jpg"),
                     std::string("top.jpg"),
                     std::string("bottom.jpg"),
                     std::string("back.jpg"),
                     std::string("front.jpg")
    );

    GUIText *tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf", 128), "Uber Game",
                              glm::vec3(0, 0, 0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(512.0f, 700.0f), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 32), "Version 0.1",
                     glm::vec3(255, 255, 255));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(1024 - 50, 100), -3.14f / 2);
    layer1->addGuiElement(tr);


    tr = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 32), "0",
                           glm::vec3(204, 204, 0));
    tr->set2dWorldTransform(glm::vec2(1024 - 100, 768 - 38), 0);
    layer1->addGuiElement(tr);

    guiLayers.push_back(layer1);

    Light *light;

    light = new Light(Light::POINT, glm::vec3(5.0f, 10.0f, 10.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    lights.push_back(light);

    light = new Light(Light::DIRECTIONAL, glm::vec3(-25.0f, 50.0f, 25.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    lights.push_back(light);


}


void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {
    // Step simulation
    long time = SDL_GetTicks();
    dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
    camera.updateTransfromFromPhysics(dynamicsWorld);

    for (int i = 0; i < objects.size(); ++i) {
        objects[i]->setupForTime(time);
        objects[i]->updateTransformFromPhysics();
    }

    //end of physics step

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
        } else {
            dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
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
        std::vector<glm::mat4> shadowTransforms = glHelper->switchRenderToShadowMapPoint(i,lights[i]->getPosition());
        shadowMapProgramPoint->setUniformArray("shadowMatrices[0]", shadowTransforms);
        shadowMapProgramPoint->setUniform("renderLightIndex", i);
        //FarPlanePoint is set at declaration, since it is a constant
        shadowMapProgramPoint->setUniform("farPlanePoint", 100.0f);
        shadowMapProgramPoint->setUniform("lightPosition", lights[i]->getPosition());
        for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            (*it)->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    glHelper->switchrenderToDefault();

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
    sky->render();

    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();
    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
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
    for (std::vector<btRigidBody *>::iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it) {
        dynamicsWorld->removeRigidBody((*it));
    }

    for (std::vector<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        delete (*it);
    }

    delete dynamicsWorld;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;

}