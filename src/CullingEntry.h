#ifndef LIMONENGINE_CULLINGENTRY_H
#define LIMONENGINE_CULLINGENTRY_H

#include <cstdint>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Utils/HashUtil.h"

class Model;
class ModelGroup;
class ModelAsset;
struct MeshMeta;//Model.h, it can't include this file's includer

//the only home of these fields. World keeps its models' entries packed, so culling reads them without touching the models
struct CullingEntry {
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);
    bool dirtyForFrustum = true;
    bool animated = false;//never changes after the model is constructed
    Model* model = nullptr;
    ModelGroup* nearestModelGroup = nullptr;//kept by WorldAPIAccessor::refreshNearestModelGroup on every parent change, culling never walks parents
    std::vector<MeshMeta*> meshMetaData;//the model owns the MeshMetas, it deletes them
    std::shared_ptr<ModelAsset> modelAsset;//shared between models, read through here, never copied out
    uint32_t worldObjectID = 0;
    //TODO: we should not copy the tags, but moving out of game object is too big to fold in. Model::addTag and removeTag keep it in sync
    std::vector<HashUtil::HashedString> tags;
    float lodScale = 1.0f;//largest axis of the scale, set by Model::setAabb on every transform change
};

#endif //LIMONENGINE_CULLINGENTRY_H
