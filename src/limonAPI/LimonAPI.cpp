//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"

const OptionsUtil::Options * LimonAPI::getOptions() {
    return limonGetOptions();
}

uint32_t LimonAPI::animateModel(uint32_t modelID, uint32_t animationID, bool looped, const std::string *soundPath) {
    return worldAddAnimationToObject(modelID, animationID, looped, soundPath);
}

bool LimonAPI::generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index) {
    return worldGenerateEditorElementsForParameters(runParameters, index);
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

bool LimonAPI::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    return worldUpdateGuiText(guiTextID, newText);
}

uint32_t LimonAPI::removeGuiElement(uint32_t guiElementID) {
    return worldRemoveGuiElement(guiElementID);

}

std::vector<LimonTypes::GenericParameter> LimonAPI::getResultOfTrigger(uint32_t TriggerObjectID, uint32_t TriggerCodeID) {
    std::vector<LimonTypes::GenericParameter> results = worldGetResultOfTrigger(TriggerObjectID, TriggerCodeID);
    return results;
}

bool LimonAPI::removeObject(uint32_t objectID, const bool &removeChildren) {
    return worldRemoveObject(objectID, removeChildren);
}

bool LimonAPI::removeTriggerObject(uint32_t TriggerObjectID) {
    return worldRemoveTriggerObject(TriggerObjectID);
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

bool LimonAPI::attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath) {
    return worldAttachSoundToObjectAndPlay(objectWorldID, soundPath);
}
bool LimonAPI::detachSoundFromObject(uint32_t objectWorldID){
    return worldDetachSoundFromObject(objectWorldID);
}
uint32_t
LimonAPI::playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped) {
    return worldPlaySound(soundPath, position, positionRelative, looped);
}

bool LimonAPI::loadAndSwitchWorld(const std::string& worldFileName) {
    return limonLoadWorld(worldFileName);
}

bool LimonAPI::returnToWorld(const std::string &worldFileName) {
    return this->limonReturnOrLoadWorld(worldFileName);
}

bool LimonAPI::LoadAndRemove(const std::string &worldFileName) {
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

bool LimonAPI::interactWithAI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation) {
    return worldInteractWithAI(AIID, interactionInformation);
}

void LimonAPI::interactWithPlayer(std::vector<LimonTypes::GenericParameter> &input) {
    return this->worldInteractWithPlayer(input);
}

void LimonAPI::simulateInput(const InputStates& input) {
    this->worldSimulateInput(input);
}

bool LimonAPI::addLightTranslate(uint32_t lightID, const LimonTypes::Vec4 &position) {
    return worldAddLightTranslate(lightID, position);
}

bool LimonAPI::setLightColor(uint32_t lightID, const LimonTypes::Vec4 &color){
    return worldSetLightColor(lightID, color);
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

bool LimonAPI::setModelAnimation(uint32_t modelID, const std::string& animationName, bool isLooped) {
    return worldSetAnimationOfModel(modelID, animationName, isLooped);
}

bool LimonAPI::setModelAnimationWithBlend(uint32_t modelID, const std::string& animationName, bool isLooped, long blendTime) {
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
