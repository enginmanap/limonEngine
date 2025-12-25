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
#include <limonAPI/TriggerInterface.h>

class ScriptManager {
private:
    // Holds the Python instance. Since Script Manager is per world, this would be also per world
    pybind11::scoped_interpreter guard{};

    // A list of active script INSTANCES (the objects created from the classes)
    std::vector<pybind11::object> activeScripts;
    const std::string directoryPath;
public:
    explicit ScriptManager(const std::string& directoryPath) : directoryPath(directoryPath) {
        // 1. Add the 'directoryPath' to Python's sys.path so it can be imported by other scripts
        try {
            auto sys = pybind11::module::import("sys");
            auto limon = pybind11::module::import("limon");
            sys.attr("path").attr("append")(directoryPath);

            // 2. Redirect Python's stdout to our C++ object. This object is just horribly inefficient, but I assume only usecase for print in python would be troubleshooting, and batching would make that way harder.
            auto redirector = limon.attr("StdOut")();
            // Tell Python: "Don't write to the console buffer, write to my C++ object"
            sys.attr("stdout") = redirector;
            sys.attr("stderr") = redirector; // Catch errors
        } catch (const std::exception& e) {
            std::cerr << "[Scripting] Error setting path: " << e.what() << std::endl;
        }
    }
    enum class CallBackTypes {
        TRIGGER,
        PLAYER_EXTENSION
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