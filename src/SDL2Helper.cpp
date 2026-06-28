//
// Created by Engin Manap on 10.02.2016.
//

#include <SDL3/SDL.h>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif
#include "SDL2Helper.h"
#include "limonAPI/Options.h"
#include "limonAPI/LimonAPI.h"
#include "limonAPI/TriggerInterface.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "limonAPI/CameraExtensionInterface.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/Graphics/RenderMethodInterface.h"


SDL2Helper::SDL2Helper(OptionsUtil::Options* options) : window(nullptr), context(nullptr), options(options) {}

void SDL2Helper::initWindow(const char* title, const GraphicsInterface::ContextInformation& contextInformation) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cout << "Unable to initialize SDL: " << SDL_GetError();
            exit(1);
        }
    }
    /* Request gpu context. */
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, contextInformation.SDL_GL_ACCELERATED_VISUAL);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, contextInformation.SDL_GL_CONTEXT_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, contextInformation.SDL_GL_CONTEXT_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, contextInformation.SDL_GL_CONTEXT_PROFILE_MASK);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextInformation.SDL_GL_CONTEXT_FLAGS);

    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_MODE_SCALING, "aspect");

    /* Create our window centered */
    window = SDL_CreateWindow(title,
                              options->getScreenWidth(), options->getScreenHeight(), SDL_WINDOW_OPENGL);
    if (!window) {
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;
        //we don't quit if failed, because there is a fallback possibility
    }

    OptionsUtil::Options::Option<bool> fullScreenOption = options->getOption<bool>(HASH("display_fullScreen"));
    bool fullScreen = fullScreenOption.getOrDefault(false);
    setFullScreen(fullScreen);
}

bool SDL2Helper::createContext() {
    /* Create context and attach it to our window */
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) {
        return false;
    }

#ifdef _WIN32
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    options->setImeWindowHandle(hwnd);
#endif
    //if window has a scaling, we are setting the values to find out;
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowSizeInPixels(window, &display_w, &display_h);
    options->setDrawableHeight(display_h);
    options->setDrawableWidth(display_w);
    options->setWindowWidth(w);
    options->setWindowHeight(h);

    std::cout << "[FS] after context: windowSize(logical)=" << w << "x" << h
              << " drawableSize(pixels)=" << display_w << "x" << display_h
              << " engineRenders=" << options->getScreenWidth() << "x" << options->getScreenHeight()
              << std::endl;

    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
#ifndef NDEBUG
    SDL_GL_SetSwapInterval(1);
#endif
    SDL_HideCursor();
    std::cout << "SDL window and context started." << std::endl;
    return true;
}

void SDL2Helper::destroyWindow() {
    if (context) {
        SDL_GL_DestroyContext(context);
        context = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

SDL2Helper::~SDL2Helper() {
    /* Delete our opengl context, destroy our window, and shutdown SDL */
    SDL_ShowCursor();
    destroyWindow();
    SDL_Quit();
}

SDL_Window *SDL2Helper::getWindow() {
    return window;
}

std::shared_ptr<GraphicsInterface> SDL2Helper::loadGraphicsBackend(const std::string &fileName, OptionsUtil::Options *options) {
    std::cout << "trying to load shared library " << fileName << std::endl;
    SDL_SharedObject* objectHandle = nullptr;
    objectHandle = SDL_LoadObject(fileName.c_str());

    const std::string registerFunctionName = "createGraphicsBackend";
    std::shared_ptr<GraphicsInterface>(*registerFunction)(OptionsUtil::Options*);
    registerFunction = (
            std::shared_ptr<GraphicsInterface>(*)(
                    OptionsUtil::Options*
                            )
            ) SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Graphics backend register method found" << std::endl;
        return registerFunction(options);
    } else {
        std::cerr << "Graphics backend load failed!" << std::endl;
        return nullptr;
    }
}


bool SDL2Helper::loadCustomTriggers(const std::string &fileName) {
    std::cout << "trying to load shared library " << fileName << std::endl;
    SDL_SharedObject* objectHandle = SDL_LoadObject(fileName.c_str());
    if(objectHandle == nullptr) {
        std::cerr << "Failed to load " << fileName << ": " << SDL_GetError() << std::endl;
        return false;
    }
    bool result = loadTriggers(objectHandle);
    result = loadActors(objectHandle) && result;
    result = loadPlayerExtensions(objectHandle) && result;
    result = loadCameraExtensions(objectHandle) && result;
    return loadRenderMethods(objectHandle) && result;
}

bool SDL2Helper::loadPlayerExtensions(SDL_SharedObject *objectHandle) const {
    const std::string registerFunctionName = "registerPlayerExtensions";
    void(*registerFunction)(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);
    registerFunction = (void(*)(
            std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*))SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Player extension register method found " << std::endl;
        //register requires parameter
        std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)> elements;
        registerFunction(&elements);
        //now add this elements to registered map
        for (auto it = elements.begin(); it != elements.end(); it++) {
            PlayerExtensionInterface::registerType(it->first, it->second);
            std::cout << "registered Player extension: " << it->first << std::endl;
        }
        return true;
    } else {
        std::cerr << "Custom Player extension load failed!" << std::endl;
        return false;
    }
}

bool SDL2Helper::loadCameraExtensions(SDL_SharedObject *objectHandle) const {
    const std::string registerFunctionName = "registerCameraExtensions";
    void(*registerFunction)(std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>*);
    registerFunction = (void(*)(
            std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>*))SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Camera extension register method found " << std::endl;
        std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)> elements;
        registerFunction(&elements);
        for (auto it = elements.begin(); it != elements.end(); it++) {
            CameraExtensionInterface::registerType(it->first, it->second);
            std::cout << "registered Camera extension: " << it->first << std::endl;
        }
    }
    // Camera extensions are an optional extension point; a plugin without them is not an error.
    return true;
}

bool SDL2Helper::loadTriggers(SDL_SharedObject *objectHandle) const {
    const std::string registerFunctionName = "registerAsTrigger";
    void(*registerFunction)(std::map<std::string, TriggerInterface*(*)(LimonAPI*)>*);
    registerFunction = (void(*)(
            std::map<std::string, TriggerInterface*(*)(LimonAPI*)>*))SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Trigger register method found" << std::endl;
        //register requires parameter
        std::map<std::string, TriggerInterface*(*)(LimonAPI*)> elements;
        registerFunction(&elements);
        //now add this elements to registered map
        for (auto it = elements.begin(); it != elements.end(); it++) {
            TriggerInterface::registerType(it->first, it->second);
            std::cout << "registered Trigger: " << it->first << std::endl;
        }
        return true;
    } else {
        std::cerr << "Custom Trigger load failed!" << std::endl;
        return false;
    }
}

bool SDL2Helper::loadActors(SDL_SharedObject *objectHandle) const {
    const std::string registerFunctionName = "registerActors";
    void(*registerFunction)(std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>*);
    registerFunction = (void(*)(
            std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>*))SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Actor register method found" << std::endl;
        //register requires parameter
        std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)> elements;
        registerFunction(&elements);
        //now add this elements to registered map
        for (auto it = elements.begin(); it != elements.end(); it++) {
            ActorInterface::registerType(it->first, it->second);
            std::cout << "registered Actor: " << it->first << std::endl;
        }
        return true;
    } else {
        std::cerr << "Custom Actor load failed!" << std::endl;
        return false;
    }
}

bool SDL2Helper::loadRenderMethods(SDL_SharedObject *objectHandle) const {
    const std::string registerFunctionName = "registerRenderMethods";
    void(*registerFunction)(std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>*);
    registerFunction = (void(*)(
            std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>*))SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "RenderMethod register method found" << std::endl;
        //register requires parameter
        std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)> elements;
        registerFunction(&elements);
        //now add this elements to registered map
        for (auto it = elements.begin(); it != elements.end(); it++) {
            RenderMethodInterface::registerDynamicRenderMethod(it->first, it->second);
            std::cout << "registered RenderMethod: " << it->first << std::endl;
        }
        return true;
    } else {
        std::cerr << "Custom RenderMethod load failed!" << std::endl;
        return false;
    }
}

void SDL2Helper::setFullScreen(bool isFullScreen) {
    if (!isFullScreen) {
        SDL_SetWindowFullscreen(window, false);
        SDL_SyncWindow(window);
        return;
    }

    SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
    if (displayID == 0) {
        displayID = SDL_GetPrimaryDisplay();
    }

    int modeCount = 0;
    SDL_DisplayMode** allModes = SDL_GetFullscreenDisplayModes(displayID, &modeCount);
    std::cout << "[FS] display " << displayID << " requested "
              << options->getScreenWidth() << "x" << options->getScreenHeight()
              << ", " << modeCount << " fullscreen modes available:" << std::endl;
    if (allModes != nullptr) {
        for (int i = 0; i < modeCount; ++i) {
            std::cout << "[FS]   " << allModes[i]->w << "x" << allModes[i]->h
                      << " density=" << allModes[i]->pixel_density
                      << " rr=" << allModes[i]->refresh_rate << std::endl;
        }
        SDL_free(allModes);
    }
    // --- END DIAGNOSTIC ---

    SDL_DisplayMode mode;
    bool haveMode = displayID != 0 &&
                    SDL_GetClosestFullscreenDisplayMode(displayID, (int)options->getScreenWidth(),
                                                        (int)options->getScreenHeight(), 0.0f, false, &mode);


    if (!SDL_SetWindowFullscreen(window, true)) {
        std::cerr << "[FS] SDL_SetWindowFullscreen failed: " << SDL_GetError() << std::endl;
    }
    SDL_SyncWindow(window);

    if (haveMode) {
        std::cout << "[FS] closest mode chosen: " << mode.w << "x" << mode.h
                  << " density=" << mode.pixel_density << " rr=" << mode.refresh_rate << std::endl;
        if (!SDL_SetWindowFullscreenMode(window, &mode)) {
            std::cerr << "[FS] SDL_SetWindowFullscreenMode failed: " << SDL_GetError() << std::endl;
        }
        SDL_SyncWindow(window); // block until the (async on Wayland) mode change is applied
    } else {
        std::cerr << "[FS] no closest fullscreen mode for " << options->getScreenWidth() << "x"
                  << options->getScreenHeight() << "; using desktop fullscreen: " << SDL_GetError() << std::endl;
    }
}

std::string SDL2Helper::getCurrentPath() {
    std::string currentPath = SDL_GetBasePath();
    return currentPath;
}
