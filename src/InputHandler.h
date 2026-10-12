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
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class InputHandler {
public:
    static constexpr int ANY_GAMEPAD = -1; // the bindings of a pad index, or this for the ones that listen to every connected pad

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

    // Everything bound on one pad. A pad is one device with any number of members, like the keyboard is one device.
    struct PadBindings {
        std::unordered_map<SDL_GamepadButton, std::vector<uint64_t>> buttons;
        std::vector<DigitalAxisBinding> digitalAxes;
        std::vector<AnalogAxisBinding> analogAxes;
        std::unordered_map<int, std::vector<uint64_t>> joystickButtons; // raw buttons outside SDL's gamepad mapping. We listen them together with buttons
        bool overridden = false; // the override file binds this pad itself, so the bindings for every pad do not apply to it
    };
    std::map<int, PadBindings> padBindings; // by pad index, ANY_GAMEPAD holds the bindings for every pad

    // slots are never erased, a vacated one is the first to be reused so a reconnecting pad gets its old index back
    struct GamepadSlot {
        SDL_Gamepad *gamepad = nullptr;
        SDL_JoystickID instanceID = 0;
        float axisValues[SDL_GAMEPAD_AXIS_COUNT] = {0.0f};
    };
    std::vector<GamepadSlot> gamepadSlots;
    bool textInputActive = false;

    // A binding source belongs to a device: the keyboard, the mouse, or a pad. An override file takes over whole devices
    enum class BindingDeviceType { KEYBOARD, MOUSE, PAD };
    static bool bindingSourceDevice(const char *source, BindingDeviceType &device);
    void clearDeviceBindings(BindingDeviceType device); // every pad for BindingDevice::PAD
    void collectBoundActions(std::set<uint64_t> &boundActions) const;
    // What a pad slot listens to: the bindings of its own pad index, and the ones for every pad unless the pad is overridden.
    // Either can be missing.
    struct PadBindingsForSlot {
        const PadBindings *own = nullptr;
        const PadBindings *everyPad = nullptr;
    };
    PadBindingsForSlot bindingsForSlot(int slotIndex) const;

    // Adds the bindings of one file to the ones already loaded, actionNames, when given, receives every action the file defines.
    bool addBindingsFromXML(const std::string &filePath, std::vector<std::pair<uint64_t, std::string>> *actionNames = nullptr);

    void loadEngineBindings(); // ./Engine/inputBindings.xml, or the hardcoded defaults when it can't be read
    bool applyOverrideBindings(const std::string &filePath);// loads ./Data/inputBindings.xml. If a device has any entry, all defaults dropped.
    void loadDefaultBindings();
    void openGamepad(SDL_JoystickID instanceID);
    void closeGamepad(SDL_JoystickID instanceID);
    int findGamepadSlot(SDL_JoystickID instanceID) const;
    void applyGamepadAnalogAxes();
    void applyKeyboardAnalogBindings();

    std::vector<std::pair<uint64_t, std::string>> engineActionNames;

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
