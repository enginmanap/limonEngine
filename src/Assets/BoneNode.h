//
// Created by engin on 22.10.2016.
//

#ifndef UBERGAME_BONENODE_H
#define UBERGAME_BONENODE_H

#include <vector>
#include "../glm/glm.hpp"
class BoneNode {
public:
    uint_fast32_t boneID;
    std::vector<BoneNode *> children;
    glm::mat4 offset;
    glm::mat4 selfTransform;
    glm::mat4 totalTransform;
};


#endif //UBERGAME_BONENODE_H
