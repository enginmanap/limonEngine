//
// Created by Engin Manap on 10.02.2016.
//

#include <iostream>

#include "GLHelper.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "Assets/AssetManager.h"

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

    options.loadOptions("./Data/Options.xml");

    SDL2Helper sdlHelper(PROGRAM_NAME.c_str(), &options);
#ifdef _WIN32
    sdlHelper.loadSharedLibrary("libcustomTriggers.dll");
#else
    sdlHelper.loadSharedLibrary("./libcustomTriggers.so");
#endif

    GLHelper glHelper(&options);
    glHelper.reshape();
    InputHandler inputHandler(sdlHelper.getWindow(), &options);
    AssetManager assetManager(&glHelper);
    WorldLoader* worldLoader = new WorldLoader(&assetManager, &glHelper, &options);
    World* world = worldLoader->loadWorld(worldName);
    if(world == nullptr) {
        std::cerr << "WorldLoader didn't hand out a valid world. exiting.." << std::endl;
        exit(-1);
    }
    glHelper.clearFrame();
    Uint32 previousTime = SDL_GetTicks();
    Uint32 currentTime, frameTime, accumulatedTime = 0;
    bool worldQuitRequest = false;
    while (!worldQuitRequest) {
        currentTime = SDL_GetTicks();
        frameTime = currentTime - previousTime;
        previousTime = currentTime;
        accumulatedTime += frameTime;
        if (accumulatedTime >= worldUpdateTime) {
            //we don't need to check for input, if we won't update world state
            inputHandler.mapInput();

            //FIXME this does not account for long operations/low framerate
            worldQuitRequest = world->play(worldUpdateTime, inputHandler);
            accumulatedTime -= worldUpdateTime;
        }
        glHelper.clearFrame();
        world->render();
        sdlHelper.swap();
    }
    return 0;
}

