//
// Created by engin on 12.02.2018.
//

#include "PhysicalPlayer.h"
#include "../Model.h"
#include "../../Utils/GLMUtils.h"


const float PhysicalPlayer::CAPSULE_HEIGHT = 1.0f;
const float PhysicalPlayer::CAPSULE_RADIUS = 1.0f;
const float PhysicalPlayer::STANDING_HEIGHT = 2.0f;

PhysicalPlayer::PhysicalPlayer(uint32_t worldID, Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                               const glm::vec3 &lookDirection, Model *attachedModel ) :
        Player(cursor, options, position, lookDirection),
        center(lookDirection),
        up(glm::vec3(0,1,0)),
        view(glm::quat(0,0,0,-1)),
        spring(nullptr),
        worldID(worldID),
        onAir(true),
        dirty(true),
        attachedModel(attachedModel) {
    right = glm::normalize(glm::cross(center, up));
    startingHeight = position.y;
    worldSettings.debugMode = DEBUG_DISABLED;
    worldSettings.audioPlaying = true;
    worldSettings.worldSimulation = true;
    worldSettings.editorShown = false;
    worldSettings.cursorFree = false;
    worldSettings.resetAnimations = false;
    worldSettings.menuInteraction = false;


    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            rayCallbackArray.push_back(btCollisionWorld::ClosestRayResultCallback(
                    GLMConverter::GLMToBlt(position + glm::vec3(0, -1 * (CAPSULE_HEIGHT + 0.1f), 0)),
                    GLMConverter::GLMToBlt(position + glm::vec3(0, -1 * (CAPSULE_HEIGHT + 0.1f + STANDING_HEIGHT), 0))));
        }
    }

    btCollisionShape *capsuleShape = new btCapsuleShape(CAPSULE_RADIUS, CAPSULE_HEIGHT);
    btDefaultMotionState *boxMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                                                                GLMConverter::GLMToBlt(position)));
    btVector3 fallInertia(0, 0, 0);
    capsuleShape->calculateLocalInertia(75, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo
            boxRigidBodyCI(75, boxMotionState, capsuleShape, fallInertia);
    player = new btRigidBody(boxRigidBodyCI);
    player->setAngularFactor(0);
    player->setFriction(1);
    player->setUserPointer(static_cast<GameObject *>(this));
    if(attachedModel != nullptr) {
        this->attachedModelOffset = attachedModel->getTransformation()->getTranslate();
        setAttachedModelTransformation(attachedModel);
    }
}

void PhysicalPlayer::move(moveDirections direction) {
    if(dead) {
        return;
    }
    if (!positionSet && onAir) {//this is because, if player is just moved from editor etc, we need to process
        return;
    }

    if(positionSet) {
        positionSet = false;
    }

    if (direction == NONE) {
        inputMovementSpeed = inputMovementSpeed / slowDownFactor;
        player->setLinearVelocity(inputMovementSpeed + groundFrictionMovementSpeed);

        if(currentSound != nullptr ) {
            if(currentSound->getState() == Sound::State::PLAYING) {
                currentSound->stopAfterFinish();
            }
        }
        return;
    }

    switch (direction) {
        case UP:
            skipSpringByJump = true;
            inputMovementSpeed = inputMovementSpeed + GLMConverter::GLMToBlt(up * options->getJumpFactor());
            spring->setEnabled(false);
            if(currentSound != nullptr ) {
                currentSound->stopAfterFinish();
            }
            break;
        case LEFT_BACKWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt(-1.0f * (right + center) * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case LEFT_FORWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt((-1.0f * right + center) * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case LEFT:
            inputMovementSpeed = GLMConverter::GLMToBlt(right * -1.0f * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case RIGHT_BACKWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt((right + -1.0f * center) * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case RIGHT_FORWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt((right + center) * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case RIGHT:
            inputMovementSpeed = GLMConverter::GLMToBlt(right * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case BACKWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt(center * -1.0f * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case FORWARD:
            inputMovementSpeed = GLMConverter::GLMToBlt(center * options->getMoveSpeed() + glm::vec3(0,inputMovementSpeed.getY(), 0));
            break;
        case NONE:break;//this is here because -Wall complaints if it is not
    }
    if(direction != UP && currentSound != nullptr) {
        currentSound->play();
    }
    //this activates user rigidbody if it moves. Otherwise island management ignores movement.
    player->activate();
}

void PhysicalPlayer::rotate(float   xPosition __attribute__((unused)), float yPosition __attribute__((unused)), float xChange, float yChange) {
    if(this->dead) {
        return;
    }
    glm::quat viewChange;
    float lookAroundSpeedX = options->getLookAroundSpeed();
    //scale look around speed with the abs(center.y). for 1 -> look around 0, for 0 -> lookaround 1.
    float lookAroundSpeedY = lookAroundSpeedX;
    if(((center.y > 0 && yChange < 0) || (center.y < 0 && yChange > 0))) { //if player is moving mouse on the direction. Looking up, moving mouse up, or vice versa. yChange is in reverse
            lookAroundSpeedY *= (1- (center.y * center.y));//slow down the movement so player wont go haywire
    }
    viewChange = glm::quat(cos(yChange * lookAroundSpeedY / 2),
                           right.x * sin(yChange * lookAroundSpeedY / 2),
                           right.y * sin(yChange * lookAroundSpeedY / 2),
                           right.z * sin(yChange * lookAroundSpeedY / 2));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * lookAroundSpeedX / 2),
                           up.x * sin(xChange * lookAroundSpeedX / 2),
                           up.y * sin(xChange * lookAroundSpeedX / 2),
                           up.z * sin(xChange * lookAroundSpeedX / 2));
    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    center.x = view.x;
    if (view.y > 0.99f) {
        center.y = 0.99f;
    } else if (view.y < -0.99f) {
        center.y = -0.99f;
    } else {
        center.y = view.y;
    }
    center.z = view.z;
    center = glm::normalize(center);
    right = glm::normalize(glm::cross(center, up));

    if(attachedModel != nullptr) {
        attachedModel->getTransformation()->setOrientation(calculatePlayerRotation());
    }
}

void PhysicalPlayer::processPhysicsWorld(const btDiscreteDynamicsWorld *world) {
    onAir = true;//base assumption is we are flying
    player->getMotionState()->getWorldTransform(worldTransformHolder);

    setAttachedModelTransformation(attachedModel);

    float highestPoint = std::numeric_limits<float>::lowest();
    btVector3 hitNormal;
    GameObject *hitObject = nullptr;

    if(!dead) { //if dead, no need to check on air. This data is used for 2 things: 1) Spring adjustment, 2) step sound
        //we will test for STEPPING_TEST_COUNT^2 times
        float requiredDelta = 1.0f / (STEPPING_TEST_COUNT - 1);

        for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
            for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
                btCollisionWorld::ClosestRayResultCallback *rayCallback = &rayCallbackArray[i * STEPPING_TEST_COUNT +
                                                                                            j];
                //set raycallback for downward raytest
                rayCallback->m_closestHitFraction = 1;
                rayCallback->m_collisionObject = nullptr;
                rayCallback->m_rayFromWorld = worldTransformHolder.getOrigin() +
                                              btVector3(-0.5f + i * requiredDelta, -1.1f,
                                                        -0.5f + j *
                                                                requiredDelta);//the second vector is preventing hitting player capsule
                rayCallback->m_rayToWorld = rayCallback->m_rayFromWorld + btVector3(0, -1 * STANDING_HEIGHT, 0);
                world->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);

                if (rayCallback->hasHit()) {
                    if(rayCallback->m_hitPointWorld.getY() > highestPoint ) {
                        highestPoint = rayCallback->m_hitPointWorld.getY();
                        hitObject = static_cast<GameObject *>(rayCallback->m_collisionObject->getUserPointer());
                        hitNormal = rayCallback->m_hitNormalWorld;
                    }
                    onAir = false;

                }
            }
        }
    } else {
        //if dead, we should change the up vector to the capsules.
        glm::quat capsuleRotation = GLMConverter::BltToGLM(worldTransformHolder.getRotation());
        this->up = capsuleRotation * glm::vec3(0,1,0);
    }


    if(skipSpringByJump) {
        player->setLinearVelocity(inputMovementSpeed + groundFrictionMovementSpeed);
        if(onAir) {
            //until player is onAir, don't remove the flag
            skipSpringByJump = false;
        }
    } else {
        if (!onAir) {
            springStandPoint = highestPoint + STANDING_HEIGHT - startingHeight;
            spring->setLimit(1, springStandPoint + 1.0f, springStandPoint + 1.0f + STANDING_HEIGHT);
            spring->setEnabled(true);

            Model *model = dynamic_cast<Model *>(hitObject);
            if (model != nullptr) {
                btVector3 groundSpeed = model->getRigidBody()->getLinearVelocity();
                groundFrictionMovementSpeed = groundFrictionMovementSpeed + groundSpeed / groundFrictionFactor;
                // cap the speed of friction to ground speed
                if (groundSpeed.getX() > 0) {
                    if (groundFrictionMovementSpeed.getX() > groundSpeed.getX()) {
                        groundFrictionMovementSpeed.setX(groundSpeed.getX());
                    }
                } else {
                    if (groundFrictionMovementSpeed.getX() < groundSpeed.getX()) {
                        groundFrictionMovementSpeed.setX(groundSpeed.getX());
                    }
                }
                if (groundSpeed.getY() > 0) {
                    if (groundFrictionMovementSpeed.getY() > groundSpeed.getY()) {
                        groundFrictionMovementSpeed.setY(groundSpeed.getY());
                    }
                } else {
                    if (groundFrictionMovementSpeed.getY() < groundSpeed.getY()) {
                        groundFrictionMovementSpeed.setY(groundSpeed.getY());
                    }
                }

                if (groundSpeed.getZ() > 0) {
                    if (groundFrictionMovementSpeed.getZ() > groundSpeed.getZ()) {
                        groundFrictionMovementSpeed.setZ(groundSpeed.getZ());
                    }
                } else {
                    if (groundFrictionMovementSpeed.getZ() < groundSpeed.getZ()) {
                        groundFrictionMovementSpeed.setZ(groundSpeed.getZ());
                    }
                }
                btVector3 totalSpeed = inputMovementSpeed + groundFrictionMovementSpeed;
                if(hitNormal.getY() < MINIMUM_CLIMP_NORMAL_Y && totalSpeed.length2() > 0.01) {
                    btVector3 playerCapsuleBottom = player->getCenterOfMassPosition() - btVector3(0, CAPSULE_HEIGHT / 2 + CAPSULE_RADIUS + 0.1, 0);
                    horizontalRayCallback.m_rayFromWorld = playerCapsuleBottom;
                    btVector3 checkDistance;
                    if(totalSpeed.length2() >= 1) {
                        checkDistance = totalSpeed;
                    } else {
                        checkDistance = totalSpeed.normalize();
                    }
                    horizontalRayCallback.m_rayToWorld = playerCapsuleBottom + checkDistance;

                    //set raycallback for downward raytest
                    horizontalRayCallback.m_closestHitFraction = 1;
                    horizontalRayCallback.m_collisionObject = nullptr;
                    world->rayTest(horizontalRayCallback.m_rayFromWorld, horizontalRayCallback.m_rayToWorld, horizontalRayCallback);

                    if(horizontalRayCallback.hasHit()) {
                        inputMovementSpeed.setX(hitNormal.getX());
                        inputMovementSpeed.setZ(hitNormal.getZ());
                        totalSpeed = inputMovementSpeed + groundFrictionMovementSpeed;
                    }
                }
                player->setLinearVelocity(totalSpeed);
                //check if the sound should change, if it does stop the old one
                if (model->getPlayerStepOnSound() != nullptr) {
                    if (currentSound != nullptr) {
                        if (currentSound->getName() != model->getPlayerStepOnSound()->getName()) {
                            currentSound->stop();
                            currentSound = model->getPlayerStepOnSound();
                        }
                    } else {
                        currentSound = model->getPlayerStepOnSound();
                    }
                } else {
                    if (currentSound != nullptr) {
                        currentSound->stopAfterFinish();
                    }
                }
            } else {
                std::cerr << "The thing under Player is not a Model object, this violates the movement assumptions."
                      << std::endl;
            }
        } else {//if on air
            if (currentSound != nullptr) {
                if(currentSound->getState() == Sound::State::PLAYING) {
                    currentSound->stopAfterFinish();
                }
            }
            inputMovementSpeed = btVector3(0, 0, 0);
            groundFrictionMovementSpeed = btVector3(0, 0, 0);
            spring->setEnabled(false);
        }
    }
}

glm::quat PhysicalPlayer::calculatePlayerRotation() const {
    glm::quat temp, temp2;
    //since we want not right, but front, it is not (right.x, right.z), instead of the following
    float angle = atan2(right.z, -1 * right.x );
    temp.x = 0;
    temp.y = 1 * sin( angle/2 );
    temp.z = 0;
    temp.w = 1 * cos( angle/2 );

    if(angle == 0) { //ATTENTION for some reason, GLM is assuming 0,0,0,1 is something else than no rotation
        temp.w = 0;
        temp.y = 1;
    }
    //this point temp has left/right axis rotation

    float angle2 = acos(dot(up, center)) - 0.5f * options->PI;
    //angle2 = angle2 / 8;

    temp2.x = 1 * sin(angle2/2);
    temp2.y = 0;
    temp2.z = 0;
    temp2.w = 1 * cos(angle2/2);

    //at this point temp2 has up/down axis rotation

    return glm::normalize(temp * temp2);

}

btGeneric6DofSpring2Constraint * PhysicalPlayer::getSpring(float minY) {
    spring = new btGeneric6DofSpring2Constraint(
            *player,
            btTransform(btQuaternion::getIdentity(), { 0.0f, -1 * STANDING_HEIGHT, 0.0f })
    );
    //don't enable the spring, player might be at some height, waiting for falling.
    spring->setStiffness(1,  35.0f);
    spring->setDamping  (1,  1.0f);

    spring->setLinearLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
    spring->setLinearUpperLimit(btVector3(0.0f, 0.0f, 0.0f));
    //spring->setEquilibriumPoint(1,  std::numeric_limits<float>::lowest());// if this is used, spring does not act as it should
    spring->setEquilibriumPoint(1,  minY - 1);//1 lower then minY
    spring->setParam(BT_CONSTRAINT_STOP_CFM, 1.0e-5f, 5);
    spring->setEnabled(false);//don't enable until player is not on air
    return spring;
}

void PhysicalPlayer::setAttachedModelOffset(const glm::vec3 &attachedModelOffset) {
    PhysicalPlayer::attachedModelOffset = attachedModelOffset;
}

void PhysicalPlayer::setAttachedModel(Model *attachedModel) {
    PhysicalPlayer::attachedModel = attachedModel;
    if(attachedModel != nullptr) {
        attachedModel->getTransformation()->setOrientation(calculatePlayerRotation());
    }
}

GameObject::ImGuiResult PhysicalPlayer::addImGuiEditorElements(const GameObject::ImGuiRequest &request) {
    ImGuiResult imGuiResult;

    Transformation tr;
    tr.setTranslate(GLMConverter::BltToGLM(player->getCenterOfMassPosition()));
    tr.setOrientation(getLookDirectionQuaternion());

    if(tr.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix)) {
        //true means transformation changed, activate rigid body
        imGuiResult.updated = true;
    }

    ImGui::DragFloat3("Attached Model Offsets", glm::value_ptr(attachedModelOffset));

    setAttachedModelTransformation(attachedModel);

    this->player->activate(true);
    this->player->setCenterOfMassTransform(btTransform(this->player->getCenterOfMassTransform().getRotation(), GLMConverter::GLMToBlt(tr.getTranslate())));

    //orientation update
    center = glm::normalize(tr.getOrientation() * glm::vec3(0,0,1));
    right = glm::normalize(glm::cross(center, up));

    return imGuiResult;
}

void PhysicalPlayer::processInput(const InputStates &inputHandler, long time) {
    Player::processInput(inputHandler, time);
    
    if(dead) {
        return;
    }
    if(playerExtension != nullptr) {
        PlayerExtensionInterface::PlayerInformation playerInformation;
        playerInformation.position = GLMConverter::GLMToLimon(this->getPosition());
        playerInformation.lookDirection = GLMConverter::GLMToLimon(this->getLookDirection());
        playerExtension->processInput(inputHandler, playerInformation, time);
    }
}

void PhysicalPlayer::interact(LimonAPI *limonAPI __attribute__((unused)), std::vector<LimonAPI::ParameterRequest> &interactionData) {
    if(playerExtension != nullptr) {
        playerExtension->interact(interactionData);
    }
}

void PhysicalPlayer::setDead() {
    player->setAngularFactor(0.1);
    spring->setEnabled(false);
    this->dead = true;
    player->activate(true);
}
