//
// Created by Engin Manap on 10.02.2016.
//

#include "main.h"
#include "GLHelper.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "ALHelper.h"

const std::string PROGRAM_NAME = "LimonEngine";

bool GameEngine::mainLoadAndChangeWorld(const std::string& worldFile) {
    World* newWorld = worldLoader->loadWorld(worldFile, limonAPI);
    if(newWorld == nullptr) {
        return false;
    }
    delete world;
    world = newWorld;
    return true;
}

GameEngine::GameEngine() {
    options = new Options();

    options->loadOptions("./Data/Options.xml");

    sdlHelper = new SDL2Helper(PROGRAM_NAME.c_str(), options);
#ifdef _WIN32
    sdlHelper->loadSharedLibrary("libcustomTriggers.dll");
#else
    sdlHelper.loadSharedLibrary("./libcustomTriggers.so");
#endif

    glHelper = new GLHelper(options);
    glHelper->reshape();

    ALHelper* alHelper = new ALHelper();

    inputHandler = new InputHandler(sdlHelper->getWindow(), options);
    assetManager = new AssetManager(glHelper, alHelper);

    worldLoader = new WorldLoader(assetManager, inputHandler, options);

    std::function<bool (const std::string&)> limonLoadWorld = bind(&GameEngine::mainLoadAndChangeWorld, this, std::placeholders::_1);
    std::function<void ()> limonExitGame = [&]{this->setWorldQuit();};

    limonAPI = new LimonAPI(limonLoadWorld, limonExitGame);

}

void GameEngine::run() {
    Uint32 worldUpdateTime = 1000 / 60;//This value is used to update world on a locked Timestep

    glHelper->clearFrame();
    Uint32 previousTime = SDL_GetTicks();
    Uint32 currentTime, frameTime, accumulatedTime = 0;
    while (!worldQuit) {
        currentTime = SDL_GetTicks();
        frameTime = currentTime - previousTime;
        previousTime = currentTime;
        accumulatedTime += frameTime;
        if (accumulatedTime >= worldUpdateTime) {
            //we don't need to check for input, if we won't update world state
            inputHandler->mapInput();

            //FIXME this does not account for long operations/low framerate
            world->play(worldUpdateTime, *inputHandler);
            accumulatedTime -= worldUpdateTime;
        }
        glHelper->clearFrame();
        world->render();
        sdlHelper->swap();
    }
}

GameEngine::~GameEngine() {
    delete world;

    delete worldLoader;
    delete limonAPI;

    delete inputHandler;

    delete alHelper;
    delete glHelper;

    delete sdlHelper;

    delete options;
}

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
        std::cout << PROGRAM_NAME + " only takes one parameter. First one is processed as Map file, rest discarded." << std::endl;
    }

    GameEngine game;

    if(!game.mainLoadAndChangeWorld(worldName)) {
        std::cerr << "WorldLoader didn't hand out a valid world. exiting.." << std::endl;
        exit(-1);
    }

    game.run();

    return 0;
}

