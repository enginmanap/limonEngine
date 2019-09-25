//
// Created by Engin Manap on 10.02.2016.
//

#include "main.h"
#include "Graphics/OpenGLGraphics.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "ALHelper.h"
#include "GameObjects/GUIImage.h"

const std::string PROGRAM_NAME = "LimonEngine";
const std::string RELEASE_FILE = "./Data/Release.xml";
const std::string OPTIONS_FILE = "./Engine/Options.xml";

bool GameEngine::loadAndChangeWorld(const std::string &worldFile) {
    glHelper->clearDepthBuffer();
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
    previousTime = SDL_GetTicks();
    return true;
}

void GameEngine::renderLoadingImage() const {
    if(loadingImage != nullptr) {
        loadingImage->setFullScreen(true);
        //FIXME: this definition here seems wrong. I can't think of another way, we should explore
        std::shared_ptr<GLSLProgram> imageRenderProgram = glHelper->createGLSLProgram("./Engine/Shaders/GUIImage/vertex.glsl", "./Engine/Shaders/GUIImage/fragment.glsl", false);
        sdlHelper->swap();
        loadingImage->renderWithProgram(imageRenderProgram);
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
    previousTime = SDL_GetTicks();
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
    previousTime = SDL_GetTicks();
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
    previousTime = SDL_GetTicks();
}

GameEngine::GameEngine() {
    options = new Options();

    options->loadOptions(OPTIONS_FILE);
    std::cout << "Options loaded successfully" << std::endl;

    sdlHelper = new SDL2Helper(PROGRAM_NAME.c_str(), options);
#ifdef _WIN32
    sdlHelper->loadSharedLibrary("libcustomTriggers.dll");
#elif __APPLE__
    sdlHelper->loadSharedLibrary("./libcustomTriggers.dylib");
#else
    sdlHelper->loadSharedLibrary("./libcustomTriggers.so");
#endif

    glHelper = new OpenGLGraphics(options);
    glHelper->reshape();

    alHelper = new ALHelper();

    inputHandler = new InputHandler(sdlHelper->getWindow(), options);
    assetManager = new AssetManager(glHelper, alHelper);

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

    for (auto iterator = loadedWorlds.begin(); iterator != loadedWorlds.end(); ++iterator) {
        delete iterator->second.first;//delete world
        delete iterator->second.second;//delete API
    }

    delete worldLoader;

    delete assetManager;

    delete inputHandler;

    delete alHelper;
    delete glHelper;

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

    GameEngine game;

    if(!game.loadAndChangeWorld(worldName)) {
        std::cerr << "WorldLoader didn't hand out a valid world. exiting.." << std::endl;
        exit(-1);
    }

    game.run();

    return 0;
}

