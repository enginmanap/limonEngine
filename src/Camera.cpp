//
// Created by Engin Manap on 17.02.2016.
//

#include "Camera.h"


Camera::Camera(Options *options) :
        dirty(false),
        position(startPosition),
        center(glm::vec3(0, 0, -1)),
        up(glm::vec3(0, 1, 0)),
        right(glm::vec3(-1, 0, 0)),
        view(glm::quat(0, 0, 0, -1)),
        onAir(true),
        options(options),
        spring(NULL) {

    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            rayCallbackArray.push_back(btCollisionWorld::ClosestRayResultCallback(
                    GLMConverter::GLMToBlt(startPosition + glm::vec3(0, -1.01f, 0)),
                    GLMConverter::GLMToBlt(startPosition + glm::vec3(0, -2.01f, 0))));
        }
    }
    cameraTransformMatrix = glm::lookAt(position, center, up);
    btCollisionShape *capsuleShape = new btCapsuleShape(1, 1);
    btDefaultMotionState *boxMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                                                                GLMConverter::GLMToBlt(startPosition)));
    btVector3 fallInertia(0, 0, 0);
    capsuleShape->calculateLocalInertia(1, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo
            boxRigidBodyCI(1, boxMotionState, capsuleShape, fallInertia);
    player = new btRigidBody(boxRigidBodyCI);
    player->setAngularFactor(0);
    player->setFriction(1);
    player->setUserPointer(&objectType);
}

void Camera::move(moveDirections direction) {
    if (onAir) {
        return;
    }

    if (direction == NONE) {
        player->setLinearVelocity(btVector3(0,0,0));
        return;
    }

    dirty = true;
    btVector3 upperLimit;
    btVector3 lowerLimit;
    switch (direction) {
        case UP:
            player->setLinearVelocity(player->getLinearVelocity() + GLMConverter::GLMToBlt(up * options->getJumpFactor()));
            spring->getLinearUpperLimit(upperLimit);
            spring->getLinearLowerLimit(lowerLimit);
            spring->setLinearLowerLimit(btVector3(0.0f,  1.0f, 0.0f) + lowerLimit);
            spring->setLinearUpperLimit(btVector3(0.0f,  1.0f, 0.0f) + upperLimit);
            //spring->enableSpring(1, false);
            break;
        case LEFT_BACKWARD:
            //position -= moveSpeed * center;
            //position -= moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt(-1.0f * (right + center) * options->getMoveSpeed()));
            break;
        case LEFT_FORWARD:
            //position += moveSpeed * center;
            //position -= moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt((-1.0f * right + center) * options->getMoveSpeed()));
            break;
        case LEFT:
            //position -= moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt(right * -1.0f * options->getMoveSpeed()));
            break;
        case RIGHT_BACKWARD:
            //position -= moveSpeed * center;
            //position += moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt((right + -1.0f * center) * options->getMoveSpeed()));
            break;
        case RIGHT_FORWARD:
            //position += moveSpeed * center;
            //position += moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt((right + center) * options->getMoveSpeed()));
            break;
        case RIGHT:
            //position += moveSpeed * right;
            player->setLinearVelocity(GLMConverter::GLMToBlt(right * options->getMoveSpeed()));
            break;
        case BACKWARD:
            //position -= moveSpeed * center;
            player->setLinearVelocity(GLMConverter::GLMToBlt(center * -1.0f * options->getMoveSpeed()));
            break;
        case FORWARD:
            //position += moveSpeed * center;
            player->setLinearVelocity(GLMConverter::GLMToBlt(center * options->getMoveSpeed()));
            break;
    }
    //this activates user rigidbody if it moves. Otherwise island management ignores movement.
    player->setActivationState(ACTIVE_TAG);
}

void Camera::rotate(float xChange, float yChange) {
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
    this->dirty = true;
}

void Camera::updateTransformFromPhysics(const btDynamicsWorld *world) {
    onAir = true;//base assumption is we are flying
    //we will test for STEPPING_TEST_COUNT^2 times
    float requiredDelta = 1.0f / (STEPPING_TEST_COUNT-1);
    float highestPoint =  std::numeric_limits<float>::lowest();
    for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
        for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
            btCollisionWorld::ClosestRayResultCallback *rayCallback = &rayCallbackArray[i*STEPPING_TEST_COUNT + j];
            world->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);

            if (rayCallback->hasHit()) {
                highestPoint = std::max(rayCallback->m_hitPointWorld.getY(), highestPoint);
                //btVector3 end = rayCallback->m_hitPointWorld;
                //std::cout << "hit id " << i << ", " << j  << "hit " << end.x() <<", " << end.y() << ", " << end.z() << std::endl;
                onAir = false;
            }
            rayCallback->m_closestHitFraction = 1;

            rayCallback->m_collisionObject = 0;


            player->getMotionState()->getWorldTransform(worldTransformHolder);

            position = GLMConverter::BltToGLM(worldTransformHolder.getOrigin());

            rayCallback->m_rayFromWorld = worldTransformHolder.getOrigin() +
                                         btVector3(-0.5f + i*requiredDelta, -1.01f,
                                                   -0.5f + j*requiredDelta);//the second vector is preventing hitting player capsule
            rayCallback->m_rayToWorld = rayCallback->m_rayFromWorld + btVector3(0, -2, 0);
        }
    }
    //onAir = false;

    if(!onAir) {
        //spring->setEquilibriumPoint(1, -6 + -1.0 * (position.y - (highestPoint + 1.0f)));
        equilibriumPoint = highestPoint + 1.0f - startPosition.y;

        spring->setLinearLowerLimit(btVector3(1.0f, equilibriumPoint + 2.0f, 1.0f));
        spring->setLinearUpperLimit(btVector3(0.0f, equilibriumPoint + 3.0f, 0.0f));
        spring->enableSpring(1,  true);
    } else {
        spring->enableSpring(1,  false);
        spring->setLinearLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
        spring->setLinearUpperLimit(btVector3(0.0f, 0.0f, 0.0f));
    }
    position.y += 1;//to make the camera at upper end, instead of center
    dirty = true;//FIXME this always returns is dirty true;
}



btGeneric6DofSpring2Constraint * Camera::getSpring(const btDynamicsWorld *world) {
    spring = new btGeneric6DofSpring2Constraint(
            *player,
            btTransform(btQuaternion::getIdentity(), { 0.0f, -1.0f, 0.0f })
    );
    //don't enable the spring, player might be at some height, waiting for falling.
    spring->setStiffness(1,  35.0f);
    spring->setDamping  (1,  1000.0f);

    spring->setLinearLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
    spring->setLinearUpperLimit(btVector3(0.0f, 0.0f, 0.0f));
    //spring->setEquilibriumPoint(1,  std::numeric_limits<float>::lowest());// if this is used, spring does not act as it should
    spring->setEquilibriumPoint(1,  -9999);
    spring->setParam(BT_CONSTRAINT_STOP_CFM, 1.0e-5f, 5);
    return spring;
}