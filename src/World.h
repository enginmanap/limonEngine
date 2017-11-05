//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_WORLD_H
#define UBERGAME_WORLD_H

#include <vector>
#include "PhysicalRenderable.h"
#include "GLHelper.h"
#include <SDL2/SDL_stdinc.h>
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "Camera.h"
#include "Model.h"
#include "SkyBox.h"
#include "BulletDebugDrawer.h"
#include "GUILayer.h"
#include "FontManager.h"
#include <btBulletDynamicsCommon.h>
#include "GUIText.h"
#include "GUIFPSCounter.h"
#include "Light.h"

class World {
    std::vector<PhysicalRenderable *> objects;
    std::vector<Light *> lights;
    std::vector<GUILayer *> guiLayers;
    SkyBox *sky;
    GLHelper *glHelper;
    AssetManager *assetManager;

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

public:
    World(GLHelper *, Options &options);

    ~World();


    void play(Uint32, InputHandler &);

    void render();
};

#endif //UBERGAME_WORLD_H
