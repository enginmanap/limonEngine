//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_SDL2HELPER_CPP_H
#define LIMONENGINE_SDL2HELPER_CPP_H

#include <SDL3/SDL.h>
#include <memory>
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Options.h"
#include "limonAPI/LimonAPI.h"


class SDL2Helper {
private:
    SDL_Window *window;
    SDL_GLContext context;
    OptionsUtil::Options* options;

public:

    void setFullScreen(bool isFullScreen);

    static std::string getCurrentPath();

    SDL2Helper(OptionsUtil::Options* options);
    void initWindow(const char* title, const GraphicsInterface::ContextInformation& contextInformation);
    bool createContext();
    void destroyWindow();
    ~SDL2Helper();

    void swap() {
        SDL_GL_SwapWindow(window);
        options->setIsWindowInFocus((SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS) != 0);
    };

    bool loadCustomTriggers(const std::string& fileName);
    std::shared_ptr<GraphicsInterface> loadGraphicsBackend(const std::string &fileName, OptionsUtil::Options *options);

    SDL_Window *getWindow();

    static inline Uint64 getTicks() {
        return SDL_GetTicks();
    }

    static uint32_t getLogicalCPUCount() {
        return SDL_GetNumLogicalCPUCores();
    }

    bool loadTriggers(SDL_SharedObject *objectHandle) const;
    bool loadPlayerExtensions(SDL_SharedObject *objectHandle) const;
    bool loadCameraExtensions(SDL_SharedObject *objectHandle) const;
    bool loadActors(SDL_SharedObject *objectHandle) const;
    bool loadRenderMethods(SDL_SharedObject *objectHandle) const;

};

#endif //LIMONENGINE_SDL2HELPER_CPP_H
