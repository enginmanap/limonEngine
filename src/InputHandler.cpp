//
// Created by Engin Manap on 14.02.2016.
//

#include <iostream>
#include "InputHandler.h"
#include "ImGuiHelper.h"

InputHandler::InputHandler(SDL_Window *window, Options *options) :
        window(window), options(options) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    inputStatus[QUIT] = false;
    inputStatus[MOUSE_MOVE] = false;
    inputStatus[MOVE_FORWARD] = false;
    inputStatus[MOVE_BACKWARD] = false;
    inputStatus[MOVE_LEFT] = false;
    inputStatus[MOVE_RIGHT] = false;
    inputStatus[JUMP] = false;
    inputEvents[JUMP] = false;
    inputStatus[DEBUG] = false;
    inputEvents[DEBUG] = false;
    inputStatus[EDITOR] = false;
    inputEvents[EDITOR] = false;
    inputStatus[KEY_ALT] = false;
    inputEvents[KEY_ALT] = false;
    inputStatus[KEY_CTRL] = false;
    inputEvents[KEY_CTRL] = false;
    inputStatus[KEY_SHIFT] = false;
    inputEvents[KEY_SHIFT] = false;
    inputStatus[KEY_SUPER] = false;
    inputEvents[KEY_SUPER] = false;
    inputStatus[MOUSE_BUTTON_LEFT] = false;
    inputEvents[MOUSE_BUTTON_LEFT] = false;
    inputStatus[MOUSE_BUTTON_RIGHT] = false;
    inputEvents[MOUSE_BUTTON_RIGHT] = false;
    inputStatus[MOUSE_BUTTON_MIDDLE] = false;
    inputEvents[MOUSE_BUTTON_MIDDLE] = false;
    inputEvents[MOUSE_WHEEL_UP] = false;
    inputEvents[MOUSE_WHEEL_DOWN] = false;
    inputEvents[TEXT_INPUT] = false;
    inputEvents[QUIT] = false;
}

void InputHandler::mapInput() {
    inputEvents[DEBUG] = false;
    inputEvents[JUMP] = false;
    inputEvents[RUN] = false;
    inputEvents[MOUSE_BUTTON_LEFT] = false;
    inputEvents[MOUSE_BUTTON_MIDDLE] = false;
    inputEvents[MOUSE_BUTTON_RIGHT] = false;
    inputEvents[KEY_ALT] = false;
    inputEvents[KEY_CTRL] = false;
    inputEvents[KEY_SHIFT] = false;
    inputEvents[KEY_SUPER] = false;
    inputEvents[EDITOR] = false;
    inputEvents[MOUSE_WHEEL_UP] = false;
    inputEvents[MOUSE_WHEEL_DOWN] = false;
    inputEvents[TEXT_INPUT] = false;
    inputEvents[QUIT] = false;

    while (SDL_PollEvent(&event)) {
        uint32_t downKey = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
        if(downKey < keyBufferElements) {
            downKeys[downKey] = event.type == SDL_KEYDOWN;
        }
        switch (event.type) {
            case SDL_QUIT:
                inputStatus[QUIT] = true;
                inputEvents[QUIT] = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        if(!inputStatus[MOUSE_BUTTON_LEFT]) {
                            inputEvents[MOUSE_BUTTON_LEFT] = true;
                        }
                        inputStatus[MOUSE_BUTTON_LEFT] = true;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        if(!inputStatus[MOUSE_BUTTON_MIDDLE]) {
                            inputEvents[MOUSE_BUTTON_MIDDLE] = true;
                        }
                        inputStatus[MOUSE_BUTTON_MIDDLE] = true;
                        break;
                    case SDL_BUTTON_RIGHT:
                        if(!inputStatus[MOUSE_BUTTON_RIGHT]) {
                            inputEvents[MOUSE_BUTTON_RIGHT] = true;
                        }
                        inputStatus[MOUSE_BUTTON_RIGHT] = true;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        inputStatus[MOUSE_BUTTON_LEFT] = false;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        inputStatus[MOUSE_BUTTON_MIDDLE] = false;
                        break;
                    case SDL_BUTTON_RIGHT:
                        inputStatus[MOUSE_BUTTON_RIGHT] = false;
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                inputStatus[MOUSE_MOVE] = true;
                xPos = (event.motion.x - (options->getScreenWidth() / 2.0f)) / (options->getScreenWidth() / 2);
                xChange = (event.motion.xrel) / (options->getScreenWidth() / 2.0f);
                yPos = (event.motion.y - (options->getScreenHeight() / 2.0f)) / (options->getScreenHeight() / 2);
                yChange = (event.motion.yrel) / (options->getScreenHeight() / 2.0f);
                break;
            case SDL_MOUSEWHEEL:
                if(event.wheel.y > 0) {
                    inputEvents[MOUSE_WHEEL_UP] = true;
                } else if(event.wheel.y < 0) {
                    inputEvents[MOUSE_WHEEL_DOWN] = true;
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if(!inputStatus[QUIT]) {
                            inputEvents[QUIT] = true;
                        }
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
                        if(!inputStatus[JUMP]) {
                            inputEvents[JUMP] = true;
                        }
                        inputStatus[JUMP] = true;
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        if(!inputStatus[RUN]) {
                            inputEvents[RUN] = true;
                        }
                        inputStatus[RUN] = true;
                        if(!inputStatus[KEY_SHIFT]) {
                            inputEvents[KEY_SHIFT] = true;
                        }
                        inputStatus[KEY_SHIFT] = true;
                        break;
                    case SDLK_LALT:
                        if(!inputStatus[KEY_ALT]) {
                            inputEvents[KEY_ALT] = true;
                        }
                        inputStatus[KEY_ALT] = true;
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        if(!inputStatus[KEY_SUPER]) {
                            inputEvents[KEY_SUPER] = true;
                        }
                        inputStatus[KEY_SUPER] = true;
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        if(!inputStatus[KEY_CTRL]) {
                            inputEvents[KEY_CTRL] = true;
                        }
                        inputStatus[KEY_CTRL] = true;
                        break;
                    case SDLK_0:
                        if(!inputStatus[DEBUG]) {
                            inputEvents[DEBUG] = true;
                        }
                        inputStatus[DEBUG] = true;
                        break;
                    case SDLK_QUOTEDBL:
                        if(!inputStatus[EDITOR]) {
                            inputEvents[EDITOR] = true;
                        }
                        inputStatus[EDITOR] = true;
                        break;
                    case SDLK_KP_PLUS:
                        options->setLookAroundSpeed(options->getLookAroundSpeed() + 1.0f);
                        break;
                    case SDLK_KP_MINUS:
                        options->setLookAroundSpeed(options->getLookAroundSpeed() - 1.0f);
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if(inputStatus[QUIT]) {
                            inputEvents[QUIT] = true;
                        }
                        inputStatus[QUIT] = false;
                        break;
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
                        if(inputStatus[JUMP]) {
                            inputEvents[JUMP] = true;
                        }
                        inputStatus[JUMP] = false;
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        if(inputStatus[RUN]) {
                            inputEvents[RUN] = true;
                        }
                        inputStatus[RUN] = false;
                        if(inputStatus[KEY_SHIFT]) {
                            inputEvents[KEY_SHIFT] = true;
                        }
                        inputStatus[KEY_SHIFT] = false;
                        break;
                    case SDLK_LALT:
                        if(inputStatus[KEY_ALT]) {
                            inputEvents[KEY_ALT] = true;
                        }
                        inputStatus[KEY_ALT] = false;
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        if(inputStatus[KEY_SUPER]) {
                            inputEvents[KEY_SUPER] = true;
                        }
                        inputStatus[KEY_SUPER] = false;
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        if(inputStatus[KEY_CTRL]) {
                            inputEvents[KEY_CTRL] = true;
                        }
                        inputStatus[KEY_CTRL] = false;
                        break;
                    case SDLK_0:
                        if(inputStatus[DEBUG]) {
                            inputEvents[DEBUG] = true;
                        }
                        inputStatus[DEBUG] = false;
                        break;
                    case SDLK_QUOTEDBL:
                        if(!inputStatus[EDITOR]) {
                            inputEvents[EDITOR] = true;
                        }
                        inputStatus[EDITOR] = false;
                        break;
                }
                break;
            case SDL_TEXTINPUT:
                this->sdlText = event.text.text;
                inputEvents[TEXT_INPUT] = true;
                break;
        }
    }
}

/**
 * xpos and ypos are actual position values. If relative mouse mode is enabled, mouse position is locked so they always
 * return the same value. In that case use change values.
 * change values return change like 0.001 if relative mouse mode is enabled.
 *
 * @param xChange
 * @param yChange
 * @param xPos
 * @param yPos
 * @return
 */
bool InputHandler::getMouseChange(float &xPosition, float &yPosition, float &xChange, float &yChange) {
    if (inputStatus[MOUSE_MOVE]) {
        xChange = this->xChange;
        yChange = this->yChange;
        xPosition = this->xPos;
        yPosition = this->yPos;
        this->inputStatus[MOUSE_MOVE] = false;
        this->xChange = 0.0f;
        this->yChange = 0.0f;
        return true;
    } else {
        return false;
    }
}