//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

static const int SKIP_LOD_LEVEL = 9999;

#include <vector>
#include <tinyxml2.h>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>
#include <Graphics/Particles/Emitter.h>
#include <Graphics/Particles/GPUParticleEmitter.h>
#include <atomic>

#include "InputHandler.h"
#include "FontManager.h"
#include "GameObjects/SkyBox.h"
#include "GameObjects/Light.h"
#include "API/LimonAPI.h"
#include "API/ActorInterface.h"
#include "ALHelper.h"
#include "GameObjects/Players/Player.h"
#include "SDL2Helper.h"
#include "Graphics/GraphicsPipeline.h"
#include "PhysicalRenderable.h"
#include "VisibilityRequest.h"
class btGhostPairCallback;
class PerspectiveCamera;
#include "GameObjects/Model.h"
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

class World {
    friend class Editor;
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

    mutable std::vector<uint32_t > modelIndicesBuffer;
    std::shared_ptr<AssetManager> assetManager;
    Options* options;
    uint32_t nextWorldID = 2;
    std::queue<uint32_t> unusedIDs;
    std::unordered_map<uint32_t, PhysicalRenderable *> objects;
    mutable std::unordered_set<uint32_t> tempRenderedObjectsSet;
    std::set<uint32_t> disconnectedModels;
    std::map<uint32_t, ModelGroup*> modelGroups;

    Sound* music = nullptr;

    /*
     * The variables below are redundant, but they allow instanced rendering, and saving frustum occlusion results.
     */
    struct ModelWithLod{
        Model* model;
        uint32_t lod = 3;
        bool operator<(const ModelWithLod& rhs) const
        {
            return model < rhs.model;
        }
        ModelWithLod(Model* model) : model(model), lod(SKIP_LOD_LEVEL) {}//intentionally not explicit
        ModelWithLod(Model* model, uint32_t lod) : model(model), lod(lod) {}
    };
    std::vector<Model*> updatedModels;
    // This map is also used as a list of Cameras, and Hashes, so if a camera is removed, it should be removed from this map
    // In case of a clear, we should not clear the hashes, as it is basically meaningless.

    /**
     * this variable is used as camera list, so outermost one should never clear, and change only on camera creation/deletion.
     * Since hashes are set by the render pipeline, first level in might reset on render pipeline changes.
     * On all other cases, only the value of hash list should be reset.
     * What is in this map?
     * For each camera
     *      there is a list of tags, that is list of tags per render stage, so any match is enough
     *      for each list of tags, there is a map, key is the asset id
     *          for each asset Id, there is a pair of vector + uint
     *              vector is the object Ids that needs rendering, uint is the Max LOD to use.
     */

    std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>, VisibilityRequest::uint64_vector_hasher>*> cullingResults;
    //std::unordered_map<Camera*, std::unordered_map<uint64_t, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>>*> cullingResults;

    /************************* End of redundant variables ******************************************/
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
    char worldSaveNameBuffer[256] = {0};
    char quitWorldNameBuffer[256] = {0};
    std::string quitWorldName;

    uint64_t gameTime = 0;
    uint64_t wallTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

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

    PerspectiveCamera* playerCamera;// This camera itself never changes, but the attachment does.
    //std::vector<Camera*> allCameras;//the info about all cameras is inferred by culling results, might need fixing.
    BulletDebugDrawer *debugDrawer;
    ImGuiRequest* request = nullptr;

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
    bool showNodeGraph = false;
    PipelineExtension *pipelineExtension;
    IterationExtension *iterationExtension;
    NodeGraph* nodeGraph = nullptr;
    std::shared_ptr<GraphicsPipeline> renderPipeline = nullptr;
    std::shared_ptr<GraphicsPipeline> renderPipelineBackup = nullptr;

    std::map<uint32_t, std::shared_ptr<Emitter>> emitters;
    std::map<uint32_t, std::shared_ptr<GPUParticleEmitter>> gpuParticleEmitters;
    bool multiThreadedCulling = true;

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

    void resetVisibilityBufferForRenderPipelineChange();
    void resetCameraTagsFromPipeline(const std::map<std::string, std::vector<std::set<std::string>>> &cameraRenderTagListMap);
    void fillVisibleObjectsUsingTags();
    std::map<VisibilityRequest*, SDL_Thread *> occlusionThreadManager();
    std::map<VisibilityRequest*, SDL_Thread *> visibilityThreadPool;

    GameObject *getPointedObject(int collisionType, int filterMask,
                                 glm::vec3 *collisionPosition = nullptr, glm::vec3 *collisionNormal = nullptr) const;

    void addActor(ActorInterface *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);

    World(const std::string &name, PlayerInfo startingPlayerType, InputHandler *inputHandler,
          std::shared_ptr<AssetManager> assetManager, Options *options);

    void afterLoadFinished();

    void switchPlayer(Player* targetPlayer, InputHandler &inputHandler);

    void setVisibilityAndPutToSets(PhysicalRenderable *PhysicalRenderable, bool removePossible);

    void setLightVisibilityAndPutToSets(size_t currentLightIndex, PhysicalRenderable *PhysicalRenderable, bool removePossible);

    bool handleQuitRequest();

/********** Editor Methods *********************/
    void addGUITextControls();
    void addGUIImageControls();
    void addGUIButtonControls();
    void addGUIAnimationControls();
    void addGUILayerControls();
    void addParticleEmitterEditor();
/********** Editor Methods *********************/
    void drawNodeEditor();

    //API methods
    Model* findModelByID(uint32_t modelID) const;
    Model* findModelByIDChildren(PhysicalRenderable* parent ,uint32_t modelID) const;

    std::vector<LimonTypes::GenericParameter>
    fillRouteInformation(std::vector<LimonTypes::GenericParameter> parameters) const;

    void clearWorldRefsBeforeAttachment(PhysicalRenderable *attachment);

    void createNodeGraph();

    std::vector<size_t> getLightIndexes(Light::LightTypes lightType) {
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
    void renderLight(unsigned int lightIndex, unsigned int renderLayer, const std::shared_ptr<GraphicsProgram> &renderProgram) const;
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

    std::string getName();

    RenderMethods buildRenderMethods();

        /************************************ Methods LimonAPI exposes *************/
    /**
    * This method fills the parameters required to run the trigger
    * @param runParameters
    * @return true if all requied parameters are set, otherwise false
    */
    bool generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index);

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
                                   const LimonTypes::Vec2 &position, const LimonTypes::Vec2 &scale, float rotation);

    uint32_t addModelApi(const std::string &modelFilePath, float modelWeight, bool physical, const glm::vec3 &position,
                         const glm::vec3 &scale, const glm::quat &orientation);
    bool setModelTemporaryAPI(uint32_t modelID, bool temporary);

    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID);

    bool updateGuiText(uint32_t guiTextID, const std::string &newText);

    bool removeObject(uint32_t objectID, const bool &removeChildren = true);
    bool removeTriggerObject(uint32_t triggerobjectID);
    bool removeGuiElement(uint32_t guiElementID);

    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);

    bool detachSoundFromObject(uint32_t objectWorldID);

    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped);

    std::vector<LimonTypes::GenericParameter> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    bool disconnectObjectFromPhysics(uint32_t objectWorldID);

    bool reconnectObjectToPhysics(uint32_t objectWorldID);

    bool disconnectObjectFromPhysicsRequest(uint32_t objectWorldID);

    bool reconnectObjectToPhysicsRequest(uint32_t objectWorldID);

    bool applyForceAPI(uint32_t objectID, const LimonTypes::Vec4 &forcePosition, const LimonTypes::Vec4 &forceAmount);

    bool applyForceToPlayerAPI(const LimonTypes::Vec4 &forceAmount);

    bool changeRenderPipeline(const std::string &pipelineFileName);

    /**
     * If nothing is hit, returns empty vector
     * returns these values:
     * 1) objectID for what is under the cursor
     * 2) hit coordinates
     * 3) hit normal
     *
     */
    std::vector<LimonTypes::GenericParameter> rayCastToCursorAPI();


    std::vector<LimonTypes::GenericParameter> getObjectTransformationAPI(uint32_t objectID) const;

    std::vector<LimonTypes::GenericParameter> getObjectTransformationMatrixAPI(uint32_t objectID) const;

    bool setObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4& position);
    bool setObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool setObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4& orientation);

    bool addObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4& position);
    bool addObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool addObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4& orientation);

    bool interactWithAIAPI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation) const;

    void interactWithPlayerAPI(std::vector<LimonTypes::GenericParameter> &interactionInformation) const;
    void simulateInputAPI(InputStates input);

    long addTimedEventAPI(uint64_t waitTime, bool useWallTime, std::function<void(const std::vector<LimonTypes::GenericParameter> &)> methodToCall,
                          std::vector<LimonTypes::GenericParameter> parameters);
    bool cancelTimedEventAPI(long handleId);

    uint32_t getPlayerAttachedModelAPI();
    std::vector<uint32_t> getModelChildrenAPI(uint32_t modelID);

    std::string getModelAnimationNameAPI(uint32_t modelID);
    bool getModelAnimationFinishedAPI(uint32_t modelID);
    bool setModelAnimationAPI(uint32_t modelID, const std::string& animationName, bool isLooped);
    bool setModelAnimationWithBlendAPI(uint32_t modelID, const std::string& animationName, bool isLooped, long blendTime);
    bool setModelAnimationSpeedAPI(uint32_t modelID, float speed);

    LimonTypes::Vec4 getPlayerModelOffsetAPI();
    bool setPlayerModelOffsetAPI(LimonTypes::Vec4 newOffset);
    void killPlayerAPI();

    bool addLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4& position);
    bool setLightColorAPI(uint32_t lightID, const LimonTypes::Vec4& color);

    bool enableParticleEmitter(uint32_t particleEmitterID);
    bool disableParticleEmitter(uint32_t particleEmitterID);
    uint32_t addParticleEmitter(const std::string &name,
                                const std::string& textureFile,
                                const LimonTypes::Vec4& startPosition,
                                const LimonTypes::Vec4& maxStartDistances,
                                const LimonTypes::Vec2& size,
                                uint32_t count,
                                uint32_t lifeTime,
                                float particlePerMs,
                                bool continuouslyEmit);
    bool removeParticleEmitter(uint32_t emitterID);
    bool setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset);
    bool setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4& gravity);

    /************************************ Methods LimonAPI exposes *************/
    void setupForPlay(InputHandler &inputHandler);

    void setupForPauseOrStop();

    void checkAndRunTimedEvents();

    void animateCustomAnimations();


    void updateActiveLights(bool forceUpdate = false);

    void addSkyBoxControls();

    void
    removeActiveCustomAnimation(const AnimationCustom &animationToRemove, const AnimationStatus *animationStatusToRemove,
                                float animationTime);

    static bool getNameOfTexture(void* data, int index, const char** outText) {
        auto& textures = *static_cast<std::vector<std::shared_ptr<Texture>>*>(data);
        if(index < 0 || (uint32_t)index >= textures.size()) {
            return false;
        }
        auto it = textures.begin();
        for (int i = 0; i < index; ++i) {
            it++;
        }

        *outText = it->get()->getName().c_str();
        return true;

    }

    static uint32_t getLodLevel(const std::vector<long>& lodDistances, float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 viewMatrix, const glm::vec3& playerPosition, PhysicalRenderable *currentRenderable) {
        if(lodDistances.empty() && skipRenderDistance == 0.0) {
            return 0;
        }
        //if(((Model*)currentRenderable)->getName().find("SM_Prop_WaterTrough") != std::string::npos) {
        //    std::cout << " this one " << std::endl;
        //}
        //find the biggest axis of this object
        glm::vec3 max = currentRenderable->getAabbMax();
        glm::vec3 min = currentRenderable->getAabbMin();

        float dx = std::max(min.x - playerPosition.x, std::max(0.0f, playerPosition.x - max.x));
        float dy = std::max(min.y - playerPosition.y, std::max(0.0f, playerPosition.y - max.y));
        float dz = std::max(min.z - playerPosition.z, std::max(0.0f, playerPosition.z - max.z));
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        if(skipRenderDistance !=0 && distance > skipRenderDistance) {
            if(abs(min.x - max.x) < maxSkipRenderSize &&
                    abs(min.y - max.y) < maxSkipRenderSize &&
                    abs(min.z - max.z) < maxSkipRenderSize) {
                //now we get to calculate the size in screen
                glm::vec4 minScreen = viewMatrix * glm::vec4(min, 1.0) ;
                glm::vec4 maxScreen = viewMatrix * glm::vec4(max, 1.0);
                minScreen /= minScreen.w;
                maxScreen /= maxScreen.w;
                float screenX = abs(maxScreen.x - minScreen.x);
                float screenY = abs(maxScreen.y - minScreen.y);
                if (screenX < skipRenderSize * 2.0 && screenY < skipRenderSize * 2.0) {
                    return SKIP_LOD_LEVEL;
                }
            }
        }

        for (size_t i = 0; i < lodDistances.size(); ++i) {
            if(distance < (float)lodDistances[i]) {
                return i;
            }
        }
        //what if the distance is bigger than the last entry? we skip
        return SKIP_LOD_LEVEL;
    }

    void resetTagsAndRefillCulling();
};

#endif //LIMONENGINE_WORLD_H
