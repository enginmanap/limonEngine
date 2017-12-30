//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

#include <vector>
#include <SDL2/SDL_stdinc.h>
#include <btBulletDynamicsCommon.h>
#include <tinyxml2.h>


#include "PhysicalRenderable.h"
#include "GLHelper.h"
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "Camera.h"
#include "Model.h"
#include "SkyBox.h"
#include "BulletDebugDrawer.h"
#include "GUI/GUILayer.h"
#include "FontManager.h"
#include "GUI/GUIText.h"
#include "GUI/GUIFPSCounter.h"
#include "Light.h"
#include "GUI/GUITextDynamic.h"
#include "AI/Actor.h"

class World {
    Options* options;
    std::vector<PhysicalRenderable *> objects;
    std::vector<Light *> lights;
    std::vector<GUILayer *> guiLayers;
    std::vector<Actor*> actors;
    SkyBox *sky;
    GLHelper *glHelper;
    AssetManager *assetManager;
    long gameTime = 0;
    GUITextDynamic* trd;

    GLSLProgram *shadowMapProgramDirectional, *shadowMapProgramPoint;
    FontManager fontManager;
    Camera camera;
    BulletDebugDrawer *debugDrawer;

    btDiscreteDynamicsWorld *dynamicsWorld;
    std::vector<btRigidBody *> rigidBodies;


    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;

    bool loadMapFromXML();

    bool loadObjectsFromXML(tinyxml2::XMLNode *worldNode);

    bool loadSkymap(tinyxml2::XMLNode *worldNode);

    bool loadLights(tinyxml2::XMLNode *worldNode);

public:
    World(GLHelper *, Options *options);

    ~World();


    void play(Uint32, InputHandler &);

    void render();

    void handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInformation fillActorInformation(int j);
};

#endif //LIMONENGINE_WORLD_H
