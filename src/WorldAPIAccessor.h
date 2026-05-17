//
// Holds all LimonAPI-exposed World methods, keeping World.cpp focused on simulation logic.
//

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "limonAPI/LimonTypes.h"
#include "limonAPI/InputStates.h"

class World;

class WorldAPIAccessor {
    World* world;
public:
    explicit WorldAPIAccessor(World* world) : world(world) {}

    // Animation
    uint32_t addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                           const std::string* soundToPlay);
    uint32_t addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad) {
        return addAnimationToObjectWithSound(modelID, animationID, looped, startOnLoad, nullptr);
    }

    // GUI
    uint32_t addGuiText(const std::string& fontFilePath, uint32_t fontSize, const std::string& name,
                        const std::string& text, const glm::vec3& color, const glm::vec2& position, float rotation);
    uint32_t addGuiImageAPI(const std::string& imageFilePath, const std::string& name,
                            const LimonTypes::Vec2& position, const LimonTypes::Vec2& scale, float rotation);
    bool updateGuiText(uint32_t guiTextID, const std::string& newText);
    bool removeGuiElement(uint32_t guiElementID);

    // Object management
    uint32_t addModelApi(const std::string& modelFilePath, float modelWeight, bool physical,
                         const glm::vec3& position, const glm::vec3& scale, const glm::quat& orientation);
    bool setModelTemporaryAPI(uint32_t modelID, bool temporary);
    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID);
    bool removeObject(uint32_t objectID, const bool& removeChildren = true);
    bool removeTriggerObject(uint32_t triggerobjectID);

    std::vector<LimonTypes::GenericParameter> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    // Physics
    bool disconnectObjectFromPhysics(uint32_t objectWorldID);
    bool reconnectObjectToPhysics(uint32_t objectWorldID);
    bool disconnectObjectFromPhysicsRequest(uint32_t objectWorldID);
    bool reconnectObjectToPhysicsRequest(uint32_t objectWorldID);
    bool applyForceAPI(uint32_t objectID, const LimonTypes::Vec4& forcePosition, const LimonTypes::Vec4& forceAmount);
    bool applyForceToPlayerAPI(const LimonTypes::Vec4& forceAmount);

    // Raycasting
    std::vector<LimonTypes::GenericParameter> rayCastToCursorAPI() const;
    std::vector<LimonTypes::GenericParameter> rayCastAPI(const LimonTypes::Vec4& start, const LimonTypes::Vec4& direction) const;

    // Object transform
    std::vector<LimonTypes::GenericParameter> getObjectTransformationAPI(uint32_t objectID) const;
    std::vector<LimonTypes::GenericParameter> getObjectTransformationMatrixAPI(uint32_t objectID) const;
    bool setObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4& position);
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
    uint32_t getPlayerAttachedModelAPI();
    std::vector<uint32_t> getModelChildrenAPI(uint32_t modelID);
    LimonTypes::Vec4 getPlayerModelOffsetAPI();
    bool setPlayerModelOffsetAPI(LimonTypes::Vec4 newOffset);
    void killPlayerAPI();

    // Animation queries / control
    std::string getModelAnimationNameAPI(uint32_t modelID);
    bool getModelAnimationFinishedAPI(uint32_t modelID);
    bool setModelAnimationAPI(uint32_t modelID, const std::string& animationName, bool isLooped);
    bool setModelAnimationWithBlendAPI(uint32_t modelID, const std::string& animationName, bool isLooped, long blendTime);
    bool setModelAnimationSpeedAPI(uint32_t modelID, float speed);

    // Sound
    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string& soundPath);
    bool detachSoundFromObject(uint32_t objectWorldID);
    uint32_t playSound(const std::string& soundPath, const glm::vec3& position, bool positionRelative, bool looped);

    // Lights
    uint32_t addLightAPI(uint32_t lightType, const LimonTypes::Vec4& position, const LimonTypes::Vec4& color);
    bool removeLightAPI(uint32_t lightID);
    bool addLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4& position);
    bool setLightColorAPI(uint32_t lightID, const LimonTypes::Vec4& color);

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
