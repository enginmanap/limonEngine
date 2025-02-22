//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_SDL2HELPER_CPP_H
#define LIMONENGINE_SDL2HELPER_CPP_H

#include <iostream>

#include <SDL2/SDL.h>
#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <functional>
#include <memory>
#include <API/Graphics/GraphicsInterface.h>
#include "API/Options.h"
#include "API/LimonAPI.h"


class SDL2Helper {
private:
    SDL_Window *window;
    SDL_GLContext context;
    OptionsUtil::Options* options;

public:

    void setFullScreen(bool isFullScreen);

    SDL2Helper(OptionsUtil::Options* options);
    void initWindow(const char*, const GraphicsInterface::ContextInformation& contextInformation);
    ~SDL2Helper();

    void swap() {
        SDL_GL_SwapWindow(window);
        options->setIsWindowInFocus(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS);
    };

    bool loadCustomTriggers(const std::string& fileName);
    std::shared_ptr<GraphicsInterface> loadGraphicsBackend(const std::string &fileName, OptionsUtil::Options *options);

    SDL_Window *getWindow();

    static uint32_t getLogicalCPUCount() {
        return SDL_GetCPUCount();
    }

    bool loadTriggers(void *objectHandle) const;
    bool loadPlayerExtensions(void *objectHandle) const;
    bool loadActors(void *objectHandle) const;
    bool loadRenderMethods(void* objectHandle) const;

};

#endif //LIMONENGINE_SDL2HELPER_CPP_H
