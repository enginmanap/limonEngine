//
// Created by engin on 31.12.2018.
//

#include <random>
#include <iostream>
#include <glm/ext.hpp>
#include "CowboyEnemyAI.h"
#include "../../Utils/LimonConverter.h"


ActorRegister<CowboyEnemyAI> CowboyEnemyAI::reg("Cowboy Enemy");


void CowboyEnemyAI::play(long time, ActorInterface::ActorInformation &information) {
    //first, check the information and decide if a state change is required
    /**
     * state changes would occur on following conditions:
     * 1) If scripted, script should decide if current information is a reason to change the state
     * 2) If IDLE, if player is visible ( in front with nothing between), and closer then DETECTION_DISTANCE
     *        1) If get hit -> HIT state
     *        2) if closer then MELEE_DISTANCE -> MELEE state
     *        3) if closer then RUN_DISTANCE -> WALKING state
     *        4) RUNNING state
     * 3) If WALKING, if player is visible
     *        1) If get hit -> HIT state
     *        2) if closer then MELEE_DISTANCE -> MELEE state
     *        3) if closer than RUN_DISTANCE -> SHOOTING state
     *        4) RUNNING state
     * 4) If RUNNING, if player is visible
     *        1) If get hit -> HIT state
     *        2) if closer then RUN_DISTANCE -> WALKING state
     *        3) %33 random KNEELING_DOWN state
     *        4) %66 random SHOOTING state
     * 5) If SHOOTING
     *        1) If get hit -> HIT state
     *        2) If SHOOTING finished, WALKING, RUNNING or KNEELING,
     * 6) If Melee
     *        1) If get hit -> HIT state
     *        2) If MELEE finished, WALKING, RUNNING
     * 7) If KNEELING_DOWN
     *        1) If get hit -> HIT state
     *        2) If finished, KNEEL_IDLE
     * 8) If KNEEL_IDLE
     *        1) If get hit -> HIT state
     *        2) If player visible, KNEEL_SHOOTING
     *        3) STANDING_UP
     * 9) If KNEEL_SHOOTING
     *        1) If get hit -> HIT state
     *        2) If finished, if player visible %50 random KNEEL_SHOOT, %50 random STAND_UP
     * 10) If HIT
     *        Select animation based on source state
     *        select one based on source state:
     *              1) WALKING
     *              2) RUNNING
     *              3) KNEEL_IDLE
     * 11) SCRIPTED
     *        Unknown and Undefined
     */

    lastSetupTime = time;

    if(information.routeReady) {
        this->routeToRequest = information.routeToRequest;
        if(information.routeFound) {
            lastWalkDirection = routeToRequest[0] - getPosition() - glm::vec3(0, 2.0f, 0);
        }
        routeGetTime = time;
        routeRequested = false;
    }

    bool isPlayerVisible = information.canSeePlayerDirectly && information.isPlayerFront && !information.playerDead && information.playerDistance < DETECTION_DISTANCE;

    currentAnimationName = limonAPI->getModelAnimationName(modelID);
    currentAnimationFinished = limonAPI->getModelAnimationFinished(modelID);

    //there is a special case, which player dies while actor is kneeling. He must get up.

    switch(currentState) {
        case State::SCRIPTED:
            //handle scripted behaviour
        break;
        case State::DEAD: {
            //do nothing
        }
        break;
        case State::IDLE: {
            if (isPlayerVisible) {
                if (information.playerDistance < MELEE_DISTANCE) {
                    transitionToMelee(information);
                } else if (information.playerDistance < RUN_DISTANCE) {
                    transitionToWalk(information);
                } else {
                    transitionToRun(information);
                }
            } else {
                transitionToIdle(information);
            }
        }
        break;
        case State::WALKING: {
            if(information.playerDistance < MELEE_DISTANCE ) {
                transitionToMelee(information);
            } else if(information.playerDistance < RUN_DISTANCE ) {
                //possible to Shoot, check last shoot time:
                if(lastShootTime + minShootTimeWait < lastSetupTime) {
                    if(randomFloats(generator) < shootChance) {
                        transitionToShoot(information);
                    }
                } else {
                    transitionToWalk(information);
                }
            } else {
                transitionToRun(information);
            }
        }
        break;
        case State::RUNNING: {
            if(information.playerDistance < MELEE_DISTANCE ) {
                transitionToMelee(information);
            } else if(information.playerDistance < RUN_DISTANCE ) {
                transitionToWalk(information);
            } else {
                if(isPlayerVisible) {
                    //possible to Shoot, check last shoot time:
                    if(lastShootTime + minShootTimeWait < lastSetupTime && randomFloats(generator) > shootChance) {
                            transitionToShoot(information);
                    } else if (randomFloats(generator) < kneelDownChance) {
                            transitionToKneel(information);
                    } else {
                        transitionToRun(information);
                    }
                } else {
                    transitionToRun(information);
                }
            }
        }
        break;
        case State::SHOOTING: {
            if(currentAnimationFinished && shootingStage == 3) {//multi stage shooting animation handling
                shootingStage = 0;//reset shooting
                if(beforeState == State::KNEEL_IDLE || beforeState == State::KNEEL_SHOOTING ||beforeState == State::STANDING_UP) {
                    transitionToKneelIdle(information);
                } else {
                    //same as IDLE
                    if (information.playerDistance < MELEE_DISTANCE) {
                        transitionToMelee(information);
                    } else if (information.playerDistance < RUN_DISTANCE) {
                        transitionToWalk(information);
                    } else {
                        transitionToRun(information);
                    }
                }
            } else {
                transitionToShoot(information);
            }
        }
        break;
        case State::MELEE: {
            if(currentAnimationFinished) {
                if (information.playerDistance < MELEE_DISTANCE) {
                    transitionToMelee(information);
                } else {
                    transitionToWalk(information);
                }
            }//If animation not finished, don't do anything
        }
        break;
        case State::KNEELING_DOWN: {
            if(currentAnimationFinished) {
                transitionToKneelIdle(information);
            }//If animation not finished, don't do anything
        }
        break;
        case State::KNEEL_IDLE: {
            if(isPlayerVisible) {
                if(lastShootTime + minShootTimeWait < lastSetupTime && randomFloats(generator) > shootChance) {
                    transitionToKneelShoot(information);
                } else {
                    transitionToKneelIdle(information);
                }
            } else {
                transitionToStandUp(information);
            }
        }
        break;
        case State::KNEEL_SHOOTING: {
            if(currentAnimationFinished) {
                if(isPlayerVisible) {
                    if(randomFloats(generator) < kneelStayChance) {
                        transitionToKneelShoot(information);
                    } else {
                        transitionToStandUp(information);
                    }
                } else {
                    transitionToStandUp(information);
                }
            }
        }
        break;
        case State::STANDING_UP: {
            if(currentAnimationFinished) {
                transitionToRun(information);//since we know player is distant
            }
        }
        break;
        case State::HIT: {
            if(currentAnimationFinished) {//same as shoot
                if(beforeState == State::KNEEL_IDLE || beforeState == State::KNEEL_SHOOTING ||beforeState == State::STANDING_UP) {
                    transitionToKneelIdle(information);
                } else {
                    //same as IDLE
                    if (information.playerDistance < MELEE_DISTANCE) {
                        transitionToMelee(information);
                    } else if (information.playerDistance < RUN_DISTANCE) {
                        transitionToWalk(information);
                    } else {
                        transitionToRun(information);
                    }
                }
            }
        }
    }

}

bool CowboyEnemyAI::interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) {
    if(interactionInformation.size() < 1) {
        return false;
    }

    if(interactionInformation[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING && std::string(interactionInformation[0].value.stringValue) == "GOT_HIT") {
        if(hitPoints < 20) {
            hitPoints =0;
        } else {
            hitPoints = hitPoints - 20;
        }
        if(hitPoints == 0) {
            transitionToDead();
        } else {
            transitionToHit();
        }
        return true;
    }
    return false;
}

std::vector<LimonAPI::ParameterRequest> CowboyEnemyAI::getParameters() const {
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

void CowboyEnemyAI::setParameters(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() > 0) {
        if(parameters[0].description == "Hit points" && parameters[0].valueType == LimonAPI::ParameterRequest::ValueTypes::LONG) {
            this->hitPoints = parameters[0].value.longValue;
        }

    }
}

void CowboyEnemyAI::transitionToMelee(const ActorInformation &information) {
    turnFaceToPlayer(information);
    //since transition requested, we know player is near
    uint32_t damage = 0;
    switch (currentGun) {
        case Gun::PISTOL: {
            limonAPI->setModelAnimationWithBlend(modelID, "Pistol Whip|", false);
            limonAPI->playSound("./Data/Sounds/shotgun.wav", this->getPosition(), false);
            damage = 10;
            std::vector<LimonAPI::ParameterRequest> prList;
            LimonAPI::ParameterRequest pr;
            pr.valueType = pr.STRING;
            strncpy(pr.value.stringValue, "MELEE_PLAYER", 63);
            prList.push_back(pr);

            LimonAPI::ParameterRequest pr2;
            pr2.valueType = pr.LONG;
            pr2.value.longValue = damage;
            prList.push_back(pr2);
            limonAPI->interactWithPlayer(prList);
            currentState = State::MELEE;
        }
        break;
        case Gun::RIFLE:
        case Gun::SHOTGUN: {
            if(lastShootTime + minShootTimeWait < lastSetupTime &&
                randomFloats(generator) < shootChance) {
                transitionToShoot(information);
            } else {
                transitionToWalk(information);// couldn't find a good animation for rifle.
            }
        }
        break;
    }
}

void CowboyEnemyAI::transitionToWalk(const ActorInformation &information) {
    std::cout << "walking " << std::endl;
    turnFaceToPlayer(information);
    //ask for route to player if we need the data
    if(routeToRequest.empty() || ((routeGetTime == 0 || routeGetTime + 1000 < lastSetupTime) && routeRequested == false)) {
        informationRequest.routeToPlayer = true;//ask for a route to player
        routeRequested = true;
    }
    //now we are walking, move along the route
    if(!routeToRequest.empty()) {
        if(currentState != State::WALKING) {
            limonAPI->setModelAnimationWithBlend(modelID, "Rifle Walk|", false);
        }
        float distanceToRouteNode = glm::length2(getPosition() + glm::vec3(0, 2.0f, 0) - routeToRequest[0]);
        if (distanceToRouteNode < 0.1f) {//if reached first element
            routeToRequest.erase(routeToRequest.begin());
            if (!routeToRequest.empty() ) {
                lastWalkDirection = routeToRequest[0] - getPosition() - glm::vec3(0, 2.0f, 0);
            } else {
                lastWalkDirection = glm::vec3(0, 0, 0);
            }
        }
        glm::vec3 moveDirection = walkSpeed * lastWalkDirection;
        limonAPI->addObjectTranslate(modelID, LimonConverter::GLMToLimon(moveDirection));

    }

    currentState = State::WALKING;
}

void CowboyEnemyAI::transitionToRun(const ActorInformation &information) {
    turnFaceToPlayer(information);
    if(currentState != State::RUNNING) {
        switch (currentGun) {
            case Gun::PISTOL: {
                limonAPI->setModelAnimationWithBlend(modelID, "Pistol Run|");
            }
                break;
            case Gun::SHOTGUN:
            case Gun::RIFLE: {
                limonAPI->setModelAnimationWithBlend(modelID, "Rifle Run|");
            }
                break;
        }
    }

    //ask for route to player if we need the data
    if(routeToRequest.empty() || ((routeGetTime == 0 || routeGetTime + 1000 < lastSetupTime) && routeRequested == false)) {
        informationRequest.routeToPlayer = true;//ask for a route to player
        routeRequested = true;
    }
    //now we are walking, move along the route
    if(!routeToRequest.empty()) {
        float distanceToRouteNode = glm::length2(getPosition() + glm::vec3(0, 2.0f, 0) - routeToRequest[0]);
        if (distanceToRouteNode < 0.1f) {//if reached first element
            routeToRequest.erase(routeToRequest.begin());
            if (!routeToRequest.empty() ) {
                lastWalkDirection = routeToRequest[0] - getPosition() - glm::vec3(0, 2.0f, 0);
            } else {
                lastWalkDirection = glm::vec3(0, 0, 0);
            }
        }
        glm::vec3 moveDirection = runSpeed * lastWalkDirection;
        limonAPI->addObjectTranslate(modelID, LimonConverter::GLMToLimon(moveDirection));

    }

    currentState = State::RUNNING;
}

void CowboyEnemyAI::transitionToKneel(const ActorInformation &information) {
    turnFaceToPlayer(information);
    switch (currentGun) {
        case Gun::PISTOL: {
            //pistol kneel is not allowed, do nothing
        }
        break;
        case Gun::SHOTGUN:
        case Gun::RIFLE: {
            limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel From Stand|", false);
            currentState = State::KNEELING_DOWN;
        }
        break;
    }
}

void CowboyEnemyAI::transitionToKneelIdle(const ActorInformation &information) {
    turnFaceToPlayer(information);
    switch (currentGun) {
        case Gun::PISTOL: {
            //pistol kneel is not allowed, how did this happen?
            std::cerr << "Kneeling Pistol Cowboy shouldn't have happened." << std::endl;
        }
        break;
        case Gun::SHOTGUN:
        case Gun::RIFLE: {
            limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel Idle|");
            currentState = State::KNEEL_IDLE;
        }
        break;
    }
}

void CowboyEnemyAI::transitionToKneelShoot(const ActorInformation &information) {
    turnFaceToPlayer(information);
    switch (currentGun) {
        case Gun::PISTOL: {
            //pistol kneel is not allowed, how did this happen?
            std::cerr << "Kneeling Pistol shoot Cowboy shouldn't have happened." << std::endl;
        }
        break;
        case Gun::SHOTGUN:
        case Gun::RIFLE: {
            limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel Fire|", false);
            currentState = State::KNEEL_SHOOTING;
        }
        break;
    }
    lastShootTime = lastSetupTime;
}

void CowboyEnemyAI::transitionToStandUp(const ActorInformation &information) {
    turnFaceToPlayer(information);
    switch (currentGun) {
        case Gun::PISTOL: {
            //pistol kneel is not allowed, how did this happen?
            std::cerr << "Kneeling Pistol standup Cowboy shouldn't have happened." << std::endl;
        }
        break;
        case Gun::SHOTGUN:
        case Gun::RIFLE: {
            limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel To Stand|", false);
            currentState = State::STANDING_UP;
        }
        break;
    }
}

void CowboyEnemyAI::transitionToShoot(const ActorInformation &information) {
    turnFaceToPlayer(information);
    uint32_t damage = 0;
    switch (currentGun) {
        case Gun::PISTOL: {
            limonAPI->setModelAnimationWithBlend(modelID, "Pistol Run 2|", false); //FIXME I couldn't find the correct animation
            damage = 10;

        }
        break;
        case Gun::SHOTGUN:
        case Gun::RIFLE: {
            //Rifle shooting has 3 animations. We will flag the state so it won't change before all 3 finishes
            switch(shootingStage) {
                case 0:
                    limonAPI->setModelAnimationWithBlend(modelID, "Rifle Down To Aim|", false);
                    shootingStage = 1;
                break;
                case 1:
                    if(currentAnimationFinished) {
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle Fire|", false);

                        std::vector<LimonAPI::ParameterRequest> prList;
                        LimonAPI::ParameterRequest pr;
                        pr.valueType = pr.STRING;
                        strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
                        prList.push_back(pr);

                        LimonAPI::ParameterRequest pr2;
                        pr2.valueType = pr.LONG;
                        pr2.value.longValue = damage;
                        prList.push_back(pr2);
                        limonAPI->interactWithPlayer(prList);

                        shootingStage = 2;
                    }
                break;
                case 2:
                    if(currentAnimationFinished) {
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Aim To Down|", false);
                        shootingStage = 3;
                    }
                break;
            }
            currentState = State::SHOOTING;
            damage = 15;
        }
        break;
    }
    lastShootTime = lastSetupTime;
}

void CowboyEnemyAI::transitionToIdle(const ActorInformation &information __attribute__((unused))) {
    if(currentState != State::IDLE) {
        switch (currentGun) {
            case Gun::PISTOL: {
                limonAPI->setModelAnimationWithBlend(modelID, "Pistol Idle|");
            }
            break;
            case Gun::SHOTGUN:
            case Gun::RIFLE: {
                float random = randomFloats(generator);
                if(random > 0.33) {
                    limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle|");
                } else if (random > 0.66){
                    limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle 2|");
                } else {
                    limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle 3|");
                }
            }
            break;
        }
        currentState = State::IDLE;
    }
}

void CowboyEnemyAI::turnFaceToPlayer(const ActorInterface::ActorInformation &information) const {
    //face the player
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
}

void CowboyEnemyAI::transitionToHit() {
    if(currentState != State::HIT) {
        switch (currentGun) {
            case Gun::PISTOL: {
                std::cerr << "pistol hit animations not set yet " << std::endl;
            }
            break;
            case Gun::SHOTGUN:
            case Gun::RIFLE: {
                switch(currentState) {
                    case State::KNEEL_SHOOTING:
                    case State::KNEEL_IDLE:
                    case State::STANDING_UP:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel Hit Reaction|", false);
                        currentState = State::HIT;
                    break;
                    case State::WALKING:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Walk Hit Reaction|", false);
                        currentState = State::HIT;
                    break;
                    case State::RUNNING:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Run Hit Reaction|", false);
                        currentState = State::HIT;
                    break;
                    case State::IDLE:
                    case State::MELEE:
                    case State::KNEELING_DOWN:
                    case State::SHOOTING:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle Hit Reaction|", false);
                        currentState = State::HIT;
                    break;
                    case State::HIT:
                    case State::DEAD:
                    {}//do nothing
                    break;
                    case State::SCRIPTED:
                    {}//should let script it got hit.
                    break;
                }
            }
            break;
        }
    }
}

void CowboyEnemyAI::transitionToDead() {
    if(currentState != State::HIT) {
        switch (currentGun) {
            case Gun::PISTOL: {
                std::cerr << "pistol dead animations not set yet " << std::endl;
            }
                break;
            case Gun::SHOTGUN:
            case Gun::RIFLE: {
                switch (currentState) {
                    case State::KNEEL_SHOOTING:
                    case State::KNEEL_IDLE:
                    case State::STANDING_UP:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Kneel Death|", false);
                        currentState = State::DEAD;
                    break;
                    case State::WALKING:
                    case State::RUNNING:
                    case State::IDLE:
                    case State::MELEE:
                    case State::KNEELING_DOWN:
                    case State::SHOOTING:
                        limonAPI->setModelAnimationWithBlend(modelID, "Rifle Death|", false);
                        currentState = State::DEAD;
                        break;
                    case State::HIT:
                    case State::DEAD: {
                    }//do nothing
                        break;
                    case State::SCRIPTED: {
                    }//should let script it got hit.
                        break;
                }
            }
                break;
        }
    }
}
