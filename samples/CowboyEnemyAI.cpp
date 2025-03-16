//
// Created by engin on 31.12.2018.
//

#include <random>
#include <iostream>
#include <glm/ext.hpp>
#include "CowboyEnemyAI.h"
#include "limonAPI/LimonConverter.h"

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

    latestInformation = information;

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
    if(currentState ==  State::SCRIPTED) {
        //not implemented
    } else if(information.playerDead) {
        if(currentState != State::DEAD) {
            transitionToIdle(information);
        }
    } else {
        switch (currentState) {
            case State::SCRIPTED:
                //not possible, but generates compiler warning
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
                if (information.playerDistance < MELEE_DISTANCE) {
                    transitionToMelee(information);
                } else if (information.playerDistance < RUN_DISTANCE) {
                    //possible to Shoot, check last shoot time:
                    //check if player can be seen, and in front
                    if (((lastShootTime + minShootTimeWait) < lastSetupTime) &&
                        information.canSeePlayerDirectly && information.cosineBetweenPlayer > 0.9f) {
                        if (randomFloats(generator) < shootChance) {
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
                if (information.playerDistance < MELEE_DISTANCE) {
                    transitionToMelee(information);
                } else if (information.playerDistance < RUN_DISTANCE) {
                    transitionToWalk(information);
                } else {
                    if (isPlayerVisible) {
                        //possible to Shoot, check last shoot time:
                        if (((lastShootTime + minShootTimeWait) < lastSetupTime) &&
                            information.canSeePlayerDirectly &&
                            information.cosineBetweenPlayer > 0.9f &&
                            randomFloats(generator) > shootChance) {
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
                if (currentAnimationFinished && shootingStage == 3) {//multi stage shooting animation handling
                    shootingStage = 0;//reset shooting
                    if (beforeState == State::KNEEL_IDLE || beforeState == State::KNEEL_SHOOTING ||
                        beforeState == State::STANDING_UP) {
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
                if (currentAnimationFinished) {
                    if (information.playerDistance < MELEE_DISTANCE) {
                        transitionToMelee(information);
                    } else {
                        transitionToWalk(information);
                    }
                }//If animation not finished, don't do anything
            }
                break;
            case State::KNEELING_DOWN: {
                if (currentAnimationFinished) {
                    transitionToKneelIdle(information);
                }//If animation not finished, don't do anything
            }
                break;
            case State::KNEEL_IDLE: {
                if (isPlayerVisible) {
                    if (lastShootTime + minShootTimeWait < lastSetupTime && randomFloats(generator) > shootChance) {
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
                if (currentAnimationFinished) {
                    if (isPlayerVisible) {
                        if (randomFloats(generator) < kneelStayChance) {
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
                if (currentAnimationFinished) {
                    transitionToRun(information);//since we know player is distant
                }
            }
                break;
            case State::HIT: {
                if (currentAnimationFinished) {//same as shoot
                    limonAPI->setModelAnimationSpeed(modelID, 1.0f);
                    if (beforeState == State::KNEEL_IDLE || beforeState == State::KNEEL_SHOOTING ||
                        beforeState == State::STANDING_UP) {
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

}

bool CowboyEnemyAI::interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) {
    if(interactionInformation.size() < 1) {
        return false;
    }

    if(interactionInformation[0].valueType == LimonTypes::GenericParameter::ValueTypes::STRING && std::string(interactionInformation[0].value.stringValue) == "GOT_HIT") {
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

std::vector<LimonTypes::GenericParameter> CowboyEnemyAI::getParameters() const {
    std::vector<LimonTypes::GenericParameter> parameters;

    LimonTypes::GenericParameter hitPointParameter;
    hitPointParameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    hitPointParameter.description = "Hit points";
    hitPointParameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    hitPointParameter.value.longValue = (long) this->hitPoints;
    hitPointParameter.isSet = true;//don't force change
    parameters.push_back(hitPointParameter);

    LimonTypes::GenericParameter kneelDownChance;
    kneelDownChance.valueType = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
    kneelDownChance.description = "AI kneel down chance in %";
    kneelDownChance.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    kneelDownChance.value.doubleValue = this->kneelDownChance;
    kneelDownChance.isSet = true;//don't force change
    parameters.push_back(kneelDownChance);

    LimonTypes::GenericParameter kneelStandChance;
    kneelStandChance.valueType = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
    kneelStandChance.description = "AI kneel stay chance in %";
    kneelStandChance.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    kneelStandChance.value.doubleValue = this->kneelStayChance;
    kneelStandChance.isSet = true;//don't force change
    parameters.push_back(kneelStandChance);

    LimonTypes::GenericParameter minShootWait;
    minShootWait.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    minShootWait.description = "Wait until shoot again (in ms.)";
    minShootWait.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    minShootWait.value.longValue = this->minShootTimeWait;
    minShootWait.isSet = true;//don't force change
    parameters.push_back(minShootWait);

    LimonTypes::GenericParameter gunDamage;
    gunDamage.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    gunDamage.description = "Damage of Gun";
    gunDamage.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    gunDamage.value.longValue = (long)this->gunDamage;
    gunDamage.isSet = true;//don't force change
    parameters.push_back(gunDamage);

    LimonTypes::GenericParameter gunType;// first one is the name of selected one
    gunType.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    gunType.description = "Gun type";
    gunType.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT;
    switch (currentGun) {
        case Gun::PISTOL:
            strncpy(gunType.value.stringValue, "Pistol", sizeof(gunType.value.stringValue) - 1);
            break;
        case Gun::RIFLE:
            strncpy(gunType.value.stringValue, "Rifle", sizeof(gunType.value.stringValue) - 1);
            break;
        case Gun::SHOTGUN:
            strncpy(gunType.value.stringValue, "Shotgun", sizeof(gunType.value.stringValue) - 1);
            break;
    }
    gunType.isSet = true;//don't force change
    parameters.push_back(gunType);

    LimonTypes::GenericParameter gunType2;
    gunType2.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    gunType2.description = "Gun type";
    gunType2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT;
    strncpy(gunType2.value.stringValue, "Pistol", sizeof(gunType.value.stringValue) -1);
    gunType2.isSet = true;//don't force change
    parameters.push_back(gunType2);

    LimonTypes::GenericParameter gunType3;
    gunType3.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    gunType3.description = "Gun type";
    gunType3.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT;
    strncpy(gunType3.value.stringValue, "Rifle", sizeof(gunType3.value.stringValue) -1);
    gunType3.isSet = true;//don't force change
    parameters.push_back(gunType3);

    LimonTypes::GenericParameter gunType4;
    gunType4.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    gunType4.description = "Gun type";
    gunType4.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MULTI_SELECT;
    strncpy(gunType4.value.stringValue, "Shotgun", sizeof(gunType4.value.stringValue) -1);
    gunType4.isSet = true;//don't force change
    parameters.push_back(gunType4);

    return parameters;
}

void CowboyEnemyAI::setParameters(std::vector<LimonTypes::GenericParameter> parameters) {
    bool gunTypeSet = false;
    for (size_t i = 0; i < parameters.size(); ++i) {
        if(parameters[i].description == "Hit points") {
            this->hitPoints = (uint32_t) parameters[i].value.longValue;
        } else if(parameters[i].description == "AI kneel down chance in %") {
            this->kneelDownChance = (float)parameters[i].value.doubleValue;
        } else if(parameters[i].description == "AI kneel stay chance in %") {
            this->kneelStayChance = (float)parameters[i].value.doubleValue;
        } else if(parameters[i].description == "Wait until shoot again (in ms.)") {
            this->minShootTimeWait = parameters[i].value.longValue;
        } else if(parameters[i].description == "Damage of Gun") {
            this->gunDamage = (uint32_t) parameters[i].value.longValue;
        } else {
            if(parameters[i].description == "Gun type" && !gunTypeSet) {
                if(!std::strcmp(parameters[i].value.stringValue, "Pistol")) {
                    this->currentGun = Gun::PISTOL;
                } else if(!std::strcmp(parameters[i].value.stringValue, "Rifle")) {
                    this->currentGun = Gun::RIFLE;
                } else if(!std::strcmp(parameters[i].value.stringValue, "Shotgun")) {
                    this->currentGun = Gun::SHOTGUN;
                } else {
                    std::cerr << "Gun type didn't match possible values, assuming pistol." << std::endl;
                    this->currentGun = Gun::PISTOL;
                }
                gunTypeSet = true;
            }
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
            damage = 10;
            std::vector<LimonTypes::GenericParameter> prList;
            LimonTypes::GenericParameter pr;
            pr.valueType = pr.STRING;
            strncpy(pr.value.stringValue, "MELEE_PLAYER", 63);
            prList.push_back(pr);

            LimonTypes::GenericParameter pr2;
            pr2.valueType = pr.LONG;
            pr2.value.longValue = damage;
            prList.push_back(pr2);
            limonAPI->interactWithPlayer(prList);
            currentState = State::MELEE;
        }
        break;
        case Gun::RIFLE:
        case Gun::SHOTGUN: {
            if(((lastShootTime + minShootTimeWait) < lastSetupTime ) &&
               information.canSeePlayerDirectly &&
               information.cosineBetweenPlayer > 0.9f  &&
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
    turnFaceToPlayer(information);
    //ask for route to player if we need the data
    if(routeToRequest.empty() || ((routeGetTime == 0 || routeGetTime + 1000 < lastSetupTime) && routeRequested == false)) {
        informationRequest.routeToPlayer = true;//ask for a route to player
        routeRequested = true;
    }
    //now we are walking, move along the route
    if(!routeToRequest.empty()) {
        if(currentState != State::WALKING) {
            if(currentGun == Gun::PISTOL) {
                limonAPI->setModelAnimationWithBlend(modelID, "Pistol Walk|");
            } else {
                limonAPI->setModelAnimationWithBlend(modelID, "Rifle Walk|");
            }
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
        currentState = State::WALKING;
    }

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
            playShootSound(currentGun);
            shootPlayer(information.playerDistance);
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
    switch (currentGun) {
        case Gun::PISTOL: {
            limonAPI->setModelAnimationWithBlend(modelID, "Pistol Run 2|"); //FIXME I couldn't find the correct animation
            playShootSound(currentGun);
            shootPlayer(information.playerDistance);

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
                        playShootSound(currentGun);
                        shootPlayer(information.playerDistance);
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
        }
        break;
    }
    lastShootTime = lastSetupTime;
}

void CowboyEnemyAI::playShootSound(Gun gunType) {
    float randomValue = randomFloats(generator);
    if(randomValue < 0.33f) {
        switch (gunType) {
            case Gun::PISTOL:
                limonAPI->playSound("./Data/Sounds/guns/pistol.wav", getPosition(), false, false);
                break;
            case Gun::RIFLE:
                limonAPI->playSound("./Data/Sounds/guns/rifle.wav", getPosition(), false, false);
                break;
            case Gun::SHOTGUN:
                limonAPI->playSound("./Data/Sounds/guns/shotgun.wav", getPosition(), false, false);
                break;
        }
    } else if(randomValue < 0.66f) {
        switch (gunType) {
            case Gun::PISTOL:
                limonAPI->playSound("./Data/Sounds/guns/pistol2.wav", getPosition(), false, false);
                break;
            case Gun::RIFLE:
                limonAPI->playSound("./Data/Sounds/guns/rifle2.wav", getPosition(), false, false);
                break;
            case Gun::SHOTGUN:
                limonAPI->playSound("./Data/Sounds/guns/shotgun2.wav", getPosition(), false, false);
                break;
        }
    } else {
        switch (gunType) {
            case Gun::PISTOL:
                limonAPI->playSound("./Data/Sounds/guns/pistol3.wav", getPosition(), false, false);
                break;
            case Gun::RIFLE:
                limonAPI->playSound("./Data/Sounds/guns/rifle3.wav", getPosition(), false, false);
                break;
            case Gun::SHOTGUN:
                limonAPI->playSound("./Data/Sounds/guns/shotgun3.wav", getPosition(), false, false);
                break;
        }
    }
}

void CowboyEnemyAI::shootPlayer(float playerDistance) {
    if(latestInformation.canSeePlayerDirectly == false) {
        //it is possible we started shooting logic then player got out of sight. If that is the case, don't hurt the player.
        // The sounds and animations will still play, creating illusion of missing
        return;
    }

    //now add misisng based on distance.
    if(playerDistance > 100.0f) {
        //miss for certain. Break
        return;
    }
    float missNumber = randomFloats(generator);
    if( missNumber < (playerDistance / 100.0f)) {
        return; // miss
    }

    std::vector<LimonTypes::GenericParameter> prList;
    LimonTypes::GenericParameter pr;
    pr.valueType = pr.STRING;
    strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
    prList.push_back(pr);

    LimonTypes::GenericParameter pr2;
    pr2.valueType = pr.LONG;
    pr2.value.longValue = gunDamage;
    prList.push_back(pr2);

    LimonTypes::GenericParameter pr3;
    pr3.valueType = pr.VEC4;
    pr3.value.vectorValue= LimonConverter::GLMToLimon(this->getPosition() + glm::vec3(0,2,0));//2 is the offset of model. 3d modeller should give this.
    prList.push_back(pr3);

    limonAPI->interactWithPlayer(prList);
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

void CowboyEnemyAI::turnFaceToPlayer(const ActorInterface::ActorInformation &information) {
    //face the player
    if(information.isPlayerLeft) {
        if(information.cosineBetweenPlayerForSide < 0.65) {
            LimonTypes::Vec4 rotateLeft(0.0f, 0.030f, 0.0f, 1.0f);
            limonAPI->addObjectOrientation(modelID, rotateLeft);
        } else if(information.cosineBetweenPlayerForSide < 0.95) {
            LimonTypes::Vec4 rotateLeft(0.0f, 0.015f, 0.0f, 1.0f);
            limonAPI->addObjectOrientation(modelID, rotateLeft);
        }
    }
    if(information.isPlayerRight) {
        //turn just a little bit to right
        if(information.cosineBetweenPlayerForSide < 0.65) {
            LimonTypes::Vec4 rotateRight(0.0f, -0.030f, 0.0f, 1.0f);
            limonAPI->addObjectOrientation(modelID, rotateRight);
        } else if(information.cosineBetweenPlayerForSide < 0.95) {
            LimonTypes::Vec4 rotateRight(0.0f, -0.015f, 0.0f, 1.0f);
            limonAPI->addObjectOrientation(modelID, rotateRight);
        }
    }
}

void CowboyEnemyAI::transitionToHit() {
    //since hit has priority over everything, make sure shooting is not left in the middle
    shootingStage = 0;
    limonAPI->setModelAnimationSpeed(modelID, 1.5f);
    switch (currentGun) {
        case Gun::PISTOL: {
            limonAPI->setModelAnimationWithBlend(modelID, "Pistol Idle Hit Reaction|", false);
            currentState = State::HIT;
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
                    limonAPI->setModelAnimationWithBlend(modelID, "Rifle Idle Hit Reaction|", false, 1000);
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

void CowboyEnemyAI::transitionToDead() {
    if(currentState != State::HIT && currentState != State::DEAD) {
        switch (currentGun) {
            case Gun::PISTOL: {
                limonAPI->setModelAnimationWithBlend(modelID, "Generic Dying|", false, 500);
                currentState = State::DEAD;
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
