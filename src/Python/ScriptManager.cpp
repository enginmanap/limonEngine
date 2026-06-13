//
// Created by engin on 25/12/2025.
//

#include <Python.h>
#include "ScriptManager.h"
#include "WorldInterpreter.h"
#include "PyTriggerInterface.h"
#include "PyPlayerExtensionInterface.h"
#include "PyActorInterface.h"

#include <utility>

// Prevents MinGW/ld from discarding PythonModuleBindings.cpp (dead-code elimination).
// That TU has no symbols referenced here except this anchor; without it the
// PYBIND11_EMBEDDED_MODULE static registration never runs and 'limon' cannot be imported.
extern void ensurePythonModuleBindingsLinked();

#ifndef PYTHON_BUNDLE_ZIP
#error "PYTHON_BUNDLE_ZIP must be defined by the build system"
#endif

pybind11::scoped_interpreter* ScriptManager::mainInterpreterGuard = nullptr;

namespace {
    // since TriggerInterface, PlayerExtensionInterface and ActorInterface have protected constructors, we need to instantiate them in a different way.
    // I tried to use factory pattern but it didn't work. This is some crazy template magic that creates MAX_PYTHON_SCRIPT_COUNT number creators,
    // so you can instantiate them

    template<size_t N>
    TriggerInterface* CreateTriggerByIndex(LimonAPI* api) {
        return ScriptManager::CreateTriggerWrapper(api, N);
    }

    template<size_t N>
    PlayerExtensionInterface* CreatePlayerExtensionByIndex(LimonAPI* api) {
        return ScriptManager::CreatePlayerExtensionWrapper(api, N);
    }

    template<size_t N>
    ActorInterface* CreateActorByIndex(uint32_t id, LimonAPI* api) {
        return ScriptManager::CreateActorWrapper(id, api, N);
    }

    template<int N>
    struct template_instantiator {
        static void instantiate() {
            TriggerInterface* (*trigger_func)(LimonAPI*) = &CreateTriggerByIndex<N>;
            PlayerExtensionInterface* (*extension_func)(LimonAPI*) = &CreatePlayerExtensionByIndex<N>;
            ActorInterface* (*actor_func)(uint32_t, LimonAPI*) = &CreateActorByIndex<N>;
            (void)trigger_func;
            (void)extension_func;
            (void)actor_func;
        }
    };

    template<int START, int END>
    struct generate_range {
        static_assert(START <= END, "Invalid range");
        static void generate() {
            template_instantiator<START>::instantiate();
            generate_range<START + 1, END>::generate();
        }
    };

    template<int N>
    struct generate_range<N, N> {
        static void generate() {
            template_instantiator<N>::instantiate();
        }
    };

#define GENERATE_UP_TO(MAX) \
static_assert(MAX > 0, "MAX must be greater than 0"); \
namespace { \
const struct _template_initializer { \
_template_initializer() { \
generate_range<0, (MAX)-1>::generate(); \
} \
} _initializer; \
}

    GENERATE_UP_TO(MAX_PYTHON_SCRIPT_COUNT);

#undef GENERATE_UP_TO

}

template<size_t... Is>
static constexpr TriggerInterface* (*GetTriggerFactoryHelper(std::index_sequence<Is...>, size_t index))(LimonAPI*) {
    constexpr std::array<TriggerInterface* (*)(LimonAPI*), sizeof...(Is)> factories = {
        &CreateTriggerByIndex<Is>...
    };
    return (index < factories.size()) ? factories[index] : nullptr;
}

template<size_t... Is>
static constexpr PlayerExtensionInterface* (*GetPlayerExtensionFactoryHelper(std::index_sequence<Is...>, size_t index))(LimonAPI*) {
    constexpr std::array<PlayerExtensionInterface* (*)(LimonAPI*), sizeof...(Is)> factories = {
        &CreatePlayerExtensionByIndex<Is>...
    };
    return (index < factories.size()) ? factories[index] : nullptr;
}

template<size_t... Is>
static constexpr ActorInterface* (*GetActorFactoryHelper(std::index_sequence<Is...>, size_t index))(uint32_t, LimonAPI*) {
    constexpr std::array<ActorInterface* (*)(uint32_t, LimonAPI*), sizeof...(Is)> factories = {
        &CreateActorByIndex<Is>...
    };
    return (index < factories.size()) ? factories[index] : nullptr;
}

ScriptManager::ScriptManager(const std::string &builtinDirectoryPath, const std::string &userDirectoryPath)
        : scriptDirectories{builtinDirectoryPath, userDirectoryPath} {
    ensurePythonModuleBindingsLinked();
    std::cout << "[ScriptManager][DEBUG] ScriptManager constructor called " << std::endl;
    try {
        try {
            std::cout << "[ScriptManager] Starting..." << std::endl;

            if (Py_IsInitialized()) {
                std::cout << "[ScriptManager] Python interpreter already running, using existing interpreter..." << std::endl;
                return;
            }

            // PYTHON_BUNDLE_ZIP is set by cmake
            std::filesystem::path bundlePath(PYTHON_BUNDLE_ZIP);
            std::filesystem::path localPython = bundlePath.parent_path().parent_path();

            if (!std::filesystem::exists(localPython)) {
                std::cerr << "CRITICAL ERROR: Bundled Python environment not found at: "
                        << localPython << std::endl;
                std::cerr << "Did you copy the 'python' folder into the build directory?" << std::endl;
                throw std::runtime_error("Python environment not found");
            }

            std::filesystem::path dynLoadPath = bundlePath.parent_path() / "lib-dynload";
            std::filesystem::path stdLibPath = localPython / "Lib";
            std::filesystem::path sitePackagesPath = localPython / "Lib" / "site-packages";

            if (!std::filesystem::exists(bundlePath) || !std::filesystem::exists(dynLoadPath)) {
                std::cerr << "CRITICAL ERROR: Required Python files not found:\n"
                        << "  " << bundlePath << "\n"
                        << "  " << dynLoadPath << std::endl;
                throw std::runtime_error("Required Python files not found");
            }

            PyConfig config;
            PyConfig_InitIsolatedConfig(&config);

            std::wstring homePath = std::filesystem::absolute(localPython).wstring();
            PyConfig_SetString(&config, &config.home, homePath.c_str());

            config.module_search_paths_set = 1;
            PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(bundlePath).wstring().c_str());
            PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(dynLoadPath).wstring().c_str());
            PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(stdLibPath).wstring().c_str());
            PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(sitePackagesPath).wstring().c_str());

            mainInterpreterGuard = new pybind11::scoped_interpreter(&config);
            PyConfig_Clear(&config);
            std::cout << "[ScriptManager] Main Python interpreter initialized successfully" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "[ScriptManager] Failed to initialize main interpreter: " << e.what() << std::endl;
            throw;
        }
        if (!Py_IsInitialized()) {
            std::cerr << "[ScriptManager] Warning: No Python interpreter available, consider this a fail" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Scripting] Error initializing sub-interpreter: " << e.what() << std::endl;
    }
}

ScriptManager::~ScriptManager() {
#ifdef PYTHON_DEBUGGING
    std::cout << "[ScriptManager] Destructor called, Py_IsInitialized: " << Py_IsInitialized() << std::endl;
    std::cout << "[ScriptManager] Destructor called, mainInterpreterGuard: " << static_cast<void*>(mainInterpreterGuard) << std::endl;
#endif
    try {
        std::cout << "[ScriptManager] Cleaning up ScriptManager instance" << std::endl;
        if (activeSubinterpreter) {
            std::cout << "[ScriptManager] Deactivating active subinterpreter" << std::endl;
            activeSubinterpreter->deactivate();
            activeSubinterpreter = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Error during ScriptManager cleanup: " << e.what() << std::endl;
    }
    try {
        if (!subInterpreters.empty()) {
            std::cerr << "[ScriptManager] WARNING: " << subInterpreters.size() << " subinterpreters were not properly cleaned up, cleaning up now" << std::endl;
            std::vector<PythonCallback>& callbacks = GetCallbacks();
            while (!subInterpreters.empty()) {
                auto it = subInterpreters.begin();
                WorldInterpreter* wi = it->second;
                subInterpreters.erase(it);
                if (Py_IsInitialized() && wi->subInterpreter) {
                    try {
                        pybind11::subinterpreter_scoped_activate temp(*wi->subInterpreter);
                        for (auto& cb : callbacks) {
                            try { cb.pyClass = pybind11::none(); } catch (...) {}
                        }
                        try {
                            pybind11::module_ gc = pybind11::module_::import("gc");
                            gc.attr("collect")();
                        } catch (...) {}
                    } catch (...) {}
                }
                delete wi;
            }
            callbacks.clear();
        }
        std::vector<PythonCallback>& callbacks = GetCallbacks();
        std::cout << "[ScriptManager] Cleaning up " << callbacks.size() << " static Python callback objects" << std::endl;
        for (auto& callback : callbacks) {
            try {
                callback.pyClass = pybind11::none();
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
        callbacks.clear();

        if (mainInterpreterGuard && Py_IsInitialized()) {
            // Run GC before finalization to collect any pybind11-wrapped instances that
            // survived via cycles (e.g. actor/trigger objects holding self-references).
            // If they are collected now (while pybind11 internals are still valid), the
            // assert(!types->empty()) in pybind11's clear_instance will not fire.
            runGC();
            delete mainInterpreterGuard;
            mainInterpreterGuard = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Error during main interpreter cleanup: " << e.what() << std::endl;
        // Don't rethrow during shutdown
    }
}

void ScriptManager::LoadScript(WorldInterpreter * worldInterpreter, const std::string &moduleName) {
    try {
        std::cout << "[ScriptManager] Loading module: " << moduleName << " for world: " << worldInterpreter->getWorldName() << std::endl;

        if (activeSubinterpreter != nullptr && activeSubinterpreter != worldInterpreter) {
            activeSubinterpreter->deactivate();
            activeSubinterpreter = nullptr;
        }
        activeSubinterpreter = worldInterpreter->activate();

        bool isMainInterpreter = true;
        try {
            PyThreadState* currentThread = PyThreadState_Get();
            PyInterpreterState* interpState = PyThreadState_GetInterpreter(currentThread);
            PyThreadState* mainThreadState = PyInterpreterState_ThreadHead(interpState);
            isMainInterpreter = (currentThread == mainThreadState);
        } catch (...) {
            isMainInterpreter = false;
        }

        if (isMainInterpreter) {
            pybind11::module_ sys = pybind11::module::import("sys");
            appendScriptDirectoriesToSysPath(sys);

            try {
                pybind11::module_ limon = pybind11::module_::import("limon");
                std::cout << "[ScriptManager] Limon module initialized for subinterpreter: " << worldInterpreter->getWorldName() << std::endl;
            } catch (const pybind11::error_already_set& e) {
                std::cerr << "[ScriptManager] Failed to initialize limon module for subinterpreter: " << e.what() << std::endl;
                PyErr_Print();
                return;
            }
        } else {
            std::cout << "[ScriptManager] Skipping limon import in subinterpreter (not main interpreter)" << std::endl;
        }

        pybind11::module_ pybindModule;
        try {
            pybindModule = pybind11::module_::import(moduleName.c_str());
        } catch (const pybind11::error_already_set& e) {
            std::cerr << "[ScriptManager] Failed to import module " << moduleName << ": " << e.what() << std::endl;
            PyErr_Print();
            return;
        }
        pybind11::list moduleItemsDict = pybind11::list(pybindModule.attr("__dict__").attr("items")());
        for (pybind11::handle item: moduleItemsDict) {
            std::pair<std::string, pybind11::object> pair = item.cast<std::pair<std::string, pybind11::object>>();
            std::string name = pair.first;
            pybind11::object obj = pair.second;

            if (!pybind11::isinstance<pybind11::type>(obj) ||
                !pybind11::hasattr(obj, "__bases__")) {
                continue;
            }

            using TriggerFactory = TriggerInterface* (*)(LimonAPI *);
            using ExtensionFactory = PlayerExtensionInterface* (*)(LimonAPI *);
            using ActorFactory = ActorInterface* (*)(uint32_t, LimonAPI *);

            if (IsSubclassOf(obj, "trigger_interface", "TriggerInterface")) {
                std::cout << "[ScriptManager] found TriggerInterface: " << moduleName << std::endl;
                size_t callbackIndex = GetCallbacks().size();
                GetCallbacks().push_back({obj, CallBackTypes::TRIGGER});

                constexpr size_t MAX_TRIGGERS = MAX_PYTHON_SCRIPT_COUNT;
                TriggerFactory factory = GetTriggerFactoryHelper(std::make_index_sequence<MAX_TRIGGERS>{}, callbackIndex);

                if (!factory) {
                    std::cerr << "[ScriptManager] Too many trigger types registered (max " << MAX_TRIGGERS << ")" << std::endl;
                    continue;
                }

                TriggerInterface::registerType(name, factory);
                std::cout << "[ScriptManager] Registered trigger: " << name << std::endl;
            }
            else if (IsSubclassOf(obj, "player_extension_interface", "PlayerExtensionInterface")) {
                std::cout << "[ScriptManager] found PlayerExtensionInterface: " << moduleName << std::endl;
                size_t callbackIndex = GetCallbacks().size();
                GetCallbacks().push_back({obj, CallBackTypes::PLAYER_EXTENSION});

                constexpr size_t MAX_EXTENSIONS = MAX_PYTHON_SCRIPT_COUNT;
                ExtensionFactory factory = GetPlayerExtensionFactoryHelper(std::make_index_sequence<MAX_EXTENSIONS>{}, callbackIndex);

                if (!factory) {
                    std::cerr << "[ScriptManager] Too many extension types registered (max " << MAX_EXTENSIONS << ")" << std::endl;
                    continue;
                }

                PlayerExtensionInterface::registerType(name, factory);
                std::cout << "[ScriptManager] Registered extension: " << name << std::endl;
            }
            else if (IsSubclassOf(obj, "actor_interface", "ActorInterface")) {
                std::cout << "[ScriptManager] found ActorInterface: " << moduleName << std::endl;
                size_t callbackIndex = GetCallbacks().size();
                GetCallbacks().push_back({obj, CallBackTypes::ACTOR});

                constexpr size_t MAX_ACTORS = MAX_PYTHON_SCRIPT_COUNT;
                ActorFactory factory = GetActorFactoryHelper(std::make_index_sequence<MAX_ACTORS>{}, callbackIndex);

                if (!factory) {
                    std::cerr << "[ScriptManager] Too many actor types registered (max " << MAX_ACTORS << ")" << std::endl;
                    continue;
                }

                ActorInterface::registerType(name, factory);
                std::cout << "[ScriptManager] Registered actor: " << name << std::endl;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[ScriptManager] Error loading script " << moduleName
                << ": " << e.what() << std::endl;
    }
}

TriggerInterface *ScriptManager::CreateTriggerWrapper(LimonAPI *api, size_t index) {
    try {
        std::vector<ScriptManager::PythonCallback>& callbacks = GetCallbacks();
        if (index < callbacks.size() && callbacks[index].callBackType == CallBackTypes::TRIGGER) {
            pybind11::object pyClass = callbacks[index].pyClass;
            pybind11::object instance = pyClass.attr("__new__")(pyClass);
            instance.attr("__init__")(api);
            return new PyTriggerInterface(api, instance);
        }
    } catch (const pybind11::error_already_set &e) {
        std::cerr << "[ScriptManager] Python error in CreateTriggerWrapper: " << e.what() << std::endl;
        PyErr_Print();
    } catch (const std::exception &e) {
        std::cerr << "[ScriptManager] Error in CreateTriggerWrapper: " << e.what() << std::endl;
    }
    return nullptr;
}

PlayerExtensionInterface* ScriptManager::CreatePlayerExtensionWrapper(LimonAPI* api, size_t index) {
    try {
        std::vector<ScriptManager::PythonCallback>& callbacks = GetCallbacks();
        if (index < callbacks.size() && callbacks[index].callBackType == CallBackTypes::PLAYER_EXTENSION) {
            try {
                pybind11::object pyClass = callbacks[index].pyClass;
                pybind11::object instance = pyClass.attr("__new__")(pyClass);
                instance.attr("__init__")(pybind11::cast(api, pybind11::return_value_policy::reference));

                if (!pybind11::hasattr(instance, "process_input") ||
                    !pybind11::hasattr(instance, "interact") ||
                    !pybind11::hasattr(instance, "get_name") ||
                    !pybind11::hasattr(instance, "get_custom_camera_attachment")) {
                    std::cerr << "[ScriptManager] Python extension class is missing required methods" << std::endl;
                    return nullptr;
                }

                std::cout << "Successfully created Python PlayerExtension instance" << std::endl;
                return new PyPlayerExtensionInterface(api, instance);

            } catch (const pybind11::error_already_set& e) {
                std::cerr << "[ScriptManager] Python error during extension creation: " << e.what() << std::endl;
                PyErr_Print();
                return nullptr;
            } catch (const std::exception& e) {
                std::cerr << "[ScriptManager] C++ error during extension creation: " << e.what() << std::endl;
                return nullptr;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Error in CreateExtensionWrapper: " << e.what() << std::endl;
    }
    return nullptr;
}

ActorInterface* ScriptManager::CreateActorWrapper(uint32_t id, LimonAPI* api, size_t index) {
    try {
        std::vector<ScriptManager::PythonCallback>& callbacks = GetCallbacks();
        if (index < callbacks.size() && callbacks[index].callBackType == CallBackTypes::ACTOR) {
            try {
                pybind11::object pyClass = callbacks[index].pyClass;
                pybind11::object instance = pyClass.attr("__new__")(pyClass);
                instance.attr("__init__")(id, pybind11::cast(api, pybind11::return_value_policy::reference));

                if (!pybind11::hasattr(instance, "get_name") ||
                    !pybind11::hasattr(instance, "play") ||
                    !pybind11::hasattr(instance, "interaction") ||
                    !pybind11::hasattr(instance, "get_parameters") ||
                    !pybind11::hasattr(instance, "set_parameters")) {
                    std::cerr << "[ScriptManager] Python actor class is missing required methods" << std::endl;
                    return nullptr;
                }

                std::cout << "Successfully created Python ActorInterface instance" << std::endl;
                return new PyActorInterface(id, api, instance);

            } catch (const pybind11::error_already_set& e) {
                std::cerr << "[ScriptManager] Python error during actor creation: " << e.what() << std::endl;
                PyErr_Print();
                return nullptr;
            } catch (const std::exception& e) {
                std::cerr << "[ScriptManager] C++ error during actor creation: " << e.what() << std::endl;
                return nullptr;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Error in CreateActorWrapper: " << e.what() << std::endl;
    }
    return nullptr;
}

// Sub-interpreter management
WorldInterpreter* ScriptManager::createWorldInterpreter(const std::string& worldName) {
    try {
        std::cout << "[ScriptManager] Creating subinterpreter for world: " << worldName << std::endl;

        if (!Py_IsInitialized()) {
            std::cerr << "[ScriptManager] Cannot create sub-interpreter: main interpreter not initialized" << std::endl;
            throw std::runtime_error("Main interpreter not initialized");
        }

        WorldInterpreter* worldInterpreter = new WorldInterpreter(worldName);
        try {
            if (activeSubinterpreter != nullptr) {
                activeSubinterpreter->deactivate();
            }
            activeSubinterpreter = worldInterpreter->activate();

            pybind11::module_ sys = pybind11::module::import("sys");
            appendScriptDirectoriesToSysPath(sys);

            pybind11::module_ limon = pybind11::module::import("limon");

            if (pybind11::hasattr(limon, "StdOut")) {
                pybind11::object redirector = limon.attr("StdOut")();
                sys.attr("stdout") = redirector;
                sys.attr("stderr") = redirector;
            } else {
#ifdef PYTHON_DEBUGGING
                std::cout << "[ScriptManager] StdOut not available in limon module, skipping redirection" << std::endl;
#endif
            }

            std::cout << "[ScriptManager] Sub-interpreter multi-phase initialization completed successfully" << std::endl;
            worldInterpreter->deactivate();

        } catch (const std::exception& e) {
            std::cerr << "[ScriptManager] Error during sub-interpreter phase 2 initialization: " << e.what() << std::endl;
            delete worldInterpreter;
            throw;
        }
        if (subInterpreters.count(worldName)) {
            std::cerr << "[ScriptManager] WARNING: interpreter for '" << worldName << "' already exists, destroying old one" << std::endl;
            removeWorldInterpreterInternal(subInterpreters[worldName]);
            subInterpreters.erase(worldName);
        }
        subInterpreters[worldName] = worldInterpreter;
        loadScripts(worldInterpreter);
        std::cout << "[ScriptManager] Added interpreter for world: " << worldName << std::endl;

        return worldInterpreter;
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Failed to create sub-interpreter: " << e.what() << std::endl;
        throw;
    }
}

void ScriptManager::removeWorldInterpreterInternal(WorldInterpreter* world_interpreter) {
    if (!world_interpreter) {
        return;
    }

    std::cout << "[ScriptManager] Removing world interpreter for: " << world_interpreter->getWorldName() << " (count: " << subInterpreters.size() << ")" << std::endl;

    auto it = subInterpreters.find(world_interpreter->getWorldName());
    if (it != subInterpreters.end()) {
        std::cout << "[ScriptManager] Removing interpreter for world: " << world_interpreter->getWorldName() << std::endl;
        subInterpreters.erase(it);
    }

    if (activeSubinterpreter == world_interpreter) {
        std::cout << "[ScriptManager] Deactivating current active world interpreter" << std::endl;
        activeSubinterpreter->deactivate();
        activeSubinterpreter = nullptr;
    }

    delete world_interpreter;
}

void ScriptManager::runGC() {
    if (!Py_IsInitialized()) {
        return;
    }
    try {
        pybind11::module_ gc = pybind11::module_::import("gc");
        gc.attr("collect")();
        gc.attr("collect")();
    } catch (...) {}
}

void ScriptManager::removeWorldInterpreter(const std::string& worldName) {
    auto it = subInterpreters.find(worldName);
    if (it != subInterpreters.end()) {
        removeWorldInterpreterInternal(it->second);
    }
}

void ScriptManager::setActiveSubInterpreter(const std::string& worldName) {
    auto it = subInterpreters.find(worldName);
    if (it != subInterpreters.end()) {
        if (it->second) {
            if (activeSubinterpreter == it->second && activeSubinterpreter) {
                return;
            }

            if (activeSubinterpreter != nullptr && activeSubinterpreter != it->second) {
                activeSubinterpreter->deactivate();
                activeSubinterpreter = nullptr;
            }
            activeSubinterpreter = it->second->activate();
            std::cout << "[ScriptManager] Activated subinterpreter for world: " << it->second->getWorldName() << std::endl;
        } else {
            if (activeSubinterpreter) {
                activeSubinterpreter->deactivate();
            }
            activeSubinterpreter = nullptr;
            std::cout << "[ScriptManager] Deactivated subinterpreter" << std::endl;
        }
    } else {
        std::cerr << "[ScriptManager] No interpreter found for world: " << worldName << std::endl;
    }
}
