//
// Created by Engin Manap on 10.02.2016.
//

#include "main.h"
#include "GLHelper.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "ALHelper.h"
#include "GameObjects/GUIImage.h"

const std::string PROGRAM_NAME = "LimonEngine";

bool GameEngine::loadAndChangeWorld(const std::string &worldFile) {
    renderLoadingImage();

    World* newWorld = worldLoader->loadWorld(worldFile, limonAPI);
    if(newWorld == nullptr) {
        return false;
    }
    if(loadedWorlds.find(worldFile) != loadedWorlds.end()) {
        delete loadedWorlds[worldFile];
    }
    currentWorld = newWorld;
    currentWorld->setupForPlay(*inputHandler);
    loadedWorlds[worldFile] = currentWorld;
    returnWorldStack.push_back(currentWorld);
    previousTime = SDL_GetTicks();
    return true;
}

void GameEngine::renderLoadingImage() const {
    loadingImage->setFullScreen(true);
    sdlHelper->swap();
    loadingImage->render();
    sdlHelper->swap();
}

bool GameEngine::returnOrLoadMap(const std::string &worldFile) {
    if(loadedWorlds.find(worldFile) != loadedWorlds.end()) {
        currentWorld = loadedWorlds[worldFile];
    } else { //world is not in the map case
        renderLoadingImage();
        World* newWorld = worldLoader->loadWorld(worldFile, limonAPI);
        if(newWorld == nullptr) {
            return false;
        }
        currentWorld = newWorld;
        loadedWorlds[worldFile] = currentWorld;
    }
    currentWorld->setupForPlay(*inputHandler);
    returnWorldStack.push_back(currentWorld);
    previousTime = SDL_GetTicks();
    return true;
}

bool GameEngine::LoadNewAndRemoveCurrent(const std::string &worldFile) {
    renderLoadingImage();
    World* temp = currentWorld;
    loadedWorlds.erase(temp->getName());
    returnOrLoadMap(worldFile);
    returnWorldStack.clear();
    returnWorldStack.push_back(currentWorld);
    previousTime = SDL_GetTicks();
    return true;
}

void GameEngine::returnPreviousMap() {
    if(returnWorldStack.size() >1) {
        returnWorldStack.pop_back();
        currentWorld = returnWorldStack[returnWorldStack.size()-1];
        currentWorld->setupForPlay(*inputHandler);
    }
    previousTime = SDL_GetTicks();
}

GameEngine::GameEngine() {
    options = new Options();

    options->loadOptions("./Engine/Options.xml");
    std::cout << "Options loaded successfully" << std::endl;

    sdlHelper = new SDL2Helper(PROGRAM_NAME.c_str(), options);
#ifdef _WIN32
    sdlHelper->loadSharedLibrary("libcustomTriggers.dll");
#elif __APPLE__
    sdlHelper->loadSharedLibrary("./libcustomTriggers.dylib");
#else
    sdlHelper.loadSharedLibrary("./libcustomTriggers.so");
#endif

    glHelper = new GLHelper(options);
    glHelper->reshape();

    ALHelper *alHelper = new ALHelper();

    inputHandler = new InputHandler(sdlHelper->getWindow(), options);
    assetManager = new AssetManager(glHelper, alHelper);

    worldLoader = new WorldLoader(assetManager, inputHandler, options);

    std::function<bool(const std::string &)> limonLoadWorld =
            bind(&GameEngine::loadAndChangeWorld, this, std::placeholders::_1);
    std::function<bool(const std::string &)> limonReturnOrLoadWorld =
            bind(&GameEngine::returnOrLoadMap, this, std::placeholders::_1);
    std::function<bool(const std::string &)> limonLoadNewAndRemoveCurrentWorld =
            bind(&GameEngine::LoadNewAndRemoveCurrent, this, std::placeholders::_1);
    std::function<void()> limonExitGame = [&] { this->setWorldQuit(); };
    std::function<void()> limonReturnPrevious = [&] { this->returnPreviousMap(); };


    limonAPI = new LimonAPI(limonLoadWorld, limonReturnOrLoadWorld, limonLoadNewAndRemoveCurrentWorld, limonExitGame,
                            limonReturnPrevious);

    //FIXME this image should not be hardcoded
    loadingImage = new GUIImage(0, options, assetManager, "loadingImage", "./Data/Textures/Menu/mayanMap-loading.png");
}

void GameEngine::run() {
    Uint32 worldUpdateTime = 1000 / 60;//This value is used to update world on a locked Timestep

    glHelper->clearFrame();
    previousTime = SDL_GetTicks();
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
            currentWorld->play(worldUpdateTime, *inputHandler);
            accumulatedTime -= worldUpdateTime;
        }
        glHelper->clearFrame();
        currentWorld->render();
        sdlHelper->swap();
    }
}

GameEngine::~GameEngine() {
    delete currentWorld;

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

    if(!game.loadAndChangeWorld(worldName)) {
        std::cerr << "WorldLoader didn't hand out a valid world. exiting.." << std::endl;
        exit(-1);
    }

    game.run();

    return 0;
}

