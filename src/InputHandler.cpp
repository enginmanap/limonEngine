//
// Created by Engin Manap on 14.02.2016.
//


#include "InputHandler.h"
#include "ImGuiHelper.h"

InputHandler::InputHandler(SDL_Window *window, OptionsUtil::Options *options) :
        window(window), options(options) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    lookAroundSpeedOption   = options->getOption<double>(HASH("player_lookAroundSpeed"));
    gamepadDeadZoneOption   = options->getOption<double>(HASH("gamepad_deadZone"));

    // ── Keyboard ─────────────────────────────────────────────────────────────
    // Each entry maps one SDL keycode to one or more logical actions.
    // Multiple actions per key are supported (e.g. Shift drives both RUN and KEY_SHIFT).
    keyboardBindings[SDLK_ESCAPE] = {InputActions::QUIT};
    keyboardBindings[SDLK_w]      = {InputActions::MOVE_FORWARD};
    keyboardBindings[SDLK_a]      = {InputActions::MOVE_LEFT};
    keyboardBindings[SDLK_s]      = {InputActions::MOVE_BACKWARD};
    keyboardBindings[SDLK_d]      = {InputActions::MOVE_RIGHT};
    keyboardBindings[SDLK_SPACE]  = {InputActions::JUMP};
    keyboardBindings[SDLK_LSHIFT] = {InputActions::RUN, InputActions::KEY_SHIFT};
    keyboardBindings[SDLK_RSHIFT] = {InputActions::RUN, InputActions::KEY_SHIFT};
    keyboardBindings[SDLK_LALT]   = {InputActions::KEY_ALT};
    keyboardBindings[SDLK_LGUI]   = {InputActions::KEY_SUPER};
    keyboardBindings[SDLK_RGUI]   = {InputActions::KEY_SUPER};
    keyboardBindings[SDLK_LCTRL]  = {InputActions::KEY_CTRL};
    keyboardBindings[SDLK_RCTRL]  = {InputActions::KEY_CTRL};
    keyboardBindings[SDLK_0]      = {InputActions::DEBUG_MODE};
    keyboardBindings[SDLK_F2]     = {InputActions::EDITOR};
    keyboardBindings[SDLK_1]      = {InputActions::NUMBER_1};
    keyboardBindings[SDLK_2]      = {InputActions::NUMBER_2};
    keyboardBindings[SDLK_F4]     = {InputActions::F4};
    keyboardBindings[SDLK_F5]     = {InputActions::F5};

    // ── Mouse buttons ─────────────────────────────────────────────────────────
    mouseButtonBindings[SDL_BUTTON_LEFT]   = InputActions::MOUSE_BUTTON_LEFT;
    mouseButtonBindings[SDL_BUTTON_MIDDLE] = InputActions::MOUSE_BUTTON_MIDDLE;
    mouseButtonBindings[SDL_BUTTON_RIGHT]  = InputActions::MOUSE_BUTTON_RIGHT;

    // ── Gamepad buttons ───────────────────────────────────────────────────────
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_A]             = {InputActions::JUMP};
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]  = {InputActions::RUN, InputActions::KEY_SHIFT};
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = {InputActions::MOUSE_BUTTON_RIGHT};

    // ── Gamepad axes — digital (threshold-based) ──────────────────────────────
    // Threshold sign defines direction: negative = trigger when value <= threshold,
    // positive = trigger when value >= threshold.
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTY,        -0.3f, InputActions::MOVE_FORWARD});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTY,         0.3f, InputActions::MOVE_BACKWARD});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTX,        -0.3f, InputActions::MOVE_LEFT});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTX,         0.3f, InputActions::MOVE_RIGHT});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_TRIGGERRIGHT,  0.3f, InputActions::MOUSE_BUTTON_LEFT});

    // ── Gamepad axes — analog (raw value scaled per frame) ───────────────────
    // Scale is expressed in the same unit as mouse xChange (xrel / screenWidth*0.5), so that
    // lookAroundSpeed tunes both mouse and gamepad proportionally.
    // GAMEPAD_LOOK_PIXELS_PER_FRAME sets full-stick speed as "mouse pixel equivalents per frame".
    float analogLookScale = GAMEPAD_LOOK_PIXELS_PER_FRAME / (options->getScreenWidth() * 0.5f);
    gamepadAnalogAxisBindings.push_back({SDL_CONTROLLER_AXIS_RIGHTX, analogLookScale, InputActions::LOOK_X});
    gamepadAnalogAxisBindings.push_back({SDL_CONTROLLER_AXIS_RIGHTY, analogLookScale, InputActions::LOOK_Y});

    // ── SDL GameController subsystem ──────────────────────────────────────────
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "InputHandler: SDL_INIT_GAMECONTROLLER failed: " << SDL_GetError() << std::endl;
    } else {
        // Open the first available controller, if any is already connected.
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                gameController = SDL_GameControllerOpen(i);
                if (gameController != nullptr) {
                    std::cout << "InputHandler: opened gamepad \"" << SDL_GameControllerName(gameController) << "\"" << std::endl;
                    break;
                }
            }
        }
    }
}

void InputHandler::applyGamepadAnalogAxes() {
    if (gameController == nullptr) {
        return;
    }
    float deadZone = static_cast<float>(gamepadDeadZoneOption.get());
    for (const AnalogAxisBinding &binding : gamepadAnalogAxisBindings) {
        float value = currentAxisValues[static_cast<int>(binding.axis)];
        if (std::abs(value) > deadZone) {
            inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
            inputState.addAnalogValue(binding.action, value * binding.scale);
        }
    }
}

void InputHandler::mapInput() {
    inputState.resetAllEvents();

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                inputState.setInputStatus(InputActions::QUIT, true);
                break;

            // ── Mouse ─────────────────────────────────────────────────────────
            case SDL_MOUSEBUTTONDOWN: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = mouseButtonBindings.find(event.button.button);
                if (it != mouseButtonBindings.end()) {
                    inputState.setInputStatus(it->second, true);
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = mouseButtonBindings.find(event.button.button);
                if (it != mouseButtonBindings.end()) {
                    inputState.setInputStatus(it->second, false);
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                inputState.setInputStatus(InputActions::MOUSE_MOVE, true);
                float xPos    = (event.motion.x   - (options->getScreenWidth()  / 2.0f)) / (options->getScreenWidth()  / 2);
                float xChange = (event.motion.xrel)                                       / (options->getScreenWidth()  / 2.0f);
                float yPos    = (event.motion.y   - (options->getScreenHeight() / 2.0f)) / (options->getScreenHeight() / 2);
                float yChange = (event.motion.yrel)                                       / (options->getScreenHeight() / 2.0f);
                inputState.setMouseChange(xPos, yPos, xChange, yChange);
                inputState.addAnalogValue(InputActions::LOOK_X, xChange);
                inputState.addAnalogValue(InputActions::LOOK_Y, yChange);
                break;
            }
            case SDL_MOUSEWHEEL:
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                if (event.wheel.y > 0) {
                    inputState.setInputStatus(InputActions::MOUSE_WHEEL_UP, true);
                } else if (event.wheel.y < 0) {
                    inputState.setInputStatus(InputActions::MOUSE_WHEEL_DOWN, true);
                }
                break;

            // ── Keyboard ──────────────────────────────────────────────────────
            case SDL_KEYDOWN: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = keyboardBindings.find(event.key.keysym.sym);
                if (it != keyboardBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                if (event.key.keysym.sym == SDLK_KP_PLUS) {
                    float lookAroundSpeed = lookAroundSpeedOption.get();
                    lookAroundSpeedOption.set(lookAroundSpeed + 1.0f);
                } else if (event.key.keysym.sym == SDLK_KP_MINUS) {
                    float lookAroundSpeed = lookAroundSpeedOption.get();
                    lookAroundSpeedOption.set(lookAroundSpeed - 1.0f);
                }
                break;
            }
            case SDL_KEYUP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = keyboardBindings.find(event.key.keysym.sym);
                if (it != keyboardBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
                }
                break;
            }
            case SDL_TEXTINPUT:
                inputState.setText(event.text.text);
                inputState.setInputStatus(InputActions::TEXT_INPUT, true);
                break;

            // ── Gamepad device connect / disconnect ───────────────────────────
            case SDL_CONTROLLERDEVICEADDED:
                if (gameController == nullptr) {
                    gameController = SDL_GameControllerOpen(event.cdevice.which);
                    if (gameController != nullptr) {
                        std::cout << "InputHandler: gamepad connected: \"" << SDL_GameControllerName(gameController) << "\"" << std::endl;
                    }
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (gameController != nullptr &&
                    SDL_GameControllerFromInstanceID(event.cdevice.which) == gameController) {
                    std::cout << "InputHandler: gamepad disconnected." << std::endl;
                    SDL_GameControllerClose(gameController);
                    gameController = nullptr;
                    // Clear axis state so stale values don't drive actions after disconnect.
                    for (float &axisValue : currentAxisValues) {
                        axisValue = 0.0f;
                    }
                }
                break;

            // ── Gamepad buttons ───────────────────────────────────────────────
            case SDL_CONTROLLERBUTTONDOWN: {
                inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(event.cbutton.button);
                auto it = gamepadButtonBindings.find(button);
                if (it != gamepadButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                break;
            }
            case SDL_CONTROLLERBUTTONUP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(event.cbutton.button);
                auto it = gamepadButtonBindings.find(button);
                if (it != gamepadButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
                }
                break;
            }

            // ── Gamepad axes ──────────────────────────────────────────────────
            case SDL_CONTROLLERAXISMOTION: {
                SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(event.caxis.axis);
                float normalizedValue = event.caxis.value / 32767.0f;

                if (axis < SDL_CONTROLLER_AXIS_MAX) {
                    currentAxisValues[static_cast<int>(axis)] = normalizedValue;
                }

                if (std::abs(normalizedValue) > static_cast<float>(gamepadDeadZoneOption.get())) {
                    inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                }

                for (const DigitalAxisBinding &binding : gamepadDigitalAxisBindings) {
                    if (binding.axis != axis) {
                        continue;
                    }
                    bool pressed;
                    if (binding.threshold >= 0.0f) {
                        pressed = normalizedValue >= binding.threshold;
                    } else {
                        pressed = normalizedValue <= binding.threshold;
                    }
                    inputState.setInputStatus(binding.action, pressed);
                }
                break;
            }
        }
    }

    // Analog axes represent absolute stick position, not deltas — apply every
    // frame so held sticks continuously contribute to LOOK_X / LOOK_Y.
    applyGamepadAnalogAxes();
}
