//
// Holds all LimonAPI-exposed World methods, keeping World.cpp focused on simulation logic.
//

#ifndef LIMONENGINE_WORLDAPIACCESSOR_H
#define LIMONENGINE_WORLDAPIACCESSOR_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "limonAPI/LimonTypes.h"
#include "limonAPI/InputStates.h"
#include "limonAPI/LimonAPI.h"

class World;
class Light;
class Attachable;
class Model;
class PhysicalRenderable;

class WorldAPIAccessor {
    World* world;

    //an empty name means the object itself, a name that is not a bone of this parent fails the attachment
    bool resolveAttachmentBone(const Attachable *parent, const std::string &boneName, int32_t &boneID) const;
    //attachments are ordinary objects, these tags are the only thing that lets a stage render the player's own parts apart
    void refreshPlayerAttachmentTags(Attachable *subtreeRoot, bool underPlayer);
    void setPlayerAttachmentTag(Model *model, const std::string &kindTag, const std::string &playerTag, bool underPlayer);
public:
    WorldAPIAccessor(World* world, LimonAPI* limonAPI);

    // Animation
    uint32_t addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                           const std::string& soundToPlay);
    uint32_t addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad) {
        return addAnimationToObjectWithSound(modelID, animationID, looped, startOnLoad, "");
    }
    uint32_t addAnimationToObjectByNameWithSound(uint32_t modelID, const std::string& animationName, bool looped,
                                                 bool startOnLoad, const std::string& soundToPlay);
    std::vector<std::string> listLoadedAnimationsAPI() const;

    // GUI
    uint32_t addGuiText(const std::string& fontFilePath, uint32_t fontSize, const std::string& name,
                        const std::string& text, const glm::vec3& color, const glm::vec2& position, float rotation);
    uint32_t addGuiImageAPI(const std::string& imageFilePath, const std::string& name,
                            const LimonTypes::Vec2& position, const LimonTypes::Vec2& scale, float rotation);
    bool updateGuiText(uint32_t guiTextID, const std::string& newText);
    bool removeGuiElement(uint32_t guiElementID);
    LimonTypes::Vec4 getGuiElementPositionAPI(uint32_t guiElementID) const;
    bool setGuiElementPositionAPI(uint32_t guiElementID, const LimonTypes::Vec4& position);
    bool setGuiElementVisibleAPI(uint32_t guiElementID, bool visible);

    // Object management
    uint32_t addModelApi(const std::string& modelFilePath, float modelWeight, bool physical,
                         const glm::vec3& position, const glm::vec3& scale, const glm::quat& orientation);
    bool setModelTemporaryAPI(uint32_t modelID, bool temporary);
    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID, const std::string& boneName);
    bool attachObjectToObjectAtWorldPosition(uint32_t objectID, uint32_t objectToAttachToID, const std::string& boneName);
    bool detachObjectFromParent(uint32_t objectID);
    bool addObjectTagAPI(uint32_t objectID, const std::string& tag);
    bool removeObjectTagAPI(uint32_t objectID, const std::string& tag);
    std::vector<std::string> getObjectTagsAPI(uint32_t objectID) const;

    enum class BodyTypes { STATIC, DYNAMIC, KINEMATIC };
    /**
     * Every parent change goes through these two, never Attachable::attachTo/detach directly. A child's body type
     * and its collision pairs depend on its parent, and nothing else would update them. Editor and WorldLoader
     * come in here instead of the ID taking calls above, their bone is an index they already hold.
     */
    void attach(Attachable *child, Attachable *parent, int32_t boneID, bool keepWorldPosition);
    void detach(Attachable *child);

    BodyTypes calculateBodyType(const Model *model) const;
    BodyTypes calculateParentBodyType(const Attachable *parent) const;
    //recurses to children, a parent switching between static and moving changes theirs too
    void applyBodyType(PhysicalRenderable *renderable);
    //writes the user index World's HierarchyFilterCallback reads, so one hierarchy doesn't collide with itself
    void setHierarchyRootIndex(Attachable *subtreeRoot, int rootIndex);
    /**
     * Changes a model's mass, reloading its collision shape (mass 0 = static triangle mesh, >0 = dynamic convex hull).
     * Removes the body from the dynamics world, reloads the shape, then re-adds it with the collision group matching
     * its new static/dynamic state, and drops its stale visibility-culling membership. No-op for animated models.
     * @return false if the object does not exist, is not a Model, or is animated.
     */
    bool changeModelMass(uint32_t objectID, float newMass);
    bool removeObject(uint32_t objectID, const bool& removeChildren = true);
    bool removeTriggerObject(uint32_t triggerobjectID);

    std::vector<LimonTypes::GenericParameter> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);
    bool isInsideTrigger(uint32_t triggerID) const;
    uint32_t getObjectByName(const std::string& name) const;
    uint32_t getObjectParent(uint32_t objectID) const;
    std::vector<uint32_t> getObjectChildren(uint32_t objectID) const;
    bool isObjectPhysicsConnected(uint32_t objectID) const;
    bool setPhysicsSimulationActive(uint32_t objectID, bool active);

    // Physics
    LimonTypes::Vec4 getObjectLinearVelocity(uint32_t objectID) const;
    bool setObjectLinearVelocity(uint32_t objectID, const LimonTypes::Vec4& velocity);
    float getObjectMass(uint32_t objectID) const;
    bool disconnectObjectFromPhysics(uint32_t objectWorldID);
    bool reconnectObjectToPhysics(uint32_t objectWorldID);
    bool disconnectObjectFromPhysicsRequest(uint32_t objectWorldID);
    bool reconnectObjectToPhysicsRequest(uint32_t objectWorldID);
    bool applyForceAPI(uint32_t objectID, const LimonTypes::Vec4& forcePosition, const LimonTypes::Vec4& forceAmount);
    bool applyForceToPlayerAPI(const LimonTypes::Vec4& forceAmount);

    uint32_t createCameraRig(const std::string& cameraRigTypeName);
    bool activateCameraRig(uint32_t cameraRigId);
    void deactivateCameraRig();
    bool removeCameraRig(uint32_t rigID);

    // Raycasting
    std::vector<LimonTypes::GenericParameter> rayCastToCursorAPI() const;
    std::vector<LimonTypes::GenericParameter> rayCastAPI(const LimonTypes::Vec4& start, const LimonTypes::Vec4& direction) const;

    // Object transform
    std::vector<LimonTypes::GenericParameter> getObjectTransformationAPI(uint32_t objectID) const;
    std::vector<LimonTypes::GenericParameter> getObjectTransformationMatrixAPI(uint32_t objectID) const;
    bool setObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4& position);
    bool setObjectMassAPI(uint32_t objectID, float mass);
    bool setObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool setObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4& orientation);
    bool addObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4& position);
    bool addObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool addObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4& orientation);

    // AI / Player interaction
    bool interactWithAIAPI(uint32_t AIID, std::vector<LimonTypes::GenericParameter>& interactionInformation) const;
    void interactWithPlayerAPI(std::vector<LimonTypes::GenericParameter>& interactionInformation) const;
    void simulateInputAPI(InputStates input);

    // Timed events
    long addTimedEventAPI(uint64_t waitTime, bool useWallTime,
                          std::function<void(const std::vector<LimonTypes::GenericParameter>&)> methodToCall,
                          std::vector<LimonTypes::GenericParameter> parameters);
    bool cancelTimedEventAPI(long handleId);

    // Player queries
    void getPlayerPositionAPI(glm::vec3& position, glm::vec3& center, glm::vec3& up, glm::vec3& right);
    LimonTypes::Vec4 getPlayerPositionVec4API();
    LimonTypes::Vec4 getPlayerLookDirectionAPI();
    LimonTypes::Vec4 getCameraPositionAPI();
    LimonTypes::Vec4 getCameraLookDirectionAPI();
    uint32_t getPlayerObjectIDAPI() const;
    void killPlayerAPI();

    // Animation queries / control
    std::string getModelAnimationNameAPI(uint32_t modelID);
    bool getModelAnimationFinishedAPI(uint32_t modelID);
    float getModelAnimationProgressAPI(uint32_t modelID) const;
    std::vector<std::string> listModelAnimationsAPI(uint32_t modelID) const;
    bool setModelAnimationAPI(uint32_t modelID, const std::string& animationName, bool isLooped);
    bool setModelAnimationWithBlendAPI(uint32_t modelID, const std::string& animationName, bool isLooped, uint64_t blendTime);
    bool setModelAnimationSpeedAPI(uint32_t modelID, float speed);

    // Sound
    uint32_t playSound(const std::string& soundPath, const glm::vec3& position, bool positionRelative, bool looped, float referenceDistance, float maxDistance, LimonTypes::AudioChannel channel = LimonTypes::AudioChannel::SFX);
    bool stopSound(uint32_t soundID);
    bool pauseSound(uint32_t soundID);
    bool resumeSound(uint32_t soundID);
    bool setSoundVolume(uint32_t soundID, float volume);
    bool setSoundLooped(uint32_t soundID, bool looped);
    bool isSoundPlaying(uint32_t soundID);
    bool setSoundTemporaryAPI(uint32_t soundID, bool temporary);

    // Music (dedicated MUSIC channel, single level track, optional crossfade)
    bool setMusic(const std::string& musicPath, float fadeSeconds, bool looped);
    bool stopMusic(float fadeSeconds);
    std::string getMusicName() const;
    bool isMusicPlaying() const;
    // Channel (bus) volumes are option-driven (see World::applyAudioVolumeOptionsIfChanged), not set here.

    // Lights
    Light* findLight(uint32_t lightID) const;
    uint32_t addLightPointAPI(const LimonTypes::Vec4& position, const LimonTypes::Vec4& color,
                              float intensity, float radius, float falloff, float edgeBrightness);
    uint32_t addLightDirectionalAPI(const LimonTypes::Vec4& direction, const LimonTypes::Vec4& color);
    bool removeLightAPI(uint32_t lightID);
    bool addLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4& position);
    bool setLightColorAPI(uint32_t lightID, const LimonTypes::Vec4& color);
    LimonTypes::Vec4 getLightPositionAPI(uint32_t lightID) const;
    LimonTypes::Vec4 getLightColorAPI(uint32_t lightID) const;
    bool setLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4& position);
    uint32_t getLightTypeAPI(uint32_t lightID) const;
    bool setLightPointParametersAPI(uint32_t lightID, float intensity, float radius, float falloff, float edgeBrightness);
    LimonTypes::Vec4 getLightPointParametersAPI(uint32_t lightID) const;
    bool setLightPointAttenuationAPI(uint32_t lightID, float constant, float linear);
    LimonTypes::Vec4 getLightPointAttenuationAPI(uint32_t lightID) const;
    LimonTypes::Vec4 solveLightPointAttenuationAPI(uint32_t lightID, float constant, float linear, float exponential) const;
    bool setLightAmbientAPI(uint32_t lightID, const LimonTypes::Vec4& ambientColor);
    LimonTypes::Vec4 getLightAmbientAPI(uint32_t lightID) const;

    // Render pipeline
    bool changeRenderPipeline(const std::string& pipelineFileName);

    // Particle emitters
    bool enableParticleEmitter(uint32_t particleEmitterId);
    bool disableParticleEmitter(uint32_t particleEmitterId);
    uint32_t addParticleEmitter(const std::string& name, const std::string& textureFile,
                                const LimonTypes::Vec4& startPosition, const LimonTypes::Vec4& maxStartDistances,
                                const LimonTypes::Vec2& size, uint32_t count, uint32_t lifeTime,
                                float particlePerMs, bool continuouslyEmit);
    bool removeParticleEmitter(uint32_t emitterID);
    bool setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset);
    bool setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4& gravity);
};

#endif //LIMONENGINE_WORLDAPIACCESSOR_H
