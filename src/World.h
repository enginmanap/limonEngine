//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

#include <vector>
#include <tinyxml2.h>
#include <unordered_map>
#include <set>
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "FontManager.h"
#include "GameObjects/SkyBox.h"
#include "GamePlay/LimonAPI.h"
#include "AI/Actor.h"
#include "ALHelper.h"
#include "GameObjects/Players/Player.h"


class btGhostPairCallback;
class Camera;
class Model;
class BulletDebugDrawer;
class Light;
class AIMovementGrid;
class TriggerInterface;
class GUIText;

class GUIRenderable;
class GUILayer;
class GUITextBase;
class GUIFPSCounter;
class GUITextDynamic;
class GUICursor;
class GUIButton;


class GameObject;
class Player;
class PhysicalPlayer;
class FreeMovingPlayer;
class FreeCursorPlayer;
class MenuPlayer;
class ImGuiHelper;
class AssetManager;
class TriggerObject;
class Sound;
class AnimationCustom;
class AnimationNode;
class AnimationSequenceInterface;
class LimonAPI;

class GLHelper;
class ALHelper;

class World {

    struct AnimationStatus {
        PhysicalRenderable* object = nullptr;
        uint32_t animationIndex;
        bool loop;
        long startTime;
        Transformation originalTransformation;
        bool wasKinematic;
        std::unique_ptr<Sound> sound;
    };

    struct ActionForOnload {
        TriggerInterface* action = nullptr;
        std::vector<LimonAPI::ParameterRequest> parameters;
        bool enabled = false;
    };

    enum collisiontypes {
        COLLIDE_NOTHING         = 0, //Collide with nothing
        COLLIDE_EVERYTHING      = 1 << 0, //Pick object etc needs to collide everything
        COLLIDE_MODELS          = 1 << 1, //Collide with All Models
        COLLIDE_PLAYER          = 1 << 2, //Collide with Player
        COLLIDE_AI              = 1 << 3, //Collide with Only AI driven Models
        COLLIDE_TRIGGER_VOLUME  = 1 << 4, //Collide with Triggers

    };

    friend class WorldLoader;
    friend class WorldSaver; //Those classes require direct access to some of the internal data

    enum PlayerModes {DEBUG_MODE, EDITOR_MODE, PHYSICAL_MODE, PAUSED_MODE}; //PAUSED mode is used by quit logic

    std::vector<uint32_t > modelIndicesBuffer;
    AssetManager* assetManager;
    Options* options;
    uint32_t nextWorldID = 1;
    std::map<uint32_t, PhysicalRenderable *> objects;
    Sound* music = nullptr;

    /*
     * The variables below are redundant, but they allow instanced rendering, and saving frustum occlusion results.
     */
    std::vector<Model*> updatedModels;
    std::vector<std::map<uint32_t , std::set<Model*>>> modelsInLightFrustum;
    std::vector<std::set<Model*>> animatedModelsInLightFrustum; //since animated models can't be instanced, they don't need to be in a map etc.

    std::map<uint32_t , std::set<Model*>> modelsInCameraFrustum;
    std::set<Model*> animatedModelsInFrustum; //since animated models can't be instanced, they don't need to be in a map etc.
    std::set<Model*> animatedModelsInAnyFrustum;

    /************************* End of redundant variables ******************************************/

    std::map<uint32_t, GUIRenderable*> guiElements;
    std::map<uint32_t, TriggerObject*> triggers;
    std::vector<ActionForOnload* > onLoadActions;
    std::vector<AnimationCustom> loadedAnimations;
    std::set<PhysicalRenderable*> onLoadAnimations;//Those renderables animations should be loaded and started on load
    std::unordered_map<PhysicalRenderable*, AnimationStatus> activeAnimations;
    std::unordered_map<uint32_t, std::unique_ptr<Sound>> sounds;
    AnimationSequenceInterface* animationInProgress = nullptr;
    std::vector<Light *> lights;
    std::vector<GUILayer *> guiLayers;
    std::unordered_map<uint32_t, Actor*> actors;
    AIMovementGrid *grid = nullptr;
    SkyBox *sky = nullptr;
    GLHelper *glHelper;
    ALHelper *alHelper;
    long gameTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

    GLSLProgram *shadowMapProgramDirectional, *shadowMapProgramPoint;
    FontManager fontManager;
    PhysicalPlayer* physicalPlayer = nullptr;
    FreeCursorPlayer* editorPlayer = nullptr;
    FreeMovingPlayer* debugPlayer = nullptr;
    MenuPlayer* menuPlayer = nullptr;
    Player* currentPlayer;
    Player* beforePlayer;
    const Player::WorldSettings* currentPlayersSettings = nullptr;

    Camera* camera;
    BulletDebugDrawer *debugDrawer;
    GameObject::ImGuiRequest* request;

    GUILayer *apiGUILayer;
    GUIText* renderCounts;
    GUIFPSCounter* fpsCounter;
    GUICursor* cursor;
    GUIButton *hoveringButton = nullptr;
    GUITextDynamic* debugOutputGUI;

    btGhostPairCallback *ghostPairCallback;
    btDiscreteDynamicsWorld *dynamicsWorld;
    std::vector<btRigidBody *> rigidBodies;

    LimonAPI* apiInstance;

    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;
    ImGuiHelper *imgGuiHelper;
    GameObject* pickedObject = nullptr;
    bool availableAssetsLoaded = false;
    bool isQuitRequest = false;//does the player requested a quit?
    bool isQuitVerified = false;//does the player set it is sure?
    bool guiPickMode = false;

    /**
     * This method checks, if IDs assigned without any empty space, and any collision
     * and sets the totalObjectCount accordingly.
     * @return true if everything ok, false if not
     */
    bool verifyIDs(){
        std::set<uint32_t > usedIDs;
        uint32_t maxID;
        /** there are 3 places that has IDs,
         * 1) sky
         * 2) objects
         * 3) AIs
         */
        //put sky first, since it is guaranteed to be single
        usedIDs.insert(this->sky->getWorldObjectID());
        maxID = this->sky->getWorldObjectID();

        for(auto object = objects.begin(); object != objects.end(); object++) {
            auto result = usedIDs.insert(object->first);
            if(result.second == false) {
                std::cerr << "world ID repetition on object detected! with id " << object->first << std::endl;
                return false;
            }
            maxID = std::max(maxID,object->first);
        }

        for(auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
            auto result = usedIDs.insert(trigger->first);
            if(result.second == false) {
                std::cerr << "world ID repetition on trigger detected! with id " << trigger->first << std::endl;
                return false;
            }
            maxID = std::max(maxID,trigger->first);
        }

        for(auto actor = actors.begin(); actor != actors.end(); actor++) {
            auto result = usedIDs.insert(actor->first);
            if(result.second == false) {
                return false;
            }
            maxID = std::max(maxID,actor->first);
        }

        for(uint32_t index = 1; index <= maxID; index++) {
            if(usedIDs.count(index) != 1) {
                //TODO this should be ok, logging just to check. Can be removed in the future
                std::cout << "found empty ID" << index << std::endl;
            }
        }

        nextWorldID = maxID+1;
        return true;
    }

    bool handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInformation fillActorInformation(Actor *actor);

    void updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);

    void addModelToWorld(Model *xmlModel);

    void fillVisibleObjects();

    GameObject * getPointedObject() const;


    void addActor(Actor *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);

    World(AssetManager *assetManager, Options *options);

    void afterLoadFinished();

    void switchPlayer(Player* targetPlayer, InputHandler &inputHandler);

    void ImGuiFrameSetup();

    void setVisibilityAndPutToSets(PhysicalRenderable *PhysicalRenderable, bool removePossible);

    void setLightVisibilityAndPutToSets(size_t currentLightIndex, PhysicalRenderable *PhysicalRenderable, bool removePossible);

/********** Editor Methods *********************/
    void addGUITextControls();
    void addGUIImageControls();
    void addGUIButtonControls();
    void addGUILayerControls();
/********** Editor Methods *********************/
    //API methods

public:
    ~World();

    bool play(Uint32, InputHandler &);

    void render();

    uint32_t getNextObjectID() {
        return nextWorldID++;
    }

    void addAnimationDefinitionToEditor();

    /************************************ Methods LimonAPI exposes *************/
    /**
    * This method fills the parameters required to run the trigger
    * @param runParameters
    * @return true if all requied parameters are set, otherwise false
    */
    bool generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters, uint32_t index);

    uint32_t addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                           const std::string *soundToPlay);

    uint32_t addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad) {
        return addAnimationToObjectWithSound(modelID, animationID, looped, startOnLoad, nullptr);
    };

    uint32_t addGuiText(const std::string &fontFilePath, uint32_t fontSize,
                        const std::string &name, const std::string &text,
                        const glm::vec3 &color,
                        const glm::vec2 &position, float rotation);

    uint32_t updateGuiText(uint32_t guiTextID, const std::string &newText);

    uint32_t removeObject(uint32_t objectID);
    uint32_t removeTriggerObject(uint32_t triggerobjectID);
    uint32_t removeGuiText(uint32_t guiElementID);

    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);

    bool detachSoundFromObject(uint32_t objectWorldID);

    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool looped);

    std::vector<LimonAPI::ParameterRequest> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    bool disconnectObjectFromPhysics(uint32_t objectWorldID);

    bool reconnectObjectToPhysics(uint32_t objectWorldID);

    /************************************ Methods LimonAPI exposes *************/
};

#endif //LIMONENGINE_WORLD_H
