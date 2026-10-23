#include "AIGridQueryContext.h"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/NarrowPhaseCollision/btPersistentManifold.h>

#include "../GameObjects/GameObject.h"
#include "../Utils/GLMConverter.h"

namespace {
    struct BlockedManifoldResult : public btManifoldResult {
        bool isBlocked = false;

        BlockedManifoldResult(const btCollisionObjectWrapper *body0Wrap, const btCollisionObjectWrapper *body1Wrap)
                : btManifoldResult(body0Wrap, body1Wrap) {}

        void addContactPoint(const btVector3 &normalOnBInWorld [[gnu::unused]], const btVector3 &pointInWorld [[gnu::unused]], btScalar depth) override {
            if (depth < 0.0f) {
                isBlocked = true;
            }
        }
    };
}

struct AIGridQueryContext::OverlapCallback : public btBroadphaseAabbCallback {
    AIGridQueryContext &context;
    int collisionGroup;
    int collisionMask;
    bool isBlocked = false;

    OverlapCallback(AIGridQueryContext &context, int collisionGroup, int collisionMask)
            : context(context), collisionGroup(collisionGroup), collisionMask(collisionMask) {}

    bool process(const btBroadphaseProxy *proxy) override {
        if (isBlocked) {
            return true;
        }
        if ((proxy->m_collisionFilterGroup & collisionMask) == 0 || (collisionGroup & proxy->m_collisionFilterMask) == 0) {
            return true;
        }
        btCollisionObject *other = static_cast<btCollisionObject *>(proxy->m_clientObject);
        btCollisionObjectWrapper probeWrapper(nullptr, context.probeObject.getCollisionShape(), &context.probeObject,
                                              context.probeObject.getWorldTransform(), -1, -1);
        btCollisionObjectWrapper otherWrapper(nullptr, other->getCollisionShape(), other, other->getWorldTransform(), -1, -1);

        btCollisionAlgorithm *algorithm = context.dispatcher.findAlgorithm(&probeWrapper, &otherWrapper, nullptr, BT_CLOSEST_POINT_ALGORITHMS);
        if (algorithm == nullptr) {
            return true;
        }
        BlockedManifoldResult result(&probeWrapper, &otherWrapper);
        result.m_closestPointDistanceThreshold = 0;
        algorithm->processCollision(&probeWrapper, &otherWrapper, context.dispatchInfo, &result);
        algorithm->~btCollisionAlgorithm();
        context.dispatcher.freeCollisionAlgorithm(algorithm);
        if (result.isBlocked) {
            isBlocked = true;
        }
        return true;
    }
};

struct AIGridQueryContext::RayCallback : public btBroadphaseRayCallback {
    btCollisionWorld::RayResultCallback &resultCallback;
    btTransform rayFromTransform;
    btTransform rayToTransform;

    RayCallback(const btVector3 &rayFrom, const btVector3 &rayTo, btCollisionWorld::RayResultCallback &resultCallback)
            : resultCallback(resultCallback) {
        rayFromTransform.setIdentity();
        rayFromTransform.setOrigin(rayFrom);
        rayToTransform.setIdentity();
        rayToTransform.setOrigin(rayTo);

        btVector3 rayDirection = rayTo - rayFrom;
        rayDirection.normalize();
        for (int i = 0; i < 3; ++i) {
            m_rayDirectionInverse[i] = rayDirection[i] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[i];
            m_signs[i] = m_rayDirectionInverse[i] < 0.0;
        }
        m_lambda_max = rayDirection.dot(rayTo - rayFrom);
    }

    bool process(const btBroadphaseProxy *proxy) override {
        if (resultCallback.m_closestHitFraction == btScalar(0.f)) {
            return false;
        }
        btCollisionObject *collisionObject = static_cast<btCollisionObject *>(proxy->m_clientObject);
        if (resultCallback.needsCollision(collisionObject->getBroadphaseHandle())) {
            btCollisionWorld::rayTestSingle(rayFromTransform, rayToTransform, collisionObject, collisionObject->getCollisionShape(),
                                            collisionObject->getWorldTransform(), resultCallback);
        }
        return true;
    }
};

struct AIGridQueryContext::RayTreeAdapter : public btDbvt::ICollide {
    RayCallback &rayCallback;

    explicit RayTreeAdapter(RayCallback &rayCallback) : rayCallback(rayCallback) {}

    void Process(const btDbvtNode *leaf) override {
        rayCallback.process(static_cast<btDbvtProxy *>(leaf->data));
    }
};

AIGridQueryContext::AIGridQueryContext(btCollisionShape *probeShape, uint64_t blockerTagHash, int rayCollisionGroup, int rayCollisionMask)
        : dispatcher(&collisionConfiguration), blockerTagHash(blockerTagHash), rayCollisionGroup(rayCollisionGroup), rayCollisionMask(rayCollisionMask) {
    probeObject.setCollisionShape(probeShape);
}

bool AIGridQueryContext::isBlocked(btCollisionWorld *world, const glm::vec3 &position, int collisionGroup, int collisionMask) {
    probeObject.setWorldTransform(btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(position)));
    btVector3 aabbMin, aabbMax;
    probeObject.getCollisionShape()->getAabb(probeObject.getWorldTransform(), aabbMin, aabbMax);
    btVector3 contactThreshold(gContactBreakingThreshold, gContactBreakingThreshold, gContactBreakingThreshold);//the ghost proxy used to carry this margin
    aabbMin -= contactThreshold;
    aabbMax += contactThreshold;

    OverlapCallback callback(*this, collisionGroup, collisionMask);
    world->getBroadphase()->aabbTest(aabbMin, aabbMax, callback);
    return callback.isBlocked;
}

bool AIGridQueryContext::setProperHeight(btCollisionWorld *world, glm::vec3 *position, float floatingHeight, float stepHeight, float checkHeight) {
    btVector3 rayFrom = GLMConverter::GLMToBlt(*position);
    btVector3 rayTo;
    if (checkHeight == 0.0) {
        rayTo = GLMConverter::GLMToBlt(*position - glm::vec3(0, 9999999, 0));//Normally, we expect world aabb min y here, but it is not passed.
    } else {
        rayTo = GLMConverter::GLMToBlt(*position - glm::vec3(0, checkHeight, 0));
    }

    btCollisionWorld::ClosestRayResultCallback closestHit(rayFrom, rayTo);
    closestHit.m_collisionFilterGroup = rayCollisionGroup;
    closestHit.m_collisionFilterMask = rayCollisionMask;
    RayCallback rayCallback(rayFrom, rayTo, closestHit);
    RayTreeAdapter treeAdapter(rayCallback);
    btDbvtBroadphase *broadphase = static_cast<btDbvtBroadphase *>(world->getBroadphase());
    for (int i = 0; i < 2; ++i) {
        broadphase->m_sets[i].rayTestInternal(broadphase->m_sets[i].m_root, rayFrom, rayTo,
                                              rayCallback.m_rayDirectionInverse, rayCallback.m_signs, rayCallback.m_lambda_max,
                                              btVector3(0, 0, 0), btVector3(0, 0, 0), rayStack, treeAdapter);
    }

    if (!closestHit.hasHit()) {
        return false;
    }
    //the closest surface decides, a blocker under a walkable one never gets here
    const GameObject *hitObject = static_cast<const GameObject *>(closestHit.m_collisionObject->getUserPointer());
    if (hitObject != nullptr && hitObject->hasTag(blockerTagHash)) {
        return false;
    }
    if (closestHit.m_hitPointWorld.getY() > (*position - glm::vec3(0, floatingHeight - stepHeight, 0)).y) {
        return false;
    }
    position->y = closestHit.m_hitPointWorld.getY() + floatingHeight;
    return true;
}
