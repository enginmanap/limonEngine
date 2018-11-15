//
// Created by engin on 12.02.2018.
//

#include "PhysicalPlayer.h"
#include "../Model.h"
#include "../../Utils/GLMUtils.h"


const float PhysicalPlayer::CAPSULE_HEIGHT = 1.0f;
const float PhysicalPlayer::CAPSULE_RADIUS = 1.0f;
const float PhysicalPlayer::STANDING_HEIGHT = 2.0f;

PhysicalPlayer::PhysicalPlayer(Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                               const glm::vec3 &lookDirection, Model *attachedModel ) :
        Player(cursor, options, position, lookDirection),
        center(lookDirection),
        up(glm::vec3(0,1,0)),
        view(glm::quat(0,0,0,-1)),
        spring(nullptr),
        onAir(true),
        dirty(true),
        attachedModel(attachedModel){
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
    capsuleShape->calculateLocalInertia(1, fallInertia);
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
            currentSound->stopAfterFinish();
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
    glm::quat viewChange;
    viewChange = glm::quat(cos(yChange * options->getLookAroundSpeed() / 2),
                           right.x * sin(yChange * options->getLookAroundSpeed() / 2),
                           right.y * sin(yChange * options->getLookAroundSpeed() / 2),
                           right.z * sin(yChange * options->getLookAroundSpeed() / 2));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * options->getLookAroundSpeed() / 2),
                           up.x * sin(xChange * options->getLookAroundSpeed() / 2),
                           up.y * sin(xChange * options->getLookAroundSpeed() / 2),
                           up.z * sin(xChange * options->getLookAroundSpeed() / 2));
    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    center.x = view.x;
    if (view.y > 1.0f) {
        center.y = 0.9999f;
    } else if (view.y < -1.0f) {
        center.y = -0.9999f;
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
    GameObject *hitObject = nullptr;

    //we will test for STEPPING_TEST_COUNT^2 times
    float requiredDelta = 1.0f / (STEPPING_TEST_COUNT - 1);

    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            btCollisionWorld::ClosestRayResultCallback *rayCallback = &rayCallbackArray[i * STEPPING_TEST_COUNT + j];
            //set raycallback for downward raytest
            rayCallback->m_closestHitFraction = 1;
            rayCallback->m_collisionObject = nullptr;
            rayCallback->m_rayFromWorld = worldTransformHolder.getOrigin() +
                                          btVector3(-0.5f + i*requiredDelta, -1.1f,
                                                    -0.5f + j*requiredDelta);//the second vector is preventing hitting player capsule
            rayCallback->m_rayToWorld = rayCallback->m_rayFromWorld + btVector3(0, -1 *STANDING_HEIGHT, 0);
            world->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);

            if (rayCallback->hasHit()) {
                highestPoint = std::max(rayCallback->m_hitPointWorld.getY(), highestPoint);
                hitObject = static_cast<GameObject*>(rayCallback->m_collisionObject->getUserPointer());
                    onAir = false;
                }
            }
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
                player->setLinearVelocity(inputMovementSpeed + groundFrictionMovementSpeed);
                //check if the sound should change, if it does stop the old one
                if (model->getPlayerStepOnSound() != nullptr) {
                    if (currentSound != nullptr) {
                        if (currentSound->getName() != model->getPlayerStepOnSound()->getName()) {
                            std::cout << "changing sound from " << currentSound->getName() << " to "
                                      << model->getPlayerStepOnSound()->getName() << std::endl;
                            currentSound->stop();
                            currentSound = model->getPlayerStepOnSound();
                        }
                    } else {
                        currentSound = model->getPlayerStepOnSound();
                    }
                } else {
                    if (currentSound != nullptr) {
                        currentSound->stopAfterFinish();
                        currentSound = nullptr;
                    }
                }


            } else {
                std::cerr << "The thing under Player is not a Model object, this violates the movement assumptions."
                          << std::endl;
            }
        } else {
            if (currentSound != nullptr) {
                currentSound->stopAfterFinish();
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


    if(attachedModel != nullptr) {
        attachedModel->addImGuiEditorElements(request);
    }

    ImGui::DragFloat("Muzzle flash distance", &muzzleFlashDistance);
    ImGui::DragFloat3("Muzzle flash offsets", glm::value_ptr(muzzleFlashOffset));
    ImGui::DragFloat3("Attached Model Offsets", glm::value_ptr(attachedModelOffset));


    setAttachedModelTransformation(attachedModel);


    this->player->activate(true);
    this->player->setCenterOfMassTransform(btTransform(btQuaternion(), GLMConverter::GLMToBlt(tr.getTranslate())));

    //orientation update
    center = glm::normalize(tr.getOrientation() * glm::vec3(0,0,1));
    right = glm::normalize(glm::cross(center, up));


    return imGuiResult;
}

void PhysicalPlayer::processInput(InputHandler &inputHandler, LimonAPI *limonAPI) {

    Player::processInput(inputHandler, limonAPI);

    static uint32_t removeCounter = 0;
    static uint32_t addedElement = 0;
    if (this->attachedModel == nullptr) {
        return;
    }

    if(removeCounter <= 0 && addedElement != 0 ) {
        limonAPI->removeObject(addedElement);
        addedElement = 0;
        std::cout << "removed by counter" << std::endl;
    } else {
        removeCounter--;
    }

    if(inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_LEFT) && inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_LEFT)) {
        if((attachedModel->getAnimationName() != "Shoot" ||  attachedModel->isAnimationFinished())) {
            attachedModel->setAnimation("Shoot", false);
            limonAPI->playSound("./Data/Sounds/EasyFPS/shot.wav", glm::vec3(0,0,0), false);
            glm::vec3 scale(10.0f,10.0f,10.0f);//it is actually 0,1 * 1/baseScale
            if(removeCounter!=0) {
                limonAPI->removeObject(addedElement);
                std::cout << "removed by reshoot" << std::endl;
            }
            glm::quat direction = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);

            addedElement = limonAPI->addObject("./Data/Models/Muzzle/Muzzle.obj", 0, false, muzzleFlashOffset * 100, scale, direction);
            bool isAttached = limonAPI->attachObjectToObject(addedElement, this->attachedModel->getWorldObjectID());
            if(!isAttached) {
                std::cerr << "attachment failed!" << std::endl;
            }
            removeCounter = 2;


            //now put bullet hole to the hit position:

            std::vector<LimonAPI::ParameterRequest>rayResult = limonAPI->rayCastToCursor();
            if(rayResult.size() > 0 ) {

                //means we hit something
                if(rayResult.size() == 4) {
                    std::cout << "hit AI" << std::endl;
                    // means we hit something with AI, handle AI interaction
                    LimonAPI::ParameterRequest parameterRequest;
                    parameterRequest.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
                    strncpy(parameterRequest.value.stringValue, "GOT_HIT", 63);
                    std::vector<LimonAPI::ParameterRequest> prList;
                    prList.push_back(parameterRequest);
                    limonAPI->interactWithAI(rayResult[3].value.longValue, prList);
                } else {
                    std::cout << "hit non AI" << std::endl;
                    //means we hit something that doesn't have AI, put a hole
                    glm::vec3 scale(0.2f, 0.2f, 0.2f);
                    glm::vec3 hitPos(rayResult[1].value.vectorValue.x, rayResult[1].value.vectorValue.y, rayResult[1].value.vectorValue.z);
                    glm::vec3 hitNormal(rayResult[2].value.vectorValue.x, rayResult[2].value.vectorValue.y, rayResult[2].value.vectorValue.z);
                    std::cout << "hit something, with position " << glm::to_string(hitPos) << ", and normal " << glm::to_string(hitNormal) << "." << std::endl;

                    hitPos +=hitNormal * 0.002f; //move hit position a bit towards the normal to prevent zfight
                    glm::quat orientation;
                    if(hitNormal.x < 0.001 && hitNormal.x > -0.001 &&
                       hitNormal.y < 1.001 && hitNormal.y >  0.999 &&
                       hitNormal.z < 0.001 && hitNormal.z > -0.001) {
                        //means the normal is up
                        orientation = glm::quat(0.707f, -0.707f, 0.0f, 0.0f);
                    } else {
                        orientation = glm::quatLookAt(-1.0 * hitNormal, glm::vec3(0,1,0));
                    }


                    std::vector<LimonAPI::ParameterRequest>modelTransformationMat = limonAPI->getObjectTransformationMatrix(rayResult[0].value.longValue);
                    if(modelTransformationMat.size() == 0) {
                        std::cerr << "Hit an object, but its ID "<< (uint32_t)rayResult[0].value.longValue << " is invalid!" << std::endl;

                        limonAPI->addObject("./Data/Models/BulletHole/BulletHole.obj", 0, false, hitPos, scale, orientation);//add with default values
                    } else {
                        //at this point, we will attach the bullet hole to the object, to do so, we need to update the scale, translate and orientation

                        // using the dirty method
                        glm::mat4 currentDesiredTransformMatrix = glm::translate(glm::mat4(1.0f), hitPos) * glm::mat4_cast(orientation) *
                                                                  glm::scale(glm::mat4(1.0f), scale);

                        glm::mat4 currentParentTransform = GLMConverter::LimonToGLM(modelTransformationMat[0].value.matrixValue);

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

                        limonAPI->attachObjectToObject(bulletHoleID, rayResult[0].value.longValue);

                    }
                }

            } else {
                std::cout << "hit nothing!" << std::endl;
            }
        }
    } else {
        if (inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_RIGHT)) {
            glm::vec3 newOffset = glm::vec3(0.075f, 0.03f,-0.045f);
            if(inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_RIGHT)) {
                attachedModel->setAnimationWithBlend("AimPose", true);
                attachedModelOffset = attachedModelOffset + newOffset;
            } else {
                attachedModel->setAnimationWithBlend("AimPose", false);
                attachedModelOffset = attachedModelOffset - newOffset;
            }
        }
        if(!inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_LEFT) &&
           !inputHandler.getInputStatus(inputHandler.MOVE_RIGHT)) {
            //standing still
            std::string finishedStr = " not finished";
            if(attachedModel->isAnimationFinished()) {
                finishedStr = " finished";
            }
            if((attachedModel->getAnimationName() == "Run" ||
                attachedModel->getAnimationName() == "Walk") ||
                attachedModel->isAnimationFinished()) {
                attachedModel->setAnimationWithBlend("Idle", true);
            }
        } else {
            //we are moving. Set only if we just started.
            if(attachedModel->getAnimationName() == "Run" ||
               attachedModel->getAnimationName() == "Walk") {
                //we were already moving, handle if player run state changed
                if (inputHandler.getInputEvents(inputHandler.RUN)) {
                    if (inputHandler.getInputStatus(inputHandler.RUN)) {
                        attachedModel->setAnimationWithBlend("Run", true);
                    } else {
                        attachedModel->setAnimationWithBlend("Walk", true);
                    }
                }
            } else {
                if(attachedModel->getAnimationName() == "Idle" ||
                attachedModel->isAnimationFinished()) {
                    //we were standing or some other animation. handle accordingly
                    if (inputHandler.getInputStatus(inputHandler.RUN)) {
                        attachedModel->setAnimationWithBlend("Run", true);
                    } else {
                        attachedModel->setAnimationWithBlend("Walk", true);
                    }
                }
            }
        }

    }

}

void PhysicalPlayer::interact(LimonAPI *limonAPI, std::vector<LimonAPI::ParameterRequest> &interactionData) {

    static uint32_t removeCounter = 0;
    static uint32_t addedElement = 0;

    if(interactionData.size() == 0 ) {
        return;
    }
    if(interactionData[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING && std::string(interactionData[0].value.stringValue) == "SHOOT_PLAYER") {
        if(addedElement == 0) {
            addedElement = limonAPI->addGuiImage("./Data/Textures/damageIndicator.png", "damageIndicator", LimonAPI::Vec2(0.5f, 0.5f),
                                                 LimonAPI::Vec2(0.7f, 0.8f), 0);
            std::cout << "add gui image by API" << std::endl;
        }
        std::cout << "SOMEONE shoot the Player!" << std::endl;
        removeCounter = 7;
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