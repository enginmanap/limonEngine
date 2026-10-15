//
// Created by Engin Manap on 14.02.2016.
//


#include "InputHandler.h"
#include "ImGuiHelper.h"
#include <tinyxml2.h>
#include <cstring>
#include "LimonFileSystem.h"
#include <iostream>

// ── XML parsing helpers ───────────────────────────────────────────────────────

static SDL_Keycode parseKeyName(const char *name) {
    // SDL names these per platform once video is up (Windows says "Left Windows"), so every spelling resolves on every platform
    if (SDL_strcasecmp(name, "Left GUI") == 0 || SDL_strcasecmp(name, "Left Windows") == 0)   return SDLK_LGUI;
    if (SDL_strcasecmp(name, "Right GUI") == 0 || SDL_strcasecmp(name, "Right Windows") == 0) return SDLK_RGUI;
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

static SDL_GamepadButton parseGamepadButtonName(const char *name) {
    if (std::strcmp(name, "A")             == 0) return SDL_GAMEPAD_BUTTON_SOUTH;
    if (std::strcmp(name, "B")             == 0) return SDL_GAMEPAD_BUTTON_EAST;
    if (std::strcmp(name, "X")             == 0) return SDL_GAMEPAD_BUTTON_WEST;
    if (std::strcmp(name, "Y")             == 0) return SDL_GAMEPAD_BUTTON_NORTH;
    if (std::strcmp(name, "BACK")          == 0) return SDL_GAMEPAD_BUTTON_BACK;
    if (std::strcmp(name, "GUIDE")         == 0) return SDL_GAMEPAD_BUTTON_GUIDE;
    if (std::strcmp(name, "START")         == 0) return SDL_GAMEPAD_BUTTON_START;
    if (std::strcmp(name, "LEFTSTICK")     == 0) return SDL_GAMEPAD_BUTTON_LEFT_STICK;
    if (std::strcmp(name, "RIGHTSTICK")    == 0) return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    if (std::strcmp(name, "L3")            == 0) return SDL_GAMEPAD_BUTTON_LEFT_STICK;   // common alias
    if (std::strcmp(name, "R3")            == 0) return SDL_GAMEPAD_BUTTON_RIGHT_STICK;  // common alias
    if (std::strcmp(name, "LEFTSHOULDER")  == 0) return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    if (std::strcmp(name, "RIGHTSHOULDER") == 0) return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    if (std::strcmp(name, "DPAD_UP")       == 0) return SDL_GAMEPAD_BUTTON_DPAD_UP;
    if (std::strcmp(name, "DPAD_DOWN")     == 0) return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    if (std::strcmp(name, "DPAD_LEFT")     == 0) return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    if (std::strcmp(name, "DPAD_RIGHT")    == 0) return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    if (std::strcmp(name, "MISC1")         == 0) return SDL_GAMEPAD_BUTTON_MISC1;
    if (std::strcmp(name, "PADDLE1")       == 0) return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1;
    if (std::strcmp(name, "PADDLE2")       == 0) return SDL_GAMEPAD_BUTTON_LEFT_PADDLE1;
    if (std::strcmp(name, "PADDLE3")       == 0) return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2;
    if (std::strcmp(name, "PADDLE4")       == 0) return SDL_GAMEPAD_BUTTON_LEFT_PADDLE2;
    if (std::strcmp(name, "TOUCHPAD")      == 0) return SDL_GAMEPAD_BUTTON_TOUCHPAD;
    return SDL_GAMEPAD_BUTTON_INVALID;
}

static SDL_GamepadAxis parseGamepadAxisName(const char *name) {
    if (std::strcmp(name, "LEFT_X")        == 0) return SDL_GAMEPAD_AXIS_LEFTX;
    if (std::strcmp(name, "LEFT_Y")        == 0) return SDL_GAMEPAD_AXIS_LEFTY;
    if (std::strcmp(name, "RIGHT_X")       == 0) return SDL_GAMEPAD_AXIS_RIGHTX;
    if (std::strcmp(name, "RIGHT_Y")       == 0) return SDL_GAMEPAD_AXIS_RIGHTY;
    if (std::strcmp(name, "LEFT_TRIGGER")  == 0) return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    if (std::strcmp(name, "RIGHT_TRIGGER") == 0) return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    return SDL_GAMEPAD_AXIS_INVALID;
}

// Absent attribute leaves ANY_GAMEPAD, returns false for a negative or non numeric value.
static bool parseGamepadIndex(const tinyxml2::XMLElement *bindingElement, int &gamepadIndex) {
    gamepadIndex = InputHandler::ANY_GAMEPAD;
    if (bindingElement->Attribute("gamepadIndex") == nullptr) {
        return true;
    }
    return bindingElement->QueryIntAttribute("gamepadIndex", &gamepadIndex) == tinyxml2::XML_SUCCESS && gamepadIndex >= 0;
}

// ── Constructor ───────────────────────────────────────────────────────────────

InputHandler::InputHandler(SDL_Window *window, OptionsUtil::Options *options) :
        window(window), options(options) {
    SDL_SetWindowMouseGrab(window, true);
    SDL_SetWindowRelativeMouseMode(window, true);
    lookAroundSpeedOption = options->getOption<double>(HASH("player_lookAroundSpeed"));
    gamepadDeadZoneOption = options->getOption<double>(HASH("gamepad_deadZone"));

    loadEngineBindings();
    // If a game wants to override bindings, we expect them to put the file under ./Data. The package's copy applies first, a loose one over it
    const std::string overrideBindingsFile = "./Data/inputBindings.xml";
    std::vector<std::string> overrideLayers = LimonFileSystem::readFileFromAllLayers(overrideBindingsFile);
    for (const std::string &layerContents : overrideLayers) {
        tinyxml2::XMLDocument document;
        if (document.Parse(layerContents.data(), layerContents.size()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "InputHandler: could not read " << overrideBindingsFile << ", the engine's bindings stay." << std::endl;
            continue;
        }
        applyOverrideBindings(document, overrideBindingsFile);
    }

    // ── SDL Gamepad subsystem ─────────────────────────────────────────────────
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        std::cerr << "InputHandler: SDL_INIT_GAMEPAD failed: " << SDL_GetError() << std::endl;
    } else {
        int joystickCount = 0;
        SDL_JoystickID *joysticks = SDL_GetJoysticks(&joystickCount);
        if (joysticks != nullptr) {
            for (int i = 0; i < joystickCount; ++i) {
                if (SDL_IsGamepad(joysticks[i])) {
                    openGamepad(joysticks[i]);
                }
            }
            SDL_free(joysticks);
        }
    }
}

// ── Gamepad slots ─────────────────────────────────────────────────────────────

int InputHandler::findGamepadSlot(SDL_JoystickID instanceID) const {
    for (size_t slotIndex = 0; slotIndex < gamepadSlots.size(); ++slotIndex) {
        if (gamepadSlots[slotIndex].gamepad != nullptr && gamepadSlots[slotIndex].instanceID == instanceID) {
            return static_cast<int>(slotIndex);
        }
    }
    return -1;
}

void InputHandler::openGamepad(SDL_JoystickID instanceID) {
    if (findGamepadSlot(instanceID) >= 0) { // SDL queues ADDED for pads we already opened at startup
        return;
    }
    SDL_Gamepad *gamepad = SDL_OpenGamepad(instanceID);
    if (gamepad == nullptr) {
        std::cerr << "InputHandler: could not open gamepad: " << SDL_GetError() << std::endl;
        return;
    }

    int slotIndex = -1;
    for (size_t candidate = 0; candidate < gamepadSlots.size(); ++candidate) {
        if (gamepadSlots[candidate].gamepad == nullptr) {
            slotIndex = static_cast<int>(candidate);
            break;
        }
    }
    if (slotIndex < 0) {
        gamepadSlots.emplace_back();
        slotIndex = static_cast<int>(gamepadSlots.size()) - 1;
    }
    gamepadSlots[slotIndex].gamepad = gamepad;
    gamepadSlots[slotIndex].instanceID = instanceID;
    std::cout << "InputHandler: gamepad " << slotIndex << " connected: \"" << SDL_GetGamepadName(gamepad) << "\"" << std::endl;
}

void InputHandler::closeGamepad(SDL_JoystickID instanceID) {
    int slotIndex = findGamepadSlot(instanceID);
    if (slotIndex < 0) {
        return;
    }
    GamepadSlot &slot = gamepadSlots[slotIndex];
    std::cout << "InputHandler: gamepad " << slotIndex << " disconnected." << std::endl;
    SDL_CloseGamepad(slot.gamepad);
    slot = GamepadSlot();
}

// ── Default bindings (used when inputBindings.xml is absent) ─────────────────

void InputHandler::loadDefaultBindings() {
    // Keyboard
    keyboardBindings[SDLK_ESCAPE] = {InputActions::QUIT};
    keyboardBindings[SDLK_W]      = {InputActions::MOVE_FORWARD};
    keyboardBindings[SDLK_A]      = {InputActions::MOVE_LEFT};
    keyboardBindings[SDLK_S]      = {InputActions::MOVE_BACKWARD};
    keyboardBindings[SDLK_D]      = {InputActions::MOVE_RIGHT};
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
    PadBindings &everyPad = padBindings[ANY_GAMEPAD];
    everyPad.buttons[SDL_GAMEPAD_BUTTON_SOUTH]         = {InputActions::JUMP};
    everyPad.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = {InputActions::RUN, InputActions::KEY_SHIFT};
    everyPad.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER]= {InputActions::MOUSE_BUTTON_RIGHT};

    // Gamepad digital axes
    everyPad.digitalAxes.push_back({SDL_GAMEPAD_AXIS_LEFTY,         -0.3f, InputActions::MOVE_FORWARD});
    everyPad.digitalAxes.push_back({SDL_GAMEPAD_AXIS_LEFTY,          0.3f, InputActions::MOVE_BACKWARD});
    everyPad.digitalAxes.push_back({SDL_GAMEPAD_AXIS_LEFTX,         -0.3f, InputActions::MOVE_LEFT});
    everyPad.digitalAxes.push_back({SDL_GAMEPAD_AXIS_LEFTX,          0.3f, InputActions::MOVE_RIGHT});
    everyPad.digitalAxes.push_back({SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,  0.3f, InputActions::MOUSE_BUTTON_LEFT});

    // Gamepad analog axes — scale normalized to match mouse unit (xrel / screenWidth*0.5)
    float analogLookScale = GAMEPAD_LOOK_PIXELS_PER_FRAME / (options->getScreenWidth() * 0.5f);
    everyPad.analogAxes.push_back({SDL_GAMEPAD_AXIS_RIGHTX, analogLookScale, InputActions::LOOK_X});
    everyPad.analogAxes.push_back({SDL_GAMEPAD_AXIS_RIGHTY, analogLookScale, InputActions::LOOK_Y});
}

// ── XML binding loader ────────────────────────────────────────────────────────

bool InputHandler::bindingSourceDevice(const char *source, BindingDeviceType &device) {
    if (std::strcmp(source, "keyboard") == 0) {
        device = BindingDeviceType::KEYBOARD;
    } else if (std::strcmp(source, "mouse_button") == 0 || std::strcmp(source, "mouse_wheel") == 0 || std::strcmp(source, "mouse_axis") == 0) {
        device = BindingDeviceType::MOUSE;
    } else if (std::strcmp(source, "gamepad_button") == 0 || std::strcmp(source, "gamepad_axis") == 0 || std::strcmp(source, "joystick_button") == 0) {
        device = BindingDeviceType::PAD;
    } else {
        return false;
    }
    return true;
}

void InputHandler::clearDeviceBindings(BindingDeviceType device) {
    switch (device) {
        case BindingDeviceType::KEYBOARD:
            keyboardBindings.clear();
            keyboardAnalogBindings.clear();
            break;
        case BindingDeviceType::MOUSE:
            mouseButtonBindings.clear();
            mouseAnalogXAction = mouseAnalogYAction = 0;
            mouseWheelUpAction = mouseWheelDownAction = mouseWheelLeftAction = mouseWheelRightAction = 0;
            break;
        case BindingDeviceType::PAD:
            padBindings.clear();
            break;
    }
}

InputHandler::PadBindingsForSlot InputHandler::bindingsForSlot(int slotIndex) const {
    PadBindingsForSlot slotBindings;
    std::map<int, PadBindings>::const_iterator ownBindings = padBindings.find(slotIndex);
    if (ownBindings != padBindings.end()) {
        slotBindings.own = &ownBindings->second;
    }
    std::map<int, PadBindings>::const_iterator everyPad = padBindings.find(ANY_GAMEPAD);
    if (everyPad != padBindings.end() && (ownBindings == padBindings.end() || !ownBindings->second.overridden)) {
        slotBindings.everyPad = &everyPad->second;
    }
    return slotBindings;
}

void InputHandler::collectBoundActions(std::set<uint64_t> &boundActions) const {
    for (const std::pair<const SDL_Keycode, std::vector<uint64_t>> &entry : keyboardBindings) {
        boundActions.insert(entry.second.begin(), entry.second.end());
    }
    for (const KeyboardAnalogBinding &binding : keyboardAnalogBindings) {
        boundActions.insert(binding.action);
    }
    for (const std::pair<const uint8_t, std::vector<uint64_t>> &entry : mouseButtonBindings) {
        boundActions.insert(entry.second.begin(), entry.second.end());
    }
    const uint64_t singleActions[] = {mouseAnalogXAction, mouseAnalogYAction, mouseWheelUpAction, mouseWheelDownAction, mouseWheelLeftAction, mouseWheelRightAction};
    for (uint64_t action : singleActions) {
        if (action != 0) {
            boundActions.insert(action);
        }
    }
    for (const std::pair<const int, PadBindings> &pad : padBindings) {
        for (const std::pair<const SDL_GamepadButton, std::vector<uint64_t>> &entry : pad.second.buttons) {
            boundActions.insert(entry.second.begin(), entry.second.end());
        }
        for (const DigitalAxisBinding &binding : pad.second.digitalAxes) {
            boundActions.insert(binding.action);
        }
        for (const AnalogAxisBinding &binding : pad.second.analogAxes) {
            boundActions.insert(binding.action);
        }
        for (const std::pair<const int, std::vector<uint64_t>> &entry : pad.second.joystickButtons) {
            boundActions.insert(entry.second.begin(), entry.second.end());
        }
    }
}

void InputHandler::loadEngineBindings() {
    engineActionNames.clear();
    if (!addBindingsFromXML("./Engine/inputBindings.xml", &engineActionNames)) {
        loadDefaultBindings();
    }
}

bool InputHandler::applyOverrideBindings(tinyxml2::XMLDocument &document, const std::string &filePath) {
    tinyxml2::XMLElement *root = document.FirstChildElement("InputBindings");
    if (root == nullptr) {
        std::cerr << "InputHandler: " << filePath << " has no <InputBindings> root element, the engine's bindings stay." << std::endl;
        return false;
    }

    // what the file binds: keyboard and mouse as a whole, and pads either all of them (a binding without gamepadIndex) or by index
    bool usesKeyboard = false;
    bool usesMouse = false;
    bool usesAllPads = false;
    std::set<int> usedPads;
    for (tinyxml2::XMLElement *actionElement = root->FirstChildElement("Action"); actionElement != nullptr; actionElement = actionElement->NextSiblingElement("Action")) {
        for (tinyxml2::XMLElement *bindingElement = actionElement->FirstChildElement("Binding"); bindingElement != nullptr; bindingElement = bindingElement->NextSiblingElement("Binding")) {
            const char *source = bindingElement->Attribute("source");
            BindingDeviceType device;
            if (source == nullptr || !bindingSourceDevice(source, device)) {
                continue;
            }
            if (device == BindingDeviceType::KEYBOARD) {
                usesKeyboard = true;
            } else if (device == BindingDeviceType::MOUSE) {
                usesMouse = true;
            } else {
                int gamepadIndex;
                if (!parseGamepadIndex(bindingElement, gamepadIndex)) {
                    continue;//the loader reports it
                }
                if (gamepadIndex == ANY_GAMEPAD) {
                    usesAllPads = true;
                } else {
                    usedPads.insert(gamepadIndex);
                }
            }
        }
    }

    std::string replaced;
    if (usesKeyboard) {
        clearDeviceBindings(BindingDeviceType::KEYBOARD);
        replaced += (replaced.empty() ? "" : ", ") + std::string("keyboard");
    }
    if (usesMouse) {
        clearDeviceBindings(BindingDeviceType::MOUSE);
        replaced += (replaced.empty() ? "" : ", ") + std::string("mouse");
    }
    if (usesAllPads) {
        clearDeviceBindings(BindingDeviceType::PAD);
        replaced += (replaced.empty() ? "" : ", ") + std::string("all gamepads");
    } else {
        // the engine's bindings of these pads go, and the ones for every pad stop applying to them
        for (int gamepadIndex : usedPads) {
            padBindings.erase(gamepadIndex);
            padBindings[gamepadIndex].overridden = true;
            replaced += (replaced.empty() ? "" : ", ") + std::string("gamepad ") + std::to_string(gamepadIndex);
        }
    }
    if (!addBindingsFromDocument(document, filePath)) {
        return false;
    }

    std::cout << "InputHandler: " << filePath << " replaces the engine's bindings for: " << (replaced.empty() ? "nothing" : replaced) << std::endl;
    std::set<uint64_t> boundActions;
    collectBoundActions(boundActions);
    std::string unbound;
    for (const std::pair<uint64_t, std::string> &engineAction : engineActionNames) {
        if (boundActions.find(engineAction.first) == boundActions.end() && unbound.find(" " + engineAction.second + ",") == std::string::npos) {
            unbound += " " + engineAction.second + ",";
        }
    }
    if (!unbound.empty()) {
        unbound.pop_back();
        std::cout << "InputHandler: engine actions with no binding left:" << unbound << std::endl;
    }
    return true;
}

bool InputHandler::addBindingsFromXML(const std::string &filePath, std::vector<std::pair<uint64_t, std::string>> *actionNames) {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLError result = LimonFileSystem::loadXML(filePath, document);
    if (result != tinyxml2::XML_SUCCESS) {
        std::cout << "InputHandler: could not load " << filePath << " — using built-in defaults." << std::endl;
        return false;
    }
    return addBindingsFromDocument(document, filePath, actionNames);
}

bool InputHandler::addBindingsFromDocument(tinyxml2::XMLDocument &document, const std::string &filePath, std::vector<std::pair<uint64_t, std::string>> *actionNames) {
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
        if (actionNames != nullptr) {
            actionNames->push_back({actionHash, actionName});
        }

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
                SDL_GamepadButton button = parseGamepadButtonName(buttonName);
                if (button == SDL_GAMEPAD_BUTTON_INVALID) {
                    std::cerr << "InputHandler: unknown gamepad button '" << buttonName << "'" << std::endl;
                    continue;
                }
                int gamepadIndex;
                if (!parseGamepadIndex(bindingElement, gamepadIndex)) {
                    std::cerr << "InputHandler: gamepadIndex must be a number >= 0 for " << actionName << std::endl;
                    continue;
                }
                padBindings[gamepadIndex].buttons[button].push_back(actionHash);

            } else if (std::strcmp(source, "joystick_button") == 0) {
                // Raw joystick button index — for buttons outside SDL's Gamepad mapping
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
                int gamepadIndex;
                if (!parseGamepadIndex(bindingElement, gamepadIndex)) {
                    std::cerr << "InputHandler: gamepadIndex must be a number >= 0 for " << actionName << std::endl;
                    continue;
                }
                padBindings[gamepadIndex].joystickButtons[buttonIndex].push_back(actionHash);

            } else if (std::strcmp(source, "gamepad_axis") == 0) {
                const char *axisName = bindingElement->Attribute("axis");
                if (axisName == nullptr) {
                    std::cerr << "InputHandler: gamepad_axis missing axis for " << actionName << std::endl;
                    continue;
                }
                SDL_GamepadAxis axis = parseGamepadAxisName(axisName);
                if (axis == SDL_GAMEPAD_AXIS_INVALID) {
                    std::cerr << "InputHandler: unknown gamepad axis '" << axisName << "'" << std::endl;
                    continue;
                }
                int gamepadIndex;
                if (!parseGamepadIndex(bindingElement, gamepadIndex)) {
                    std::cerr << "InputHandler: gamepadIndex must be a number >= 0 for " << actionName << std::endl;
                    continue;
                }
                const char *thresholdAttr = bindingElement->Attribute("threshold");
                const char *scaleAttr     = bindingElement->Attribute("scale");
                if (thresholdAttr != nullptr) {
                    padBindings[gamepadIndex].digitalAxes.push_back({axis, std::stof(thresholdAttr), actionHash});
                } else {
                    float scale = (scaleAttr != nullptr) ? std::stof(scaleAttr) : 1.0f;
                    padBindings[gamepadIndex].analogAxes.push_back({axis, scale, actionHash});
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
    float deadZone = static_cast<float>(gamepadDeadZoneOption.get());
    for (size_t slotIndex = 0; slotIndex < gamepadSlots.size(); ++slotIndex) {
        if (gamepadSlots[slotIndex].gamepad == nullptr) {
            continue;
        }
        const PadBindingsForSlot slotBindings = bindingsForSlot(static_cast<int>(slotIndex));
        const PadBindings *bindingSets[] = {slotBindings.own, slotBindings.everyPad};
        for (const PadBindings *bindings : bindingSets) {
            if (bindings == nullptr) {
                continue;
            }
            for (const AnalogAxisBinding &binding : bindings->analogAxes) {
                float value = gamepadSlots[slotIndex].axisValues[static_cast<int>(binding.axis)];
                if (std::abs(value) > deadZone) {
                    inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                    inputState.addAnalogValue(binding.action, value * binding.scale);
                }
            }
        }
    }
}

void InputHandler::applyKeyboardAnalogBindings() {
    if (keyboardAnalogBindings.empty()) {
        return;
    }
    int numKeys = 0;
    const bool *sdlKeyStates = SDL_GetKeyboardState(&numKeys);
    for (const KeyboardAnalogBinding &binding : keyboardAnalogBindings) {
        SDL_Scancode scancode = SDL_GetScancodeFromKey(binding.key, NULL);
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
            case SDL_EVENT_QUIT:
                inputState.setInputStatus(InputActions::QUIT, true);
                break;

            // ── Mouse ─────────────────────────────────────────────────────────
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = mouseButtonBindings.find(event.button.button);
                if (it != mouseButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = mouseButtonBindings.find(event.button.button);
                if (it != mouseButtonBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                inputState.setInputStatus(InputActions::MOUSE_MOVE, true);
                // HiDPI scaling messes with the sizes, so SDL capture that is in window coordinates, might not be
                // equal to pixels in our own calculations. This part does the normalizing
                float normalizeWidth  = options->getWindowWidth()  > 0 ? (float)options->getWindowWidth()  : (float)options->getScreenWidth();
                float normalizeHeight = options->getWindowHeight() > 0 ? (float)options->getWindowHeight() : (float)options->getScreenHeight();
                float xPos    = (event.motion.x    - (normalizeWidth  / 2.0f)) / (normalizeWidth  / 2.0f);
                float xChange = (event.motion.xrel)                            / (normalizeWidth  / 2.0f);
                float yPos    = (event.motion.y    - (normalizeHeight / 2.0f)) / (normalizeHeight / 2.0f);
                float yChange = (event.motion.yrel)                            / (normalizeHeight / 2.0f);
                inputState.setMouseChange(xPos, yPos, xChange, yChange);
                if (mouseAnalogXAction != 0) inputState.addAnalogValue(mouseAnalogXAction, xChange);
                if (mouseAnalogYAction != 0) inputState.addAnalogValue(mouseAnalogYAction, yChange);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                if (event.wheel.y > 0 && mouseWheelUpAction    != 0) inputState.setInputStatus(mouseWheelUpAction,    true);
                if (event.wheel.y < 0 && mouseWheelDownAction  != 0) inputState.setInputStatus(mouseWheelDownAction,  true);
                if (event.wheel.x > 0 && mouseWheelRightAction != 0) inputState.setInputStatus(mouseWheelRightAction, true);
                if (event.wheel.x < 0 && mouseWheelLeftAction  != 0) inputState.setInputStatus(mouseWheelLeftAction,  true);
                break;
            }

            // ── Keyboard ──────────────────────────────────────────────────────
            case SDL_EVENT_KEY_DOWN: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = keyboardBindings.find(event.key.key);
                if (it != keyboardBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, true);
                    }
                }
                if (event.key.key == SDLK_KP_PLUS) {
                    float lookAroundSpeed = lookAroundSpeedOption.get();
                    lookAroundSpeedOption.set(lookAroundSpeed + 1.0f);
                } else if (event.key.key == SDLK_KP_MINUS) {
                    float lookAroundSpeed = lookAroundSpeedOption.get();
                    lookAroundSpeedOption.set(lookAroundSpeed - 1.0f);
                }
                break;
            }
            case SDL_EVENT_KEY_UP: {
                inputState.setActiveDevice(InputStates::ActiveDevice::KEYBOARD_MOUSE);
                auto it = keyboardBindings.find(event.key.key);
                if (it != keyboardBindings.end()) {
                    for (uint64_t action : it->second) {
                        inputState.setInputStatus(action, false);
                    }
                }
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                inputState.setText(event.text.text);
                inputState.setInputStatus(InputActions::TEXT_INPUT, true);
                break;

            // ── Gamepad device connect / disconnect ───────────────────────────
            case SDL_EVENT_GAMEPAD_ADDED:
                openGamepad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                closeGamepad(event.gdevice.which);
                break;

            // ── Gamepad buttons ───────────────────────────────────────────────
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                int slotIndex = findGamepadSlot(event.gbutton.which);
                if (slotIndex < 0) {
                    break;
                }
                inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                bool pressed = (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event.gbutton.button);
                const PadBindingsForSlot slotBindings = bindingsForSlot(slotIndex);
                const PadBindings *bindingSets[] = {slotBindings.own, slotBindings.everyPad};
                for (const PadBindings *bindings : bindingSets) {
                    if (bindings == nullptr) {
                        continue;
                    }
                    std::unordered_map<SDL_GamepadButton, std::vector<uint64_t>>::const_iterator bindingIterator = bindings->buttons.find(button);
                    if (bindingIterator != bindings->buttons.end()) {
                        for (uint64_t action : bindingIterator->second) {
                            inputState.setInputStatus(action, pressed);
                        }
                    }
                }
                break;
            }

            // ── Gamepad axes ──────────────────────────────────────────────────
            case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
                int slotIndex = findGamepadSlot(event.gaxis.which);
                if (slotIndex < 0) {
                    break;
                }
                SDL_GamepadAxis axis = static_cast<SDL_GamepadAxis>(event.gaxis.axis);
                float normalizedValue = event.gaxis.value / 32767.0f;

                if (axis < SDL_GAMEPAD_AXIS_COUNT) {
                    gamepadSlots[slotIndex].axisValues[static_cast<int>(axis)] = normalizedValue;
                }

                if (std::abs(normalizedValue) > static_cast<float>(gamepadDeadZoneOption.get())) {
                    inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                }

                const PadBindingsForSlot slotBindings = bindingsForSlot(slotIndex);
                const PadBindings *bindingSets[] = {slotBindings.own, slotBindings.everyPad};
                for (const PadBindings *bindings : bindingSets) {
                    if (bindings == nullptr) {
                        continue;
                    }
                    for (const DigitalAxisBinding &binding : bindings->digitalAxes) {
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
                }
                break;
            }

            // ── Raw joystick buttons (for buttons outside SDL Gamepad mapping) ──
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP: {
                int slotIndex = findGamepadSlot(event.jbutton.which);
                if (slotIndex < 0) {
                    break;
                }
                bool pressed = (event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN);
                if (pressed) {
                    inputState.setActiveDevice(InputStates::ActiveDevice::GAMEPAD);
                }
                const PadBindingsForSlot slotBindings = bindingsForSlot(slotIndex);
                const PadBindings *bindingSets[] = {slotBindings.own, slotBindings.everyPad};
                for (const PadBindings *bindings : bindingSets) {
                    if (bindings == nullptr) {
                        continue;
                    }
                    std::unordered_map<int, std::vector<uint64_t>>::const_iterator bindingIterator = bindings->joystickButtons.find(static_cast<int>(event.jbutton.button));
                    if (bindingIterator != bindings->joystickButtons.end()) {
                        for (uint64_t action : bindingIterator->second) {
                            inputState.setInputStatus(action, pressed);
                        }
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
