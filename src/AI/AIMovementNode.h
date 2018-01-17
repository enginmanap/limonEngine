//
// Created by engin on 1.01.2018.
//

#ifndef LIMONENGINE_AIMOVEMENTNODE_H
#define LIMONENGINE_AIMOVEMENTNODE_H


#include <glm/vec3.hpp>

class AIMovementNode {
    glm::vec3 position;

    /**
     * from looking up
     * 1 4 6
     * 2 X 7
     * 3 5 8
     * so, -x -z,    -z, -z +x
     *     -x   , na   ,    +x
     *     -x +z,    +z, +z +x
     */
    AIMovementNode *neighbours[9] = {0};

    bool isMovable = false;

public:

    explicit AIMovementNode(glm::vec3 position) : position(position) {}

    void setIsMovable(bool isMovable) {
        AIMovementNode::isMovable = isMovable;
    }

    void setNeighbour(int index, AIMovementNode *neighbour) {
        neighbours[index] = neighbour;
    }

    const glm::vec3 &getPosition() const {
        return position;
    }

    AIMovementNode *getNeighbour(int i) const {
        return (neighbours[i]);
    }

    bool isIsMovable() const {
        return isMovable;
    }
};

#endif //LIMONENGINE_AIMOVEMENTNODE_H
