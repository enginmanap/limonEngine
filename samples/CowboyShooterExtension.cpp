//
// Created by engin on 26.12.2018.
//

#include "CowboyShooterExtension.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "API/LimonConverter.h"

void CowboyShooterExtension::processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                                          long time) {
    this->lastInputTime = time;
    if(inputState.isSimulated()) {
        return;
    }

    this->latestPlayerInformation = playerInformation;
    if (playerAttachedModelID == 0) {
        return;
    }
    currentAnimationName = limonAPI->getModelAnimationName(playerAttachedModelID);
    currentAnimationFinished = limonAPI->getModelAnimationFinished(playerAttachedModelID);

    if(hitReaction) {
        processHitReaction();
    }

    if(inputState.getInputEvents(InputStates::Inputs::MOUSE_BUTTON_LEFT) && inputState.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
        transitionState = State::SHOOTING;
    } else {
        if(!inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_BACKWARD) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_RIGHT)) {
            //standing still
            transitionState = State::IDLE;
        } else {
            if (inputState.getInputStatus(InputStates::Inputs::RUN)) {
                transitionState = State::RUNNING;
            } else {
                transitionState = State::WALKING;

            }
        }
    }

    if(inputState.getInputEvents(InputStates::Inputs::NUMBER_2)) {
        transitionGun = Gun::RIFLE;
    }
    if(inputState.getInputEvents(InputStates::Inputs::NUMBER_1)) {
        transitionGun = Gun::PISTOL;
    }

    switch (transitionState) {
        case State::WALKING:  walkingTransition();  break;
        case State::RUNNING:  runningTransition();  break;
        case State::IDLE:     idleTransition();     break;
        case State::SHOOTING: shootingTransition(); break;
    }
}

void CowboyShooterExtension::walkingTransition() {
    bool transitionValidate = false;
    switch(currentState) {
        case State::WALKING:
            if(changeGuns()) {
                transitionValidate = true;
            };
        break;
        case State::IDLE:
        case State::RUNNING:
            transitionValidate = true;
        break;
        case State::SHOOTING:
            if(currentAnimationFinished) {
                limonAPI->setModelAnimationSpeed(playerAttachedModelID, 1.0);
                transitionValidate = true;
            } else {
                //if shooting animation is playing, we do nothing. state transition fails.
            }
        break;
    }

    if(transitionValidate) {
        changeGuns();
        currentState = State::WALKING;
        switch (currentGun) {
            case Gun::PISTOL:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Pistol Walk|", true);
                break;
            case Gun::RIFLE:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Rifle Walk|", true);
                break;
        }
    }
}

void CowboyShooterExtension::runningTransition() {
    bool transitionValidate = false;
    switch(currentState) {
        case State::RUNNING:
            if(changeGuns()) {
                transitionValidate = true;
            };
            break;
        case State::IDLE:
        case State::WALKING:
            transitionValidate = true;
            break;
        case State::SHOOTING:
            if(currentAnimationFinished) {
                limonAPI->setModelAnimationSpeed(playerAttachedModelID, 1.0);
                transitionValidate = true;
            } else {
                //if shooting animation is playing, we do nothing. state transition fails.
            }
            break;
    }

    if(transitionValidate) {
        changeGuns();
        currentState = State::RUNNING;
        switch (currentGun) {
            case Gun::PISTOL:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Pistol Run|", true);
                break;
            case Gun::RIFLE:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Rifle Run|", true);
                break;
        }
    }
}

void CowboyShooterExtension::idleTransition() {
    bool transitionValidate = false;
    switch(currentState) {
        case State::IDLE:
            if(changeGuns()) {
                transitionValidate = true;
            };
            break;
        case State::RUNNING:
        case State::WALKING:
            transitionValidate = true;
            break;
        case State::SHOOTING:
            if(currentAnimationFinished) {
                limonAPI->setModelAnimationSpeed(playerAttachedModelID, 1.0);
                transitionValidate = true;
            } else {
                //if shooting animation is playing, we do nothing. state transition fails.
            }
            break;
    }

    if(transitionValidate) {
        changeGuns();
        currentState = State::IDLE;
        switch (currentGun) {
            case Gun::PISTOL:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Pistol Idle|", true);
                break;
            case Gun::RIFLE:
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Rifle Idle|", true);
                break;
        }
    }
}

void CowboyShooterExtension::shootingTransition() {
    if(currentState == State::SHOOTING) {
        return;
    }

    //SHOOTING has priority over all other states, so no need to check state
    /*************** Set animation and play sound ************/
    limonAPI->setModelAnimationSpeed(playerAttachedModelID, 1.5);
    switch(currentGun) {
        case Gun::PISTOL:
            limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Shooting|", false, 50);
            limonAPI->playSound("./Data/Sounds/guns/pistol3.wav", glm::vec3(0, 0, 0), true, false);
            break;
        case Gun::RIFLE:
            limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Shoot Rifle|", false, 50);
            limonAPI->playSound("./Data/Sounds/guns/rifle.wav", glm::vec3(0, 0, 0), true, false);
            break;
    }

    /*************** Set animation and play sound ************/

    /*************** Create muzzle flash *********************/
    glm::vec3 scale(0.25f, 0.25f, 0.25f);//it is actually 0,1 * 1/baseScale
    uint32_t muzzleFlashObjectID = limonAPI->addObject("./Data/Models/Muzzle/Muzzle.obj", 0, false, currentMuzzleFlashOffset, scale, direction);
    bool isAttached = limonAPI->attachObjectToObject(muzzleFlashObjectID, playerAttachedPistolID);
    if(!isAttached) {
        std::cerr << "attachment failed!" << std::endl;
    }

    /*************** Create muzzle flash *********************/

    /*************** Set timed event for muzzle flash removal *********************/
    std::vector<LimonAPI::ParameterRequest> removeParameters;
    LimonAPI::ParameterRequest removeID;
    removeID.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    removeID.value.longValue = (long)muzzleFlashObjectID;
    removeParameters.push_back(removeID);
    limonAPI->addTimedEvent(250, std::bind(&CowboyShooterExtension::removeMuzzleFlash, this, std::placeholders::_1), removeParameters);
    /*************** Set timed event for muzzle flash removal *********************/


    /*************** Check hit *********************/
    std::vector<LimonAPI::ParameterRequest>rayResult = limonAPI->rayCastToCursor();

    if(rayResult.size() > 0 ) {// we hit something
        if(rayResult.size() == 4) { // 4 parameter input means we hit an AI actor

            /*************** Inform AI that it got hit *********************/
            LimonAPI::ParameterRequest parameterRequest;
            parameterRequest.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
            strncpy(parameterRequest.value.stringValue, "GOT_HIT", sizeof(parameterRequest.value.stringValue)-1);
            std::vector<LimonAPI::ParameterRequest> prList;
            prList.push_back(parameterRequest);
            limonAPI->interactWithAI(rayResult[3].value.longValue, prList);
            /*************** Inform AI that it got hit *********************/
        } else { // we hit something, but it doesn't have AI, just put bullet hole


            glm::vec3 scale(0.2f, 0.2f, 0.2f);
            glm::vec3 hitPos(rayResult[1].value.vectorValue.x, rayResult[1].value.vectorValue.y, rayResult[1].value.vectorValue.z);//hit position is returned in parameter 1
            glm::vec3 hitNormal(rayResult[2].value.vectorValue.x, rayResult[2].value.vectorValue.y, rayResult[2].value.vectorValue.z);//hit normal is returned in parameter 2

            hitPos +=hitNormal * 0.002f; //move hit position a bit towards the normal to prevent zfight
            glm::quat orientation;
            if(hitNormal.x < 0.001 && hitNormal.x > -0.001 &&
               hitNormal.y < 1.001 && hitNormal.y >  0.999 &&
               hitNormal.z < 0.001 && hitNormal.z > -0.001) {
                //means the normal is up
                orientation = glm::quat(0.707f, -0.707f, 0.0f, 0.0f);
            } else {
#if (GLM_VERSION_MAJOR !=0 || GLM_VERSION_MINOR > 9 || (GLM_VERSION_MINOR == 9 && GLM_VERSION_PATCH >= 9))
                orientation =  glm::quatLookAt(hitNormal * -1.0f, glm::vec3(0,1,0));
#else
                glm::mat3 Result;

                Result[2] = -1.0f * hitNormal;//this -1 shouldn't be needed
                Result[0] = glm::normalize(cross(glm::vec3(0,1,0), Result[2]));
                Result[1] = glm::cross(Result[2], Result[0]);

                orientation = glm::quat_cast(Result);
#endif
            }

            std::vector<LimonAPI::ParameterRequest>modelTransformationMat = limonAPI->getObjectTransformationMatrix(rayResult[0].value.longValue);
            if(modelTransformationMat.size() == 0) {
                std::cerr << "Hit an object, but its ID "<< (uint32_t)rayResult[0].value.longValue << " is invalid!" << std::endl;

                uint32_t bulletHoleID = limonAPI->addObject("./Data/Models/BulletHole/BulletHole.obj", 0, false, hitPos, scale, orientation);//add with default values
                limonAPI->setObjectTemporary(bulletHoleID, true);//don't save bullet holes when saving the map
            } else {
                //at this point, we will attach the bullet hole to the object, to do so, we need to update the scale, translate and orientation

                // using the dirty method
                glm::mat4 currentDesiredTransformMatrix = glm::translate(glm::mat4(1.0f), hitPos) * glm::mat4_cast(orientation) *
                                                          glm::scale(glm::mat4(1.0f), scale);

                glm::mat4 currentParentTransform = LimonConverter::LimonToGLM(modelTransformationMat[0].value.matrixValue);

                //what is needed to get parent to child?
                // currentDesired = currentParent * x
                // x = 'currentParent * currentDesired

                glm::mat4 delta = glm::inverse(currentParentTransform) * currentDesiredTransformMatrix;
                glm::vec3 temp1;//these are not used
                glm::vec4 temp2;
                glm::vec3 posdif;
                glm::vec3 scaledif;
                glm::quat orientDif;
                glm::decompose(delta, scaledif, orientDif, posdif, temp1, temp2);

                uint32_t bulletHoleID = limonAPI->addObject("./Data/Models/BulletHole/BulletHole.obj", 0, false, posdif, scaledif, orientDif);
                limonAPI->attachObjectToObject(bulletHoleID, rayResult[0].value.longValue);
            }
        }

    } else {
        /*************** Hit nothing case *********************/
    }

    /*************** Check hit  *********************/
    currentState = State::SHOOTING;

}

void CowboyShooterExtension::removeDamageIndicator(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() > 0 && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG_ARRAY) {
        for (int i = 0; i < parameters[0].value.longValues[0]; ++i) {
            limonAPI->removeGuiElement(parameters[0].value.longValues[i+1]);
        }
    }
}

void CowboyShooterExtension::removeMuzzleFlash(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() > 0 && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
        limonAPI->removeObject(parameters[0].value.longValue);
    }
}

void CowboyShooterExtension::interact(std::vector<LimonAPI::ParameterRequest> &interactionData) {
    if(interactionData.size() == 0 ) {
        return;
    }

    if(interactionData[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING &&
            (std::string(interactionData[0].value.stringValue) == "SHOOT_PLAYER" || std::string(interactionData[0].value.stringValue) == "MELEE_PLAYER")) {
        if(interactionData.size() < 2 ) {
            hitPoints -= 20;
        } else if(interactionData[1].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
            hitPoints -= interactionData[1].value.longValue;
        }

        limonAPI->updateGuiText(678, std::to_string(hitPoints));
        if(hitPoints <= 0) {
            limonAPI->killPlayer();
        }

        uint32_t addedElementcount = 0;
        std::vector<LimonAPI::ParameterRequest> removeParameters;
        LimonAPI::ParameterRequest removeIDs;
        removeIDs.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG_ARRAY;

        //find which way the damage came from
        if(interactionData.size() < 3) {
            //damage origin not sent, can't calculate indicator position, render full damage indicator
            uint32_t addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicator.png", "damageIndicator", LimonAPI::Vec2(0.5f, 0.5f),
                                                          LimonAPI::Vec2(0.7f, 0.8f), 0);
            removeIDs.value.longValues[++addedElementcount] = static_cast<long>(addedElement);

        } else {
            glm::vec3 damageOrigin = LimonConverter::LimonToGLM(interactionData[2].value.vectorValue);
            glm::vec3 damageVector = glm::normalize(glm::vec3(LimonConverter::LimonToGLM(latestPlayerInformation.position)) - damageOrigin);
            glm::vec3 lookDirection = LimonConverter::LimonToGLM(latestPlayerInformation.lookDirection);

            //now we know if it is front or back. we can check up, down, left, right
            //remove the y component, and test for left, right
            glm::vec3 rayDirWithoutY = damageVector;
            rayDirWithoutY.y = 0;
            glm::vec3 frontWithoutY = lookDirection;
            frontWithoutY.y = 0;
            glm::vec3 crossBetween = cross(normalize(frontWithoutY), normalize(rayDirWithoutY));

            if (crossBetween.y < 0) {
                uint32_t addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicatorLeft.png", "damageIndicatorLeft", LimonAPI::Vec2(0.5f, 0.5f),
                                                              LimonAPI::Vec2(0.7f, 0.8f), 0);
                removeIDs.value.longValues[++addedElementcount] = static_cast<long>(addedElement);
            } else {
                uint32_t addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicatorRight.png", "damageIndicatorRight", LimonAPI::Vec2(0.5f, 0.5f),
                                                              LimonAPI::Vec2(0.7f, 0.8f), 0);
                removeIDs.value.longValues[++addedElementcount] = static_cast<long>(addedElement);
            }

            float cosineBetween = glm::dot(normalize(lookDirection), normalize(glm::vec3(1,0,-1)));
            float crossToCheck;
            if(fabs(damageVector.x) + fabs(lookDirection.x) < fabs(damageVector.z) + fabs(lookDirection.z)) {//use the longer axis to minimize presicion errors
                damageVector.x = 0;
                lookDirection.x = 0;
                crossBetween = glm::cross(normalize(lookDirection), normalize(damageVector));
                crossToCheck = crossBetween.x;
            } else {
                damageVector.z = 0;
                lookDirection.z = 0;
                crossBetween = glm::cross(normalize(lookDirection), normalize(damageVector));
                crossToCheck = crossBetween.z;
            }
            if(cosineBetween > 0 ) {
                crossToCheck =  crossToCheck * -1; // cross product contains sin(q), so it might need reversing
            }
            if (crossToCheck < 0) {
                uint32_t addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicatorDown.png", "damageIndicatorDown", LimonAPI::Vec2(0.5f, 0.5f),
                                                              LimonAPI::Vec2(0.7f, 0.8f), 0);
                removeIDs.value.longValues[++addedElementcount] = static_cast<long>(addedElement);
            } else {
                uint32_t addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicatorUp.png", "damageIndicatorUp", LimonAPI::Vec2(0.5f, 0.5f),
                                                              LimonAPI::Vec2(0.7f, 0.8f), 0);
                removeIDs.value.longValues[++addedElementcount] = static_cast<long>(addedElement);
            }


        }



        removeIDs.value.longValues[0] = static_cast<long>(addedElementcount);
        removeParameters.push_back(removeIDs);
        limonAPI->addTimedEvent(250, std::bind(&CowboyShooterExtension::removeDamageIndicator, this, std::placeholders::_1), removeParameters);
        if(!hitReaction) {
            hitTime = lastInputTime;
            hitReaction = true;
        }
    }

}

std::string CowboyShooterExtension::getName() const {
    return "CowboyShooterExtension";
}

bool CowboyShooterExtension::changeGuns() {
    if(currentGun == transitionGun) {
        return false;
    }

    switch(transitionGun) {
        case Gun::PISTOL:
            limonAPI->addObjectTranslate(playerAttachedPistolID, addOffset);
            limonAPI->addObjectTranslate(playerAttachedRifleID, removeOffset);
            currentMuzzleFlashOffset = pistolMuzzleFlashOffset;
            break;
        case Gun::RIFLE:
            limonAPI->addObjectTranslate(playerAttachedRifleID, addOffset);
            limonAPI->addObjectTranslate(playerAttachedPistolID, removeOffset);
            currentMuzzleFlashOffset = rifleMuzzleFlashOffset;
        break;
    }
    currentGun = transitionGun;
    return true;
}

/**
 * If player is hit, move the camera in a circle we will do this by calculating a difference for mouse position
 * and simulating that input
 */
void CowboyShooterExtension::processHitReaction() {
    hitReaction = false;
    return;

    //this part requires more tuning, it works as it should, but effect is not nice
    /*
    float ANIMATION_DURATION = 250/(2.0f * 3.14);//in ms
    float animationTime = (lastInputTime - (hitTime)) / ANIMATION_DURATION;
    if(animationTime >= (2.0f * 3.14)) {
        animationTime = (2.0f * 3.14);
    }
    float xMove = cos(animationTime) * 0.001;
    float yMove = sin(animationTime) * -0.005;
    inputState.setMouseChange(0.0f,0.0f, xMove, yMove);
    inputState.setInputStatus(InputStates::Inputs::MOUSE_MOVE, true);
    limonAPI->simulateInput(inputState);

    if(animationTime >= (2.0f * 3.14)) {
        hitReaction = false;
    }
     */
}
