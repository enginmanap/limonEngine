//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H
#include "Utils/AABBConverter.hpp"

static const int SKIP_LOD_LEVEL = 9999;

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>
#include <Graphics/Particles/Emitter.h>
#include <Graphics/Particles/GPUParticleEmitter.h>

#include "InputHandler.h"
#include "FontManager.h"
#include "GameObjects/SkyBox.h"
#include "GameObjects/Light.h"
#include "GameObjects/Sound.h"
#include "limonAPI/LimonAPI.h"
#include "limonAPI/ActorInterface.h"
#include "ALHelper.h"
#include "GameObjects/Players/Player.h"
#include "SDL2Helper.h"
#include "Graphics/GraphicsPipeline.h"
#include "Attachable.h"
#include "PhysicalRenderable.h"
#include "VisibilityRequest.h"
#include "GameObjects/Model.h"
#include "Profiler/ProfilerSystem.h"

class Editor;
class btGhostPairCallback;
class PerspectiveCamera;
class BulletDebugDrawer;

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

class QuadRender;

class GraphicsInterface;
class GraphicsPipelineStage;
class TextureAsset;
class ALHelper;

class PipelineExtension;
class IterationExtension;
class NodeGraph;
class VisibilityManager;

/*
 * This is a workaround to access the timedEvent priority queue container.
 * https://stackoverflow.com/a/1385520
 */
template <class T, class S, class C>
S& Container(std::priority_queue<T, S, C>& q) {
struct HackedQueue : private std::priority_queue<T, S, C> {
        static S& Container(std::priority_queue<T, S, C>& q) {
            return q.*&HackedQueue::c;
        }
    };
    return HackedQueue::Container(q);
}

class WorldAPIAccessor;

class World {
    friend class Editor;
    friend class VisibilityManager;
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
        uint64_t callTime;
        long handleId;
        bool active = true;
        bool useWallTime = false;
        std::function<void(const std::vector<LimonTypes::GenericParameter>&)> methodToCall;
        std::vector<LimonTypes::GenericParameter> parameters;

        TimedEvent(long handleId, uint64_t callTime, bool useWallTime, std::function<void(const std::vector<LimonTypes::GenericParameter>&)> methodToCall,
                   std::vector<LimonTypes::GenericParameter> parameters) :
                callTime(callTime), handleId(handleId), useWallTime(useWallTime), methodToCall(std::move(methodToCall)), parameters(std::move(parameters)) {}

        bool operator>(const TimedEvent &timedEventRight) const {
            return callTime > timedEventRight.callTime;
        }
        void run() const {
            if(active && this->methodToCall != nullptr) {
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
        std::vector<LimonTypes::GenericParameter> parameters;
        bool enabled = false;
    };

    enum CollisionTypes {
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

    enum class ModelTypes { NON_ANIMATED_OPAQUE, ANIMATED, TRANSPARENT };

    friend class WorldLoader;
    friend class WorldSaver; //Those classes require direct access to some of the internal data
    friend class WorldAPIAccessor;

    mutable std::vector<uint32_t > modelIndicesBuffer;
    std::shared_ptr<AssetManager> assetManager;
    std::unique_ptr<Editor> editor;
    OptionsUtil::Options* options;
    ProfilerSystem* profilerSystem;
    uint32_t nextWorldID = 2;
    uint32_t nextRigID = 1;
    std::queue<uint32_t> unusedIDs;
    std::unordered_map<uint32_t, PhysicalRenderable *> objects;
    std::unordered_map<uint32_t, const std::vector<glm::mat4>*> changedBoneTransforms;//These are used for uploading to GPU. Don't put in if not passing culling.
    mutable std::unordered_set<uint32_t> tempRenderedObjectsSet;
    std::set<uint32_t> disconnectedModels;
    std::map<uint32_t, ModelGroup*> modelGroups;

    Sound* music = nullptr;

    OptionsUtil::Options::Option<bool> renderInformationsOption;
    OptionsUtil::Options::Option<long> maxLightsOption;
    std::vector<Model*> updatedModels;
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<>> timedEvents;
    long timedEventHandleIndex = 1;//we don't need to keep them, just have them unique


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
    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;
    std::string name;
    std::string loadingImage;
    std::string quitWorldName;
    ALHelper::DistanceModel soundDistanceModel = ALHelper::DistanceModel::LINEAR_CLAMPED;

    uint64_t gameTime = 0;
    uint64_t wallTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

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

    PerspectiveCamera* playerCamera;// This camera itself never changes, but the attachment does.
    //std::vector<Camera*> allCameras;//the info about all cameras is inferred by culling results, might need fixing.
    BulletDebugDrawer *debugDrawer;

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
    WorldAPIAccessor* apiAccessor = nullptr;

    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;

    std::shared_ptr<QuadRender> quadRender;
    std::map<uint32_t, SDL2MultiThreading::Thread*> routeThreads;

    bool guiPickMode = false;
    enum class QuitResponse
    {
        QUIT_GAME,
        RETURN_PREVIOUS,
        LOAD_WORLD
    };
    QuitResponse currentQuitResponse = QuitResponse::QUIT_GAME;
    std::shared_ptr<GraphicsPipeline> renderPipeline = nullptr;
    std::shared_ptr<GraphicsPipeline> renderPipelineBackup = nullptr;

    std::map<uint32_t, std::shared_ptr<Emitter>> emitters;
    std::map<uint32_t, std::shared_ptr<GPUParticleEmitter>> gpuParticleEmitters;
    std::unique_ptr<VisibilityManager> visibilityManager;

    static bool addPlayerAttachmentUsedIDs(const Attachable *attachment, std::set<uint32_t> &usedIDs, uint32_t &maxID);
    static Attachable* findAttachableInSubtree(Attachable *root, uint32_t objectID);

    /**
         * This method checks, if IDs assigned without any empty space, and any collision
         * and sets the totalObjectCount accordingly.
         * @return true if everything ok, false if not
         */
    bool verifyIDs();
    bool isIDUsed(uint32_t id) const;

    bool handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInterface::ActorInformation fillActorInformation(ActorInterface *actor);

    void updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);

    bool addModelToWorld(Model *xmlModel);
    bool addGUIElementToWorld(GUIRenderable *guiRenderable, GUILayer *guiLayer);

    void setPlayerAttachmentsForChangedBoneTransforms(Model *playerAttachment);

    btVector3 extendRayToWorldAABB(glm::vec3 from, glm::vec3 direction) const;
    GameObject* rayCastClosest(glm::vec3 from, glm::vec3 direction,int collisionType, int filterMask,
                                     glm::vec3 *collisionPosition = nullptr, glm::vec3 *collisionNormal = nullptr) const;
    GameObject* rayCastClosestOther(glm::vec3 from, glm::vec3 direction, int collisionType, int filterMask, const GameObject* ignoreObject,
                                     glm::vec3 *collisionPosition, glm::vec3 *collisionNormal) const;
    GameObject *getPointedObject(int collisionType, int filterMask,
                                 glm::vec3 *collisionPosition = nullptr, glm::vec3 *collisionNormal = nullptr) const;

    void addActor(ActorInterface *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);
    void addSound(Sound* sound);

    void setupRenderForPipeline() const;

    World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
          std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options *options, ProfilerSystem* profilerSystem);

    void afterLoadFinished();

    void switchPlayer(Player* targetPlayer, InputHandler &inputHandler);

    void setVisibilityAndPutToSets(PhysicalRenderable *PhysicalRenderable, bool removePossible);

    void setLightVisibilityAndPutToSets(size_t currentLightIndex, PhysicalRenderable *PhysicalRenderable, bool removePossible);

    bool handleQuitRequest();

    //API methods
    Model* findModelByID(uint32_t modelID) const;
    Model* findModelByIDChildren(PhysicalRenderable* parent ,uint32_t modelID) const;
    Attachable* findAttachableByID(uint32_t objectID) const;

    std::vector<LimonTypes::GenericParameter>
    fillRouteInformation(std::vector<LimonTypes::GenericParameter> parameters) const;

    void clearWorldRefsBeforeAttachment(PhysicalRenderable *attachment, bool removeChildren);
    void onModelMaterialChanged(uint32_t modelID);

    std::vector<size_t> getLightIndexes(Light::LightTypes lightType) const {
        std::vector<size_t> lights;
        for (unsigned int i = 0; i < activeLights.size(); ++i) {
            if(activeLights[i]->getLightType() != lightType) {
                continue;
            }
            lights.emplace_back(i);
        }
        return lights;
    }

    void ImGuiFrameSetup(std::shared_ptr<GraphicsProgram> graphicsProgram, const std::string& cameraName[[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]);
    void renderLight(unsigned int lightIndex, unsigned int renderLayer, const std::shared_ptr<GraphicsProgram> &renderProgram, const std::vector<HashUtil::HashedString> &tags) const;
    void renderParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;
    void renderGPUParticleEmitters(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;
    void renderGUIImages(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;
    void renderGUITexts(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;
    void renderSky(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;
    void renderDebug(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;

    void renderPlayerAttachmentsRecursiveByTag(PhysicalRenderable *attachment, uint64_t renderTag, const std::shared_ptr<GraphicsProgram> &renderProgram,
                                               std::vector<uint32_t> &alreadyRenderedModelIds) const;

    std::vector<std::shared_ptr<GraphicsProgram>> getAllAvailablePrograms();
    void getAllAvailableProgramsRecursive(const AssetManager::AvailableAssetsNode * currentNode, std::vector<std::shared_ptr<GraphicsProgram>> &programs);

    void renderCameraByTag(const std::shared_ptr<GraphicsProgram>& renderProgram, const std::string &cameraName, const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) const;

public:
    ~World();

    void play(Uint32, InputHandler &, uint64_t wallTime);

    void render();

    uint32_t getNextObjectID() {
        if(unusedIDs.size() > 0) {
            uint32_t id = unusedIDs.front();
            unusedIDs.pop();
            return id;
        }
        return nextWorldID++;
    }

    uint32_t getNextRigId() {
        return nextRigID++;
    }

    std::string getName();

    RenderMethods buildRenderMethods();

    void setupRender();

    /************************************ Methods LimonAPI exposes *************/
    void setupForPlay(InputHandler &inputHandler);

    void setupForPauseOrStop();

    void setupForUnpause();

    void checkAndRunTimedEvents();

    void animateCustomAnimations();


    void updateActiveLights(bool forceUpdate = false);


    void
    removeActiveCustomAnimation(const AnimationCustom &animationToRemove, const AnimationStatus *animationStatusToRemove,
                                float animationTime);

    static bool getNameOfTexture(void* data, int index, const char** outText) {
        auto& textures = *static_cast<std::vector<std::shared_ptr<Texture>>*>(data);
        if(index < 0 || (size_t)index >= textures.size()) {
            return false;
        }
        auto it = textures.begin();
        for (int i = 0; i < index; ++i) {
            it++;
        }

        *outText = it->get()->getName().c_str();
        return true;

    }

};

#endif //LIMONENGINE_WORLD_H
