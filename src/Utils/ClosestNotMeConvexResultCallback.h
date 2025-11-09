//
// Created by engin on 09/11/2025.
//

#ifndef LIMONENGINE_CLOSEST_NOT_ME_CONVEX_RESULT_CALLBACK_H
#define LIMONENGINE_CLOSEST_NOT_ME_CONVEX_RESULT_CALLBACK_H
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"

class ClosestNotMeConvexResultCallback : public btDynamicsWorld::ClosestConvexResultCallback{
public:
    explicit ClosestNotMeConvexResultCallback( btCollisionObject* me )
    : ClosestConvexResultCallback( btVector3( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, 0.0f, 0.0f ) ),
    me( me ){}

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace ) override {
        if( convexResult.m_hitCollisionObject == this->me ){
            return 1.0f;
        }
        return ClosestConvexResultCallback::addSingleResult( convexResult, normalInWorldSpace );
    }
protected:
    btCollisionObject* me;
};

#endif //LIMONENGINE_CLOSEST_NOT_ME_CONVEX_RESULT_CALLBACK_H