//
// Created by Engin Manap on 14.02.2016.
//


#include "InputHandler.h"
#include "ImGuiHelper.h"
#include <tinyxml2.h>
#include <cstring>
#include <iostream>

// ── XML parsing helpers ───────────────────────────────────────────────────────

static SDL_Keycode parseKeyName(const char *name) {
    return SDL_GetKeyFromName(name); // SDLK_UNKNOWN on failure (case-insensitive)
}

// Returns 0 on failure. Supports named constants and numeric fallback.
static uint8_t parseMouseButtonName(const char *name) {
    if (std::strcmp(name, "LEFT")   == 0) return SDL_BUTTON_LEFT;
    if (std::strcmp(name, "MIDDLE") == 0) return SDL_BUTTON_MIDDLE;
    if (std::strcmp(name, "RIGHT")  == 0) return SDL_BUTTON_RIGHT;
    if (std::strcmp(name, "X1")     == 0) return SDL_BUTTON_X1;
    if (std::strcmp(name, "X2")     == 0) return SDL_BUTTON_X2;
    // Numeric fallback for mice with more than 5 buttons (e.g. "6", "7", ...)
    int index = std::atoi(name);
    if (index > 0 && index <= 255) return static_cast<uint8_t>(index);
    return 0;
}

static SDL_GameControllerButton parseGamepadButtonName(const char *name) {
    if (std::strcmp(name, "A")             == 0) return SDL_CONTROLLER_BUTTON_A;
    if (std::strcmp(name, "B")             == 0) return SDL_CONTROLLER_BUTTON_B;
    if (std::strcmp(name, "X")             == 0) return SDL_CONTROLLER_BUTTON_X;
    if (std::strcmp(name, "Y")             == 0) return SDL_CONTROLLER_BUTTON_Y;
    if (std::strcmp(name, "BACK")          == 0) return SDL_CONTROLLER_BUTTON_BACK;
    if (std::strcmp(name, "GUIDE")         == 0) return SDL_CONTROLLER_BUTTON_GUIDE;
    if (std::strcmp(name, "START")         == 0) return SDL_CONTROLLER_BUTTON_START;
    if (std::strcmp(name, "LEFTSTICK")     == 0) return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    if (std::strcmp(name, "RIGHTSTICK")    == 0) return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    if (std::strcmp(name, "L3")            == 0) return SDL_CONTROLLER_BUTTON_LEFTSTICK;  // common alias
    if (std::strcmp(name, "R3")            == 0) return SDL_CONTROLLER_BUTTON_RIGHTSTICK; // common alias
    if (std::strcmp(name, "LEFTSHOULDER")  == 0) return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    if (std::strcmp(name, "RIGHTSHOULDER") == 0) return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    if (std::strcmp(name, "DPAD_UP")       == 0) return SDL_CONTROLLER_BUTTON_DPAD_UP;
    if (std::strcmp(name, "DPAD_DOWN")     == 0) return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    if (std::strcmp(name, "DPAD_LEFT")     == 0) return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    if (std::strcmp(name, "DPAD_RIGHT")    == 0) return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    if (std::strcmp(name, "MISC1")         == 0) return SDL_CONTROLLER_BUTTON_MISC1;
    if (std::strcmp(name, "PADDLE1")       == 0) return SDL_CONTROLLER_BUTTON_PADDLE1;
    if (std::strcmp(name, "PADDLE2")       == 0) return SDL_CONTROLLER_BUTTON_PADDLE2;
    if (std::strcmp(name, "PADDLE3")       == 0) return SDL_CONTROLLER_BUTTON_PADDLE3;
    if (std::strcmp(name, "PADDLE4")       == 0) return SDL_CONTROLLER_BUTTON_PADDLE4;
    if (std::strcmp(name, "TOUCHPAD")      == 0) return SDL_CONTROLLER_BUTTON_TOUCHPAD;
    return SDL_CONTROLLER_BUTTON_INVALID;
}

static SDL_GameControllerAxis parseGamepadAxisName(const char *name) {
    if (std::strcmp(name, "LEFT_X")        == 0) return SDL_CONTROLLER_AXIS_LEFTX;
    if (std::strcmp(name, "LEFT_Y")        == 0) return SDL_CONTROLLER_AXIS_LEFTY;
    if (std::strcmp(name, "RIGHT_X")       == 0) return SDL_CONTROLLER_AXIS_RIGHTX;
    if (std::strcmp(name, "RIGHT_Y")       == 0) return SDL_CONTROLLER_AXIS_RIGHTY;
    if (std::strcmp(name, "LEFT_TRIGGER")  == 0) return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    if (std::strcmp(name, "RIGHT_TRIGGER") == 0) return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    return SDL_CONTROLLER_AXIS_INVALID;
}

// ── Constructor ───────────────────────────────────────────────────────────────

InputHandler::InputHandler(SDL_Window *window, OptionsUtil::Options *options) :
        window(window), options(options) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    lookAroundSpeedOption = options->getOption<double>(HASH("player_lookAroundSpeed"));
    gamepadDeadZoneOption = options->getOption<double>(HASH("gamepad_deadZone"));

    if (!loadBindingsFromXML("./Engine/inputBindings.xml")) {
        loadDefaultBindings();
    }

    // ── SDL GameController subsystem ──────────────────────────────────────────
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "InputHandler: SDL_INIT_GAMECONTROLLER failed: " << SDL_GetError() << std::endl;
    } else {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                gameController = SDL_GameControllerOpen(i);
                if (gameController != nullptr) {
                    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gameController);
                    joystickInstanceID = SDL_JoystickInstanceID(joystick);
                    std::cout << "InputHandler: opened gamepad \"" << SDL_GameControllerName(gameController) << "\"" << std::endl;
                    break;
                }
            }
        }
    }
}

// ── Default bindings (used when inputBindings.xml is absent) ─────────────────

void InputHandler::loadDefaultBindings() {
    // Keyboard
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

    // Mouse buttons
    mouseButtonBindings[SDL_BUTTON_LEFT]   = {InputActions::MOUSE_BUTTON_LEFT};
    mouseButtonBindings[SDL_BUTTON_MIDDLE] = {InputActions::MOUSE_BUTTON_MIDDLE};
    mouseButtonBindings[SDL_BUTTON_RIGHT]  = {InputActions::MOUSE_BUTTON_RIGHT};

    // Mouse analog and wheel
    mouseAnalogXAction   = InputActions::LOOK_X;
    mouseAnalogYAction   = InputActions::LOOK_Y;
    mouseWheelUpAction   = InputActions::MOUSE_WHEEL_UP;
    mouseWheelDownAction = InputActions::MOUSE_WHEEL_DOWN;

    // Gamepad buttons
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_A]             = {InputActions::JUMP};
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]  = {InputActions::RUN, InputActions::KEY_SHIFT};
    gamepadButtonBindings[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = {InputActions::MOUSE_BUTTON_RIGHT};

    // Gamepad digital axes
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTY,        -0.3f, InputActions::MOVE_FORWARD});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTY,         0.3f, InputActions::MOVE_BACKWARD});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTX,        -0.3f, InputActions::MOVE_LEFT});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_LEFTX,         0.3f, InputActions::MOVE_RIGHT});
    gamepadDigitalAxisBindings.push_back({SDL_CONTROLLER_AXIS_TRIGGERRIGHT,  0.3f, InputActions::MOUSE_BUTTON_LEFT});

    // Gamepad analog axes — scale normalized to match mouse unit (xrel / screenWidth*0.5)
    float analogLookScale = GAMEPAD_LOOK_PIXELS_PER_FRAME / (options->getScreenWidth() * 0.5f);
    gamepadAnalogAxisBindings.push_back({SDL_CONTROLLER_AXIS_RIGHTX, analogLookScale, InputActions::LOOK_X});
    gamepadAnalogAxisBindings.push_back({SDL_CONTROLLER_AXIS_RIGHTY, analogLookScale, InputActions::LOOK_Y});
}

// ── XML binding loader ────────────────────────────────────────────────────────

bool InputHandler::loadBindingsFromXML(const std::string &filePath) {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLError result = document.LoadFile(filePath.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        std::cout << "InputHandler: could not load " << filePath << " — using built-in defaults." << std::endl;
        return false;
    }

    tinyxml2::XMLElement *root = document.FirstChildElement("InputBindings");
    if (root == nullptr) {
        std::cerr << "InputHandler: " << filePath << " has no <InputBindings> root element." << std::endl;
        return false;
    }

    for (tinyxml2::XMLElement *actionElement = root->FirstChildElement("Action");
         actionElement != nullptr;
         actionElement = actionElement->NextSiblingElement("Action")) {

        const char *actionName = actionElement->Attribute("name");
        if (actionName == nullptr) {
            std::cerr << "InputHandler: <Action> missing name attribute, skipping." << std::endl;
            continue;
        }

        uint64_t actionHash = hash(actionName);

        for (tinyxml2::XMLElement *bindingElement = actionElement->FirstChildElement("Binding");
             bindingElement != nullptr;
             bindingElement = bindingElement->NextSiblingElement("Binding")) {

            const char *source = bindingElement->Attribute("source");
            if (source == nullptr) {
                std::cerr << "InputHandler: <Binding> missing source for action " << actionName << ", skipping." << std::endl;
                continue;
            }

            if (std::strcmp(source, "keyboard") == 0) {
                const char *keyName = bindingElement->Attribute("key");
                if (keyName == nullptr) {
                    std::cerr << "InputHandler: keyboard binding missing key for " << actionName << std::endl;
                    continue;
                }
                SDL_Keycode keycode = parseKeyName(keyName);
                if (keycode == SDLK_UNKNOWN) {
                    std::cerr << "InputHandler: unknown key name '" << keyName << "' for action " << actionName << std::endl;
                    continue;
                }
                const char *analogValueAttr = bindingElement->Attribute("analog_value");
                if (analogValueAttr != nullptr) {
                    keyboardAnalogBindings.push_back({keycode, std::stof(analogValueAttr), actionHash});
                } else {
                    keyboardBindings[keycode].push_back(actionHash);
                }

            } else if (std::strcmp(source, "mouse_button") == 0) {
                const char *buttonName = bindingElement->Attribute("button");
                if (buttonName == nullptr) {
                    std::cerr << "InputHandler: mouse_button binding missing button for " << actionName << std::endl;
                    continue;
                }
                uint8_t button = parseMouseButtonName(buttonName);
                if (button == 0) {
                    std::cerr << "InputHandler: unknown mouse button '" << buttonName << "'" << std::endl;
                    continue;
                }
                mouseButtonBindings[button].push_back(actionHash);

            } else if (std::strcmp(source, "mouse_wheel") == 0) {
                const char *axisName = bindingElement->Attribute("axis");
                if (axisName == nullptr) {
                    std::cerr << "InputHandler: mouse_wheel binding missing axis for " << actionName << std::endl;
                    continue;
                }
                if      (std::strcmp(axisName, "UP")    == 0) mouseWheelUpAction    = actionHash;
                else if (std::strcmp(axisName, "DOWN")  == 0) mouseWheelDownAction  = actionHash;
                else if (std::strcmp(axisName, "LEFT")  == 0) mouseWheelLeftAction  = actionHash;
                else if (std::strcmp(axisName, "RIGHT") == 0) mouseWheelRightAction = actionHash;
                else std::cerr << "InputHandler: unknown mouse_wheel axis '" << axisName << "' (expected UP/DOWN/LEFT/RIGHT)" << std::endl;

            } else if (std::strcmp(source, "mouse_axis") == 0) {
                const char *axisName = bindingElement->Attribute("axis");
                if (axisName == nullptr) {
                    std::cerr << "InputHandler: mouse_axis binding missing axis for " << actionName << std::endl;
                    continue;
                }
                if      (std::strcmp(axisName, "X") == 0) mouseAnalogXAction = actionHash;
                else if (std::strcmp(axisName, "Y") == 0) mouseAnalogYAction = actionHash;
                else std::cerr << "InputHandler: unknown mouse axis '" << axisName << "' (expected X or Y)" << std::endl;

            } else if (std::strcmp(source, "gamepad_button") == 0) {
                const char *buttonName = bindingElement->Attribute("button");
                if (buttonName == nullptr) {
                    std::cerr << "InputHandler: gamepad_button missing button for " << actionName << std::endl;
                    continue;
                }
                SDL_GameControllerButton button = parseGamepadButtonName(buttonName);
                if (button == SDL_CONTROLLER_BUTTON_INVALID) {
                    std::cerr << "InputHandler: unknown gamepad button '" << buttonName << "'" << std::endl;
                    continue;
                }
                gamepadButtonBindings[button].push_back(actionHash);

            } else if (std::strcmp(source, "joystick_button") == 0) {
                // Raw joystick button index — for buttons outside SDL's GameController mapping
                // (e.g. proprietary extra buttons). May fire alongside gamepad_button for mapped buttons.
                const char *indexAttr = bindingElement->Attribute("index");
                if (indexAttr == nullptr) {
                    std::cerr << "InputHandler: joystick_button missing index for " << actionName << std::endl;
                    continue;
                }
                int buttonIndex = std::atoi(indexAttr);
                if (buttonIndex < 0) {
                    std::cerr << "InputHandler: joystick_button index must be >= 0 for " << actionName << std::endl;
                    continue;
                }
                joystickButtonBindings[buttonIndex].push_back(actionHash);

            } else if (std::strcmp(source, "gamepad_axis") == 0) {
                const char *axisName = bindingElement->Attribute("axis");
                if (axisName == nullptr) {
                    std::cerr << "InputHandler: gamepad_axis missing axis for " << actionName << std::endl;
                    continue;
                }
                SDL_GameControllerAxis axis = parseGamepadAxisName(axisName);
                if (axis == SDL_CONTROLLER_AXIS_INVALID) {
                    std::cerr << "InputHandler: unknown gamepad axis '" << axisName << "'" << std::endl;
                    continue;
                }
                const char *thresholdAttr = bindingElement->Attribute("threshold");
                const char *scaleAttr     = bindingElement->Attribute("scale");
                if (thresholdAttr != nullptr) {
                    gamepadDigitalAxisBindings.push_back({axis, std::stof(thresholdAttr), actionHash});
                } else {
                    float scale = (scaleAttr != nullptr) ? std::stof(scaleAttr) : 1.0f;
                    gamepadAnalogAxisBindings.push_back({axis, scale, actionHash});
                }

            } else {
                std::cerr << "InputHandler: unknown binding source '" << source << "' for " << actionName << std::endl;
            }
        }
    }

    std::cout << "InputHandler: loaded bindings from " << filePath << std::endl;
    return true;
}

// ── Per-frame analog axis application ────────────────────────────────────────

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

void InputHandler::applyKeyboardAnalogBindings() {
    if (keyboardAnalogBindings.empty()) {
        return;
    }
    int numKeys = 0;
    const Uint8 *sdlKeyStates = SDL_GetKeyboardState(&numKeys);
    for (const KeyboardAnalogBinding &binding : keyboardAnalogBindings) {
        SDL_Scancode scancode = SDL_GetScancodeFromKey(binding.key);
        if (static_cast<int>(scancode) < numKeys && sdlKeyStates[scancode]) {
            inputState.addAnalogValue(binding.action, binding.value);
        }
    }
}

// ── Main event pump ───────────────────────────────────────────────────────────

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
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = mouseButtonBindings.find(event.button.button);
                if (it != mouseButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
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
                if (mouseAnalogXAction != 0) inputState.addAnalogValue(mouseAnalogXAction, xChange);
                if (mouseAnalogYAction != 0) inputState.addAnalogValue(mouseAnalogYAction, yChange);
                break;
            }
            case SDL_MOUSEWHEEL: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                if (event.wheel.y > 0 && mouseWheelUpAction    != 0) inputState.setInputStatus(mouseWheelUpAction,    true);
                if (event.wheel.y < 0 && mouseWheelDownAction  != 0) inputState.setInputStatus(mouseWheelDownAction,  true);
                if (event.wheel.x > 0 && mouseWheelRightAction != 0) inputState.setInputStatus(mouseWheelRightAction, true);
                if (event.wheel.x < 0 && mouseWheelLeftAction  != 0) inputState.setInputStatus(mouseWheelLeftAction,  true);
                break;
            }

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
                        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gameController);
                        joystickInstanceID = SDL_JoystickInstanceID(joystick);
                        std::cout << "InputHandler: gamepad connected: \"" << SDL_GameControllerName(gameController) << "\"" << std::endl;
                    }
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (gameController != nullptr &&
                    SDL_GameControllerFromInstanceID(event.cdevice.which) == gameController) {
                    std::cout << "InputHandler: gamepad disconnected." << std::endl;
                    SDL_GameControllerClose(gameController);
                    gameController     = nullptr;
                    joystickInstanceID = -1;
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

            // ── Raw joystick buttons (for buttons outside SDL GameController mapping) ──
            case SDL_JOYBUTTONDOWN: {
                if (event.jbutton.which != joystickInstanceID) break;
                inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                auto it = joystickButtonBindings.find(static_cast<int>(event.jbutton.button));
                if (it != joystickButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                break;
            }
            case SDL_JOYBUTTONUP: {
                if (event.jbutton.which != joystickInstanceID) break;
                auto it = joystickButtonBindings.find(static_cast<int>(event.jbutton.button));
                if (it != joystickButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
                }
                break;
            }
        }
    }

    // Analog axes represent absolute stick/key position — apply every frame so
    // held inputs continuously contribute to their analog actions.
    applyGamepadAnalogAxes();
    applyKeyboardAnalogBindings();
}
