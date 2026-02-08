//
// Created by engin on 25/12/2025.
//

#ifndef LIMONENGINE_PYTHONSYSTEM_H
#define LIMONENGINE_PYTHONSYSTEM_H

#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>
#include "pybind11/stl_bind.h"
#include <filesystem>

// Check if we have C++17 support
#if !defined(PYBIND11_CPP17)
#error "C++17 support is required. Please enable C++17 in your compiler settings."
#endif
#include <limonAPI/LimonTypes.h>
#include <limonAPI/LimonAPI.h>
#include <limonAPI/PlayerExtensionInterface.h>
#include <limonAPI/ActorInterface.h>
#include <limonAPI/TriggerInterface.h>

class WorldInterpreter;
class ScriptManager {
    const std::string directoryPath;

    WorldInterpreter* activeSubinterpreter = nullptr;
    std::unordered_map<std::string, WorldInterpreter*> subInterpreters;

    //Static because python allows only one instance of this per process.
    static pybind11::scoped_interpreter* mainInterpreterGuard;

    enum class CallBackTypes {
        TRIGGER,
        PLAYER_EXTENSION,
        ACTOR
    };
    struct PythonCallback {
        pybind11::object pyClass;
        CallBackTypes callBackType;
    };

    static std::vector<PythonCallback>& GetCallbacks() {
        static std::vector<PythonCallback> callbacks;
        return callbacks;
    }
    void LoadScript(WorldInterpreter * worldInterpreter, const std::string& moduleName);

    void removeWorldInterpreterInternal(WorldInterpreter* world_interpreter);
public:
    explicit ScriptManager(const std::string& directoryPath);
    ~ScriptManager();

    explicit ScriptManager(ScriptManager& other) = delete;
    ScriptManager& operator=(ScriptManager other) = delete;

    WorldInterpreter* createWorldInterpreter(const std::string& worldName);
    void removeWorldInterpreter(const std::string& worldName);

    void setActiveSubInterpreter(const std::string& worldName);

    static TriggerInterface* CreateTriggerWrapper(LimonAPI* api, size_t index);
    static PlayerExtensionInterface* CreatePlayerExtensionWrapper(LimonAPI* api, size_t index);
    static ActorInterface* CreateActorWrapper(uint32_t id, LimonAPI* api, size_t index);
private:
    // scans the directory and loads every compatible script
    void loadScripts(WorldInterpreter * worldInterpreter) {
        if (!std::filesystem::exists(directoryPath)) {
            std::cerr << "[Scripting] Directory not found: " << directoryPath << std::endl;
            return;
        }

        std::cout << "[Scripting] Scanning: " << directoryPath << "..." << std::endl;

        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.path().extension() == ".py" &&
                entry.path().filename() != "__init__.py") {
                std::string moduleName = entry.path().stem().string();
                LoadScript(worldInterpreter, moduleName);
                }
        }
    }

    bool IsSubclassOf(const pybind11::object& obj, const std::string& baseName) {
        try {
            pybind11::object bases = obj.attr("__bases__");
            for (auto base : bases) {
                // Convert base (which is a handle) to an object
                pybind11::object baseObj = pybind11::reinterpret_borrow<pybind11::object>(base);
                std::string baseTypeName = pybind11::str(baseObj.attr("__name__"));

                if (baseTypeName == baseName) {
                    return true;
                }
                // Check base classes recursively
                if (IsSubclassOf(baseObj, baseName)) {
                    return true;
                }
            }
            return false;
        } catch (...) {
            return false;
        }
    }
};

#endif //LIMONENGINE_PYTHONSYSTEM_H
