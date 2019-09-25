//
// Created by engin on 3.08.2018.
//

#ifndef LIMONENGINE_MAIN_H
#define LIMONENGINE_MAIN_H


#include <string>
#include <vector>
#include <unordered_map>

class World;
class WorldLoader;
class Options;
class ALHelper;
class GraphicsInterface;
class InputHandler;
class AssetManager;
class SDL2Helper;
class LimonAPI;
class GUIImage;

class GameEngine {
    WorldLoader* worldLoader = nullptr;
    World* currentWorld = nullptr;
    bool worldQuit = false;

    Options* options = nullptr;
    ALHelper* alHelper = nullptr;
    GraphicsInterface* graphicsWrapper = nullptr;
    InputHandler* inputHandler = nullptr;
    AssetManager* assetManager = nullptr;
    SDL2Helper* sdlHelper = nullptr;

    std::unordered_map<std::string, std::pair<World*, LimonAPI*>> loadedWorlds;
    std::vector<World*> returnWorldStack;//stack doesn't have clear, so I am using vector
    GUIImage* loadingImage = nullptr;
    uint32_t previousTime = 0;
public:

    GameEngine();
    ~GameEngine();

    void setWorldQuit() {
        GameEngine::worldQuit = true;
    }

    bool loadAndChangeWorld(const std::string &worldFile);

    void returnPreviousMap();

    bool returnOrLoadMap(const std::string &worldFile);
    bool LoadNewAndRemoveCurrent(const std::string &worldFile);

    void run();

    void renderLoadingImage() const;

    LimonAPI *getNewLimonAPI();
};


#endif //LIMONENGINE_MAIN_H
