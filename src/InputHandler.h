//
// Created by Engin Manap on 14.02.2016.
//

#ifndef LIMONENGINE_INPUTHANDLER_H
#define LIMONENGINE_INPUTHANDLER_H

#include <unordered_map>
#include <vector>
#include <cmath>
#include <SDL2/SDL.h>

#include "limonAPI/Options.h"
#include "limonAPI/InputStates.h"

class InputHandler {
public:
    struct DigitalAxisBinding {
        SDL_GameControllerAxis axis;
        float threshold; // positive = trigger when value >= threshold, negative = trigger when value <= threshold
        uint64_t action;
    };

    struct AnalogAxisBinding {
        SDL_GameControllerAxis axis;
        float scale;
        uint64_t action;
    };

private:
    // Gamepad look scale expressed as "mouse pixel equivalents per frame" at full stick.
    // Matches mouse normalization (xrel / screenWidth*0.5) so lookAroundSpeed tunes both equally.
    static constexpr float GAMEPAD_LOOK_PIXELS_PER_FRAME = 10.0f;

    SDL_Window *window;
    OptionsUtil::Options *options;
    SDL_Event event;
    InputStates inputState;
    OptionsUtil::Options::Option<double> lookAroundSpeedOption;
    OptionsUtil::Options::Option<double> gamepadDeadZoneOption;

    std::unordered_map<SDL_Keycode, std::vector<uint64_t>>          keyboardBindings;
    std::unordered_map<uint8_t, uint64_t>                           mouseButtonBindings;
    std::unordered_map<SDL_GameControllerButton, std::vector<uint64_t>> gamepadButtonBindings;
    std::vector<DigitalAxisBinding>                                  gamepadDigitalAxisBindings;
    std::vector<AnalogAxisBinding>                                   gamepadAnalogAxisBindings;

    SDL_GameController *gameController = nullptr;
    float currentAxisValues[SDL_CONTROLLER_AXIS_MAX] = {0.0f};

    void applyGamepadAnalogAxes();

public:
    InputHandler(SDL_Window *, OptionsUtil::Options *options);

    ~InputHandler() {
        if (gameController != nullptr) {
            SDL_GameControllerClose(gameController);
        }
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    void setMouseModeRelative() {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_ShowCursor(SDL_FALSE);
    }

    void setMouseModeFree() {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_SetWindowGrab(window, SDL_TRUE);
        SDL_ShowCursor(SDL_TRUE);
    }

    void mapInput();

    const InputStates &getInputStates() const {
        return this->inputState;
    }
};

#endif //LIMONENGINE_INPUTHANDLER_H
