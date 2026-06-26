//
// Created by engin on 14.01.2019.
//

#include "InputStates.h"
#include <cstring>

void InputStates::resetAllEvents() {
    for (auto &eventIt : inputEvents) {
        eventIt.second = false;
    }
    for (auto &analogIt : analogValues) {
        analogIt.second = 0.0f;
    }
    this->xChange = 0.0f;
    this->yChange = 0.0f;
    inputStatus[InputActions::MOUSE_MOVE]      = false;
    inputStatus[InputActions::TEXT_INPUT]      = false;
    inputStatus[InputActions::MOUSE_WHEEL_UP]  = false;
    inputStatus[InputActions::MOUSE_WHEEL_DOWN]= false;
    memset(this->sdlText, 0, sizeof(this->sdlText));
}
