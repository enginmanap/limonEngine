//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_WORLD_H
#define UBERGAME_WORLD_H

#include <vector>
#include "Renderable.h"
#include "GLHelper.h"
#include <SDL2/SDL_stdinc.h>
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "Camera.h"
#include "Model.h"
#include "SkyBox.h"

class World {
    std::vector<Renderable*> objects;
    SkyBox* sky;
    GLHelper *glHelper;
    Camera camera;
public:
    World(GLHelper*);

    void render();
    void play(Uint32, InputHandler&);

};

#endif //UBERGAME_WORLD_H
