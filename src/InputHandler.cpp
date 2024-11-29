//
// Created by Engin Manap on 14.02.2016.
//

#include <iostream>
#include "InputHandler.h"
#include "ImGuiHelper.h"

InputHandler::InputHandler(SDL_Window *window, OptionsUtil::Options *options) :
        window(window), options(options) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    lookAroundSpeedOption = options->getOption<double>(HASH("lookAroundSpeed"));
}

void InputHandler::mapInput() {
    inputState.resetAllEvents();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            uint32_t downKey = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
            if(downKey < InputStates::keyBufferElements) {
                inputState.setRawInputState(downKey, event.type == SDL_KEYDOWN);
            }
        }
        switch (event.type) {
            case SDL_QUIT:
                inputState.setInputStatus(InputStates::Inputs::QUIT, true);
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT, true);
                        break;
                    case SDL_BUTTON_MIDDLE:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_MIDDLE, true);
                        break;
                    case SDL_BUTTON_RIGHT:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT, true);
                        break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT, false);
                        break;
                    case SDL_BUTTON_MIDDLE:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_MIDDLE, false);
                        break;
                    case SDL_BUTTON_RIGHT:
                        inputState.setInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT, false);
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                {
                    inputState.setInputStatus(InputStates::Inputs::MOUSE_MOVE, true);
                    float xPos = (event.motion.x - (options->getScreenWidth() / 2.0f)) / (options->getScreenWidth() / 2);
                    float xChange = (event.motion.xrel) / (options->getScreenWidth() / 2.0f);
                    float yPos = (event.motion.y - (options->getScreenHeight() / 2.0f)) / (options->getScreenHeight() / 2);
                    float yChange = (event.motion.yrel) / (options->getScreenHeight() / 2.0f);
                    inputState.setMouseChange(xPos, yPos, xChange, yChange);
                }
                break;
            case SDL_MOUSEWHEEL:
                if(event.wheel.y > 0) {
                    inputState.setInputStatus(InputStates::Inputs::MOUSE_WHEEL_UP, true);
                } else if(event.wheel.y < 0) {
                    inputState.setInputStatus(InputStates::Inputs::MOUSE_WHEEL_DOWN, true);
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputState.setInputStatus(InputStates::Inputs::QUIT, true);
                        break;
                    case SDLK_w:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_FORWARD, true);
                        break;
                    case SDLK_a:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_LEFT, true);
                        break;
                    case SDLK_s:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_BACKWARD, true);
                        break;
                    case SDLK_d:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_RIGHT, true);
                        break;
                    case SDLK_SPACE:
                        inputState.setInputStatus(InputStates::Inputs::JUMP, true);
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        inputState.setInputStatus(InputStates::Inputs::RUN, true);
                        inputState.setInputStatus(InputStates::Inputs::KEY_SHIFT, true);
                        break;
                    case SDLK_LALT:
                        inputState.setInputStatus(InputStates::Inputs::KEY_ALT, true);
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        inputState.setInputStatus(InputStates::Inputs::KEY_SUPER, true);
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        inputState.setInputStatus(InputStates::Inputs::KEY_CTRL, true);
                        break;
                    case SDLK_0:
                        inputState.setInputStatus(InputStates::Inputs::DEBUG, true);
                        break;
                    case SDLK_F2:
                        inputState.setInputStatus(InputStates::Inputs::EDITOR, true);
                        break;
                    case SDLK_1:
                        inputState.setInputStatus(InputStates::Inputs::NUMBER_1, true);
                        break;
                    case SDLK_2:
                        inputState.setInputStatus(InputStates::Inputs::NUMBER_2, true);
                        break;
                    case SDLK_F4:
                        inputState.setInputStatus(InputStates::Inputs::F4, true);
                        break;
                    case SDLK_KP_PLUS: {
                        float lookAroundSpeed;
                        lookAroundSpeed = lookAroundSpeedOption.get();
                        lookAroundSpeed += 1.0f;
                        lookAroundSpeedOption.set(lookAroundSpeed);
                    }
                        break;
                    case SDLK_KP_MINUS: {
                        float lookAroundSpeed = lookAroundSpeedOption.get();
                        lookAroundSpeed -= 1.0f;
                        lookAroundSpeedOption.set(lookAroundSpeed);
                    }
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputState.setInputStatus(InputStates::Inputs::QUIT, false);
                        break;
                    case SDLK_w:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_FORWARD, false);
                        break;
                    case SDLK_a:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_LEFT, false);
                        break;
                    case SDLK_s:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_BACKWARD, false);
                        break;
                    case SDLK_d:
                        inputState.setInputStatus(InputStates::Inputs::MOVE_RIGHT, false);
                        break;
                    case SDLK_SPACE:
                        inputState.setInputStatus(InputStates::Inputs::JUMP, false);
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        inputState.setInputStatus(InputStates::Inputs::RUN, false);
                        inputState.setInputStatus(InputStates::Inputs::KEY_SHIFT, false);
                        break;
                    case SDLK_LALT:
                        inputState.setInputStatus(InputStates::Inputs::KEY_ALT, false);
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        inputState.setInputStatus(InputStates::Inputs::KEY_SUPER, false);
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        inputState.setInputStatus(InputStates::Inputs::KEY_CTRL, false);
                        break;
                    case SDLK_0:
                        inputState.setInputStatus(InputStates::Inputs::DEBUG, false);
                        break;
                    case SDLK_F2:
                        inputState.setInputStatus(InputStates::Inputs::EDITOR, false);
                        break;
                    case SDLK_1:
                        inputState.setInputStatus(InputStates::Inputs::NUMBER_1, false);
                        break;
                    case SDLK_2:
                        inputState.setInputStatus(InputStates::Inputs::NUMBER_2, false);
                        break;
                    case SDLK_F4:
                        inputState.setInputStatus(InputStates::Inputs::F4, false);
                        break;
                }
                break;
            case SDL_TEXTINPUT:
                inputState.setText(event.text.text);
                inputState.setInputStatus(InputStates::Inputs::TEXT_INPUT, true);
                break;
        }
    }
}