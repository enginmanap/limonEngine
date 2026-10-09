#ifndef LIMONENGINE_CULLINGENTRY_H
#define LIMONENGINE_CULLINGENTRY_H

#include <glm/glm.hpp>

class Model;

//the only home of these fields. World keeps its models' entries packed, so culling reads them without touching the models
struct CullingEntry {
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);
    bool dirtyForFrustum = true;
    bool animated = false;//never changes after the model is constructed
    Model* model = nullptr;
};

#endif //LIMONENGINE_CULLINGENTRY_H
