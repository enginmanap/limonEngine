//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_SDL2HELPER_CPP_H
#define LIMONENGINE_SDL2HELPER_CPP_H

#include <iostream>

#include <SDL2/SDL.h>


class SDL2Helper {
    SDL_Window *window;
    SDL_GLContext context;
public:

    SDL2Helper(const char *, int, int);

    ~SDL2Helper();

    void swap() {
        SDL_GL_SwapWindow(window);
    };

    SDL_Window *getWindow();
};

#endif //LIMONENGINE_SDL2HELPER_CPP_H
