//
// Created by engin on 13.05.2018.
//

#ifndef LIMONENGINE_LIMONAPI_H
#define LIMONENGINE_LIMONAPI_H

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <glm/glm.hpp>
#include <functional>
#include <iostream>

#include "InputStates.h"
#include "LimonTypes.h"

class Model;
class AnimationCustom;
class World;
class WorldLoader;
class PhysicalRenderable;
class APISerializer;


class LimonAPI {
    friend class APISerializer;
public:

    bool generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index);

    uint32_t animateModel(uint32_t modelID, uint32_t animationID, bool looped, const std::string *soundPath);
    uint32_t addGuiText(const std::string &fontFilePath, uint32_t fontSize,
                        const std::string &name, const std::string &text,
                               const glm::vec3 &color,
                               const glm::vec2 &position, float rotation);
    uint32_t addGuiImage(const std::string &imageFilePath, const std::string &name, const LimonTypes::Vec2 &position,
                             const LimonTypes::Vec2 &scale, float rotation);

    bool updateGuiText(uint32_t guiTextID, const std::string &newText);
    uint32_t removeGuiElement(uint32_t guiElementID);

    uint32_t addObject(const std::string &modelFilePath, float modelWeight, bool physical, const glm::vec3 &position,
                       const glm::vec3 &scale, const glm::quat &orientation);
    bool setObjectTemporary(uint32_t modelID, bool temporary);
    bool removeObject(uint32_t objectID, const bool &removeChildren = true);
    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID);//second one is
    bool removeTriggerObject(uint32_t TriggerObjectID);
    bool disconnectObjectFromPhysics(uint32_t modelID);
    bool reconnectObjectToPhysics(uint32_t modelID);
    bool applyForce(uint32_t modelID, const LimonTypes::Vec4 &forcePosition, const LimonTypes::Vec4 &forceAmount);
    bool applyForceToPlayer(const LimonTypes::Vec4 &forceAmount);


    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);
    bool detachSoundFromObject(uint32_t objectWorldID);
    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative = false, bool looped = false);

    bool interactWithAI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation);

    bool disableParticleEmitter(uint32_t particleEmitterId);
    bool enableParticleEmitter(uint32_t particleEmitterId);

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

    /**
     * * If nothing is hit, returns empty vector
     * returns these values:
     * 1) objectID for what is under the cursor
     * 2) hit coordinates
     * 3) hit normal
     * 4) If object has AI, id of that AI
     */
    std::vector<LimonTypes::GenericParameter> rayCastToCursor();

    /**
     * If object not found, returns empty vector
     *
     * Returns these values:
     * 1) translate
     * 2) scale
     * 3) orientation
     */
    std::vector<LimonTypes::GenericParameter> getObjectTransformation(uint32_t objectID);

    bool setObjectTranslate(uint32_t objectID, const LimonTypes::Vec4& position);
    bool setObjectScale(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool setObjectOrientation(uint32_t objectID, const LimonTypes::Vec4& orientation);

    bool addObjectTranslate(uint32_t objectID, const LimonTypes::Vec4& position);
    bool addObjectScale(uint32_t objectID, const LimonTypes::Vec4& scale);
    bool addObjectOrientation(uint32_t objectID, const LimonTypes::Vec4& orientation);

    /**
     * Returns mat4 with objects transform
     *
     * It might be required for object that has custom matrix generation
     * @param objectID
     * @return
     */
    std::vector<LimonTypes::GenericParameter> getObjectTransformationMatrix(uint32_t objectID);

    uint32_t getPlayerAttachedModel();
    LimonTypes::Vec4 getPlayerAttachedModelOffset();
    bool setPlayerAttachedModelOffset(LimonTypes::Vec4 newOffset);
    void killPlayer();

    std::string getModelAnimationName(uint32_t modelID);
    bool getModelAnimationFinished(uint32_t modelID);
    bool setModelAnimation(uint32_t modelID, const std::string& animationName, bool isLooped = true);
    bool setModelAnimationWithBlend(uint32_t modelID, const std::string& animationName, bool isLooped = true, long blendTime = 100);
    bool setModelAnimationSpeed(uint32_t modelID, float speed);
    std::vector<uint32_t> getModelChildren(uint32_t modelID);


    long addTimedEvent(uint64_t waitTime, bool useWallTime, std::function<void(const std::vector<LimonTypes::GenericParameter>&)> methodToCall, std::vector<LimonTypes::GenericParameter> parameters);
    bool cancelTimedEvent(long handleId);


    void interactWithPlayer(std::vector<LimonTypes::GenericParameter>& input);
    void simulateInput(const InputStates& input);


    bool addLightTranslate(uint32_t lightID, const LimonTypes::Vec4& translate);
    bool setLightColor(uint32_t lightID, const LimonTypes::Vec4& color);

    bool changeRenderPipeline(const std::string& pipelineFileName) {
        return worldChangeRenderPipeline(pipelineFileName);
    }

    bool loadAndSwitchWorld(const std::string& worldFileName);
    bool returnToWorld(const std::string& worldFileName);//if world is not loaded, loads first
    bool LoadAndRemove(const std::string& worldFileName); // removes current world after loading the new one

    void returnPreviousWorld();
    void quitGame();




    std::vector<LimonTypes::GenericParameter> getResultOfTrigger(uint32_t TriggerObjectID, uint32_t TriggerCodeID);

    /**
     * This method Returns a parameter request reference that you can update. If the variable was never set,
     * it creates one with the default values. There are no safety checks, user is fully responsible for the variables.
     *
     * Don't forget, these variables are not saved in world save, so they should be considered temporary.
     *
     * @param variableName
     * @return variable itself
     */
    LimonTypes::GenericParameter& getVariable(const std::string& variableName) {
        if(variableStore.find(variableName) == variableStore.end()) {
            variableStore[variableName] = LimonTypes::GenericParameter();
        }
        return variableStore[variableName];
    }

    LimonAPI(std::function<bool (const std::string&)> worldLoadMethod,
             std::function<bool (const std::string&)> worldReturnOrLoadMethod,
             std::function<bool (const std::string&)> worldLoadNewAndRemoveCurrentMethod,
             std::function<void ()> worldExitMethod,
             std::function<void ()> worldReturnPreviousMethod) {
        limonLoadWorld = std::move(worldLoadMethod);
        limonReturnOrLoadWorld = std::move(worldReturnOrLoadMethod);
        limonLoadNewAndRemoveCurrentWorld = std::move(worldLoadNewAndRemoveCurrentMethod);
        limonExitGame = std::move(worldExitMethod);
        limonReturnPrevious = std::move(worldReturnPreviousMethod);
    }
private:
    friend class WorldLoader;

    std::map<std::string, LimonTypes::GenericParameter> variableStore;

    std::function<bool(std::vector<LimonTypes::GenericParameter> &, uint32_t)> worldGenerateEditorElementsForParameters;
    std::function<uint32_t(uint32_t , uint32_t , bool, const std::string* )> worldAddAnimationToObject;
    std::function<uint32_t(const std::string &, uint32_t, const std::string &, const std::string &, const glm::vec3 &, const glm::vec2 &, float)> worldAddGuiText;
    std::function<uint32_t(const std::string &, const std::string &, const LimonTypes::Vec2 &, const LimonTypes::Vec2 &, float)> worldAddGuiImage;
    std::function<uint32_t(const std::string &, float, bool, const glm::vec3 &, const glm::vec3 &, const glm::quat &)> worldAddModel;
    std::function<bool(uint32_t, bool)> worldSetModelTemporary;
    std::function<bool(uint32_t, const std::string &)> worldUpdateGuiText;
    std::function<uint32_t (uint32_t)> worldRemoveGuiElement;
    std::function<std::vector<LimonTypes::GenericParameter>(uint32_t , uint32_t )> worldGetResultOfTrigger;
    std::function<bool (uint32_t, bool)> worldRemoveObject;
    std::function<std::vector<LimonTypes::GenericParameter>(uint32_t)> worldGetObjectTransformation;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldSetObjectTranslate;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldSetObjectScale;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldSetObjectOrientation;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldAddObjectTranslate;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldAddObjectScale;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldAddObjectOrientation;

    std::function<std::vector<LimonTypes::GenericParameter>(uint32_t)> worldGetObjectTransformationMatrix;
    std::function<bool (uint32_t, uint32_t)> worldAttachObjectToObject;
    std::function<bool (uint32_t)> worldRemoveTriggerObject;
    std::function<bool (uint32_t)> worldDisconnectObjectFromPhysics;
    std::function<bool (uint32_t)> worldReconnectObjectToPhysics;

    std::function<bool (uint32_t, const LimonTypes::Vec4&, const LimonTypes::Vec4&)> worldApplyForce;
    std::function<bool (const LimonTypes::Vec4&)> worldApplyForceToPlayer;

    std::function<bool (uint32_t, const std::string&)> worldAttachSoundToObjectAndPlay;
    std::function<bool (uint32_t)> worldDetachSoundFromObject;
    std::function<uint32_t (const std::string&, const glm::vec3&, bool, bool)> worldPlaySound;

    std::function<std::vector<LimonTypes::GenericParameter>()> worldRayCastToCursor;
    std::function<bool (uint32_t, std::vector<LimonTypes::GenericParameter>&)> worldInteractWithAI;
    std::function<void (std::vector<LimonTypes::GenericParameter>&)> worldInteractWithPlayer;
    std::function<void (InputStates)> worldSimulateInput;

    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldAddLightTranslate;
    std::function<bool (uint32_t, const LimonTypes::Vec4&)> worldSetLightColor;

    std::function<long (uint64_t, bool, std::function<void(const std::vector<LimonTypes::GenericParameter>&)>, std::vector<LimonTypes::GenericParameter>)> worldAddTimedEvent;
    std::function<bool (long)> worldCancelTimedEvent;

    std::function<bool (uint32_t)> worldEnableParticleEmitter;
    std::function<bool (uint32_t)> worldDisableParticleEmitter;
    std::function<uint32_t (const std::string&, const std::string&, const LimonTypes::Vec4&, const LimonTypes::Vec4&, const LimonTypes::Vec2&, uint32_t, uint32_t, float, bool)> worldAddParticleEmitter;
    std::function<bool (uint32_t)> worldRemoveParticleEmitter;
    std::function<bool (uint32_t, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset)> worldSetEmitterParticleSpeed;
    std::function<bool (uint32_t, const LimonTypes::Vec4& gravity)> worldSetEmitterParticleGravity;

    std::function<LimonTypes::Vec4 ()> worldGetPlayerAttachmentOffset;
    std::function<bool (LimonTypes::Vec4)> worldSetPlayerAttachmentOffset;
    std::function<uint32_t ()> worldGetPlayerAttachedModel;
    std::function<std::vector<uint32_t> (uint32_t)> worldGetModelChildren;
    std::function<void ()> worldKillPlayer;

    std::function<std::string(uint32_t)> worldGetModelAnimationName;
    std::function<bool(uint32_t)> worldGetModelAnimationFinished;
    std::function<bool(uint32_t, const std::string&, bool)> worldSetAnimationOfModel;
    std::function<bool(uint32_t, const std::string&, bool, long)> worldSetAnimationOfModelWithBlend;
    std::function<bool(uint32_t, float)> worldSetModelAnimationSpeed;

    std::function<bool(const std::string&)> worldChangeRenderPipeline;

    /*** Non World API calls *******************************************************/
    std::function<bool (const std::string&)> limonLoadWorld;
    std::function<bool (const std::string&)> limonReturnOrLoadWorld;
    std::function<bool (const std::string&)> limonLoadNewAndRemoveCurrentWorld;

    std::function<void ()> limonExitGame;
    std::function<void ()> limonReturnPrevious;
    /*** Non World API calls *******************************************************/
};


#endif //LIMONENGINE_LIMONAPI_H
