//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"
#include "ProfileScope.h"

const OptionsUtil::Options * LimonAPI::getOptions() {
    return limonGetOptions();
}

bool LimonAPI::saveOptions() {
    return limonSaveOptions();
}

uint32_t LimonAPI::animateModel(uint32_t modelID, uint32_t animationID, bool looped, const std::string& soundPath) {
    return worldAddAnimationToObject(modelID, animationID, looped, soundPath);
}

uint32_t LimonAPI::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &name, const std::string &text,
                              const glm::vec3 &color, const glm::vec2 &position, float rotation) {
    return worldAddGuiText(fontFilePath, fontSize, name, text, color, position,rotation);
}

uint32_t LimonAPI::addGuiImage(const std::string &imageFilePath, const std::string &name, const LimonTypes::Vec2 &position,
                               const LimonTypes::Vec2 &scale, float rotation) {
    return worldAddGuiImage(imageFilePath, name, position, scale, rotation);
}

uint32_t LimonAPI::addObject(const std::string &modelFilePath, float modelWeight, bool physical,
                             const glm::vec3 &position,
                             const glm::vec3 &scale, const glm::quat &orientation) {
    return worldAddModel(modelFilePath, modelWeight, physical, position, scale, orientation);
}

bool LimonAPI::attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID) {
    return worldAttachObjectToObject(objectID, objectToAttachToID);
}

bool LimonAPI::attachObjectToObjectAtWorldPosition(uint32_t objectID, uint32_t objectToAttachToID) {
    return worldAttachObjectToObjectAtWorldPosition(objectID, objectToAttachToID);
}

bool LimonAPI::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    return worldUpdateGuiText(guiTextID, newText);
}

bool LimonAPI::removeGuiElement(uint32_t guiElementID) {
    return worldRemoveGuiElement(guiElementID);
}

LimonTypes::Vec4 LimonAPI::getGuiElementPosition(uint32_t guiElementID) {
    return worldGetGuiElementPosition(guiElementID);
}

bool LimonAPI::setGuiElementPosition(uint32_t guiElementID, const LimonTypes::Vec4& position) {
    return worldSetGuiElementPosition(guiElementID, position);
}

bool LimonAPI::setGuiElementVisible(uint32_t guiElementID, bool visible) {
    return worldSetGuiElementVisible(guiElementID, visible);
}

std::vector<LimonTypes::GenericParameter> LimonAPI::getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID) {
    std::vector<LimonTypes::GenericParameter> results = worldGetResultOfTrigger(triggerObjectID, triggerCodeID);
    return results;
}

bool LimonAPI::isInsideTrigger(uint32_t triggerID) {
    return worldIsInsideTrigger(triggerID);
}

uint32_t LimonAPI::getObjectByName(const std::string& name) {
    return worldGetObjectByName(name);
}

uint32_t LimonAPI::getObjectParent(uint32_t objectID) {
    return worldGetObjectParent(objectID);
}

bool LimonAPI::isObjectPhysicsConnected(uint32_t objectID) {
    return worldIsObjectPhysicsConnected(objectID);
}

bool LimonAPI::removeObject(uint32_t objectID, const bool &removeChildren) {
    return worldRemoveObject(objectID, removeChildren);
}

bool LimonAPI::removeTriggerObject(uint32_t triggerObjectID) {
    return worldRemoveTriggerObject(triggerObjectID);
}

LimonTypes::Vec4 LimonAPI::getObjectLinearVelocity(uint32_t objectID) {
    return worldGetObjectLinearVelocity(objectID);
}

bool LimonAPI::setObjectLinearVelocity(uint32_t objectID, const LimonTypes::Vec4& velocity) {
    return worldSetObjectLinearVelocity(objectID, velocity);
}

float LimonAPI::getObjectMass(uint32_t objectID) {
    return worldGetObjectMass(objectID);
}

bool LimonAPI::disconnectObjectFromPhysics(uint32_t modelID) {
    return worldDisconnectObjectFromPhysics(modelID);
}

bool LimonAPI::reconnectObjectToPhysics(uint32_t modelID) {
    return worldReconnectObjectToPhysics(modelID);
}

bool LimonAPI::applyForce(uint32_t modelID, const LimonTypes::Vec4 &forcePosition, const LimonTypes::Vec4 &forceAmount) {
    return worldApplyForce(modelID, forcePosition, forceAmount);
}

bool LimonAPI::applyForceToPlayer(const LimonTypes::Vec4 &forceAmount) {
    return worldApplyForceToPlayer(forceAmount);
}

uint32_t
LimonAPI::playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped, float referenceDistance, float maxDistance) {
    return worldPlaySound(soundPath, position, positionRelative, looped, referenceDistance, maxDistance);
}

bool LimonAPI::stopSound(uint32_t soundID) {
    return worldStopSound(soundID);
}

bool LimonAPI::pauseSound(uint32_t soundID) {
    return worldPauseSound(soundID);
}

bool LimonAPI::resumeSound(uint32_t soundID) {
    return worldResumeSound(soundID);
}

bool LimonAPI::setSoundVolume(uint32_t soundID, float volume) {
    return worldSetSoundVolume(soundID, volume);
}

bool LimonAPI::setSoundLooped(uint32_t soundID, bool looped) {
    return worldSetSoundLooped(soundID, looped);
}

bool LimonAPI::isSoundPlaying(uint32_t soundID) {
    return worldIsSoundPlaying(soundID);
}

bool LimonAPI::setSoundTemporary(uint32_t soundID, bool temporary) {
    return worldSetSoundTemporary(soundID, temporary);
}

bool LimonAPI::loadAndSwitchWorld(const std::string& worldFileName) {
    return limonLoadWorld(worldFileName);
}

bool LimonAPI::returnToWorld(const std::string &worldFileName) {
    return this->limonReturnOrLoadWorld(worldFileName);
}

bool LimonAPI::loadAndRemove(const std::string &worldFileName) {
    return this->limonLoadNewAndRemoveCurrentWorld(worldFileName);
}

void LimonAPI::returnPreviousWorld() {
    this->limonReturnPrevious();
}

void LimonAPI::quitGame() {
    limonExitGame();
}

std::vector<LimonTypes::GenericParameter> LimonAPI::rayCastToCursor() {
    return worldRayCastToCursor();
}

std::vector<LimonTypes::GenericParameter> LimonAPI::rayCastFirstHit(const LimonTypes::Vec4& start, const LimonTypes::Vec4& direction) {
    return worldRayCast(start, direction);
}


std::vector<LimonTypes::GenericParameter> LimonAPI::getObjectTransformation(uint32_t objectID) {
    return worldGetObjectTransformation(objectID);
}

std::vector<LimonTypes::GenericParameter> LimonAPI::getObjectTransformationMatrix(uint32_t objectID) {
    return worldGetObjectTransformationMatrix(objectID);
}

LimonTypes::Vec4 LimonAPI::getObjectPosition(uint32_t objectID) {
    std::vector<LimonTypes::GenericParameter> transformation = getObjectTransformation(objectID);
    if (transformation.size() >= 1) {
        return transformation[0].value.vectorValue;// element 0 is the translate component
    }
    return LimonTypes::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

LimonTypes::Vec4 LimonAPI::getObjectFrontVector(uint32_t objectID) {
    std::vector<LimonTypes::GenericParameter> transformation = getObjectTransformation(objectID);
    // Default orientation matches the historic ActorInterface::getFrontVector packing of
    // glm::quat(0, 0, 1, 0) interpreted as (w, x, y, z).
    float w = 0.0f, x = 0.0f, y = 1.0f, z = 0.0f;
    if (transformation.size() >= 3) {
        const LimonTypes::Vec4 &orientation = transformation[2].value.vectorValue;// element 2 is orientation
        // Same packing the previous implementation used: glm::quat(vectorValue.x, .y, .z, .w).
        w = orientation.x;
        x = orientation.y;
        y = orientation.z;
        z = orientation.w;
    }
    // Rotate the forward vector (0, 0, 1) by the quaternion, plain-float port of the glm math so the
    // ABI-stable LimonAPI carries no glm dependency.
    float forwardX = 0.0f, forwardY = 0.0f, forwardZ = 1.0f;
    float dotUF = x * forwardX + y * forwardY + z * forwardZ;
    float dotUU = x * x + y * y + z * z;
    float crossX = y * forwardZ - z * forwardY;
    float crossY = z * forwardX - x * forwardZ;
    float crossZ = x * forwardY - y * forwardX;
    LimonTypes::Vec4 result;
    result.x = 2.0f * dotUF * x + (w * w - dotUU) * forwardX + 2.0f * w * crossX;
    result.y = 2.0f * dotUF * y + (w * w - dotUU) * forwardY + 2.0f * w * crossY;
    result.z = 2.0f * dotUF * z + (w * w - dotUU) * forwardZ + 2.0f * w * crossZ;
    result.w = 0.0f;
    return result;
}

bool LimonAPI::interactWithAI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation) {
    return worldInteractWithAI(AIID, interactionInformation);
}

void LimonAPI::interactWithPlayer(std::vector<LimonTypes::GenericParameter> &input) {
    return this->worldInteractWithPlayer(input);
}

void LimonAPI::simulateInput(const InputStates& input) {
    this->worldSimulateInput(input);
}

uint32_t LimonAPI::addLight(uint32_t lightType, const LimonTypes::Vec4 &position, const LimonTypes::Vec4 &color) {
    return worldAddLight(lightType, position, color);
}

bool LimonAPI::removeLight(uint32_t lightID) {
    return worldRemoveLight(lightID);
}

bool LimonAPI::addLightTranslate(uint32_t lightID, const LimonTypes::Vec4 &position) {
    return worldAddLightTranslate(lightID, position);
}

bool LimonAPI::setLightColor(uint32_t lightID, const LimonTypes::Vec4 &color){
    return worldSetLightColor(lightID, color);
}

LimonTypes::Vec4 LimonAPI::getLightPosition(uint32_t lightID) {
    return worldGetLightPosition(lightID);
}

LimonTypes::Vec4 LimonAPI::getLightColor(uint32_t lightID) {
    return worldGetLightColor(lightID);
}

bool LimonAPI::setLightTranslate(uint32_t lightID, const LimonTypes::Vec4& position) {
    return worldSetLightTranslate(lightID, position);
}

bool LimonAPI::changeRenderPipeline(const std::string& pipelineFileName) {
    return worldChangeRenderPipeline(pipelineFileName);
}

//Wall time parameter is used to determine using wall time or game time. These differ because loading other worlds (menus) or editor will stop game time.
long LimonAPI::addTimedEvent(uint64_t waitTime, bool useWallTime,
                             std::function<void(const std::vector<LimonTypes::GenericParameter> &)> methodToCall,
                             std::vector<LimonTypes::GenericParameter> parameters) {
    return worldAddTimedEvent(waitTime, useWallTime, methodToCall, parameters);
}

bool LimonAPI::cancelTimedEvent(long handleId) {
    return worldCancelTimedEvent(handleId);
}

bool LimonAPI::disableParticleEmitter(uint32_t particleEmitterId) {
    return worldDisableParticleEmitter(particleEmitterId);
}
bool LimonAPI::enableParticleEmitter(uint32_t particleEmitterId) {
    return worldEnableParticleEmitter(particleEmitterId);
}

uint32_t LimonAPI::addParticleEmitter(const std::string &name,
                            const std::string& textureFile,
                            const LimonTypes::Vec4& startPosition,
                            const LimonTypes::Vec4& maxStartDistances,
                            const LimonTypes::Vec2& size,
                            uint32_t count,
                            uint32_t lifeTime,
                            float particlePerMs,
                            bool continuouslyEmit){
    return this->worldAddParticleEmitter(name,
    textureFile,
    startPosition,
    maxStartDistances,
    size,
    count,
    lifeTime,
    particlePerMs,
    continuouslyEmit);
}
bool LimonAPI::removeParticleEmitter(uint32_t emitterID) {
    return worldRemoveParticleEmitter(emitterID);
}
bool LimonAPI::setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4& speedMultiplier, const LimonTypes::Vec4& speedOffset){
    return worldSetEmitterParticleSpeed(emitterID, speedMultiplier, speedOffset);
}
bool LimonAPI::setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4& gravity){
    return worldSetEmitterParticleGravity(emitterID, gravity);
}

void LimonAPI::getPlayerPosition(glm::vec3& position, glm::vec3& center, glm::vec3& up, glm::vec3& right) {
    worldGetPlayerPosition(position, center, up, right);
}

LimonTypes::Vec4 LimonAPI::getPlayerPosition() {
    return worldGetPlayerPositionVec4();
}

LimonTypes::Vec4 LimonAPI::getPlayerLookDirection() {
    return worldGetPlayerLookDirection();
}

LimonTypes::Vec4 LimonAPI::getCameraPosition() {
    return worldGetCameraPosition();
}

LimonTypes::Vec4 LimonAPI::getCameraLookDirection() {
    return worldGetCameraLookDirection();
}

LimonTypes::Vec4 LimonAPI::getPlayerAttachedModelOffset() {
    return worldGetPlayerAttachmentOffset();
}

bool LimonAPI::setPlayerAttachedModelOffset(LimonTypes::Vec4 newOffset) {
    return worldSetPlayerAttachmentOffset(newOffset);
}

uint32_t LimonAPI::getPlayerAttachedModel() {
    return worldGetPlayerAttachedModel();
}

std::vector<uint32_t> LimonAPI::getModelChildren(uint32_t modelID) {
    return worldGetModelChildren(modelID);
}

std::string LimonAPI::getModelAnimationName(uint32_t modelID) {
    return worldGetModelAnimationName(modelID);
}

bool LimonAPI::getModelAnimationFinished(uint32_t modelID) {
    return worldGetModelAnimationFinished(modelID);
}

float LimonAPI::getModelAnimationProgress(uint32_t modelID) {
    return worldGetModelAnimationProgress(modelID);
}

std::vector<std::string> LimonAPI::listModelAnimations(uint32_t modelID) {
    return worldListModelAnimations(modelID);
}

bool LimonAPI::setModelAnimation(uint32_t modelID, const std::string& animationName, bool isLooped) {
    return worldSetAnimationOfModel(modelID, animationName, isLooped);
}

bool LimonAPI::setModelAnimationWithBlend(uint32_t modelID, const std::string& animationName, bool isLooped, uint64_t blendTime) {
    return worldSetAnimationOfModelWithBlend(modelID, animationName, isLooped, blendTime);
}

bool LimonAPI::setModelAnimationSpeed(uint32_t modelID, float speed) {
    return worldSetModelAnimationSpeed(modelID, speed);
}

void LimonAPI::killPlayer() {
    worldKillPlayer();
}

bool LimonAPI::setObjectTranslate(uint32_t objectID, const LimonTypes::Vec4 &position) {
    return worldSetObjectTranslate(objectID, position);
}

bool LimonAPI::setObjectScale(uint32_t objectID, const LimonTypes::Vec4 &scale) {
    return worldSetObjectScale(objectID, scale);
}

bool LimonAPI::setObjectOrientation(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
    return worldSetObjectOrientation(objectID, orientation);
}

bool LimonAPI::addObjectTranslate(uint32_t objectID, const LimonTypes::Vec4 &position) {
    return worldAddObjectTranslate(objectID, position);
}

bool LimonAPI::addObjectScale(uint32_t objectID, const LimonTypes::Vec4 &scale) {
    return worldAddObjectScale(objectID, scale);
}

bool LimonAPI::addObjectOrientation(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
    return worldAddObjectOrientation(objectID, orientation);
}


bool LimonAPI::setObjectTemporary(uint32_t modelID, bool temporary) {
    return worldSetModelTemporary(modelID, temporary);
}

ProfileScope LimonAPI::profileScope(const std::string& name) {
    return ProfileScope(worldEndProfileZone, worldBeginProfileZone(name.c_str(), name.size()));
}

void LimonAPI::log(Logger::Subsystem subsystem, Logger::Level level, const std::string& text) {
    worldLog(subsystem, level, text);
}

uint32_t LimonAPI::drawDebugLine(const LimonTypes::Vec4& from, const LimonTypes::Vec4& to,
                                  const LimonTypes::Vec4& fromColor, const LimonTypes::Vec4& toColor,
                                  bool requireCameraTransform) {
    return worldDrawDebugLine(from, to, fromColor, toColor, requireCameraTransform);
}

bool LimonAPI::addToDebugLine(uint32_t bufferIndex,
                               const LimonTypes::Vec4& from, const LimonTypes::Vec4& to,
                               const LimonTypes::Vec4& fromColor, const LimonTypes::Vec4& toColor,
                               bool requireCameraTransform) {
    return worldAddToDebugLine(bufferIndex, from, to, fromColor, toColor, requireCameraTransform);
}

bool LimonAPI::clearDebugLines(uint32_t bufferIndex) {
    return worldClearDebugLines(bufferIndex);
}
