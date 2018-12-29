//
// Created by engin on 26.12.2018.
//

#include "CowboyShooterExtension.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "../../Utils/LimonConverter.h"

PlayerExtensionRegister<CowboyShooterExtension> CowboyShooterExtension::reg("CowboyShooterExtension");

void CowboyShooterExtension::processInput(InputHandler &inputHandler) {
    if (playerAttachedModelID == 0) {
        return;
    }
    currentAnimationName = limonAPI->getModelAnimationName(playerAttachedModelID);
    currentAnimationFinished = limonAPI->getModelAnimationFinished(playerAttachedModelID);

    if(inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_LEFT) && inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_LEFT)) {
        transitionState = State::SHOOTING;
    } else {
        if(!inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_LEFT) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_RIGHT)) {
            //standing still
            transitionState = State::IDLE;
        } else {
            if (inputHandler.getInputStatus(inputHandler.RUN)) {
                transitionState = State::RUNNING;
            } else {
                transitionState = State::WALKING;

            }
        }
    }

    if(inputHandler.getInputEvents(inputHandler.NUMBER_2)) {
        transitionGun = Gun::RIFLE;
    }
    if(inputHandler.getInputEvents(inputHandler.NUMBER_1)) {
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
    switch(currentGun) {
        case Gun::PISTOL:
            limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Shooting|", false, 50);
            limonAPI->playSound("./Data/Sounds/EasyFPS/shot.wav", glm::vec3(0,0,0), false);
            break;
        case Gun::RIFLE:
            limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Shoot Rifle|", false, 50);
            limonAPI->playSound("./Data/Sounds/EasyFPS/shot.wav", glm::vec3(0,0,0), false);
            break;
    }

    /*************** Set animation and play sound ************/

    /*************** Create muzzle flash *********************/
    glm::vec3 scale(0.25f, 0.25f, 0.25f);//it is actually 0,1 * 1/baseScale
    uint32_t muzzleFlashObjectID = limonAPI->addObject("./Data/Models/Muzzle/Muzzle.obj", 0, false, muzzleFlashOffset, scale, direction);
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
#endif            }


            std::vector<LimonAPI::ParameterRequest>modelTransformationMat = limonAPI->getObjectTransformationMatrix(rayResult[0].value.longValue);
            if(modelTransformationMat.size() == 0) {
                std::cerr << "Hit an object, but its ID "<< (uint32_t)rayResult[0].value.longValue << " is invalid!" << std::endl;

                limonAPI->addObject("./Data/Models/BulletHole/BulletHole.obj", 0, false, hitPos, scale, orientation);//add with default values
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
    if(parameters.size() > 0 && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
        limonAPI->removeGuiElement(parameters[0].value.longValue);
    }
}

void CowboyShooterExtension::removeMuzzleFlash(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() > 0 && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
        limonAPI->removeObject(parameters[0].value.longValue);
    }
}

void CowboyShooterExtension::interact(std::vector<LimonAPI::ParameterRequest> &interactionData) {
    static uint32_t removeCounter = 0;
    static uint32_t addedElement = 0;

    if(interactionData.size() == 0 ) {
        return;
    }

    if(interactionData[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING && std::string(interactionData[0].value.stringValue) == "SHOOT_PLAYER") {
        hitPoints -= 20;

        limonAPI->updateGuiText(40, std::to_string(hitPoints));
        if(hitPoints <= 0) {
            limonAPI->killPlayer();
        }

        if(addedElement == 0) {
            addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicator.png", "damageIndicator", LimonAPI::Vec2(0.5f, 0.5f),
                                                 LimonAPI::Vec2(0.7f, 0.8f), 0);

        }
        std::vector<LimonAPI::ParameterRequest> removeParameters;
        LimonAPI::ParameterRequest removeID;
        removeID.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
        removeID.value.longValue = addedElement;
        removeParameters.push_back(removeID);
        limonAPI->addTimedEvent(250, std::bind(&CowboyShooterExtension::removeDamageIndicator, this, std::placeholders::_1), removeParameters);
    }

    if(addedElement !=0) {
        if(removeCounter == 0) {
            //limonAPI->removeGuiElement(muzzleFlashObjectID);
            addedElement = 0;
        } else {
            removeCounter--;
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
        case Gun::RIFLE:
            limonAPI->addObjectTranslate(playerAttachedRifleID, addOffset);
            limonAPI->addObjectTranslate(playerAttachedPistolID, removeOffset);
        break;
        case Gun::PISTOL:
            limonAPI->addObjectTranslate(playerAttachedPistolID, addOffset);
            limonAPI->addObjectTranslate(playerAttachedRifleID, removeOffset);
            break;
    }
    currentGun = transitionGun;
    return true;
}
