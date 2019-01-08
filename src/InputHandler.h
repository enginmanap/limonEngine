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
    static const uint32_t keyBufferElements = 512;
    static const uint32_t keyBufferSize = sizeof(bool) * keyBufferElements;
    enum states {
        QUIT, MOUSE_MOVE, MOUSE_BUTTON_LEFT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_RIGHT, MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN, MOVE_FORWARD, MOVE_BACKWARD, MOVE_LEFT, MOVE_RIGHT, JUMP, RUN, DEBUG, EDITOR, KEY_SHIFT, KEY_CTRL, KEY_ALT, KEY_SUPER, TEXT_INPUT, NUMBER_1, NUMBER_2
    };
private:
    bool downKeys[keyBufferElements] = {0};


    SDL_Window *window;
    Options *options;
    SDL_Event event;
    std::map<states, bool> inputStatus;
    std::map<states, bool> inputEvents;
    float xPos, yPos, xChange, yChange;
    char* sdlText;
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

    bool getInputStatus(const states input) const {
        return inputStatus.at(input);
    }

    bool getInputEvents(const states input) const {
        return inputEvents.at(input);
    }

    const char * getText() const {
        return sdlText;
    }

    bool getMouseChange(float &xPosition, float &yPosition, float &xChange, float &yChange);

    const bool *  getAllKeyStates() const {
        return downKeys;
    }

};

#endif //LIMONENGINE_INPUTHANDLER_H
