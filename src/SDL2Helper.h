//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_SDL2HELPER_CPP_H
#define LIMONENGINE_SDL2HELPER_CPP_H

#include <iostream>

#include <SDL2/SDL.h>
#include "Options.h"


class SDL2Helper {
    SDL_Window *window;
    SDL_GLContext context;
    Options* options;
public:

    void setFullScreen(bool isFullScreen);

    SDL2Helper(const char *, Options* options);

    ~SDL2Helper();

    void swap() {
        SDL_GL_SwapWindow(window);
        options->setIsWindowInFocus(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS);
    };

    bool loadSharedLibrary(const std::string& fileName);

    SDL_Window *getWindow();
};

#endif //LIMONENGINE_SDL2HELPER_CPP_H
