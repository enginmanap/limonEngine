#include "WorldAPIAccessor.h"
#include "World.h"
#include "GameObjects/GUIText.h"
#include "GameObjects/GUIImage.h"
#include "GameObjects/GUIButton.h"
#include "GUI/GUITextBase.h"
#include "GUI/GUILayer.h"
#include "GameObjects/TriggerObject.h"
#include "GameObjects/Players/PhysicalPlayer.h"
#include "GameObjects/Model.h"
#include "GameObjects/Sound.h"
#include "Assets/Animations/AnimationCustom.h"
#include "Utils/GLMConverter.h"
#include "Utils/GLMUtils.h"
#include "GameObjects/Light.h"
#include "Graphics/GraphicsPipeline.h"
#include "Occlusion/VisibilityManager.h"
#include "Graphics/Particles/Emitter.h"

uint32_t WorldAPIAccessor::addAnimationToObjectWithSound(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad,
                                                         const std::string& soundToPlay) {
    World::AnimationStatus* as = new World::AnimationStatus;
    PhysicalRenderable* physicalPointer = nullptr;
    if(world->objects.find(modelID) != world->objects.end()) {
        as->object = world->objects[modelID];
        physicalPointer = world->objects[modelID];
        as->wasPhysical = true;
    } else if(world->guiElements.find(modelID) != world->guiElements.end()) {
        as->object = world->guiElements[modelID];
    } else {
        std::cerr << "add animation called for non existent object, skipping. " << std::endl;
        delete as;
        return 0;
    }
    if(world->loadedAnimations.size() <= animationID) {
        std::cerr << "add animation called for non existent animation, skipping. " << std::endl;
        delete as;
        return 0;
    }
    as->animationIndex = animationID;
    as->loop = looped;
    if(physicalPointer != nullptr) {
        as->wasKinematic = physicalPointer->getRigidBody()->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    as->startTime = world->gameTime;
    if(world->activeAnimations.count(as->object) != 0) {
        world->options->getLogger()->log(Logger::log_Subsystem_ANIMATION, Logger::log_level_WARN, "Model had custom animation, overriding.");
        as->wasKinematic = world->activeAnimations[as->object]->wasKinematic;
        if(world->activeAnimations[as->object]->loop) {
            as->originalTransformation = world->activeAnimations[as->object]->originalTransformation;
        } else {
            const AnimationCustom* oldAnimation = &world->loadedAnimations[world->activeAnimations[as->object]->animationIndex];
            float duration = oldAnimation->getDuration();
            oldAnimation->calculateTransform("", duration, *as->object->getTransformation());

            as->object->getTransformation()->getWorldTransform();
            glm::vec3 tempScale, tempTranslate;
            glm::quat tempOrientation;
            tempScale       = as->object->getTransformation()->getScale();
            tempTranslate   = as->object->getTransformation()->getTranslate();
            tempOrientation = as->object->getTransformation()->getOrientation();

            as->object->getTransformation()->removeParentTransform();
            as->object->getTransformation()->setTransformations(tempTranslate,
            tempScale,
            tempOrientation);
            as->object->setCustomAnimation(false);
            as->originalTransformation = *as->object->getTransformation();
        }
        delete world->activeAnimations[as->object];
    } else {
        as->originalTransformation = *as->object->getTransformation();
    }
    as->object->getTransformation()->setTransformations(glm::vec3(0.0f,0.0f,0.0f),
    glm::vec3(1.0f,1.0f,1.0f),
    glm::quat(1.0f,0.0f,0.0f, 0.0f));

    as->object->getTransformation()->setParentTransform(&as->originalTransformation);
    as->object->setCustomAnimation(true);
    if(physicalPointer != nullptr) {
        physicalPointer->getRigidBody()->setCollisionFlags(physicalPointer->getRigidBody()->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        physicalPointer->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);
    }
    if(startOnLoad) {
        world->onLoadAnimations.insert(as->object);
        as->startTime = 0;
    } else {
        as->startTime = world->gameTime;
    }

    if(!soundToPlay.empty()) {
        as->sound = std::make_unique<Sound>(world->getNextObjectID(), world->assetManager, soundToPlay);
        as->sound->setLoop(looped);
        as->sound->setWorldPosition(as->object->getTransformation()->getTranslate());
        as->sound->play();
    }
    world->activeAnimations[as->object] = as;
    return modelID;
}

uint32_t WorldAPIAccessor::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &name,
                                      const std::string &text, const glm::vec3 &color,
                                      const glm::vec2 &position, float rotation) {
    GUIText* tr = new GUIText(world->graphicsWrapper, world->getNextObjectID(), name, world->fontManager.getFont(fontFilePath, fontSize),
                              text, color);
    glm::vec2 screenPosition;
    screenPosition.x = position.x * world->options->getScreenWidth();
    screenPosition.y = position.y * world->options->getScreenHeight();

    tr->set2dWorldTransform(screenPosition, rotation);
    world->guiElements[tr->getWorldObjectID()] = tr;
    world->apiGUILayer->addGuiElement(tr);
    return tr->getWorldObjectID();
}

uint32_t WorldAPIAccessor::addGuiImageAPI(const std::string &imageFilePath, const std::string &name,
                                          const LimonTypes::Vec2 &position, const LimonTypes::Vec2 &scale, float rotation) {
    GUIImage* guiImage = new GUIImage(world->getNextObjectID(), world->options, world->assetManager, name, imageFilePath);

    glm::vec2 screenPosition;
    screenPosition.x = position.x * world->options->getScreenWidth();
    screenPosition.y = position.y * world->options->getScreenHeight();

    glm::vec2 screenScale;
    screenScale.x = scale.x * world->options->getScreenWidth() / 2;
    screenScale.y = scale.y * world->options->getScreenHeight() / 2;

    guiImage->setScale(screenScale);
    guiImage->set2dWorldTransform(screenPosition, rotation);
    world->guiElements[guiImage->getWorldObjectID()] = guiImage;
    world->apiGUILayer->addGuiElement(guiImage);
    return guiImage->getWorldObjectID();
}

bool WorldAPIAccessor::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    if(world->guiElements.find(guiTextID) != world->guiElements.end()) {
        dynamic_cast<GUITextBase*>(world->guiElements[guiTextID])->updateText(newText);
        return true;
    }
    return false;
}

bool WorldAPIAccessor::removeGuiElement(uint32_t guiElementID) {
    if(world->guiElements.find(guiElementID) != world->guiElements.end()) {
        GUIRenderable* temp = world->guiElements[guiElementID];
        if(world->hoveringButton != nullptr && world->hoveringButton->getWorldObjectID() == guiElementID) {
            world->hoveringButton = nullptr;
        }
        if(world->activeAnimations.find(temp) != world->activeAnimations.end()) {
            delete world->activeAnimations[temp];
            world->activeAnimations.erase(temp);
        }
        world->onLoadAnimations.erase(temp);

        world->guiElements.erase(guiElementID);
        delete temp;
        return true;
    }
    return false;
}

uint32_t WorldAPIAccessor::addModelApi(const std::string &modelFilePath, float modelWeight, bool physical,
                                       const glm::vec3 &position, const glm::vec3 &scale, const glm::quat &orientation) {
    uint32_t objectID = world->getNextObjectID();

    Model* newModel = new Model(objectID, world->assetManager, modelWeight, modelFilePath, !physical);
    newModel->getTransformation()->setTransformations(position, scale, orientation);

    world->addModelToWorld(newModel);

    return objectID;
}

bool WorldAPIAccessor::setModelTemporaryAPI(uint32_t modelID, bool temporary) {
    Model* model = world->findModelByID(modelID);
    if(model == nullptr) {
        return false;
    }
    model->setTemporary(temporary);
    return true;
}

bool WorldAPIAccessor::attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID) {
    if(objectID == objectToAttachToID) {
        return false;
    }

    Attachable* objectToAttach = world->findAttachableByID(objectID);
    if(objectToAttach == nullptr) {
        return false;
    }

    Attachable* objectToAttachTo = world->findAttachableByID(objectToAttachToID);
    if(objectToAttachTo == nullptr) {
        return false;
    }

    PhysicalRenderable* physicalParent = dynamic_cast<PhysicalRenderable*>(objectToAttachTo);
    if(physicalParent != nullptr) {
        objectToAttach->getTransformation()->addTranslate(-1 * physicalParent->getCenterOffset());
    }

    objectToAttach->attachTo(objectToAttachTo);
    return true;
}

bool WorldAPIAccessor::removeObject(uint32_t objectID, const bool &removeChildren) {
    Model* modelToRemove = world->findModelByID(objectID);
    if(modelToRemove == nullptr) {
        return false;
    }
    world->clearWorldRefsBeforeAttachment(modelToRemove, removeChildren);
    if(removeChildren) {
        // clearWorldRefsBeforeAttachment already handles child removal recursively
    }
    delete modelToRemove;
    return true;
}

bool WorldAPIAccessor::removeTriggerObject(uint32_t triggerobjectID) {
    if(world->triggers.find(triggerobjectID) != world->triggers.end()) {
        TriggerObject* objectToRemove = world->triggers[triggerobjectID];
        world->dynamicsWorld->removeCollisionObject(objectToRemove->getGhostObject());
        delete world->triggers[triggerobjectID];
        world->triggers.erase(triggerobjectID);
        world->unusedIDs.push(triggerobjectID);
        return true;
    }
    return false;
}

std::vector<LimonTypes::GenericParameter> WorldAPIAccessor::getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID) {
    std::vector<LimonTypes::GenericParameter> result;
    if(world->triggers.find(triggerObjectID) != world->triggers.end()) {
        TriggerObject* to = world->triggers[triggerObjectID];
        result = to->getResultOfCode(triggerCodeID);
    }
    return result;
}

bool WorldAPIAccessor::disconnectObjectFromPhysics(uint32_t objectWorldID) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    Model* model = dynamic_cast<Model*>(world->objects.at(objectWorldID));
    if(model == nullptr) {
        return false;
    }
    model->disconnectFromPhysicsWorld(world->dynamicsWorld);
    return true;
}

bool WorldAPIAccessor::reconnectObjectToPhysics(uint32_t objectWorldID) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    Model* model = dynamic_cast<Model*>(world->objects.at(objectWorldID));
    if(model == nullptr) {
        return false;
    }
    model->connectToPhysicsWorld(world->dynamicsWorld, World::COLLIDE_MODELS, World::COLLIDE_MODELS | World::COLLIDE_PLAYER | World::COLLIDE_EVERYTHING);
    return true;
}

bool WorldAPIAccessor::disconnectObjectFromPhysicsRequest(uint32_t objectWorldID) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    Model* model = dynamic_cast<Model*>(world->objects.at(objectWorldID));
    if(model == nullptr) {
        return false;
    }
    world->disconnectedModels.insert(model->getWorldObjectID());
    return true;
}

bool WorldAPIAccessor::reconnectObjectToPhysicsRequest(uint32_t objectWorldID) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    Model* model = dynamic_cast<Model*>(world->objects.at(objectWorldID));
    if(model == nullptr) {
        return false;
    }
    world->disconnectedModels.erase(model->getWorldObjectID());
    return true;
}

bool WorldAPIAccessor::applyForceAPI(uint32_t objectID, const LimonTypes::Vec4 &forcePosition, const LimonTypes::Vec4 &forceAmount) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getRigidBody()->activate(true);
    btVector3 forcePositionRelative = GLMConverter::LimonToBlt(forcePosition) - model->getRigidBody()->getCenterOfMassTransform().getOrigin();
    model->getRigidBody()->applyForce(GLMConverter::LimonToBlt(forceAmount), forcePositionRelative);
    return true;
}

bool WorldAPIAccessor::applyForceToPlayerAPI(const LimonTypes::Vec4 &forceAmount) {
    if(world->physicalPlayer == nullptr) {
        return false;
    }
    world->physicalPlayer->getRigidBody()->activate(true);
    world->physicalPlayer->getRigidBody()->applyForce(GLMConverter::LimonToBlt(forceAmount), btVector3(0,0,0));
    return true;
}

std::vector<LimonTypes::GenericParameter> WorldAPIAccessor::rayCastToCursorAPI() const {
    std::vector<LimonTypes::GenericParameter> result;
    glm::vec3 position, normal;
    GameObject* gameObject = world->getPointedObject(World::COLLIDE_EVERYTHING, World::COLLIDE_MODELS | World::COLLIDE_EVERYTHING, &position, &normal);

    if(gameObject == nullptr) {
        return result;
    }
    LimonTypes::GenericParameter objectIDParam;
    objectIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    objectIDParam.value.longValue = gameObject->getWorldObjectID();
    result.push_back(objectIDParam);

    LimonTypes::GenericParameter positionParam;
    positionParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    positionParam.value.vectorValue.x = position.x;
    positionParam.value.vectorValue.y = position.y;
    positionParam.value.vectorValue.z = position.z;
    result.push_back(positionParam);

    LimonTypes::GenericParameter normalParam;
    normalParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    normalParam.value.vectorValue.x = normal.x;
    normalParam.value.vectorValue.y = normal.y;
    normalParam.value.vectorValue.z = normal.z;
    result.push_back(normalParam);

    if(gameObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
        Model* foundModel = dynamic_cast<Model*>(gameObject);
        if(foundModel != nullptr && foundModel->getAIID() != 0) {
            LimonTypes::GenericParameter aiIDParam;
            aiIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
            aiIDParam.value.longValue = foundModel->getAIID();
            result.push_back(aiIDParam);
        }
    }

    return result;
}

std::vector<LimonTypes::GenericParameter> WorldAPIAccessor::rayCastAPI(const LimonTypes::Vec4 &start, const LimonTypes::Vec4 &direction) const {
    std::vector<LimonTypes::GenericParameter> result;
    glm::vec3 position, normal;
    glm::vec3 startGLM = GLMConverter::LimonToGLMV3(start);
    glm::vec3 directionGLM = GLMConverter::LimonToGLMV3(direction);
    GameObject* gameObject = world->rayCastClosest(startGLM, directionGLM, World::COLLIDE_EVERYTHING, World::COLLIDE_MODELS | World::COLLIDE_EVERYTHING, &position, &normal);

    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 color2 = glm::vec3(0.0f, 0.0f, 0.0f);
    static uint32_t drawLineBufferId = 0;

    OptionsUtil::Options::Option<bool> debugDrawLinesOption = world->options->getOption<bool>(HASH("DebugDrawLines"));
    bool debugDrawLines = debugDrawLinesOption.getOrDefault(false);

    if(debugDrawLines) {
        if(drawLineBufferId != 0) {
            world->options->getLogger()->clearLineBuffer(drawLineBufferId);
        }
        drawLineBufferId = world->options->getLogger()->drawLine(startGLM, startGLM + directionGLM, color, color2, true);
    }

    if(gameObject == nullptr) {
        return result;
    }
    LimonTypes::GenericParameter objectIDParam;
    objectIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    objectIDParam.value.longValue = gameObject->getWorldObjectID();
    objectIDParam.description = "objectID for what we hit";
    objectIDParam.isSet = true;
    result.push_back(objectIDParam);

    LimonTypes::GenericParameter positionParam;
    positionParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    positionParam.value.vectorValue.x = position.x;
    positionParam.value.vectorValue.y = position.y;
    positionParam.value.vectorValue.z = position.z;
    positionParam.description = "hit coordinates";
    positionParam.isSet = true;
    result.push_back(positionParam);

    LimonTypes::GenericParameter normalParam;
    normalParam.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    normalParam.value.vectorValue.x = normal.x;
    normalParam.value.vectorValue.y = normal.y;
    normalParam.value.vectorValue.z = normal.z;
    normalParam.description = "hit normal";
    normalParam.isSet = true;
    result.push_back(normalParam);

    if(gameObject->getTypeID() == GameObject::ObjectTypes::MODEL) {
        Model* foundModel = dynamic_cast<Model*>(gameObject);
        if(foundModel != nullptr && foundModel->getAIID() != 0) {
            LimonTypes::GenericParameter aiIDParam;
            aiIDParam.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
            aiIDParam.value.longValue = foundModel->getAIID();
            aiIDParam.description = "AI ID for what we hit";
            aiIDParam.isSet = true;
            result.push_back(aiIDParam);
        }
    }

    return result;
}

std::vector<LimonTypes::GenericParameter> WorldAPIAccessor::getObjectTransformationAPI(uint32_t objectID) const {
    std::vector<LimonTypes::GenericParameter> result;
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return result;
    }
    const Transformation* transformation = model->getTransformation();

    LimonTypes::GenericParameter translate;
    translate.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    translate.value.vectorValue = GLMConverter::GLMToLimon(transformation->getTranslate());
    result.push_back(translate);

    LimonTypes::GenericParameter scale;
    scale.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    scale.value.vectorValue = GLMConverter::GLMToLimon(transformation->getScale());
    result.push_back(scale);

    LimonTypes::GenericParameter orientation;
    orientation.valueType = LimonTypes::GenericParameter::ValueTypes::VEC4;
    orientation.value.vectorValue.x = transformation->getOrientation().x;
    orientation.value.vectorValue.y = transformation->getOrientation().y;
    orientation.value.vectorValue.z = transformation->getOrientation().z;
    orientation.value.vectorValue.w = transformation->getOrientation().w;
    result.push_back(orientation);

    return result;
}

std::vector<LimonTypes::GenericParameter> WorldAPIAccessor::getObjectTransformationMatrixAPI(uint32_t objectID) const {
    std::vector<LimonTypes::GenericParameter> result;
    if(world->objects.find(objectID) == world->objects.end()) {
        return result;
    }

    LimonTypes::GenericParameter transform;
    transform.valueType = LimonTypes::GenericParameter::ValueTypes::MAT4;
    transform.value.matrixValue = GLMConverter::GLMToLimon(world->objects.at(objectID)->getTransformation()->getWorldTransform());

    result.push_back(transform);
    return result;
}

bool WorldAPIAccessor::setObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4 &position) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getTransformation()->setTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
    return true;
}

bool WorldAPIAccessor::setObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4 &scale) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getTransformation()->setScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
    return true;
}

bool WorldAPIAccessor::setObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    glm::quat orientationQuat(orientation.w, orientation.x, orientation.y, orientation.z);
    model->getTransformation()->setOrientation(orientationQuat);
    return true;
}

bool WorldAPIAccessor::addObjectTranslateAPI(uint32_t objectID, const LimonTypes::Vec4 &position) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getTransformation()->addTranslate(glm::vec3(GLMConverter::LimonToGLM(position)));
    return true;
}

bool WorldAPIAccessor::addObjectScaleAPI(uint32_t objectID, const LimonTypes::Vec4 &scale) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    model->getTransformation()->addScale(glm::vec3(GLMConverter::LimonToGLM(scale)));
    return true;
}

bool WorldAPIAccessor::addObjectOrientationAPI(uint32_t objectID, const LimonTypes::Vec4 &orientation) {
    Model* model = world->findModelByID(objectID);
    if(model == nullptr) {
        return false;
    }
    glm::quat orientationQuat(orientation.w, orientation.x, orientation.y, orientation.z);
    model->getTransformation()->addOrientation(orientationQuat);
    return true;
}

bool WorldAPIAccessor::interactWithAIAPI(uint32_t AIID, std::vector<LimonTypes::GenericParameter> &interactionInformation) const {
    if(world->actors.find(AIID) == world->actors.end()) {
        return false;
    }
    return world->actors.at(AIID)->interaction(interactionInformation);
}

void WorldAPIAccessor::interactWithPlayerAPI(std::vector<LimonTypes::GenericParameter> &interactionInformation) const {
    if(world->physicalPlayer != nullptr) {
        world->physicalPlayer->interact(world->apiInstance, interactionInformation);
    }
}

void WorldAPIAccessor::simulateInputAPI(InputStates input) {
    if(world->physicalPlayer != nullptr) {
        input.setSimulated(true);
        world->physicalPlayer->processInput(input, world->gameTime);
    }
}

long WorldAPIAccessor::addTimedEventAPI(uint64_t waitTime, bool useWallTime,
                                        std::function<void(const std::vector<LimonTypes::GenericParameter> &)> methodToCall,
                                        std::vector<LimonTypes::GenericParameter> parameters) {
    long handleId = world->timedEventHandleIndex++;
    uint64_t callTime = useWallTime ? world->wallTime : world->gameTime;
    callTime += waitTime;
    world->timedEvents.emplace(handleId, callTime, useWallTime, std::move(methodToCall), std::move(parameters));
    return handleId;
}

bool WorldAPIAccessor::cancelTimedEventAPI(long handleId) {
    std::vector<World::TimedEvent>& container = Container(world->timedEvents);
    for(size_t i = 0; i < container.size(); ++i) {
        if(container[i].handleId == handleId) {
            container[i].active = false;
            return true;
        }
    }
    return false;
}

void WorldAPIAccessor::getPlayerPositionAPI(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) {
    world->currentPlayer->getCameraVariables(position, center, up, right);
}

uint32_t WorldAPIAccessor::getPlayerAttachedModelAPI() {
    if(world->startingPlayer.attachedModel != nullptr) {
        return world->startingPlayer.attachedModel->getWorldObjectID();
    }
    return 0;
}

std::vector<uint32_t> WorldAPIAccessor::getModelChildrenAPI(uint32_t modelID) {
    std::vector<uint32_t> result;
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        std::vector<Attachable*> children = model->getChildren();
        for(auto child = children.begin(); child != children.end(); ++child) {
            Model* childModel = dynamic_cast<Model*>(*child);
            if(childModel != nullptr) {
                result.push_back(childModel->getWorldObjectID());
            }
        }
    }
    return result;
}

LimonTypes::Vec4 WorldAPIAccessor::getPlayerModelOffsetAPI() {
    if(world->startingPlayer.attachedModel != nullptr) {
        if(world->physicalPlayer != nullptr) {
            return GLMConverter::GLMToLimon(world->physicalPlayer->getAttachedModelOffset());
        }
    }
    return LimonTypes::Vec4(0, 0, 0);
}

bool WorldAPIAccessor::setPlayerModelOffsetAPI(LimonTypes::Vec4 newOffset) {
    if(world->startingPlayer.attachedModel != nullptr) {
        if(world->physicalPlayer != nullptr) {
            world->physicalPlayer->setAttachedModelOffset(glm::vec3(GLMConverter::LimonToGLM(newOffset)));
            return true;
        }
    }
    return false;
}

void WorldAPIAccessor::killPlayerAPI() {
    world->currentPlayer->setDead();
}

std::string WorldAPIAccessor::getModelAnimationNameAPI(uint32_t modelID) {
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        return model->getAnimationName();
    }
    return "";
}

bool WorldAPIAccessor::getModelAnimationFinishedAPI(uint32_t modelID) {
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        return model->isAnimationFinished();
    }
    return false;
}

bool WorldAPIAccessor::setModelAnimationAPI(uint32_t modelID, const std::string &animationName, bool isLooped) {
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimation(animationName, isLooped);
        return true;
    }
    return false;
}

bool WorldAPIAccessor::setModelAnimationWithBlendAPI(uint32_t modelID, const std::string &animationName, bool isLooped, uint64_t blendTime) {
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimationWithBlend(animationName, isLooped, blendTime);
        return true;
    }
    return false;
}

bool WorldAPIAccessor::setModelAnimationSpeedAPI(uint32_t modelID, float speed) {
    if(speed < 0.001f) {
        return false;
    }
    Model* model = world->findModelByID(modelID);
    if(model != nullptr) {
        model->setAnimationTimeScale(speed);
        return true;
    }
    return false;
}

bool WorldAPIAccessor::attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath, bool looped) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    auto sound = std::make_unique<Sound>(world->getNextObjectID(), world->assetManager, soundPath);
    sound->setLoop(looped);
    world->objects[objectWorldID]->setSoundAttachmentAndPlay(std::move(sound));
    return true;
}

bool WorldAPIAccessor::detachSoundFromObject(uint32_t objectWorldID) {
    if(world->objects.find(objectWorldID) == world->objects.end()) {
        return false;
    }
    world->objects[objectWorldID]->detachSound();
    return true;
}

uint32_t WorldAPIAccessor::playSound(const std::string &soundPath, const glm::vec3 &position, bool positionRelative, bool looped) {
    std::unique_ptr<Sound> sound = std::make_unique<Sound>(world->getNextObjectID(), world->assetManager, soundPath);
    sound->setLoop(looped);
    sound->setWorldPosition(position, positionRelative);
    sound->play();
    uint32_t soundID = sound->getWorldObjectID();
    world->sounds[soundID] = std::move(sound);
    return soundID;
}

uint32_t WorldAPIAccessor::addLightAPI(uint32_t lightType, const LimonTypes::Vec4 &position, const LimonTypes::Vec4 &color) {
    Light::LightTypes type;
    switch(lightType) {
        case 1: type = Light::LightTypes::DIRECTIONAL; break;
        case 2: type = Light::LightTypes::POINT; break;
        default: return 0;
    }
    uint32_t lightID = world->getNextObjectID();
    Light* light = new Light(world->graphicsWrapper, lightID, type,
                             GLMConverter::LimonToGLMV3(position),
                             GLMConverter::LimonToGLMV3(color));
    world->addLight(light);
    return lightID;
}

bool WorldAPIAccessor::removeLightAPI(uint32_t lightID) {
    for (auto iterator = world->lights.begin(); iterator != world->lights.end(); ++iterator) {
        if ((*iterator)->getWorldObjectID() == lightID) {
            world->unusedIDs.push(lightID);
            const std::vector<Camera*>& cameras = (*iterator)->getCameras();
            for (auto camera : cameras) {
                for (auto entry : world->visibilityManager->visibilityThreadPool) {
                    if (entry.first->camera == camera) {
                        entry.first->running = false;
                        world->visibilityManager->wakeThreadsCondition.signalWaiting();
                        SDL_WaitThread(entry.second, nullptr);
                        auto visRequest = entry.first;
                        world->visibilityManager->visibilityThreadPool.erase(visRequest);
                        delete visRequest;
                        break;
                    }
                }
                world->visibilityManager->getCullingResults().erase(camera);
            }
            if ((*iterator)->getLightType() == Light::LightTypes::DIRECTIONAL) {
                world->directionalLightIndex = -1;
            }
            delete *iterator;
            world->lights.erase(iterator);
            world->updateActiveLights(true);
            return true;
        }
    }
    return false;
}

bool WorldAPIAccessor::addLightTranslateAPI(uint32_t lightID, const LimonTypes::Vec4 &position) {
    Light* light = nullptr;
    for(size_t i = 0; i < world->lights.size(); ++i) {
        if(world->lights[i]->getWorldObjectID() == lightID) {
            light = world->lights[i];
            break;
        }
    }
    if(light == nullptr) {
        return false;
    }
    light->setPosition(light->getPosition() + glm::vec3(GLMConverter::LimonToGLM(position)), world->playerCamera);
    return true;
}

bool WorldAPIAccessor::setLightColorAPI(uint32_t lightID, const LimonTypes::Vec4 &color) {
    Light* light = nullptr;
    for(size_t i = 0; i < world->lights.size(); ++i) {
        if(world->lights[i]->getWorldObjectID() == lightID) {
            light = world->lights[i];
            break;
        }
    }
    if(light == nullptr) {
        return false;
    }
    light->setColor(glm::vec3(GLMConverter::LimonToGLM(color)));
    return true;
}

bool WorldAPIAccessor::changeRenderPipeline(const std::string &pipelineFileName) {
    std::unique_ptr<GraphicsPipeline> newPipeline = GraphicsPipeline::deserialize(pipelineFileName, world->graphicsWrapper, world->assetManager, world->options, world->buildRenderMethods());
    if(newPipeline != nullptr) {
        world->renderPipeline = std::move(newPipeline);
        world->setupRenderForPipeline();
        world->visibilityManager->onPipelineChange();
        return true;
    }
    return false;
}

bool WorldAPIAccessor::enableParticleEmitter(uint32_t particleEmitterId) {
    auto emitterIt = world->emitters.find(particleEmitterId);
    if(emitterIt == world->emitters.end()) {
        return false;
    }
    emitterIt->second->setEnabled(true);
    return true;
}

bool WorldAPIAccessor::disableParticleEmitter(uint32_t particleEmitterId) {
    auto emitterIt = world->emitters.find(particleEmitterId);
    if(emitterIt == world->emitters.end()) {
        return false;
    }
    emitterIt->second->setEnabled(false);
    return true;
}

uint32_t WorldAPIAccessor::addParticleEmitter(const std::string &name, const std::string &textureFile,
                                               const LimonTypes::Vec4 &startPosition,
                                               const LimonTypes::Vec4 &maxStartDistances,
                                               const LimonTypes::Vec2 &size,
                                               uint32_t count, uint32_t lifeTime,
                                               float particlePerMs, bool continuouslyEmit) {
    if(count > 10000) {
        std::cerr << "Can't create particle emitter with more than 10000 particles" << std::endl;
        return 0;
    }
    if(count < 1) {
        std::cerr << "Can't create particle emitter with 0 particles" << std::endl;
        return 0;
    }
    std::shared_ptr<Emitter> newEmitter;
    if(particlePerMs <= 0) {
        newEmitter = std::make_shared<Emitter>(world->getNextObjectID(), name,
                                               world->assetManager, textureFile,
                                               GLMConverter::LimonToGLMV3(startPosition),
                                               GLMConverter::LimonToGLMV3(maxStartDistances),
                                               GLMConverter::LimonToGLM(size),
                                               count, lifeTime);
    } else {
        newEmitter = std::make_shared<Emitter>(world->getNextObjectID(), name,
                                               world->assetManager, textureFile,
                                               GLMConverter::LimonToGLMV3(startPosition),
                                               GLMConverter::LimonToGLMV3(maxStartDistances),
                                               GLMConverter::LimonToGLM(size), count,
                                               lifeTime, particlePerMs);
    }
    newEmitter->setContinuousEmit(continuouslyEmit);
    world->emitters[newEmitter->getWorldObjectID()] = newEmitter;
    return newEmitter->getWorldObjectID();
}

bool WorldAPIAccessor::removeParticleEmitter(uint32_t emitterID) {
    auto emitterIT = world->emitters.find(emitterID);
    if(emitterIT == world->emitters.end()) {
        return false;
    }
    world->emitters.erase(emitterIT);
    return true;
}

bool WorldAPIAccessor::setEmitterParticleSpeed(uint32_t emitterID, const LimonTypes::Vec4 &speedMultiplier, const LimonTypes::Vec4 &speedOffset) {
    auto emitterIT = world->emitters.find(emitterID);
    if(emitterIT == world->emitters.end()) {
        return false;
    }
    emitterIT->second->setSpeedMultiplier(GLMConverter::LimonToGLMV3(speedMultiplier));
    emitterIT->second->setSpeedOffset(GLMConverter::LimonToGLMV3(speedOffset));
    return true;
}

bool WorldAPIAccessor::setEmitterParticleGravity(uint32_t emitterID, const LimonTypes::Vec4 &gravity) {
    auto emitterIT = world->emitters.find(emitterID);
    if(emitterIT == world->emitters.end()) {
        return false;
    }
    emitterIT->second->setGravity(GLMConverter::LimonToGLMV3(gravity));
    return true;
}
