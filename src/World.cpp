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

    if(!loadMapFromXML()) {
        exit(-1);
    }

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
    if(sky!=NULL) {
        sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    }
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

bool World::loadMapFromXML() {

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile("./Data/Maps/World001.xml");
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "Error loading XML: " <<  eResult << std::endl;
    }

    tinyxml2::XMLNode * worldNode = xmlDoc.FirstChild();
    if (worldNode == NULL) {
        std::cerr << "World xml is not a valid XML." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* worldName =  worldNode->FirstChildElement("Name");
    if (worldName == NULL) {
        std::cerr << "World must have a name." << std::endl;
        return false;
    }
    std::cout << "read name as " << worldName->GetText() << std::endl;

    //load objects
    if(!loadObjectsFromXML(worldNode)) {
        return false;
    }
    //loadSkymap
    loadSkymap(worldNode);


    return true;
}

bool World::loadObjectsFromXML(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement* objectsListNode =  worldNode->FirstChildElement("Objects");
    if (objectsListNode == NULL) {
        std::cerr << "World Must have and Objects clause." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* objectNode =  objectsListNode->FirstChildElement("Object");
    if (objectNode == NULL) {
        std::cerr << "World Must have at least one object." << std::endl;
        return false;
    }
    Model *xmlModel;
    tinyxml2::XMLElement* objectAttribute;
    tinyxml2::XMLElement* objectAttributeAttribute;
    std::string modelFile;
    float modelMass;
    float x,y,z,w;
    while(objectNode != NULL) {
        objectAttribute =  objectNode->FirstChildElement("File");
        if (objectAttribute == NULL) {
            std::cerr << "Object must have a source file." << std::endl;
            return false;
        }
        modelFile = objectAttribute->GetText();
        objectAttribute =  objectNode->FirstChildElement("Mass");
        if (objectAttribute == NULL) {
            std::cout << "Object does not have mass, assume 0." << std::endl;
            modelMass = 0;
        } else {
            modelMass = std::stof(objectAttribute->GetText());
        }

        xmlModel = new Model(assetManager,modelMass, modelFile);

        objectAttribute =  objectNode->FirstChildElement("Scale");
        if (objectAttribute == NULL) {
            std::cout << "Object does not have scale." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != NULL) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != NULL) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != NULL) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 1.0;
            }
            xmlModel->addScale(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Translate");
        if (objectAttribute == NULL) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != NULL) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != NULL) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != NULL) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            xmlModel->addTranslate(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Rotate");
        if (objectAttribute == NULL) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != NULL) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != NULL) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != NULL) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("W");
            if(objectAttributeAttribute != NULL) {
                w = std::stof(objectAttributeAttribute->GetText());
            } else {
                w = 0.0;
            }
            xmlModel->addOrientation(glm::quat(x, y, z, w));
        }

        xmlModel->getWorldTransform();
        objects.push_back(xmlModel);
        rigidBodies.push_back(xmlModel->getRigidBody());
        dynamicsWorld->addRigidBody(xmlModel->getRigidBody());
        objectNode =  objectNode->NextSiblingElement("Object");
    }
    return true;
}

bool World::loadSkymap(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement* skyNode =  worldNode->FirstChildElement("Sky");
    if (skyNode == NULL) {
        std::cerr << "Sky clause not found." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* imagesPath =  skyNode->FirstChildElement("ImagesPath");
    if (imagesPath == NULL) {
        std::cerr << "Sky map must have the root path." << std::endl;
        return false;
    }

    std::string path, left, right, top, bottom, front, back;
    path = imagesPath->GetText();

    tinyxml2::XMLElement* leftNode =  skyNode->FirstChildElement("Left");
    if (leftNode == NULL) {
        std::cerr << "Sky map must have left image name." << std::endl;
        return false;
    }
    left = leftNode->GetText();

    tinyxml2::XMLElement* rightNode =  skyNode->FirstChildElement("Right");
    if (rightNode == NULL) {
        std::cerr << "Sky map must have right image name." << std::endl;
        return false;
    }
    right = rightNode->GetText();

    tinyxml2::XMLElement* topNode =  skyNode->FirstChildElement("Top");
    if (topNode == NULL) {
        std::cerr << "Sky map must have top image name." << std::endl;
        return false;
    }
    top = topNode->GetText();

    tinyxml2::XMLElement* bottomNode =  skyNode->FirstChildElement("Bottom");
    if (bottomNode == NULL) {
        std::cerr << "Sky map must have bottom image name." << std::endl;
        return false;
    }
    bottom = bottomNode->GetText();

    tinyxml2::XMLElement* backNode =  skyNode->FirstChildElement("Back");
    if (backNode == NULL) {
        std::cerr << "Sky map must have back image name." << std::endl;
        return false;
    }
    back = backNode->GetText();

    tinyxml2::XMLElement* frontNode =  skyNode->FirstChildElement("Front");
    if (frontNode == NULL) {
        std::cerr << "Sky map must have front image name." << std::endl;
        return false;
    }
    front = frontNode->GetText();

    sky = new SkyBox(assetManager,
                     std::string(path),
                     std::string(right),
                     std::string(left),
                     std::string(top),
                     std::string(bottom),
                     std::string(back),
                     std::string(front)
    );
}
