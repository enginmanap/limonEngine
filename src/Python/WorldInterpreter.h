#pragma once

#include <pybind11/subinterpreter.h>
#include <pybind11/pybind11.h>
#include <string>

/**
 * WorldInterpreter - Wrapper for a single pybind11 subinterpreter
 */
class WorldInterpreter {
    friend class ScriptManager;
    explicit WorldInterpreter(const std::string& worldName) : worldName(worldName) {
#ifdef PYTHON_DEBUGGING
        std::cout << "[WorldInterpreter] Created subinterpreter for world: " << worldName << std::endl;
#endif
        subInterpreter = std::make_unique<pybind11::subinterpreter>(pybind11::subinterpreter::create());
#ifdef PYTHON_DEBUGGING
        std::cout << "[WorldInterpreter] Subinterpreter created: " << worldName << std::endl;
#endif
    }

    ~WorldInterpreter() {
#ifdef PYTHON_DEBUGGING
        std::cout << "[WorldInterpreter] Destroying subinterpreter for world: " << worldName << std::endl;
#endif
        
        // Deactivate if still active - this must happen before any other cleanup
        if (activationGuard) {
            activationGuard.reset();
        }
        
        // Force cleanup of any remaining Python objects in this subinterpreter
        if (Py_IsInitialized() && subInterpreter) {
            // Switch to this subinterpreter temporarily to clean up
            pybind11::subinterpreter_scoped_activate temp_activate(*subInterpreter);

            // Clear any pending exceptions
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }

            // Force garbage collection to clean up any remaining objects
            pybind11::module_ gc = pybind11::module_::import("gc");
            (void) gc.attr("collect")();
        }

        if (subInterpreter) {
            subInterpreter.reset();
        }
        
#ifdef PYTHON_DEBUGGING
        std::cout << "[WorldInterpreter] Subinterpreter destroyed: " << worldName << std::endl;
#endif
    }

    std::unique_ptr<pybind11::subinterpreter> subInterpreter;
    std::string worldName;
    std::unique_ptr<pybind11::subinterpreter_scoped_activate> activationGuard;

public:
    const std::string& getWorldName() const {
        return worldName;
    }
    
    WorldInterpreter* activate() {
        if (!activationGuard) {
#ifdef PYTHON_DEBUGGING
            std::cout << "[WorldInterpreter] Activating subinterpreter for world: " << worldName << std::endl;
#endif
            activationGuard = std::make_unique<pybind11::subinterpreter_scoped_activate>(*subInterpreter);
#ifdef PYTHON_DEBUGGING
            std::cout << "[WorldInterpreter] Subinterpreter activated for world: " << worldName << std::endl;
#endif
        }
        return this;
    }
    
    void deactivate() {
        if (activationGuard) {
#ifdef PYTHON_DEBUGGING
            std::cout << "[WorldInterpreter] Deactivating subinterpreter for world: " << worldName << std::endl;
#endif
            activationGuard.reset();
#ifdef PYTHON_DEBUGGING
            std::cout << "[WorldInterpreter] Subinterpreter deactivated for world: " << worldName << std::endl;
#endif
        }
    }
};
