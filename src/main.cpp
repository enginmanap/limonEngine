//
// Created by Engin Manap on 10.02.2016.
//

#include <iostream>

#include "GLHelper.h"
#include "SDL2Helper.h"
#include "World.h"
#include "InputHandler.h"

#define PROGRAM_NAME "UberGame"

int main(int argc, char *argv[]){
    int height=768, width=1024;
    Uint32 worldUpdateTime = 1000/60;//This value is used to update world on a locked Timestep

    bool quit=false;

    SDL2Helper sdlHelper(PROGRAM_NAME, height,width);

    GLHelper glHelper;
    glHelper.reshape(height,width);
    InputHandler inputHandler(sdlHelper.getWindow(), height,width);

    World world(&glHelper);
    glHelper.clearFrame();
    Uint32 previousTime= SDL_GetTicks();
    Uint32 currentTime, frameTime, accumulatedTime=0;
    while(!inputHandler.getInputStatus(inputHandler.QUIT)){
        currentTime = SDL_GetTicks();
        frameTime = currentTime - previousTime;
        previousTime = currentTime;
        accumulatedTime += frameTime;
        if(accumulatedTime >= worldUpdateTime) {
            //we don't need to check for input, if we won't update world state
            inputHandler.mapInput();

            //FIXME this does not account for long operations/low framerate
            world.play(worldUpdateTime, inputHandler);
            accumulatedTime -= worldUpdateTime;
        }
        glHelper.clearFrame();
        world.render();
        sdlHelper.swap();
    }
    return 0;
}

