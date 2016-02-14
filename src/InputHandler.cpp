//
// Created by Engin Manap on 14.02.2016.
//

#include "InputHandler.h"


void InputHandler::mapInput() {
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                inputStates["quit"] = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputStates["quit"] = true;
                        break;
                }
                break;

        }
    }
}
