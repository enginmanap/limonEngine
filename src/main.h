//
// Created by engin on 3.08.2018.
//

#ifndef LIMONENGINE_MAIN_H
#define LIMONENGINE_MAIN_H


#include <string>

class World;
class WorldLoader;
class Options;
class ALHelper;
class GLHelper;
class InputHandler;
class AssetManager;
class SDL2Helper;
class LimonAPI;

class GameEngine {
    WorldLoader* worldLoader = nullptr;
    World* world = nullptr;
    bool worldQuit = false;

    Options* options = nullptr;
    ALHelper* alHelper = nullptr;
    GLHelper* glHelper = nullptr;
    InputHandler* inputHandler = nullptr;
    AssetManager* assetManager = nullptr;
    SDL2Helper* sdlHelper = nullptr;
    LimonAPI* limonAPI = nullptr;

public:

    GameEngine();
    ~GameEngine();

    World *getWorld() {
        return world;
    }

    bool isWorldQuit() const {
        return worldQuit;
    }

    void setWorldQuit() {
        GameEngine::worldQuit = true;
    }

    bool mainLoadAndChangeWorld(const std::string& worldFile);


    void run();
};


#endif //LIMONENGINE_MAIN_H
