//
// Created by Engin Manap on 8.03.2016.
//

#ifndef UBERGAME_BULLETGLMCONVERTER_H
#define UBERGAME_BULLETGLMCONVERTER_H

#include <btBulletDynamicsCommon.h>
#include "../glm/glm.hpp"

class BulletGLMConverter {
public:
    static glm::vec3 BltToGLM(const btVector3 &vector) {
        return glm::vec3(vector.getX(), vector.getY(), vector.getZ());
    }
};


#endif //UBERGAME_BULLETGLMCONVERTER_H
