//
// Created by engin on 25/12/2025.
//

#ifndef LIMONENGINE_PYTHONSYSTEM_H
#define LIMONENGINE_PYTHONSYSTEM_H

#include <pybind11/embed.h>
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

class ScriptManager {
private:
    // Holds the Python instance. Since Script Manager is per world, this would be also per world

    // A list of active script INSTANCES (the objects created from the classes)
    std::vector<pybind11::object> activeScripts;
    const std::string directoryPath;
public:
    explicit ScriptManager(const std::string& directoryPath);
    ~ScriptManager() {
        pybind11::finalize_interpreter();
    }

    explicit ScriptManager(ScriptManager& other) = delete;
    ScriptManager& operator=(ScriptManager other) = delete;

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

    static TriggerInterface* CreateTriggerWrapper(LimonAPI* api, size_t index);

    static PlayerExtensionInterface* CreatePlayerExtensionWrapper(LimonAPI* api, size_t index);

    static ActorInterface* CreateActorWrapper(uint32_t id, LimonAPI* api, size_t index);

    // scans the directory and loads every compatible script
    void LoadScripts() {
        if (!std::filesystem::exists(directoryPath)) {
            std::cerr << "[Scripting] Directory not found: " << directoryPath << std::endl;
            return;
        }

        std::cout << "[Scripting] Scanning: " << directoryPath << "..." << std::endl;

        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.path().extension() == ".py" &&
                entry.path().filename() != "__init__.py") {
                std::string moduleName = entry.path().stem().string();
                LoadScript(moduleName);
            }
        }
    }

private:
    void LoadScript(const std::string& moduleName);

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