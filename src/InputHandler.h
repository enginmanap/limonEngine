//
// Created by Engin Manap on 14.02.2016.
//

#ifndef LIMONENGINE_INPUTHANDLER_H
#define LIMONENGINE_INPUTHANDLER_H

#include <map>
#include <SDL2/SDL.h>

#include "Options.h"
#include "InputStates.h"


class InputHandler {
private:
    SDL_Window *window;
    Options *options;
    SDL_Event event;
    InputStates inputState;
public:
    InputHandler(SDL_Window *, Options *options);

    ~InputHandler() {
        SDL_SetWindowGrab(window, SDL_FALSE);
    }

    void setMouseModeRelative() {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_FALSE);
    }

    void setMouseModeFree() {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_TRUE);
    }

    void mapInput();

    const InputStates &getInputStates() const {
        return this->inputState;
    }
};

#endif //LIMONENGINE_INPUTHANDLER_H
