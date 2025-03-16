//
// Created by engin on 16.11.2018.
//

#include "ShooterPlayerExtension.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "limonAPI/LimonConverter.h"
#include "CowboyShooterExtension.h"
#include "WesternMenuPlayerExtension.h"

const glm::quat ShooterPlayerExtension::direction = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);//this is used to reverse hit normal

void ShooterPlayerExtension::processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                                          long time [[gnu::unused]]) {

    if(inputState.isSimulated()) {
        return;
    }

    if (playerAttachedModelID == 0) {
        return;
    }

    if(removeCounter <= 0 && addedElement != 0 ) {
        limonAPI->removeObject(addedElement);
        addedElement = 0;
        std::cout << "removed by counter" << std::endl;
    } else {
        removeCounter--;
    }

    if(inputState.getInputEvents(InputStates::Inputs::MOUSE_BUTTON_LEFT) && inputState.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
        if((limonAPI->getModelAnimationName(playerAttachedModelID) != "Shoot" ||  limonAPI->getModelAnimationFinished(playerAttachedModelID))) {
            limonAPI->setModelAnimation(playerAttachedModelID, "Shoot", false);
            limonAPI->playSound("./Data/Sounds/EasyFPS/shot.wav", glm::vec3(0, 0, 0), false, false);
            glm::vec3 scale(10.0f,10.0f,10.0f);//it is actually 0,1 * 1/baseScale
            if(removeCounter!=0) {
                limonAPI->removeObject(addedElement);
                std::cout << "removed by reshoot" << std::endl;
            }

            addedElement = limonAPI->addObject("./Data/Models/Muzzle/Muzzle.obj", 0, false, muzzleFlashOffset * 100.0f, scale, direction);
            bool isAttached = limonAPI->attachObjectToObject(addedElement, playerAttachedModelID);
            if(!isAttached) {
                std::cerr << "attachment failed!" << std::endl;
            }
            removeCounter = 2;


            //now put bullet hole to the hit position:

            std::vector<LimonTypes::GenericParameter>rayResult = limonAPI->rayCastToCursor();
            if(rayResult.size() > 0 ) {

                //means we hit something
                if(rayResult.size() == 4) {
                    std::cout << "hit AI" << std::endl;
                    // means we hit something with AI, handle AI interaction
                    LimonTypes::GenericParameter parameterRequest;
                    parameterRequest.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
                    strncpy(parameterRequest.value.stringValue, "GOT_HIT", 63);
                    std::vector<LimonTypes::GenericParameter> prList;
                    prList.push_back(parameterRequest);
                    limonAPI->interactWithAI(rayResult[3].value.longValue, prList);

                    //How about some particles?
                    uint32_t particleEmitterID = limonAPI->addParticleEmitter("HitEmitter", "./Data/Textures/BloodParticle.png", rayResult[1].value.vectorValue, LimonTypes::Vec4(0.01, 0.01, 0.01, 0),
                                                                              LimonTypes::Vec2(0.05, 0.05), 50, 2000, 2000, false);
                    //For speed we want to use the normal, first parameter is multiplier for random, second is applied to all.
                    LimonTypes::Vec4 randomStartMultiplier = LimonTypes::Vec4(0.05, 0.05, 0.05, 0.05);
                    LimonTypes::Vec4 offset = rayResult[2].value.vectorValue * 0.05;
                    std::cout << "creating particles with " << offset.x << ", " << offset.y << ", " << offset.z << ", " << offset.w << " ." << std::endl;
                    limonAPI->setEmitterParticleSpeed(particleEmitterID, randomStartMultiplier, offset);
                    limonAPI->setEmitterParticleGravity(particleEmitterID, LimonTypes::Vec4(0,-1, 0));
                } else {
                    std::cout << "hit non AI" << std::endl;
                    //means we hit something that doesn't have AI, put a hole
                    glm::vec3 scale(0.2f, 0.2f, 0.2f);
                    glm::vec3 hitPos(rayResult[1].value.vectorValue.x, rayResult[1].value.vectorValue.y, rayResult[1].value.vectorValue.z);
                    glm::vec3 hitNormal(rayResult[2].value.vectorValue.x, rayResult[2].value.vectorValue.y, rayResult[2].value.vectorValue.z);

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


                    std::vector<LimonTypes::GenericParameter>modelTransformationMat = limonAPI->getObjectTransformationMatrix(rayResult[0].value.longValue);
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

                        //now

                        uint32_t bulletHoleID = limonAPI->addObject("./Data/Models/BulletHole/BulletHole.obj", 0, false, posdif, scaledif, orientDif);
                        limonAPI->setObjectTemporary(bulletHoleID, true);

                        limonAPI->attachObjectToObject(bulletHoleID, rayResult[0].value.longValue);

                        //now apply force to the object
                        //what it the force direction? It is the difference between player and hit position normalized

                        glm::vec3 forceDirection = glm::normalize(hitPos - glm::vec3(LimonConverter::LimonToGLM(playerInformation.position)));
                        limonAPI->applyForce(rayResult[0].value.longValue, LimonConverter::GLMToLimon(hitPos), LimonConverter::GLMToLimon(forceDirection * bulletForce));

                    }
                    //How about some particles?
                    uint32_t particleEmitterID = limonAPI->addParticleEmitter("HitEmitter", "./Data/Textures/BaseParticle.png", rayResult[1].value.vectorValue, LimonTypes::Vec4(0.01, 0.01, 0.01, 0),
                                                 LimonTypes::Vec2(0.05, 0.05), 50, 2000, 2000, false);
                    //For speed we want to use the normal, first parameter is multiplier for random, second is applied to all.
                    LimonTypes::Vec4 randomStartMultiplier = LimonTypes::Vec4(0.1, 0.1, 0.1, 0.1);
                    LimonTypes::Vec4 offset = rayResult[2].value.vectorValue * 0.2;
                    std::cout << "creating particles with " << offset.x << ", " << offset.y << ", " << offset.z << ", " << offset.w << " ." << std::endl;
                    limonAPI->setEmitterParticleSpeed(particleEmitterID, randomStartMultiplier, offset);
                    limonAPI->setEmitterParticleGravity(particleEmitterID, LimonTypes::Vec4(0,-1, 0));

                }

            } else {
                std::cout << "hit nothing!" << std::endl;
            }
        }
    } else {
        if (inputState.getInputEvents(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
            LimonTypes::Vec4 newOffset = LimonTypes::Vec4(0.075f, 0.03f,-0.045f);
            LimonTypes::Vec4 attachedModelOffset = limonAPI->getPlayerAttachedModelOffset();
            if(inputState.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "AimPose", true);

                attachedModelOffset = attachedModelOffset + newOffset;
            } else {
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "AimPose", false);
                attachedModelOffset = attachedModelOffset - newOffset;
            }
            limonAPI->setPlayerAttachedModelOffset(attachedModelOffset);
        }

        bool isAnimationFinished = limonAPI->getModelAnimationFinished(playerAttachedModelID);
        std::string currentAnimationName = limonAPI->getModelAnimationName(playerAttachedModelID);

        if(!inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_BACKWARD) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT) &&
           !inputState.getInputStatus(InputStates::Inputs::MOVE_RIGHT)) {
            //standing still
            std::string finishedStr = " not finished";


            if(isAnimationFinished) {
                finishedStr = " finished";
            }

            if((currentAnimationName == "Run" ||
                currentAnimationName == "Walk") ||
               isAnimationFinished) {
                limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Idle", true);
            }
        } else {
            //we are moving. Set only if we just started.
            if(currentAnimationName == "Run" ||
               currentAnimationName == "Walk") {
                //we were already moving, handle if player run state changed
                if (inputState.getInputEvents(InputStates::Inputs::RUN)) {
                    if (inputState.getInputStatus(InputStates::Inputs::RUN)) {
                        limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Run", true);
                    } else {
                        limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Walk", true);
                    }
                }
            } else {
                if(currentAnimationName == "Idle" ||
                   isAnimationFinished) {
                    //we were standing or some other animation. handle accordingly
                    if (inputState.getInputStatus(InputStates::Inputs::RUN)) {
                        limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Run", true);
                    } else {
                        limonAPI->setModelAnimationWithBlend(playerAttachedModelID, "Walk", true);
                    }
                }
            }
        }

    }

}

void ShooterPlayerExtension::removeDamageIndicator(std::vector<LimonTypes::GenericParameter> parameters) {
    if(parameters.size() > 0 && parameters[0].valueType == LimonTypes::GenericParameter::ValueTypes::LONG) {
        limonAPI->removeGuiElement(parameters[0].value.longValue);
    }
}

void ShooterPlayerExtension::interact(std::vector<LimonTypes::GenericParameter> &interactionData) {
    static uint32_t removeCounter = 0;
    static uint32_t addedElement = 0;

    if(interactionData.size() == 0 ) {
        return;
    }

    if(interactionData[0].valueType == LimonTypes::GenericParameter::ValueTypes::STRING && std::string(interactionData[0].value.stringValue) == "SHOOT_PLAYER") {
        hitPoints -= 20;

        limonAPI->updateGuiText(40, std::to_string(hitPoints));
        limonAPI->animateModel(40, 2, false, nullptr);

        if(hitPoints <= 0) {
            limonAPI->killPlayer();
        }

        if(addedElement == 0) {
            addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicator.png", "damageIndicator", LimonTypes::Vec2(0.5f, 0.5f),
                                                 LimonTypes::Vec2(0.7f, 0.8f), 0);

        }
        std::vector<LimonTypes::GenericParameter> removeParameters;
        LimonTypes::GenericParameter removeID;
        removeID.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
        removeID.value.longValue = addedElement;
        removeParameters.push_back(removeID);
        limonAPI->addTimedEvent(250, false, std::bind(&ShooterPlayerExtension::removeDamageIndicator, this, std::placeholders::_1), removeParameters);
    }

    if(addedElement !=0) {
        if(removeCounter == 0) {
            //limonAPI->removeGuiElement(addedElement);
            addedElement = 0;
        } else {
            removeCounter--;
        }
    }


}

std::string ShooterPlayerExtension::getName() const {
    return "ShooterPlayerExtension";
}


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>* playerExtensionMap) {
    (*playerExtensionMap)["ShooterPlayerExtension"] = &createPlayerExtension<ShooterPlayerExtension>;
    (*playerExtensionMap)["CowboyShooterExtension"] = &createPlayerExtension<CowboyShooterExtension>;
    (*playerExtensionMap)["WesternMenuExtension"] = &createPlayerExtension<WesternMenuPlayerExtension>;

}