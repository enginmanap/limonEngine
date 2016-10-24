//
// Created by engin on 22.10.2016.
//

#ifndef UBERGAME_BONENODE_H
#define UBERGAME_BONENODE_H

#include <vector>
#include <string>

#include "../glm/glm.hpp"

class BoneNode {
public:
    std::string name;
    uint_fast32_t boneID;
    std::vector<BoneNode *> children;
    glm::mat4 offset;

    BoneNode() {};

    BoneNode(const BoneNode &originalNode) {
        this->name = originalNode.name;
        this->offset = originalNode.offset;
        for (int i = 0; i < originalNode.children.size(); ++i) {
            this->children.push_back(new BoneNode((*originalNode.children.at(i))));
        }
    }
};


#endif //UBERGAME_BONENODE_H
