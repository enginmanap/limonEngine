//
// Created by Engin Manap on 10.02.2016.
//

#include "main.h"
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "SDL2Helper.h"
#include "World.h"
#include "WorldLoader.h"
#include "GameObjects/GUIImage.h"
#include "Python/ScriptManager.h"
#include "Profiler/ProfilerSystem.h"
#include <pthread.h>
#include "Profiler/ProfilerMacros.h"

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
    if(currentWorld != nullptr) {
        currentWorld->setupForPauseOrStop();//We have to make sure occlusion threads stopped before interpreter change
    }
#ifdef PYTHON_DEBUGGING
    std::cout << "[ScriptManager] calling create world Interpreter for " << worldFile << std::endl;
#endif
    scriptManager->createWorldInterpreter(worldFile);
    scriptManager->setActiveSubInterpreter(worldFile);

    World* newWorld = worldLoader->loadWorld(worldFile, apiInstance);
    
    if(newWorld == nullptr) {
        delete apiInstance;
        scriptManager->removeWorldInterpreter(worldFile);
        return false;
    }

    if(loadedWorlds.find(worldFile) != loadedWorlds.end()) {
        delete loadedWorlds[worldFile].second;
        delete loadedWorlds[worldFile].first;
        scriptManager->removeWorldInterpreter(worldFile);
    }
    currentWorld = newWorld;
    scriptManager->setActiveSubInterpreter(worldFile);
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
                                                                                                     "./Engine/Shaders/GUIImage/fragment.glsl");
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
            currentWorld->setupForUnpause();
            // Activate the corresponding interpreter when switching to this world
            scriptManager->setActiveSubInterpreter(worldFile);
        }

    } else { //world is not in the map case
        renderLoadingImage();
        LimonAPI* apiInstance = getNewLimonAPI();
        if (currentWorld != nullptr) {
            currentWorld->setupForPauseOrStop();//We have to make sure occlusion threads stopped before interpreter change
        }
        scriptManager->createWorldInterpreter(worldFile);
        scriptManager->setActiveSubInterpreter(worldFile);
        World* newWorld = worldLoader->loadWorld(worldFile, apiInstance);
        if(newWorld == nullptr) {
            delete apiInstance;
            scriptManager->removeWorldInterpreter(worldFile);
            return false;
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
    std::string oldWorldName = temp->getName();
    loadedWorlds.erase(oldWorldName);
    if(returnOrLoadMap(worldFile)) {
        //means success
        returnWorldStack.clear();
        returnWorldStack.push_back(currentWorld);
        // Defer deletion: if this was called from inside World::play() (e.g. via a
        // trigger), deleting temp here would free the object whose play() is still
        // on the stack.  Queue it for deletion after play() returns instead.
        pendingWorldDeletes.push_back({oldWorldName, temp, tempAPI});
    } else {
        //means new world load failed
        std::cerr << "World load for file " << worldFile << " failed" << std::endl;
        loadedWorlds[oldWorldName].first = temp;
        loadedWorlds[oldWorldName].second = tempAPI;
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
        scriptManager->setActiveSubInterpreter(currentWorld->getName());
        currentWorld->setupForUnpause();
        currentWorld->setupForPlay(*inputHandler);
    }
    previousGameTime = SDL_GetTicks64();
}

GameEngine::GameEngine() {
    scriptManager = new ScriptManager("./Engine/Scripts");

    options = new OptionsUtil::Options([](){return SDL_GetTicks();});

    options->loadOptionsNew(OPTIONS_FILE);
    std::cout << "Options loaded successfully" << std::endl;
    profilerSystem = new ProfilerSystem(options);

    sdlHelper = new SDL2Helper(options);

    std::string backendName = options->getOption<std::string>(options->getHash("GraphicsBackend")).getOrDefault("libOpenGLGraphicsBackend");
    std::cout << "Selected Graphics Backend: " << backendName << std::endl;

    std::string graphicsBackendFileName = "./";

#ifdef _WIN32
    graphicsBackendFileName += backendName + ".dll";
#elif __APPLE__
    graphicsBackendFileName += backendName + ".dylib";
#else
    graphicsBackendFileName += backendName + ".so";
#endif
    graphicsWrapper = sdlHelper->loadGraphicsBackend(graphicsBackendFileName, options);
    if(graphicsWrapper == nullptr) {
        std::cerr << "failed to load graphics backend. Please check " << graphicsBackendFileName << std::endl;
        exit(1);
    }
    sdlHelper->initWindow(PROGRAM_NAME.c_str(), graphicsWrapper->getContextInformation());
    if(!sdlHelper->createContext()) {
        std::cout << "Context creation failed, trying fallback..." << std::endl;
        sdlHelper->destroyWindow();
        GraphicsInterface::ContextInformation fallbackContext;
        if(graphicsWrapper->getFallbackContextInformation(fallbackContext)) {
            sdlHelper->initWindow(PROGRAM_NAME.c_str(), fallbackContext);
            if(!sdlHelper->createContext()) {
                std::cerr << "Failed to create fallback context. Exiting." << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Failed to create context and no fallback is available. Exiting." << std::endl;
            exit(1);
        }
    }
    
    if(!graphicsWrapper->verifyContext()) {
        std::cerr << "Context verification failed." << std::endl;
        exit(1);
    }

    if(!graphicsWrapper->createGraphicsBackend()) {
        std::cerr << "failed to create graphics backend. Please check " << graphicsBackendFileName << std::endl;
        exit(1);
    }
    graphicsWrapper->initGpuContext();
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

    worldLoader = new WorldLoader(assetManager, inputHandler, options, profilerSystem);
}

void GameEngine::applyPendingSwitch() {
    PendingSwitch ps = pendingSwitch;
    pendingSwitch = {};
    switch (ps.type) {
        case PendingSwitchType::LOAD_AND_CHANGE:         loadAndChangeWorld(ps.worldFile);       break;
        case PendingSwitchType::RETURN_OR_LOAD:          returnOrLoadMap(ps.worldFile);           break;
        case PendingSwitchType::LOAD_NEW_REMOVE_CURRENT: LoadNewAndRemoveCurrent(ps.worldFile);   break;
        case PendingSwitchType::RETURN_PREVIOUS:         returnPreviousMap();                     break;
        default: break;
    }
}

LimonAPI *GameEngine::getNewLimonAPI() {
    std::function<bool(const std::string &)> limonLoadWorld = [this](const std::string& worldFile) {
        pendingSwitch = {PendingSwitchType::LOAD_AND_CHANGE, worldFile};
        return true;
    };
    std::function<bool(const std::string &)> limonReturnOrLoadWorld = [this](const std::string& worldFile) {
        pendingSwitch = {PendingSwitchType::RETURN_OR_LOAD, worldFile};
        return true;
    };
    std::function<bool(const std::string &)> limonLoadNewAndRemoveCurrentWorld = [this](const std::string& worldFile) {
        pendingSwitch = {PendingSwitchType::LOAD_NEW_REMOVE_CURRENT, worldFile};
        return true;
    };
    std::function<void()> limonExitGame = [this] { setWorldQuit(); };
    std::function<void()> limonReturnPrevious = [this] {
        pendingSwitch = {PendingSwitchType::RETURN_PREVIOUS, ""};
    };
    std::function<const OptionsUtil::Options*()> limonGetOptions = [this] { return options; };

    return new LimonAPI(limonLoadWorld, limonReturnOrLoadWorld, limonLoadNewAndRemoveCurrentWorld, limonExitGame,
                            limonReturnPrevious, limonGetOptions);
}

void GameEngine::run() {
    float worldUpdateTime = 1000 / TICK_PER_SECOND;//This value is used to update world on a locked Timestep

    graphicsWrapper->clearFrame();
    previousGameTime = SDL_GetTicks64();
    uint64_t currentGameTime, frameTime, accumulatedTime = 0;
    while (!worldQuit) {
        PROFILE_OVERALL("Frame");
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

            if (pendingSwitch.type != PendingSwitchType::NONE && !worldQuit) {
                applyPendingSwitch();
            }

            // Process worlds that were queued for deletion during play().
            // Must happen after play() returns so we never delete a World whose
            // play() method is still on the call stack.
            if (!pendingWorldDeletes.empty()) {
                for (auto& pd : pendingWorldDeletes) {
                    scriptManager->setActiveSubInterpreter(pd.name);
                    delete pd.api;
                    delete pd.world;
                    scriptManager->removeWorldInterpreter(pd.name);
                }
                pendingWorldDeletes.clear();
                // Restore the current world's interpreter after cleanup
                if (currentWorld != nullptr) {
                    scriptManager->setActiveSubInterpreter(currentWorld->getName());
                }
            }
        }
        graphicsWrapper->clearFrame();
        {
            PROFILE_RENDERING("Render");
            currentWorld->setupRender();
            currentWorld->render();
        }
        sdlHelper->swap();
        graphicsWrapper->collectGpuProfilingData();
        if (profilerSystem) profilerSystem->Update();
        PROFILE_FRAME();
    }
}

GameEngine::~GameEngine() {
    for (auto iterator = loadedWorlds.begin(); iterator != loadedWorlds.end(); ++iterator) {
        scriptManager->setActiveSubInterpreter(iterator->first);
        // Run GC before deleting the world so that cyclic Python references (e.g. actor
        // objects holding a ref to themselves via a callback) are broken while the
        // sub-interpreter's pybind11 internals and C++ objects are still alive.
        // Without this, those cyclic objects survive until Py_Finalize() in the main
        // interpreter, where the pybind11 internals are gone and all_type_info() returns
        // an empty vector, triggering assert(!types->empty()) in clear_instance().
        scriptManager->runGC();
        delete iterator->second.first;//delete world
        delete iterator->second.second;//delete API
        scriptManager->removeWorldInterpreter(iterator->first);
    }

    delete scriptManager;
    this->assetManager = nullptr;
    delete worldLoader;
    delete inputHandler;
    delete alHelper;
    graphicsWrapper = nullptr;//FIXME this should be part of SdlHelper, because it is created and deleted by it. now it is order dependent because if it.
    delete sdlHelper;
    delete profilerSystem;
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
