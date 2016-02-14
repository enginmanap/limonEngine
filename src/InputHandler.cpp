//
// Created by Engin Manap on 14.02.2016.
//

#include <iostream>
#include "InputHandler.h"

InputHandler::InputHandler(int height, int width):
height(height), width(width){
    inputStatus[QUIT] = false;
    inputStatus[MOUSE_MOVE] = false;

}

void InputHandler::mapInput() {
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                inputStatus[QUIT] = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputStatus[QUIT] = true;
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                xPos = (event.motion.x - (width / 2.0f)) / (width/2);
                xRel = event.motion.xrel;
                yPos = (event.motion.y - (height / 2.0f)) / (height/2);
                yRel = event.motion.yrel;
        }
    }
}

void InputHandler::getMousePosition(float &xPos, float &yPos) {
    xPos = this->xPos;
    yPos = this->yPos;
}