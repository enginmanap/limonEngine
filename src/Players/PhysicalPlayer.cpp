//
// Created by engin on 12.02.2018.
//

#include "PhysicalPlayer.h"

PhysicalPlayer::PhysicalPlayer(Options *options) :
        center(glm::vec3(0,0,-1)),
        up(glm::vec3(0,1,0)),
        right(glm::vec3(-1,0,0)),
        view(glm::quat(0,0,0,-1)),
        spring(nullptr),
        onAir(true),
        options(options),
        dirty(true){

    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            rayCallbackArray.push_back(btCollisionWorld::ClosestRayResultCallback(
                    GLMConverter::GLMToBlt(startPosition + glm::vec3(0, -1.01f, 0)),
                    GLMConverter::GLMToBlt(startPosition + glm::vec3(0, -2.01f, 0))));
        }
    }

    btCollisionShape *capsuleShape = new btCapsuleShape(1, 1);
    btDefaultMotionState *boxMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                                                                GLMConverter::GLMToBlt(startPosition)));
    btVector3 fallInertia(0, 0, 0);
    capsuleShape->calculateLocalInertia(1, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo
            boxRigidBodyCI(75, boxMotionState, capsuleShape, fallInertia);
    player = new btRigidBody(boxRigidBodyCI);
    player->setAngularFactor(0);
    player->setFriction(1);
    player->setUserPointer(&objectType);
}


void PhysicalPlayer::move(moveDirections direction) {
    if (!positionSet && onAir) {
        return;
    }

    if (direction == NONE) {
        if(positionSet) {
            player->setLinearVelocity(GLMConverter::GLMToBlt(up / 10.0f));
            positionSet = false;
        } else {
            player->setLinearVelocity(player->getLinearVelocity() / slowDownFactor);
        }
        return;
    }

    positionSet = false;
    switch (direction) {
        case UP:
            player->setLinearVelocity(player->getLinearVelocity() + GLMConverter::GLMToBlt(up * options->getJumpFactor()));
            spring->setEnabled(false);
            break;
        case LEFT_BACKWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt(-1.0f * (right + center) * options->getMoveSpeed()));
            break;
        case LEFT_FORWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt((-1.0f * right + center) * options->getMoveSpeed()));
            break;
        case LEFT:
            player->setLinearVelocity(GLMConverter::GLMToBlt(right * -1.0f * options->getMoveSpeed()));
            break;
        case RIGHT_BACKWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt((right + -1.0f * center) * options->getMoveSpeed()));
            break;
        case RIGHT_FORWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt((right + center) * options->getMoveSpeed()));
            break;
        case RIGHT:
            player->setLinearVelocity(GLMConverter::GLMToBlt(right * options->getMoveSpeed()));
            break;
        case BACKWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt(center * -1.0f * options->getMoveSpeed()));
            break;
        case FORWARD:
            player->setLinearVelocity(GLMConverter::GLMToBlt(center * options->getMoveSpeed()));
            break;
        case NONE:break;//this is here because -Wall complaints if it is not
    }
    //this activates user rigidbody if it moves. Otherwise island management ignores movement.
    player->setActivationState(ACTIVE_TAG);
}

void PhysicalPlayer::rotate(float xChange, float yChange) {
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
}

void PhysicalPlayer::processPhysicsWorld(const btDiscreteDynamicsWorld *world) {
    onAir = true;//base assumption is we are flying
    player->getMotionState()->getWorldTransform(worldTransformHolder);

    //we will test for STEPPING_TEST_COUNT^2 times
    float requiredDelta = 1.0f / (STEPPING_TEST_COUNT-1);
    float highestPoint =  std::numeric_limits<float>::lowest();
    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            btCollisionWorld::ClosestRayResultCallback *rayCallback = &rayCallbackArray[i*STEPPING_TEST_COUNT + j];
            //set raycallback for downward raytest
            rayCallback->m_closestHitFraction = 1;
            rayCallback->m_collisionObject = nullptr;
            rayCallback->m_rayFromWorld = worldTransformHolder.getOrigin() +
                                          btVector3(-0.5f + i*requiredDelta, -1.01f,
                                                    -0.5f + j*requiredDelta);//the second vector is preventing hitting player capsule
            rayCallback->m_rayToWorld = rayCallback->m_rayFromWorld + btVector3(0, -1, 0);
            world->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);

            if (rayCallback->hasHit()) {
                highestPoint = std::max(rayCallback->m_hitPointWorld.getY(), highestPoint);
                onAir = false;
            }
        }
    }

    if(!onAir) {
        springStandPoint = highestPoint + 1.0f - startPosition.y;
        spring->setLimit(1,springStandPoint + 1.0f, springStandPoint + 2.0f);
        spring->setEnabled(true);
    } else {
        spring->setEnabled(false);
    }
}

btGeneric6DofSpring2Constraint * PhysicalPlayer::getSpring(float minY) {
    spring = new btGeneric6DofSpring2Constraint(
            *player,
            btTransform(btQuaternion::getIdentity(), { 0.0f, -1.0f, 0.0f })
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