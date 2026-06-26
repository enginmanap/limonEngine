//
// Created by engin on 14.01.2019.
//

#ifndef LIMONENGINE_INPUTSTATES_H
#define LIMONENGINE_INPUTSTATES_H


#include <cstdint>
#include <cstring>
#include <unordered_map>
#include "util/HashUtil.h"

namespace InputActions {
    constexpr uint64_t QUIT               = HASH("QUIT");
    constexpr uint64_t MOUSE_MOVE         = HASH("MOUSE_MOVE");
    constexpr uint64_t MOUSE_BUTTON_LEFT  = HASH("MOUSE_BUTTON_LEFT");
    constexpr uint64_t MOUSE_BUTTON_MIDDLE= HASH("MOUSE_BUTTON_MIDDLE");
    constexpr uint64_t MOUSE_BUTTON_RIGHT = HASH("MOUSE_BUTTON_RIGHT");
    constexpr uint64_t MOUSE_WHEEL_UP     = HASH("MOUSE_WHEEL_UP");
    constexpr uint64_t MOUSE_WHEEL_DOWN   = HASH("MOUSE_WHEEL_DOWN");
    constexpr uint64_t MOVE_FORWARD       = HASH("MOVE_FORWARD");
    constexpr uint64_t MOVE_BACKWARD      = HASH("MOVE_BACKWARD");
    constexpr uint64_t MOVE_LEFT          = HASH("MOVE_LEFT");
    constexpr uint64_t MOVE_RIGHT         = HASH("MOVE_RIGHT");
    constexpr uint64_t JUMP               = HASH("JUMP");
    constexpr uint64_t RUN                = HASH("RUN");
    constexpr uint64_t DEBUG_MODE         = HASH("DEBUG_MODE");
    constexpr uint64_t EDITOR             = HASH("EDITOR");
    constexpr uint64_t KEY_SHIFT          = HASH("KEY_SHIFT");
    constexpr uint64_t KEY_CTRL           = HASH("KEY_CTRL");
    constexpr uint64_t KEY_ALT            = HASH("KEY_ALT");
    constexpr uint64_t KEY_SUPER          = HASH("KEY_SUPER");
    constexpr uint64_t TEXT_INPUT         = HASH("TEXT_INPUT");
    constexpr uint64_t NUMBER_1           = HASH("NUMBER_1");
    constexpr uint64_t NUMBER_2           = HASH("NUMBER_2");
    constexpr uint64_t F4                 = HASH("F4");
    constexpr uint64_t F5                 = HASH("F5");
    // Analog camera actions — populated by mouse and/or gamepad right stick
    constexpr uint64_t LOOK_X             = HASH("LOOK_X");
    constexpr uint64_t LOOK_Y             = HASH("LOOK_Y");
}

class InputStates {
public:
    InputStates() = default;

private:
    std::unordered_map<uint64_t, bool>  inputStatus;
    std::unordered_map<uint64_t, bool>  inputEvents;
    std::unordered_map<uint64_t, float> analogValues;
    float xPos = 0.0f, yPos = 0.0f, xChange = 0.0f, yChange = 0.0f;
    mutable char sdlText[128] = {0};
    mutable char temporaryTextBuffer[128] = {0};
    bool simulated = false;

public:

    /**
     * Sets a digital action state. Returns true if this is a new event (state changed).
     */
    bool setInputStatus(uint64_t action, bool pressed) {
        bool previous = inputStatus[action];
        if (previous != pressed) {
            inputEvents[action] = true;
        }
        inputStatus[action] = pressed;
        return inputEvents[action];
    }

    bool getInputStatus(uint64_t action) const {
        auto it = inputStatus.find(action);
        return it != inputStatus.end() ? it->second : false;
    }

    bool getInputEvents(uint64_t action) const {
        auto it = inputEvents.find(action);
        return it != inputEvents.end() ? it->second : false;
    }

    void setAnalogValue(uint64_t action, float value) {
        analogValues[action] = value;
    }

    void addAnalogValue(uint64_t action, float value) {
        analogValues[action] += value;
    }

    float getAnalogValue(uint64_t action) const {
        auto it = analogValues.find(action);
        return it != analogValues.end() ? it->second : 0.0f;
    }

    const char *getText() const {
        std::strncpy(this->temporaryTextBuffer, sdlText, sizeof(temporaryTextBuffer));
        return temporaryTextBuffer;
    }

    void setText(char* text) {
        size_t currentLength = std::strlen(this->sdlText);
        std::strncpy(this->sdlText + currentLength, text, sizeof(this->sdlText) - currentLength);
    }

    /**
     * Resets all event flags and per-frame analog/mouse values for a new frame.
     */
    void resetAllEvents();

    /**
     * Mouse position and delta. If relative mouse mode is enabled, position is locked;
     * use change values for camera look. For analog camera input prefer getAnalogValue(InputActions::LOOK_X/Y).
     */
    bool getMouseChange(float &xPosition, float &yPosition, float &xChangeOut, float &yChangeOut) const {
        auto it = inputEvents.find(InputActions::MOUSE_MOVE);
        if (it != inputEvents.end() && it->second) {
            xChangeOut = this->xChange;
            yChangeOut = this->yChange;
            xPosition  = this->xPos;
            yPosition  = this->yPos;
            return true;
        } else {
            return false;
        }
    }

    void setMouseChange(const float &xPosition, const float &yPosition, const float &xChangeIn, const float &yChangeIn) {
        this->xPos     = xPosition;
        this->yPos     = yPosition;
        this->xChange  = xChangeIn;
        this->yChange  = yChangeIn;
    }

    bool isSimulated() const {
        return simulated;
    }

    void setSimulated(bool isSimulated) {
        InputStates::simulated = isSimulated;
    }
};


#endif //LIMONENGINE_INPUTSTATES_H
