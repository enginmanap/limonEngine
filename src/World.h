//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

#include <vector>
#include <tinyxml2.h>
#include <unordered_map>
#include <set>
#include <queue>

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
public:
    struct PlayerInfo {
        enum class Types {
            //ATTENTION if another type is added, typeNames must be updated
            PHYSICAL_PLAYER, DEBUG_PLAYER, EDITOR_PLAYER, MENU_PLAYER
        };

        static const std::map<Types, std::string> typeNames;

        Types type = Types::PHYSICAL_PLAYER;
        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 orientation = glm::vec3(0,0,-1);
        Model* attachedModel = nullptr;

        PlayerInfo() {
            position = glm::vec3(-15, 7,25);
            orientation = glm::vec3(1, 0, 0);

        }

        PlayerInfo(const std::string& name) {
            position = glm::vec3(-15, 7,25);
            orientation = glm::vec3(1, 0, 0);
            setType(name);
        }

        std::string typeToString() const {
            assert(typeNames.find(type) != typeNames.end());
            return typeNames.at(type);
        }

        bool setType(const std::string& name) {
            bool isSet = false;
            for (auto iterator = typeNames.begin(); iterator != typeNames.end(); ++iterator) {
                if(iterator->second == name) {
                    type = iterator->first;
                    isSet = true;
                }
            }
            if(!isSet) {
                std::cerr << "World starting player not match options, will default to Physical player." << std::endl;
            }
            return isSet;
        }

    };
private:

    struct TimedEvent {
        long callTime;
        std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall;
        std::vector<LimonAPI::ParameterRequest> parameters;

        TimedEvent(long callTime, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall,
                   std::vector<LimonAPI::ParameterRequest> parameters) :
        callTime(callTime), methodToCall(std::move(methodToCall)), parameters(std::move(parameters)) {}

        bool operator>(const TimedEvent &timedEventRight) const {
            return callTime > timedEventRight.callTime;
        }
        void run() const {
            if(this->methodToCall != nullptr) {
                this->methodToCall(parameters);
            } else {
                std::cerr << "Timed method call failed, because method is null" << std::endl;
            }
        }
    };

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
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<TimedEvent>> timedEvents;


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
    std::string name;
    char worldSaveNameBuffer[256] = {0};
    char quitWorldNameBuffer[256] = {0};
    std::string quitWorldName;

    long gameTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

    GLSLProgram *shadowMapProgramDirectional, *shadowMapProgramPoint;
    FontManager fontManager;

    PlayerInfo startingPlayer;
    PhysicalPlayer* physicalPlayer = nullptr;
    Model* playerPlaceHolder = nullptr;
    FreeCursorPlayer* editorPlayer = nullptr;
    FreeMovingPlayer* debugPlayer = nullptr;
    MenuPlayer* menuPlayer = nullptr;
    Player* currentPlayer = nullptr;
    Player* beforePlayer = nullptr;
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
    bool guiPickMode = false;
    enum class QuitResponse
    {
        QUIT_GAME,
        RETURN_PREVIOUS,
        LOAD_WORLD
    };
    QuitResponse currentQuitResponse = QuitResponse::QUIT_GAME;

    /**
     * This method checks, if IDs assigned without any empty space, and any collision
     * and sets the totalObjectCount accordingly.
     * @return true if everything ok, false if not
     */
    bool verifyIDs(){
        std::set<uint32_t > usedIDs;
        uint32_t maxID = 0;
        /** there are 3 places that has IDs,
         * 1) sky
         * 2) objects
         * 3) AIs
         */
        //put sky first, since it is guaranteed to be single
        if(this->sky != nullptr) {
            usedIDs.insert(this->sky->getWorldObjectID());
            maxID = this->sky->getWorldObjectID();
        }

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
                std::cerr << "world ID repetition on trigger detected! Actor with id " << actor->first << std::endl;
                return false;
            }
            maxID = std::max(maxID,actor->first);
        }

        for (auto guiElement = guiElements.begin(); guiElement != guiElements.end(); ++guiElement) {
            auto result = usedIDs.insert(guiElement->first);
            if(result.second == false) {
                std::cerr << "world ID repetition on trigger detected! gui element with id " << guiElement->first << std::endl;
                return false;
            }
            maxID = std::max(maxID, guiElement->first);
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

    bool addModelToWorld(Model *xmlModel);
    bool addGUIElementToWorld(GUIRenderable *guiRenderable, GUILayer *guiLayer);

    void fillVisibleObjects();

    GameObject *getPointedObject(int collisionType, int filterMask,
                                 glm::vec3 *collisionPosition = nullptr, glm::vec3 *collisionNormal = nullptr) const;

    void addActor(Actor *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);

    World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
              AssetManager *assetManager, Options *options);

    void afterLoadFinished();

    void switchPlayer(Player* targetPlayer, InputHandler &inputHandler);

    void ImGuiFrameSetup();

    void setVisibilityAndPutToSets(PhysicalRenderable *PhysicalRenderable, bool removePossible);

    void setLightVisibilityAndPutToSets(size_t currentLightIndex, PhysicalRenderable *PhysicalRenderable, bool removePossible);

    bool handleQuitRequest();

/********** Editor Methods *********************/
    void addGUITextControls();
    void addGUIImageControls();
    void addGUIButtonControls();
    void addGUIAnimationControls();
    void addGUILayerControls();
/********** Editor Methods *********************/
    //API methods

public:
    ~World();

    void play(Uint32, InputHandler &);

    void render();

    uint32_t getNextObjectID() {
        return nextWorldID++;
    }

    void addAnimationDefinitionToEditor();

    std::string getName();

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

    uint32_t addGuiImageAPI(const std::string &imageFilePath, const std::string &name,
                                   const LimonAPI::Vec2 &position, const LimonAPI::Vec2 &scale, float rotation);

    uint32_t addModelApi(const std::string &modelFilePath, float modelWeight, bool physical, const glm::vec3 &position,
                         const glm::vec3 &scale, const glm::quat &orientation);
    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID) {
        if(objectID == objectToAttachToID) {
            //can't attach to self
            return false;
        }

        Transformation* transform1,* transform2;
        //there is another possibility, that is the player attachment
        if(objects.find(objectID) == objects.end() ) {
            if(objectID != startingPlayer.attachedModel->getWorldObjectID()) {
                return false;
            } else {
                transform1 = startingPlayer.attachedModel->getTransformation();
            }
        } else {
            transform1 = objects[objectID]->getTransformation();
        }

        //there is another possibility, that is the player attachment
        if(objects.find(objectToAttachToID) == objects.end() ) {
            if(objectToAttachToID != startingPlayer.attachedModel->getWorldObjectID()) {
                return false;
            } else {
                transform2 = startingPlayer.attachedModel->getTransformation();
            }
        } else {
            transform2 = objects[objectToAttachToID]->getTransformation();
        }

        transform1->setParentTransform(transform2);
        return true;
    }


    bool updateGuiText(uint32_t guiTextID, const std::string &newText);

    bool removeObject(uint32_t objectID);
    bool removeTriggerObject(uint32_t triggerobjectID);
    uint32_t removeGuiText(uint32_t guiElementID);

    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);

    bool detachSoundFromObject(uint32_t objectWorldID);

    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool looped);

    std::vector<LimonAPI::ParameterRequest> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    bool disconnectObjectFromPhysics(uint32_t objectWorldID);

    bool reconnectObjectToPhysics(uint32_t objectWorldID);

    /**
     * If nothing is hit, returns empty vector
     * returns these values:
     * 1) objectID for what is under the cursor
     * 2) hit coordinates
     * 3) hit normal
     *
     */
    std::vector<LimonAPI::ParameterRequest> rayCastToCursorAPI();


    std::vector<LimonAPI::ParameterRequest> getObjectTransformationAPI(uint32_t objectID) const;

    std::vector<LimonAPI::ParameterRequest> getObjectTransformationMatrixAPI(uint32_t objectID) const;

    bool interactWithAIAPI(uint32_t AIID, std::vector<LimonAPI::ParameterRequest> &interactionInformation) const;

    void interactWithPlayerAPI(std::vector<LimonAPI::ParameterRequest> &interactionInformation) const;

    void addTimedEventAPI(long waitTime, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall,
                              std::vector<LimonAPI::ParameterRequest> parameters);

    uint32_t getPlayerAttachedModelAPI();
    /************************************ Methods LimonAPI exposes *************/
    void setupForPlay(InputHandler &inputHandler);

    void checkAndRunTimedEvents();
};

#endif //LIMONENGINE_WORLD_H
