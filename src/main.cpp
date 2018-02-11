//
// Created by Engin Manap on 10.02.2016.
//

#include <iostream>

#include "GLHelper.h"
#include "SDL2Helper.h"
#include "World.h"

const std::string PROGRAM_NAME = "LimonEngine";

int main(int argc, char *argv[]) {

    std::string worldName;
    if(argc == 1) {
        std::cout << "No world file specified, trying to load ./Data/Maps/World001.xml" << std::endl;
        worldName = "./Data/Maps/World001.xml";
    } else if(argc == 2) {
        worldName = argv[1];
        std::cout << "Trying to load " <<  worldName << std::endl;
    } else {
        worldName = argv[1];
        std::cout << "Trying to load " <<  worldName << std::endl;
        std::cout << PROGRAM_NAME + " only takes one parameter. First one is processed as Map file, rest discarted." << std::endl;
    }

    Uint32 worldUpdateTime = 1000 / 60;//This value is used to update world on a locked Timestep
    Options options;

    SDL2Helper sdlHelper(PROGRAM_NAME.c_str(), options.getScreenHeight(), options.getScreenWidth());


    GLHelper glHelper(&options);
    glHelper.reshape();
    InputHandler inputHandler(sdlHelper.getWindow(), &options);


    World world(&glHelper, &options, worldName);
    glHelper.clearFrame();
    Uint32 previousTime = SDL_GetTicks();
    Uint32 currentTime, frameTime, accumulatedTime = 0;
    while (!inputHandler.getInputStatus(inputHandler.QUIT)) {
        currentTime = SDL_GetTicks();
        frameTime = currentTime - previousTime;
        previousTime = currentTime;
        accumulatedTime += frameTime;
        if (accumulatedTime >= worldUpdateTime) {
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

