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
    struct DigitalAxisBinding {
        SDL_GamepadAxis axis;
        float threshold; // positive = trigger when value >= threshold, negative = trigger when value <= threshold
        uint64_t action;
    };

    struct AnalogAxisBinding {
        SDL_GamepadAxis axis;
        float scale;
        uint64_t action;
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
    std::unordered_map<SDL_GamepadButton, std::vector<uint64_t>>   gamepadButtonBindings;
    std::vector<DigitalAxisBinding>                                 gamepadDigitalAxisBindings;
    std::vector<AnalogAxisBinding>                                  gamepadAnalogAxisBindings;
    std::unordered_map<int, std::vector<uint64_t>>                 joystickButtonBindings;

    SDL_Gamepad    *gameController     = nullptr;
    SDL_JoystickID  joystickInstanceID = 0;
    float currentAxisValues[SDL_GAMEPAD_AXIS_COUNT] = {0.0f};

    bool loadBindingsFromXML(const std::string &filePath);
    void loadDefaultBindings();
    void applyGamepadAnalogAxes();
    void applyKeyboardAnalogBindings();

public:
    InputHandler(SDL_Window *, OptionsUtil::Options *options);

    ~InputHandler() {
        if (gameController != nullptr) {
            SDL_CloseGamepad(gameController);
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
};

#endif //LIMONENGINE_INPUTHANDLER_H
