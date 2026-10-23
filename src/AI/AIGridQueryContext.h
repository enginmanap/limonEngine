#ifndef LIMONENGINE_AIGRIDQUERYCONTEXT_H
#define LIMONENGINE_AIGRIDQUERYCONTEXT_H

#include <glm/vec3.hpp>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvt.h>

// This class allows us to keep all the information that bullet needs/uses, so there is no race condition on our side.
// On bullet side, we only query not mutating methods. We have to have btDbvtBroadphase as world broadphase, and
// Noone (including walk generation should edit the world, because we are holding random pointers to the bullet representation.

class AIGridQueryContext {
    struct OverlapCallback;
    struct RayCallback;
    struct RayTreeAdapter;

    btDefaultCollisionConfiguration collisionConfiguration;
    btCollisionDispatcher dispatcher;
    btDispatcherInfo dispatchInfo;
    btCollisionObject probeObject;//never added to the world
    btAlignedObjectArray<const btDbvtNode *> rayStack;
    uint64_t blockerTagHash;
    int rayCollisionGroup;
    int rayCollisionMask;

public:
    //the ground ray sees what the probe collides with, so the player body and trigger volumes are not ground
    AIGridQueryContext(btCollisionShape *probeShape, uint64_t blockerTagHash, int rayCollisionGroup, int rayCollisionMask);

    AIGridQueryContext(const AIGridQueryContext &) = delete;
    AIGridQueryContext &operator=(const AIGridQueryContext &) = delete;

    bool isBlocked(btCollisionWorld *world, const glm::vec3 &position, int collisionGroup, int collisionMask);

    //position is a node height, the ground has to be within stepHeight of the ground that node stands on
    bool setProperHeight(btCollisionWorld *world, glm::vec3 *position, float floatingHeight, float stepHeight, float checkHeight);
};

#endif //LIMONENGINE_AIGRIDQUERYCONTEXT_H
