//
// Created by Engin Manap on 14.02.2016.
//


#include "InputHandler.h"
#include "ImGuiHelper.h"

InputHandler::InputHandler(SDL_Window *window, OptionsUtil::Options *options) :
        window(window), options(options) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    lookAroundSpeedOption = options->getOption<double>(HASH("player_lookAroundSpeed"));
}

void InputHandler::mapInput() {
    inputState.resetAllEvents();

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                inputState.setInputStatus(InputActions::QUIT, true);
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_LEFT, true);
                        break;
                    case SDL_BUTTON_MIDDLE:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_MIDDLE, true);
                        break;
                    case SDL_BUTTON_RIGHT:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_RIGHT, true);
                        break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_LEFT, false);
                        break;
                    case SDL_BUTTON_MIDDLE:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_MIDDLE, false);
                        break;
                    case SDL_BUTTON_RIGHT:
                        inputState.setInputStatus(InputActions::MOUSE_BUTTON_RIGHT, false);
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                {
                    inputState.setInputStatus(InputActions::MOUSE_MOVE, true);
                    float xPos = (event.motion.x - (options->getScreenWidth() / 2.0f)) / (options->getScreenWidth() / 2);
                    float xChange = (event.motion.xrel) / (options->getScreenWidth() / 2.0f);
                    float yPos = (event.motion.y - (options->getScreenHeight() / 2.0f)) / (options->getScreenHeight() / 2);
                    float yChange = (event.motion.yrel) / (options->getScreenHeight() / 2.0f);
                    inputState.setMouseChange(xPos, yPos, xChange, yChange);
                    inputState.addAnalogValue(InputActions::LOOK_X, xChange);
                    inputState.addAnalogValue(InputActions::LOOK_Y, yChange);
                }
                break;
            case SDL_MOUSEWHEEL:
                if (event.wheel.y > 0) {
                    inputState.setInputStatus(InputActions::MOUSE_WHEEL_UP, true);
                } else if (event.wheel.y < 0) {
                    inputState.setInputStatus(InputActions::MOUSE_WHEEL_DOWN, true);
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        inputState.setInputStatus(InputActions::QUIT, true);
                        break;
                    case SDLK_w:
                        inputState.setInputStatus(InputActions::MOVE_FORWARD, true);
                        break;
                    case SDLK_a:
                        inputState.setInputStatus(InputActions::MOVE_LEFT, true);
                        break;
                    case SDLK_s:
                        inputState.setInputStatus(InputActions::MOVE_BACKWARD, true);
                        break;
                    case SDLK_d:
                        inputState.setInputStatus(InputActions::MOVE_RIGHT, true);
                        break;
                    case SDLK_SPACE:
                        inputState.setInputStatus(InputActions::JUMP, true);
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        inputState.setInputStatus(InputActions::RUN, true);
                        inputState.setInputStatus(InputActions::KEY_SHIFT, true);
                        break;
                    case SDLK_LALT:
                        inputState.setInputStatus(InputActions::KEY_ALT, true);
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        inputState.setInputStatus(InputActions::KEY_SUPER, true);
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        inputState.setInputStatus(InputActions::KEY_CTRL, true);
                        break;
                    case SDLK_0:
                        inputState.setInputStatus(InputActions::DEBUG_MODE, true);
                        break;
                    case SDLK_F2:
                        inputState.setInputStatus(InputActions::EDITOR, true);
                        break;
                    case SDLK_1:
                        inputState.setInputStatus(InputActions::NUMBER_1, true);
                        break;
                    case SDLK_2:
                        inputState.setInputStatus(InputActions::NUMBER_2, true);
                        break;
                    case SDLK_F4:
                        inputState.setInputStatus(InputActions::F4, true);
                        break;
                    case SDLK_F5:
                        inputState.setInputStatus(InputActions::F5, true);
                        break;
                    case SDLK_KP_PLUS: {
                        float lookAroundSpeed = lookAroundSpeedOption.get();
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
                        inputState.setInputStatus(InputActions::QUIT, false);
                        break;
                    case SDLK_w:
                        inputState.setInputStatus(InputActions::MOVE_FORWARD, false);
                        break;
                    case SDLK_a:
                        inputState.setInputStatus(InputActions::MOVE_LEFT, false);
                        break;
                    case SDLK_s:
                        inputState.setInputStatus(InputActions::MOVE_BACKWARD, false);
                        break;
                    case SDLK_d:
                        inputState.setInputStatus(InputActions::MOVE_RIGHT, false);
                        break;
                    case SDLK_SPACE:
                        inputState.setInputStatus(InputActions::JUMP, false);
                        break;
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                        inputState.setInputStatus(InputActions::RUN, false);
                        inputState.setInputStatus(InputActions::KEY_SHIFT, false);
                        break;
                    case SDLK_LALT:
                        inputState.setInputStatus(InputActions::KEY_ALT, false);
                        break;
                    case SDLK_LGUI:
                    case SDLK_RGUI:
                        inputState.setInputStatus(InputActions::KEY_SUPER, false);
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        inputState.setInputStatus(InputActions::KEY_CTRL, false);
                        break;
                    case SDLK_0:
                        inputState.setInputStatus(InputActions::DEBUG_MODE, false);
                        break;
                    case SDLK_F2:
                        inputState.setInputStatus(InputActions::EDITOR, false);
                        break;
                    case SDLK_1:
                        inputState.setInputStatus(InputActions::NUMBER_1, false);
                        break;
                    case SDLK_2:
                        inputState.setInputStatus(InputActions::NUMBER_2, false);
                        break;
                    case SDLK_F4:
                        inputState.setInputStatus(InputActions::F4, false);
                        break;
                    case SDLK_F5:
                        inputState.setInputStatus(InputActions::F5, false);
                        break;
                }
                break;
            case SDL_TEXTINPUT:
                inputState.setText(event.text.text);
                inputState.setInputStatus(InputActions::TEXT_INPUT, true);
                break;
        }
    }
}
