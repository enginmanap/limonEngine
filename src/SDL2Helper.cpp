//
// Created by Engin Manap on 10.02.2016.
//

#include <SDL_syswm.h>
#include <memory>
#include "SDL2Helper.h"
#include "limonAPI/Options.h"
#include "limonAPI/LimonAPI.h"
#include "limonAPI/TriggerInterface.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/Graphics/RenderMethodInterface.h"


SDL2Helper::SDL2Helper(OptionsUtil::Options* options) : options(options) {}

void SDL2Helper::initWindow(const char* title, const GraphicsInterface::ContextInformation& contextInformation) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { /* Initialize SDL's Video subsystem */
        std::cout << "Unable to initialize SDL";
        exit(1);
    }
    /* Request opengl 4.4 context. */
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, contextInformation.SDL_GL_ACCELERATED_VISUAL);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, contextInformation.SDL_GL_CONTEXT_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, contextInformation.SDL_GL_CONTEXT_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, contextInformation.SDL_GL_CONTEXT_PROFILE_MASK);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextInformation.SDL_GL_CONTEXT_FLAGS);

    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* Create our window centered at 512x512 resolution */
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              options->getScreenWidth(), options->getScreenHeight(), SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) { /* Die if creation failed */
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit(1);
    }

    OptionsUtil::Options::Option<bool> fullScreenOption = options->getOption<bool>(HASH("fullScreen"));
    bool fullScreen = fullScreenOption.getOrDefault(false);
    setFullScreen(fullScreen);

    /* Create our opengl context and attach it to our window */
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) {
        std::cout << "SDL2: OpenGL context creation failed." << std::endl;
        exit(1);

    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
#ifdef _WIN32
    options->setImeWindowHandle(wmInfo.info.win.window);
#endif
    //if window has a scaling, we are setting the values to find out;
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &display_w, &display_h);
    options->setDrawableHeight(display_h);
    options->setDrawableWidth(display_w);
    options->setWindowWidth(w);
    options->setWindowHeight(h);

    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
#ifndef NDEBUG
    SDL_GL_SetSwapInterval(1);
#endif
    SDL_ShowCursor(SDL_DISABLE);
    std::cout << "SDL window started." << std::endl;
}

SDL2Helper::~SDL2Helper() {
    /* Delete our opengl context, destroy our window, and shutdown SDL */
    SDL_ShowCursor(SDL_ENABLE);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Window *SDL2Helper::getWindow() {
    return window;
}

std::shared_ptr<GraphicsInterface> SDL2Helper::loadGraphicsBackend(const std::string &fileName, OptionsUtil::Options *options) {
    std::cout << "trying to load shared library " << fileName << std::endl;
    void* objectHandle = nullptr;
    objectHandle = SDL_LoadObject(fileName.c_str());

    const std::string registerFunctionName = "createGraphicsBackend";
    std::shared_ptr<GraphicsInterface>(*registerFunction)(OptionsUtil::Options*);
    registerFunction = (
            std::shared_ptr<GraphicsInterface>(*)(
                    OptionsUtil::Options*
                            )
            ) SDL_LoadFunction(objectHandle, registerFunctionName.c_str());
    if(registerFunction != nullptr) {
        std::cout << "Trigger register method found" << std::endl;
        return registerFunction(options);
    } else {
        std::cerr << "Graphics backend load failed!" << std::endl;
        return nullptr;
    }
}


bool SDL2Helper::loadCustomTriggers(const std::string &fileName) {
    std::cout << "trying to load shared library " << fileName << std::endl;
    void* objectHandle = nullptr;
    objectHandle = SDL_LoadObject(fileName.c_str());
    bool result = loadTriggers(objectHandle);
    result = loadActors(objectHandle) && result;
    result = loadPlayerExtensions(objectHandle) && result;
    return loadRenderMethods(objectHandle) && result;
}

bool SDL2Helper::loadPlayerExtensions(void *objectHandle) const {
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

bool SDL2Helper::loadTriggers(void *objectHandle) const {
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

bool SDL2Helper::loadActors(void *objectHandle) const {
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

bool SDL2Helper::loadRenderMethods(void *objectHandle) const {
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
    if(isFullScreen == true) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    } else {
        SDL_SetWindowFullscreen(window, SDL_FALSE);
    }

}
