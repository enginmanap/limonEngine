//
// Created by Engin Manap on 14.02.2016.
//

#ifndef LIMONENGINE_INPUTHANDLER_H
#define LIMONENGINE_INPUTHANDLER_H

#include <map>
#include <SDL2/SDL.h>

#include "Options.h"


class InputHandler {
public:
    //FIXME access modifiers should not be like this
    enum states {
        QUIT, MOUSE_MOVE, MOUSE_BUTTON_LEFT, MOVE_FORWARD, MOVE_BACKWARD, MOVE_LEFT, MOVE_RIGHT, JUMP, RUN, DEBUG, EDITOR
    };
private:
    SDL_Window *window;
    Options *options;
    SDL_Event event;
    std::map<states, bool> inputStatus;
    std::map<states, bool> inputEvents;
    float xPos, yPos, xChange, yChange;
public:
    InputHandler(SDL_Window *, Options *options);

    ~InputHandler() {
        SDL_SetWindowGrab(window, SDL_FALSE);
    }

    void setMouseModeRelative() {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void setMouseModeFree() {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    void mapInput();

    bool getInputStatus(const states input) const {
        return inputStatus.at(input);
    }

    bool getInputEvents(const states input) const {
        return inputEvents.at(input);
    }

    bool getMouseChange(float &xPosition, float &yPosition, float &xChange, float &yChange);

};

#endif //LIMONENGINE_INPUTHANDLER_H
