//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_MODEL_H
#define UBERGAME_MODEL_H


#include <vector>

//TODO maybe we should not have direct dependency to glm and gl
#include "glm/glm.hpp"
#include "Renderable.h"




class Model :public Renderable {
public:
    Model(GLHelper*);
    void render();

};
#endif //UBERGAME_MODEL_H
