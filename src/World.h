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

#include "InputHandler.h"
#include "FontManager.h"
#include "GameObjects/SkyBox.h"
#include "API/LimonAPI.h"
#include "API/ActorInterface.h"
#include "ALHelper.h"
#include "GameObjects/Players/Player.h"
#include "SDL2Helper.h"


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
class ModelGroup;

class QuadRenderBase;
class CombinePostProcess;
class SSAOPostProcess;
class SSAOBlurPostProcess;

class GLHelper;
class GraphicsPipelineStage;
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
        std::string extensionName;

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
        Renderable* object = nullptr;
        uint32_t animationIndex;
        bool loop;
        bool originChange = false;
        long startTime;
        Transformation originalTransformation;
        bool wasKinematic;
        bool wasPhysical = false;
        std::unique_ptr<Sound> sound;
    };

    struct ActionForOnload {
        TriggerInterface* action = nullptr;
        std::vector<LimonAPI::ParameterRequest> parameters;
        bool enabled = false;
    };

    enum collisiontypes {
        COLLIDE_NOTHING          = 0, //Collide with nothing
        COLLIDE_EVERYTHING       = 1 << 0, //Pick object etc needs to collide everything
        COLLIDE_MODELS           = 1 << 1, //Collide with All Models
        COLLIDE_PLAYER           = 1 << 2, //Collide with Player
        COLLIDE_AI               = 1 << 3, //Collide with Only AI driven Models
        COLLIDE_TRIGGER_VOLUME   = 1 << 4, //Collide with Triggers
        COLLIDE_STATIC_MODELS    = 1 << 5, //Collide with All Models
        COLLIDE_KINEMATIC_MODELS = 1 << 6, //Collide with All Models
        COLLIDE_DYNAMIC_MODELS   = 1 << 7, //Collide with All Models
        COLLIDE_END_ELEMENT      = -1
    };

    friend class WorldLoader;
    friend class WorldSaver; //Those classes require direct access to some of the internal data

    std::vector<uint32_t > modelIndicesBuffer;
    AssetManager* assetManager;
    Options* options;
    uint32_t nextWorldID = 2;
    std::queue<uint32_t> unusedIDs;
    std::map<uint32_t, PhysicalRenderable *> objects;
    std::set<uint32_t> disconnectedModels;
    std::map<uint32_t, ModelGroup*> modelGroups;

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

    std::map<uint32_t , std::set<Model*>> transparentModelsInCameraFrustum;

    /************************* End of redundant variables ******************************************/
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<TimedEvent>> timedEvents;


    std::map<uint32_t, GUIRenderable*> guiElements;
    std::map<uint32_t, TriggerObject*> triggers;
    std::vector<ActionForOnload* > onLoadActions;
    std::vector<AnimationCustom> loadedAnimations;
    std::set<Renderable*> onLoadAnimations;//Those renderables animations should be loaded and started on load
    std::unordered_map<Renderable*, AnimationStatus*> activeAnimations;
    std::unordered_map<uint32_t, std::unique_ptr<Sound>> sounds;
    AnimationSequenceInterface* animationInProgress = nullptr;
    std::vector<Light *> lights;
    int32_t directionalLightIndex = -1;
    glm::vec3 lastLightUpdatePlayerPosition = glm::vec3(0,0,0);
    std::vector<Light *> activeLights; //this contains redundant pointers at most MAX_LIGHT elements, from lights array.
    std::vector<GUILayer *> guiLayers;
    std::unordered_map<uint32_t, ActorInterface*> actors;
    AIMovementGrid *grid = nullptr;
    SkyBox *sky = nullptr;
    GLHelper *glHelper;
    ALHelper *alHelper;
    std::string name;
    std::string loadingImage;
    char worldSaveNameBuffer[256] = {0};
    char quitWorldNameBuffer[256] = {0};
    std::string quitWorldName;

    long gameTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

    GLSLProgram *shadowMapProgramDirectional = nullptr;
    GLSLProgram *shadowMapProgramPoint = nullptr;
    GLSLProgram *depthBufferProgram = nullptr;
    FontManager fontManager;

    PlayerInfo startingPlayer;
    char extensionNameBuffer[32] {};
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
    GameObject::ImGuiRequest* request = nullptr;

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
    uint32_t pickedObjectID = 0xFFFFFFFF;//FIXME not 0 because 0 is used by player and lights, they should get real ids.
    Model* objectToAttach = nullptr;
    CombinePostProcess* combiningObject;
    SSAOPostProcess* ssaoPostProcess;
    SSAOBlurPostProcess* ssaoBlurPostProcess;
    std::map<uint32_t, SDL2Helper::Thread*> routeThreads;

    bool guiPickMode = false;
    enum class QuitResponse
    {
        QUIT_GAME,
        RETURN_PREVIOUS,
        LOAD_WORLD
    };
    QuitResponse currentQuitResponse = QuitResponse::QUIT_GAME;
    GraphicsPipelineStage* ssaoBlurStage = nullptr;
    GraphicsPipelineStage* ssaoGenerationStage = nullptr;
    GraphicsPipelineStage* directionalShadowStage = nullptr;
    bool addPlayerAttachmentUsedIDs(const PhysicalRenderable *attachment, std::set<uint32_t> &usedIDs, uint32_t &maxID);

    /**
         * This method checks, if IDs assigned without any empty space, and any collision
         * and sets the totalObjectCount accordingly.
         * @return true if everything ok, false if not
         */
    bool verifyIDs();

    bool handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInterface::ActorInformation fillActorInformation(ActorInterface *actor);

    void updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);

    bool addModelToWorld(Model *xmlModel);
    bool addGUIElementToWorld(GUIRenderable *guiRenderable, GUILayer *guiLayer);

    void fillVisibleObjects();

    GameObject *getPointedObject(int collisionType, int filterMask,
                                 glm::vec3 *collisionPosition = nullptr, glm::vec3 *collisionNormal = nullptr) const;

    void addActor(ActorInterface *actor);

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

    Model* findModelByID(uint32_t modelID) const;
    Model* findModelByIDChildren(PhysicalRenderable* parent ,uint32_t modelID) const;

    std::vector<LimonAPI::ParameterRequest>
    fillRouteInformation(std::vector<LimonAPI::ParameterRequest> parameters) const;

    void renderPlayerAttachments(GameObject *attachment) const;
    void clearWorldRefsBeforeAttachment(PhysicalRenderable *attachment);

public:
    ~World();

    void play(Uint32, InputHandler &);

    void render();

    uint32_t getNextObjectID() {
        if(unusedIDs.size() > 0) {
            uint32_t id = unusedIDs.front();
            unusedIDs.pop();
            return id;
        }
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
    bool setModelTemporaryAPI(uint32_t modelID, bool temporary);

    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID);

    bool updateGuiText(uint32_t guiTextID, const std::string &newText);

    bool removeObject(uint32_t objectID);
    bool removeTriggerObject(uint32_t triggerobjectID);
    bool removeGuiElement(uint32_t guiElementID);

    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);

    bool detachSoundFromObject(uint32_t objectWorldID);

    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped);

    std::vector<LimonAPI::ParameterRequest> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    bool disconnectObjectFromPhysics(uint32_t objectWorldID);

    bool reconnectObjectToPhysics(uint32_t objectWorldID);

    bool disconnectObjectFromPhysicsRequest(uint32_t objectWorldID);

    bool reconnectObjectToPhysicsRequest(uint32_t objectWorldID);

    bool applyForceAPI(uint32_t objectID, const LimonAPI::Vec4 &forcePosition, const LimonAPI::Vec4 &forceAmount);

    bool applyForceToPlayerAPI(const LimonAPI::Vec4 &forceAmount);

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

    bool setObjectTranslateAPI(uint32_t objectID, const LimonAPI::Vec4& position);
    bool setObjectScaleAPI(uint32_t objectID, const LimonAPI::Vec4& scale);
    bool setObjectOrientationAPI(uint32_t objectID, const LimonAPI::Vec4& orientation);

    bool addObjectTranslateAPI(uint32_t objectID, const LimonAPI::Vec4& position);
    bool addObjectScaleAPI(uint32_t objectID, const LimonAPI::Vec4& scale);
    bool addObjectOrientationAPI(uint32_t objectID, const LimonAPI::Vec4& orientation);

    bool interactWithAIAPI(uint32_t AIID, std::vector<LimonAPI::ParameterRequest> &interactionInformation) const;

    void interactWithPlayerAPI(std::vector<LimonAPI::ParameterRequest> &interactionInformation) const;
    void simulateInputAPI(InputStates input);

    void addTimedEventAPI(long waitTime, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall,
                              std::vector<LimonAPI::ParameterRequest> parameters);

    uint32_t getPlayerAttachedModelAPI();
    std::vector<uint32_t> getModelChildrenAPI(uint32_t modelID);

    std::string getModelAnimationNameAPI(uint32_t modelID);
    bool getModelAnimationFinishedAPI(uint32_t modelID);
    bool setModelAnimationAPI(uint32_t modelID, const std::string& animationName, bool isLooped);
    bool setModelAnimationWithBlendAPI(uint32_t modelID, const std::string& animationName, bool isLooped, long blendTime);
    bool setModelAnimationSpeedAPI(uint32_t modelID, float speed);

    LimonAPI::Vec4 getPlayerModelOffsetAPI();
    bool setPlayerModelOffsetAPI(LimonAPI::Vec4 newOffset);
    void killPlayerAPI();

    bool addLightTranslateAPI(uint32_t lightID, const LimonAPI::Vec4& position);
    bool setLightColorAPI(uint32_t lightID, const LimonAPI::Vec4& color);

    /************************************ Methods LimonAPI exposes *************/
    void setupForPlay(InputHandler &inputHandler);

    void setupForPauseOrStop();

    void checkAndRunTimedEvents();

    void animateCustomAnimations();

    void buildTreeFromAllGameObjects();

    void createObjectTreeRecursive(PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID, int nodeFlags, int leafFlags,
                                       std::vector<uint32_t> parentage);

    void updateActiveLights(bool forceUpdate = false);

    void addSkyBoxControls();

    void
    removeActiveCustomAnimation(const AnimationCustom &animationToRemove, const AnimationStatus *animationStatusToRemove,
                                float animationTime);
};

#endif //LIMONENGINE_WORLD_H
