//
// Created by engin on 14.01.2019.
//

#include "InputStates.h"

InputStates::InputStates() {
    /*
     * why not iterate? Because these elements are not added yet, so can't iterate.
     */
    inputEvents[Inputs::QUIT] = false;
    inputEvents[Inputs::MOUSE_MOVE] = false;
    inputEvents[Inputs::MOUSE_BUTTON_LEFT] = false;
    inputEvents[Inputs::MOUSE_BUTTON_MIDDLE] = false;
    inputEvents[Inputs::MOUSE_BUTTON_RIGHT] = false;
    inputEvents[Inputs::MOUSE_WHEEL_UP] = false;
    inputEvents[Inputs::MOUSE_WHEEL_DOWN] = false;
    inputEvents[Inputs::MOVE_FORWARD] = false;
    inputEvents[Inputs::MOVE_BACKWARD] = false;
    inputEvents[Inputs::MOVE_LEFT] = false;
    inputEvents[Inputs::MOVE_RIGHT] = false;
    inputEvents[Inputs::JUMP] = false;
    inputEvents[Inputs::RUN] = false;
    inputEvents[Inputs::DEBUG] = false;
    inputEvents[Inputs::EDITOR] = false;
    inputEvents[Inputs::KEY_SHIFT] = false;
    inputEvents[Inputs::KEY_CTRL] = false;
    inputEvents[Inputs::KEY_ALT] = false;
    inputEvents[Inputs::KEY_SUPER] = false;
    inputEvents[Inputs::TEXT_INPUT] = false;
    inputEvents[Inputs::NUMBER_1] = false;
    inputEvents[Inputs::NUMBER_2] = false;
    inputEvents[Inputs::F4] = false;

    for (const auto &item: inputEvents) {
        inputStatus[item.first] = item.second;
    }
}

void InputStates::resetAllEvents() {
    for (auto eventIt = inputEvents.begin(); eventIt != inputEvents.end(); ++eventIt) {
        eventIt->second = false;
    }
    this->xChange = 0.0f;
    this->yChange = 0.0f;
    this->inputStatus[Inputs::MOUSE_MOVE] = false;
    this->inputStatus[Inputs::TEXT_INPUT] = false;
    this->inputStatus[Inputs::MOUSE_WHEEL_UP] = false;
    this->inputStatus[Inputs::MOUSE_WHEEL_DOWN] = false;
    memset(this->sdlText, 0, sizeof(this->sdlText));
}
