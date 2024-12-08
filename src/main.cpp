//
// Created by Engin Manap on 10.02.2016.
//

#include "main.h"
#include "API/Graphics/GraphicsInterface.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "GameObjects/GUIImage.h"
#include <pthread.h>

const std::string PROGRAM_NAME = "LimonEngine";
const std::string RELEASE_FILE = "./Data/Release.xml";
const std::string OPTIONS_FILE = "./Engine/Options.xml";

bool GameEngine::loadAndChangeWorld(const std::string &worldFile) {
    graphicsWrapper->clearDepthBuffer();
    std::unique_ptr<std::string> loadingImagePath = worldLoader->getLoadingImage(worldFile);
    if(loadingImagePath != nullptr) {
        this->loadingImage = new GUIImage(0, options, assetManager, "loadingImage", *loadingImagePath);
        renderLoadingImage();
    }
    LimonAPI* apiInstance = getNewLimonAPI();
    World* newWorld = worldLoader->loadWorld(worldFile, apiInstance);
    if(newWorld == nullptr) {
        delete apiInstance;
        return false;
    }
    if(loadedWorlds.find(worldFile) != loadedWorlds.end()) {
        delete loadedWorlds[worldFile].second;
        delete loadedWorlds[worldFile].first;
    }
    if(currentWorld != nullptr) {
        currentWorld->setupForPauseOrStop();
    }
    currentWorld = newWorld;
    currentWorld->setupForPlay(*inputHandler);
    loadedWorlds[worldFile].first = currentWorld;
    loadedWorlds[worldFile].second = apiInstance;
    returnWorldStack.push_back(currentWorld);
    previousGameTime = SDL_GetTicks64();
    return true;
}

void GameEngine::renderLoadingImage() const {
    if(loadingImage != nullptr) {
        loadingImage->setFullScreen(true);
        //FIXME: this definition here seems wrong. I can't think of another way, we should explore
        std::shared_ptr<GraphicsProgram> imageRenderProgram = std::make_shared<GraphicsProgram>(assetManager.get(),"./Engine/Shaders/GUIImage/vertex.glsl",
                                                                                                     "./Engine/Shaders/GUIImage/fragment.glsl", false);
        sdlHelper->swap();
        loadingImage->renderWithProgram(imageRenderProgram, 0);
        sdlHelper->swap();
    }
}

bool GameEngine::returnOrLoadMap(const std::string &worldFile) {
    if(loadedWorlds.find(worldFile) != loadedWorlds.end()) {
        if(currentWorld != loadedWorlds[worldFile].first) {
            if (currentWorld != nullptr) {
                currentWorld->setupForPauseOrStop();
            }
            currentWorld = loadedWorlds[worldFile].first;
        }

    } else { //world is not in the map case
        renderLoadingImage();
        LimonAPI* apiInstance = getNewLimonAPI();
        World* newWorld = worldLoader->loadWorld(worldFile, apiInstance);
        if(newWorld == nullptr) {
            delete apiInstance;
            return false;
        }
        if (currentWorld != nullptr) {
            currentWorld->setupForPauseOrStop();
        }
        currentWorld = newWorld;
        loadedWorlds[worldFile].first = currentWorld;
        loadedWorlds[worldFile].second = apiInstance;
    }
    currentWorld->setupForPlay(*inputHandler);
    returnWorldStack.push_back(currentWorld);
    previousGameTime = SDL_GetTicks64();
    return true;
}

bool GameEngine::LoadNewAndRemoveCurrent(const std::string &worldFile) {
    if(currentWorld->getName() == worldFile) {
        return false;
    }
    renderLoadingImage();
    World* temp = currentWorld;
    LimonAPI* tempAPI = loadedWorlds[temp->getName()].second;
    loadedWorlds.erase(temp->getName());
    if(returnOrLoadMap(worldFile)) {
        //means success
        returnWorldStack.clear();
        returnWorldStack.push_back(currentWorld);
        delete tempAPI;
        delete temp;
    } else {
        //means new world load failed
        std::cerr << "World load for file " << worldFile << " failed" << std::endl;
        loadedWorlds[temp->getName()].first = temp;
        loadedWorlds[temp->getName()].second = tempAPI;
    }
    previousGameTime = SDL_GetTicks64();
    return true;
}

void GameEngine::returnPreviousMap() {
    if(returnWorldStack.size() >1) {
        returnWorldStack.pop_back();
        if (currentWorld != nullptr) {
            currentWorld->setupForPauseOrStop();
        }
        currentWorld = returnWorldStack[returnWorldStack.size()-1];
        currentWorld->setupForPlay(*inputHandler);
    }
    previousGameTime = SDL_GetTicks64();
}

GameEngine::GameEngine() {
    options = new OptionsUtil::Options();

    options->loadOptionsNew(OPTIONS_FILE);
    std::cout << "Options loaded successfully" << std::endl;

    sdlHelper = new SDL2Helper(options);

    std::string graphicsBackendFileName;
#ifdef _WIN32
    graphicsBackendFileName = "libGraphicsBackend.dll";
#elif __APPLE__
    graphicsBackendFileName = "./libGraphicsBackend.dylib";
#else
    graphicsBackendFileName = "./libGraphicsBackend.so";
#endif
    graphicsWrapper = sdlHelper->loadGraphicsBackend(graphicsBackendFileName, options);
    sdlHelper->initWindow(PROGRAM_NAME.c_str(), graphicsWrapper->getContextInformation());
    if(graphicsWrapper == nullptr) {
        std::cerr << "failed to load graphics backend. Please check " << graphicsBackendFileName << std::endl;
        exit(1);
    }
    if(!graphicsWrapper->createGraphicsBackend()) {
        std::cerr << "failed to create graphics backend. Please check " << graphicsBackendFileName << std::endl;
        exit(1);
    }
    graphicsWrapper->reshape();

#ifdef _WIN32
    sdlHelper->loadCustomTriggers("libcustomTriggers.dll");
#elif __APPLE__
    sdlHelper->loadCustomTriggers("./libcustomTriggers.dylib");
#else
    sdlHelper->loadCustomTriggers("./libcustomTriggers.so");
#endif

    alHelper = new ALHelper();

    inputHandler = new InputHandler(sdlHelper->getWindow(), options);
    assetManager = std::make_shared<AssetManager>(graphicsWrapper.get(), alHelper);

    worldLoader = new WorldLoader(assetManager, inputHandler, options);
}

LimonAPI *GameEngine::getNewLimonAPI() {
    std::function<bool(const std::string &)> limonLoadWorld =
            bind(&GameEngine::loadAndChangeWorld, this, std::placeholders::_1);
    std::function<bool(const std::string &)> limonReturnOrLoadWorld =
            bind(&GameEngine::returnOrLoadMap, this, std::placeholders::_1);
    std::function<bool(const std::string &)> limonLoadNewAndRemoveCurrentWorld =
            bind(&GameEngine::LoadNewAndRemoveCurrent, this, std::placeholders::_1);
    std::function<void()> limonExitGame = [&] { setWorldQuit(); };
    std::function<void()> limonReturnPrevious = [&] { returnPreviousMap(); };


    return new LimonAPI(limonLoadWorld, limonReturnOrLoadWorld, limonLoadNewAndRemoveCurrentWorld, limonExitGame,
                            limonReturnPrevious);
}

void GameEngine::run() {
    float worldUpdateTime = 1000 / TICK_PER_SECOND;//This value is used to update world on a locked Timestep

    graphicsWrapper->clearFrame();
    previousGameTime = SDL_GetTicks64();
    uint64_t currentGameTime, frameTime, accumulatedTime = 0;
    while (!worldQuit) {
        currentGameTime = SDL_GetTicks64();
        frameTime = currentGameTime - previousGameTime;
        previousGameTime = currentGameTime;
        accumulatedTime += frameTime;
        if (accumulatedTime >= worldUpdateTime) {
            //we don't need to check for input, if we won't update world state
            inputHandler->mapInput();

            //FIXME this does not account for long operations/low framerate
            currentWorld->play(worldUpdateTime, *inputHandler, SDL_GetTicks64());
            accumulatedTime -= worldUpdateTime;
        }
        graphicsWrapper->clearFrame();
        currentWorld->render();
        sdlHelper->swap();
    }
}

GameEngine::~GameEngine() {

    for (auto iterator = loadedWorlds.begin(); iterator != loadedWorlds.end(); ++iterator) {
        delete iterator->second.first;//delete world
        delete iterator->second.second;//delete API
    }

    this->assetManager = nullptr;
    delete worldLoader;
    delete inputHandler;
    delete alHelper;
    graphicsWrapper = nullptr;//FIXME this should be part of SdlHelper, because it is created and deleted by it. now it is order dependent because if it.
    delete sdlHelper;
    delete options;
}

bool getWorldNameFromReleaseXML(std::string &worldName) {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(RELEASE_FILE.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading release settings from "<< RELEASE_FILE << ": " <<  xmlDoc.ErrorName() << std::endl;
        return false;
    }

    tinyxml2::XMLNode* releaseNode = xmlDoc.FirstChild();
    if (releaseNode == nullptr) {
        std::cerr << "Release settings is not valid." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* startingWorldNode =  releaseNode->FirstChildElement("StartingWorld");
    if (startingWorldNode == nullptr || startingWorldNode->GetText() == nullptr) {
        std::cerr << "Starting world is not set." << std::endl;
        return false;
    }
    std::cout << "read starting world as " << startingWorldNode->GetText() << std::endl;
    std::string worldNameTemp = startingWorldNode->GetText();
    worldName = worldNameTemp;
    return true;
}

int main(int argc, char *argv[]) {

    std::string worldName;
    if(argc == 1) {
        std::cout << "No world file specified, world select from release settings" << std::endl;
        if(!getWorldNameFromReleaseXML(worldName)) {
            std::cout << "release settings read failed, defaulting to ./Data/Maps/World001.xml" << std::endl;
            worldName = "./Data/Maps/World001.xml";
        }
    } else if(argc == 2) {
        worldName = argv[1];
        std::cout << "Trying to load " <<  worldName << std::endl;
    } else {
        worldName = argv[1];
        std::cout << "Trying to load " <<  worldName << std::endl;
        std::cout << PROGRAM_NAME + " only takes one parameter. First one is processed as Map file, rest discarded." << std::endl;
    }

    pthread_setname_np(pthread_self(), "Main thread");

    GameEngine game;

    if(!game.loadAndChangeWorld(worldName)) {
        std::cerr << "WorldLoader didn't hand out a valid world. exiting.." << std::endl;
        exit(-1);
    }

    game.run();

    return 0;
}

