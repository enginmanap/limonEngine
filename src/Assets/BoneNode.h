//
// Created by engin on 22.10.2016.
//

#ifndef LIMONENGINE_BONENODE_H
#define LIMONENGINE_BONENODE_H

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <memory>

class BoneNode {
public:
    std::string name;
    uint32_t boneID;
    std::vector<std::shared_ptr<BoneNode>> children;
    glm::mat4 transformation;

    BoneNode() = default;

    BoneNode(const BoneNode &originalNode) {
        this->name = originalNode.name;
        this->transformation = originalNode.transformation;
        for (unsigned int i = 0; i < originalNode.children.size(); ++i) {
            this->children.push_back(std::make_shared<BoneNode>(*originalNode.children.at(i)));
        }
    }

    template<class Archive>
    void serialize(Archive & archive){
        archive(name, boneID, children, transformation);
    }
};


#endif //LIMONENGINE_BONENODE_H
