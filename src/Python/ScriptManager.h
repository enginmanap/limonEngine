//
// Created by engin on 25/12/2025.
//

#ifndef LIMONENGINE_PYTHONSYSTEM_H
#define LIMONENGINE_PYTHONSYSTEM_H

#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>
#include "pybind11/stl_bind.h"
#include <filesystem>
#include <set>
#include <vector>

// Check if we have C++17 support
#if !defined(PYBIND11_CPP17)
#error "C++17 support is required. Please enable C++17 in your compiler settings."
#endif
#include <limonAPI/LimonTypes.h>
#include <limonAPI/LimonAPI.h>
#include <limonAPI/PlayerExtensionInterface.h>
#include <limonAPI/ActorInterface.h>
#include <limonAPI/TriggerInterface.h>
#include <limonAPI/CameraExtensionInterface.h>

class WorldInterpreter;
class ScriptManager {
    // Ordered list of directories scanned for scripts. The first entry is the
    // built-in directory (./Engine/Scripts) and is authoritative; the rest are
    // user directories (./Data/Scripts). Order matters: built-ins take
    // precedence so user scripts can import the built-in base classes, and a
    // user script may not shadow a built-in module name.
    const std::vector<std::string> scriptDirectories;

    WorldInterpreter* activeSubinterpreter = nullptr;
    std::unordered_map<std::string, WorldInterpreter*> subInterpreters;

    //Static because python allows only one instance of this per process.
    static pybind11::scoped_interpreter* mainInterpreterGuard;

    enum class CallBackTypes {
        TRIGGER,
        PLAYER_EXTENSION,
        ACTOR,
        CAMERA_EXTENSION
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
    // builtinDirectoryPath is the engine's own scripts (./Engine/Scripts);
    // userDirectoryPath is the game-provided scripts (./Data/Scripts). The user
    // directory is optional - it is silently skipped if it does not exist.
    ScriptManager(const std::string& builtinDirectoryPath, const std::string& userDirectoryPath);
    ~ScriptManager();

    explicit ScriptManager(ScriptManager& other) = delete;
    ScriptManager& operator=(ScriptManager other) = delete;

    WorldInterpreter* createWorldInterpreter(const std::string& worldName);
    void removeWorldInterpreter(const std::string& worldName);

    void setActiveSubInterpreter(const std::string& worldName);

    void runGC();

    static TriggerInterface* CreateTriggerWrapper(LimonAPI* api, size_t index);
    static PlayerExtensionInterface* CreatePlayerExtensionWrapper(LimonAPI* api, size_t index);
    static ActorInterface* CreateActorWrapper(uint32_t id, LimonAPI* api, size_t index);
    static CameraExtensionInterface* CreateCameraExtensionWrapper(LimonAPI* api, size_t index);
private:
    // Appends every configured script directory to sys.path, in order and
    // without introducing duplicates. Built-in directory is added first so its
    // modules win on name collisions during import.
    void appendScriptDirectoriesToSysPath(pybind11::module_& sys) {
        pybind11::list pathList = sys.attr("path");
        for (const std::string& directoryPath : scriptDirectories) {
            bool alreadyPresent = false;
            for (pybind11::handle entry : pathList) {
                if (pybind11::isinstance<pybind11::str>(entry) &&
                    entry.cast<std::string>() == directoryPath) {
                    alreadyPresent = true;
                    break;
                }
            }
            if (!alreadyPresent) {
                (void) sys.attr("path").attr("append")(directoryPath);
            }
        }
    }

    // Scans every configured directory and loads every compatible script.
    // Built-in directory is scanned first; a user script whose module name
    // collides with an already-seen (built-in or earlier) module is skipped,
    // because Python caches modules by name and the first one wins on import.
    void loadScripts(WorldInterpreter * worldInterpreter) {
        std::set<std::string> seenModuleNames;
        bool isBuiltinDirectory = true;
        for (const std::string& directoryPath : scriptDirectories) {
            if (!std::filesystem::exists(directoryPath)) {
                if (isBuiltinDirectory) {
                    // The built-in directory is required; its absence is an error.
                    std::cerr << "[Scripting] Built-in script directory not found: " << directoryPath << std::endl;
                } else {
                    // User directories are optional.
                    std::cout << "[Scripting] No user script directory at: " << directoryPath << " (skipping)" << std::endl;
                }
                isBuiltinDirectory = false;
                continue;
            }

            std::cout << "[Scripting] Scanning: " << directoryPath << "..." << std::endl;

            for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                if (entry.path().extension() == ".py" &&
                    entry.path().filename() != "__init__.py") {
                    std::string moduleName = entry.path().stem().string();
                    if (seenModuleNames.count(moduleName) != 0) {
                        std::cerr << "[Scripting] Skipping '" << entry.path().string()
                                  << "': module name '" << moduleName
                                  << "' is already provided by a built-in script. Rename the user script to load it."
                                  << std::endl;
                        continue;
                    }
                    seenModuleNames.insert(moduleName);
                    LoadScript(worldInterpreter, moduleName);
                }
            }
            isBuiltinDirectory = false;
        }
    }

    // Identity-based subclass check against a canonical pure-Python base class.
    // moduleName/className name one of the Engine/Scripts base classes
    // (e.g. "trigger_interface"/"TriggerInterface"). The base is imported in the
    // CURRENT interpreter, so the comparison uses the same class object the
    // candidate's `from <module> import <Class>` produced — correct per
    // sub-interpreter. Matching by identity (not by __name__ string) means a
    // subclass of the unrelated C++ type `limon.TriggerInterface` no longer
    // matches: scripts must subclass the local pure-Python base. The base class
    // itself is excluded (issubclass(Base, Base) is True).
    bool IsSubclassOf(const pybind11::object& obj, const std::string& moduleName, const std::string& className) {
        try {
            pybind11::object base = pybind11::module_::import(moduleName.c_str()).attr(className.c_str());
            if (obj.is(base)) {
                return false;
            }
            int result = PyObject_IsSubclass(obj.ptr(), base.ptr());
            if (result < 0) {
                PyErr_Clear();
                return false;
            }
            return result == 1;
        } catch (...) {
            return false;
        }
    }
};

#endif //LIMONENGINE_PYTHONSYSTEM_H
