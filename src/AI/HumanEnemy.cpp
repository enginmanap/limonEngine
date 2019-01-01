//
// Created by engin on 19.11.2018.
//

#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "HumanEnemy.h"
#include "../GamePlay/Samples/CowboyEnemyAI.h"

ActorRegister<HumanEnemy> HumanEnemy::reg("ENEMY_AI_SWAT");


void HumanEnemy::play(long time, ActorInterface::ActorInformation &information) {
    if(information.routeReady) {
        this->routeTorequest = information.routeToRequest;
        if(information.routeFound) {
            lastWalkDirection = routeTorequest[0] - getPosition() - glm::vec3(0, 2.0f, 0);
        }
        routeGetTime = time;
        routeRequested = false;
    }
    lastSetupTime = time;

    //first check if we just died
    if(hitPoints <= 0) {
        if(dieAnimationStartTime == 0) {
            limonAPI->setModelAnimationWithBlend(modelID, "death from the front|mixamo.com", false);
            dieAnimationStartTime = time;
        }
        return;
    }

    if(information.playerDead) {
        if(limonAPI->getModelAnimationName(modelID) !="idle|mixamo.com") {
            limonAPI->setModelAnimationWithBlend(modelID,"idle|mixamo.com");

            playerPursuitStartTime = 0L;
        }
    }



    if(limonAPI->getModelAnimationName(modelID) == "Shoot Rifle|mixamo.com"  && limonAPI->getModelAnimationFinished(modelID)) {
        limonAPI->setModelAnimationWithBlend(modelID,"run forward|mixamo.com");
    }
    //check if the player can be seen
    if(information.canSeePlayerDirectly && information.isPlayerFront && !information.playerDead) {
        if((routeGetTime == 0 || routeGetTime + 1000 < time) && routeRequested == false) {
            informationRequest.routeToPlayer = true;//ask for a route to player
            routeRequested = true;
        }
        if (playerPursuitStartTime == 0) {
            limonAPI->setModelAnimationWithBlend(modelID,"run forward|mixamo.com");
            //means we will just start pursuit, mark the position so we can return.
            initialPosition = this->getPosition();
            returnToPositionAfterPursuit = true;
        }
        playerPursuitStartTime = time;
        if(shootPlayerTimer == 0) {
            shootPlayerTimer = time;
        } else {
            if(time - shootPlayerTimer > PLAYER_SHOOT_TIMEOUT) {
                limonAPI->setModelAnimationWithBlend(modelID,"Shoot Rifle|mixamo.com", false);
                std::vector<LimonAPI::ParameterRequest> prList;
                LimonAPI::ParameterRequest pr;
                pr.valueType = pr.STRING;
                strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
                prList.push_back(pr);
                limonAPI->interactWithPlayer(prList);
                shootPlayerTimer = time;
                limonAPI->playSound("./Data/Sounds/shotgun.wav", this->getPosition(), false);
            }
        }
    }

    if(time - playerPursuitStartTime >= playerPursuitTimeout) {
        playerPursuitStartTime = 0;
    }

    if(playerPursuitStartTime == 0) {
        //if not in player pursuit
        if(returnToPositionAfterPursuit) {

            //TODO search for route to initial position and return
        }
    } else {
        if(routeGetTime != 0 && routeGetTime + 1000 < time && routeRequested == false) {
            //its been 1 second since route request, refresh
            this->informationRequest.routeToPlayer = true;
            routeRequested = true;
        }
        //if player pursuit mode

        //first check if we are hit
        if(hitAnimationAwaiting) {
            limonAPI->setModelAnimationWithBlend(modelID,"Hit Reaction|mixamo.com", false);
            hitAnimationStartTime = time;
            hitAnimationAwaiting = false;//don't request another hit again
        }

        //now check if hit animation should be finished
        if(hitAnimationStartTime != 0 && time - hitAnimationStartTime > 500) { //play hit animation for 500 ms only
            limonAPI->setModelAnimationWithBlend(modelID,"run forward|mixamo.com");
            hitAnimationStartTime = 0;
        }
        if(!routeTorequest.empty()) {
            float distanceToRouteNode = glm::length2(getPosition() + glm::vec3(0, 2.0f, 0) - routeTorequest[0]);
            if (distanceToRouteNode < 0.1f) {//if reached first element
                routeTorequest.erase(routeTorequest.begin());
                if (!routeTorequest.empty() ) {
                    lastWalkDirection = routeTorequest[0] - getPosition() - glm::vec3(0, 2.0f, 0);
                } else {
                    lastWalkDirection = glm::vec3(0, 0, 0);
                }
            }
        }
        glm::vec3 moveDirection = 0.1f * lastWalkDirection;

        limonAPI->addObjectTranslate(modelID, LimonConverter::GLMToLimon(moveDirection));
        if(information.isPlayerLeft) {
            if(information.cosineBetweenPlayerForSide < 0.95) {
                LimonAPI::Vec4 rotateLeft(0.0f, 0.015f, 0.0f, 1.0f);
                limonAPI->addObjectOrientation(modelID, rotateLeft);
            }
        }
        if(information.isPlayerRight) {
            //turn just a little bit to right
            if(information.cosineBetweenPlayerForSide < 0.95) {
                LimonAPI::Vec4 rotateRight(0.0f, -0.015f, 0.0f, 1.0f);
                limonAPI->addObjectOrientation(modelID, rotateRight);
            }
        }
        if(information.isPlayerUp) {
            //std::cout << "Up." << std::endl;
        }
        if(information.isPlayerDown) {
            //std::cout << "Down." << std::endl;
        }
    }
}

bool HumanEnemy::interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) {
    if(interactionInformation.size() < 1) {
        return false;
    }

    if(interactionInformation[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING && std::string(interactionInformation[0].value.stringValue) == "GOT_HIT") {
        playerPursuitStartTime = lastSetupTime;
        hitAnimationAwaiting = true;
        if(hitPoints < 20) {
            hitPoints =0;
        } else {
            hitPoints = hitPoints - 20;
        }
        return true;
    }
    return false;
}

std::vector<LimonAPI::ParameterRequest> HumanEnemy::getParameters() const {
    std::vector<LimonAPI::ParameterRequest> parameters;

    LimonAPI::ParameterRequest hitPointParameter;
    hitPointParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    hitPointParameter.description = "Hit points";
    hitPointParameter.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_NUMBER;
    hitPointParameter.value.longValue = (long) this->hitPoints;
    hitPointParameter.isSet = true;//don't force change
    parameters.push_back(hitPointParameter);
    return parameters;
}

void HumanEnemy::setParameters(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() > 0) {
        if(parameters[0].description == "Hit points" && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
            this->hitPoints = parameters[0].value.longValue;
        }

    }
}

extern "C" void registerActors(std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>* actorMap) {
    (*actorMap)["ENEMY_AI_SWAT"] = &createActorT<HumanEnemy>;
    (*actorMap)["Cowboy Enemy"] = &createActorT<CowboyEnemyAI>;
}