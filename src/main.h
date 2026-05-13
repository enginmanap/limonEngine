//
// Created by engin on 3.08.2018.
//

#ifndef LIMONENGINE_MAIN_H
#define LIMONENGINE_MAIN_H


#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "limonAPI/Options.h"

class World;
class WorldLoader;
class ALHelper;
class GraphicsInterface;
class InputHandler;
class AssetManager;
class SDL2Helper;
class LimonAPI;
class GUIImage;
class ScriptManager;
class ProfilerSystem;

class GameEngine {
    WorldLoader* worldLoader = nullptr;
    World* currentWorld = nullptr;
    bool worldQuit = false;

    OptionsUtil::Options* options = nullptr;
    ALHelper* alHelper = nullptr;
    std::shared_ptr<GraphicsInterface> graphicsWrapper = nullptr;
    InputHandler* inputHandler = nullptr;
    std::shared_ptr<AssetManager> assetManager = nullptr;
    SDL2Helper* sdlHelper = nullptr;
    ProfilerSystem* profilerSystem = nullptr;

    std::unordered_map<std::string, std::pair<World*, LimonAPI*>> loadedWorlds;

    std::vector<World*> returnWorldStack;//stack doesn't have clear, so I am using vector

    // Worlds queued for deletion after the current play() frame completes.
    // LoadNewAndRemoveCurrent defers deletion here instead of deleting mid-play()
    // to prevent use-after-free when a trigger causes a world switch while
    // World::play() is still on the stack.
    struct PendingDelete { std::string name; World* world; LimonAPI* api; };
    std::vector<PendingDelete> pendingWorldDeletes;
    GUIImage* loadingImage = nullptr;
    uint64_t previousGameTime = 0;

    ScriptManager* scriptManager = nullptr;

    // World switch requested by an API call during play(). Applied after the tick
    // completes so play() always runs to the end of the tick.
    enum class PendingSwitchType { NONE, LOAD_AND_CHANGE, RETURN_OR_LOAD, LOAD_NEW_REMOVE_CURRENT, RETURN_PREVIOUS };
    struct PendingSwitch {
        PendingSwitchType type = PendingSwitchType::NONE;
        std::string worldFile;
    } pendingSwitch;

    void applyPendingSwitch();

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
