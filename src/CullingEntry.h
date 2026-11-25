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
//ordered by alignment, so nothing is padded. Check sizeof before adding a field, it is walked every frame per camera
struct CullingEntry {
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);
    uint32_t worldObjectID = 0;
    float lodScale = 1.0f;//largest axis of the scale, set by Model::setAabb on every transform change
    uint32_t rigId = 0;//0 is not animated. Model::getRigId reads it from here
    bool dirtyForFrustum = true;
    bool animated = false;//never changes after the model is constructed
    bool meshTagsUniform = true;//set by Model::refreshTags. When false, culling checks each mesh's tags, not only the union below
    bool hangsFromBone = false;//the pose pass has to run for it. Model::parentBoneChanged keeps it
    Model* model = nullptr;
    ModelGroup* nearestModelGroup = nullptr;//kept by WorldAPIAccessor::refreshNearestModelGroup on every parent change, culling never walks parents
    std::vector<MeshMeta*> meshMetaData;//the model owns the MeshMetas, it deletes them
    std::shared_ptr<ModelAsset> modelAsset;//shared between models, read through here, never copied out
    //TODO: we should not copy the tags, but moving out of game object is too big to fold in. Model::addTag and removeTag keep it in sync
    std::vector<HashUtil::HashedString> tags;
};

#endif //LIMONENGINE_CULLINGENTRY_H
