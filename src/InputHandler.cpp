//
// Created by Engin Manap on 14.02.2016.
//

#include <iostream>
#include "InputHandler.h"

InputHandler::InputHandler(SDL_Window* window, int height, int width):
window(window), height(height), width(width){
    SDL_SetWindowGrab(window, SDL_TRUE);
    inputStatus[QUIT] = false;
    inputStatus[MOUSE_MOVE] = false;
    inputStatus[MOVE_FORWARD] = false;
    inputStatus[MOVE_BACKWARD] = false;
    inputStatus[MOVE_LEFT] = false;
    inputStatus[MOVE_RIGHT] = false;
    inputStatus[JUMP] = false;

}

void InputHandler::mapInput() {
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                inputStatus[QUIT] = true;
                break;
            case SDL_MOUSEMOTION:
                inputStatus[MOUSE_MOVE] = true;
                xPos = (event.motion.x - (width / 2.0f)) / (width/2);
                xChange =(event.motion.xrel) / (width/2.0f);
                yPos = (event.motion.y - (height / 2.0f)) / (height/2);
                yChange = (event.motion.yrel) / (height/2.0f);
                //fixme this is wrong, we need window position to fix it.
                SDL_WarpMouseInWindow(window, width/2, height/2);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputStatus[QUIT] = true;
                        break;
                    case SDLK_w:
                        inputStatus[MOVE_FORWARD] = true;
                        break;
                    case SDLK_a:
                        inputStatus[MOVE_LEFT] = true;
                        break;
                    case SDLK_s:
                        inputStatus[MOVE_BACKWARD] = true;
                        break;
                    case SDLK_d:
                        inputStatus[MOVE_RIGHT] = true;
                        break;
                    case SDLK_SPACE:
                        inputStatus[JUMP] = true;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        inputStatus[MOVE_FORWARD] = false;
                        break;
                    case SDLK_a:
                        inputStatus[MOVE_LEFT] = false;
                        break;
                    case SDLK_s:
                        inputStatus[MOVE_BACKWARD] = false;
                        break;
                    case SDLK_d:
                        inputStatus[MOVE_RIGHT] = false;
                        break;
                    case SDLK_SPACE:
                        inputStatus[JUMP] = false;
                        break;
                }
                break;
        }
    }

}

void InputHandler::getMousePosition(float &xPos, float &yPos) const {
    xPos = this->xPos;
    yPos = this->yPos;
}

bool InputHandler::getMouseChange(float &xChange, float &yChange) {
    if(inputStatus[MOUSE_MOVE]) {
        xChange = this->xChange;
        yChange = this->yChange;
        this->inputStatus[MOUSE_MOVE] = false;
        this->xChange = 0.0f;
        this->yChange = 0.0f;
        return true;
    } else {
        return false;
    }
}