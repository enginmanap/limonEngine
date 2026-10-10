//
// Created by Engin Manap on 14.02.2016.
//

#ifndef LIMONENGINE_INPUTHANDLER_H
#define LIMONENGINE_INPUTHANDLER_H

#include <unordered_map>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>

#include "limonAPI/Options.h"
#include "limonAPI/InputStates.h"

class InputHandler {
public:
    static constexpr int ANY_GAMEPAD = -1; // binding without a gamepadIndex listens to every connected pad

    struct GamepadButtonBinding {
        uint64_t action;
        int gamepadIndex = ANY_GAMEPAD;
    };

    struct DigitalAxisBinding {
        SDL_GamepadAxis axis;
        float threshold; // positive = trigger when value >= threshold, negative = trigger when value <= threshold
        uint64_t action;
        int gamepadIndex = ANY_GAMEPAD;
    };

    struct AnalogAxisBinding {
        SDL_GamepadAxis axis;
        float scale;
        uint64_t action;
        int gamepadIndex = ANY_GAMEPAD;
    };

    struct KeyboardAnalogBinding {
        SDL_Keycode key;
        float value;
        uint64_t action;
    };

private:
    // Default gamepad look scale: full-stick speed in "mouse pixel equivalents per frame".
    // Matches mouse normalization (xrel / screenWidth*0.5) so lookAroundSpeed tunes both equally.
    static constexpr float GAMEPAD_LOOK_PIXELS_PER_FRAME = 10.0f;

    SDL_Window *window;
    OptionsUtil::Options *options;
    SDL_Event event;
    InputStates inputState;
    OptionsUtil::Options::Option<double> lookAroundSpeedOption;
    OptionsUtil::Options::Option<double> gamepadDeadZoneOption;

    std::unordered_map<SDL_Keycode, std::vector<uint64_t>>         keyboardBindings;
    std::vector<KeyboardAnalogBinding>                              keyboardAnalogBindings;
    std::unordered_map<uint8_t, std::vector<uint64_t>>             mouseButtonBindings;
    uint64_t mouseAnalogXAction    = 0;
    uint64_t mouseAnalogYAction    = 0;
    uint64_t mouseWheelUpAction    = 0;
    uint64_t mouseWheelDownAction  = 0;
    uint64_t mouseWheelLeftAction  = 0;
    uint64_t mouseWheelRightAction = 0;
    std::unordered_map<SDL_GamepadButton, std::vector<GamepadButtonBinding>> gamepadButtonBindings;
    std::vector<DigitalAxisBinding>                                 gamepadDigitalAxisBindings;
    std::vector<AnalogAxisBinding>                                  gamepadAnalogAxisBindings;
    std::unordered_map<int, std::vector<GamepadButtonBinding>>      joystickButtonBindings;

    // slots are never erased, a vacated one is the first to be reused so a reconnecting pad gets its old index back
    struct GamepadSlot {
        SDL_Gamepad *gamepad = nullptr;
        SDL_JoystickID instanceID = 0;
        float axisValues[SDL_GAMEPAD_AXIS_COUNT] = {0.0f};
    };
    std::vector<GamepadSlot> gamepadSlots;
    bool textInputActive = false;

    bool loadBindingsFromXML(const std::string &filePath);
    void loadDefaultBindings();
    void openGamepad(SDL_JoystickID instanceID);
    void closeGamepad(SDL_JoystickID instanceID);
    int findGamepadSlot(SDL_JoystickID instanceID) const;
    void applyGamepadAnalogAxes();
    void applyKeyboardAnalogBindings();

    static bool bindingMatchesSlot(int bindingGamepadIndex, int slotIndex) {
        return bindingGamepadIndex == ANY_GAMEPAD || bindingGamepadIndex == slotIndex;
    }

public:
    InputHandler(SDL_Window *, OptionsUtil::Options *options);

    ~InputHandler() {
        for (GamepadSlot &slot : gamepadSlots) {
            if (slot.gamepad != nullptr) {
                SDL_CloseGamepad(slot.gamepad);
            }
        }
        SDL_SetWindowMouseGrab(window, false);
        SDL_SetWindowRelativeMouseMode(window, false);
    }

    void setMouseModeRelative() {
        SDL_SetWindowRelativeMouseMode(window, true);
        SDL_SetWindowMouseGrab(window, false);
        SDL_HideCursor();
    }

    void setMouseModeFree() {
        SDL_SetWindowRelativeMouseMode(window, false);
        SDL_SetWindowMouseGrab(window, true);
        SDL_ShowCursor();
    }

    void mapInput();

    const InputStates &getInputStates() const {
        return this->inputState;
    }

    void setTextInputMode(bool active) {
        if (active == textInputActive) {
            return;
        }
        if (active) {
            SDL_StartTextInput(window);
        } else {
            SDL_StopTextInput(window);
        }
        textInputActive = active;
    }
};

#endif //LIMONENGINE_INPUTHANDLER_H
