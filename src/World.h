//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

#include <vector>
#include <tinyxml2.h>
#include <unordered_map>
#include "PhysicalRenderable.h"
#include "GLHelper.h"
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "FontManager.h"
#include "AI/Actor.h"
#include "GameObjects/SkyBox.h"
#include "GamePlay/LimonAPI.h"

class btGhostPairCallback;
class Camera;
class Model;
class BulletDebugDrawer;
class Light;
class AIMovementGrid;

class GUILayer;
class GUIText;
class GUIFPSCounter;
class GUITextDynamic;

class GameObject;
class Player;
class PhysicalPlayer;
class FreeMovingPlayer;
class FreeCursorPlayer;
class ImGuiHelper;
class AssetManager;
class TriggerObject;
class AnimationCustom;
class AnimationNode;

class World {

    struct AnimationStatus {
        Model* model = nullptr;
        const AnimationCustom *animation;
        bool loop;
        long startTime;
        Transformation originalTransformation;
        bool wasKinematic;

        AnimationNode* animationNode;
    };

    friend class WorldLoader;
    friend class WorldSaver; //Those classes require direct access to some of the internal data

    enum PlayerModes {DEBUG_MODE, EDITOR_MODE, PHYSICAL_MODE, PAUSED_MODE}; //PAUSED mode is used by quit logic
    AssetManager* assetManager;
    Options* options;
    uint32_t totalObjectCount = 1;
    std::map<uint32_t, PhysicalRenderable *> objects;
    std::map<uint32_t, TriggerObject*> triggers;
    std::vector<AnimationCustom> loadedAnimations;
    std::unordered_map<Model*, AnimationStatus> activeAnimations;
    AnimationStatus* animationInProgress = nullptr;
    std::vector<Light *> lights;
    std::vector<GUILayer *> guiLayers;
    std::unordered_map<uint32_t, Actor*> actors;
    AIMovementGrid *grid = nullptr;
    SkyBox *sky = nullptr;
    GLHelper *glHelper;
    long gameTime = 0;
    GUITextDynamic* trd;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

    GLSLProgram *shadowMapProgramDirectional, *shadowMapProgramPoint;
    FontManager fontManager;
    PhysicalPlayer* physicalPlayer;
    FreeCursorPlayer* editorPlayer = nullptr;
    FreeMovingPlayer* debugPlayer = nullptr;
    Player* currentPlayer;
    Camera* camera;
    BulletDebugDrawer *debugDrawer;
    GUIText *cursor;
    btGhostPairCallback *ghostPairCallback;
    btDiscreteDynamicsWorld *dynamicsWorld;
    std::vector<btRigidBody *> rigidBodies;


    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;
    PlayerModes currentMode = PHYSICAL_MODE;
    PlayerModes beforeMode = PHYSICAL_MODE;
    ImGuiHelper *imgGuiHelper;
    GameObject* pickedObject = nullptr;
    bool availableAssetsLoaded = false;
    bool isQuitRequest = false;//does the player requested a quit?
    bool isQuitVerified = false;//does the player set it is sure?


    bool handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInformation fillActorInformation(Actor *actor);

    void updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);

    void addModelToWorld(Model *xmlModel);

    GameObject * getPointedObject() const;


    void addActor(Actor *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);

    World(AssetManager *assetManager, GLHelper *, Options *options);

    void switchToEditorMode(InputHandler &inputHandler);

    void switchToPhysicalPlayer(InputHandler &inputHandler);

    void switchToDebugMode(InputHandler &inputHandler);

    void ImGuiFrameSetup();

    //API methods

    friend const std::map<uint32_t, PhysicalRenderable *> & LimonAPI::getObjects();
    friend const std::vector<AnimationCustom> & LimonAPI::getAnimations();
public:
    ~World();

    bool play(Uint32, InputHandler &);

    void render();

    uint32_t getNextObjectID() {
        return totalObjectCount++;
    }

    void addAnimationToObject(Model *model, const AnimationCustom *animation, bool looped);
};

#endif //LIMONENGINE_WORLD_H
