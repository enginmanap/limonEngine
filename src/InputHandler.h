//
// Created by Engin Manap on 14.02.2016.
//

#ifndef UBERGAME_INPUTHANDLER_H
#define UBERGAME_INPUTHANDLER_H

#include <map>
#include <SDL2/SDL.h>


class InputHandler {
public:
    //FIXME access modifiers should not be like this
    enum states {
        QUIT, MOUSE_MOVE, MOVE_FORWARD, MOVE_BACKWARD, MOVE_LEFT, MOVE_RIGHT, JUMP
    };
private:
    SDL_Window *window;
    int height, width;
    SDL_Event event;
    std::map<states, bool> inputStatus;
    float xPos, yPos, xChange, yChange;
public:
    InputHandler(SDL_Window *, int, int);

    ~InputHandler() {
        SDL_SetWindowGrab(window, SDL_FALSE);
    }

    void mapInput();

    bool getInputStatus(const states input) const {
        return inputStatus.at(input);
    }

    void getMousePosition(float &, float &) const;

    bool getMouseChange(float &xChange, float &yChange);

};

#endif //UBERGAME_INPUTHANDLER_H
