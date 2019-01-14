//
// Created by engin on 14.01.2019.
//

#ifndef LIMONENGINE_INPUTSTATES_H
#define LIMONENGINE_INPUTSTATES_H


#include <cstdint>
#include <map>
#include <cstring>

class InputStates {
public:
    InputStates();
    static const uint32_t keyBufferElements = 512;
    static const uint32_t keyBufferSize = sizeof(bool) * keyBufferElements;
    enum class Inputs {
        QUIT, MOUSE_MOVE, MOUSE_BUTTON_LEFT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_RIGHT, MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN, MOVE_FORWARD, MOVE_BACKWARD, MOVE_LEFT, MOVE_RIGHT, JUMP, RUN, DEBUG, EDITOR, KEY_SHIFT, KEY_CTRL, KEY_ALT, KEY_SUPER, TEXT_INPUT, NUMBER_1, NUMBER_2
    };
private:
    bool downKeys[keyBufferElements] = {0};
    std::map<Inputs, bool> inputStatus;
    std::map<Inputs, bool> inputEvents;
    float xPos, yPos, xChange, yChange;
    mutable char sdlText[128] = {0};
    mutable char temporaryTextBuffer[128] = {0};


public:

    /**
     * returns if this is an event
     * @param input
     * @param pressed
     * @return
     */
    bool setInputStatus(const Inputs input, bool pressed) {
        if(inputStatus[input] != pressed) {
            inputEvents[input] = true;
        }
        inputStatus[input] = pressed;
        return inputEvents[input];
    }

    bool getInputStatus(const Inputs input) const {
        return inputStatus.at(input);
    }

    bool getInputEvents(const Inputs input) const {
        return inputEvents.at(input);
    }

    const char *getText() const {
        std::strncpy(this->temporaryTextBuffer, sdlText, sizeof(temporaryTextBuffer));
        return temporaryTextBuffer;
    }

    void setText(char* sdlText) {
        size_t currentLength =std::strlen(this->sdlText);
        std::strncpy(this->sdlText+currentLength, sdlText, sizeof(this->sdlText)-currentLength);
    }

    /**
     * switches all event flags to false, for new frame.
     */
    void resetAllEvents();


/**
 * xpos and ypos are actual position values. If relative mouse mode is enabled, mouse position is locked so they always
 * return the same value. In that case use change values.
 * change values return change like 0.001 if relative mouse mode is enabled.
 *
 * @param xChange
 * @param yChange
 * @param xPos
 * @param yPos
 * @return
 */
    bool getMouseChange(float &xPosition, float &yPosition, float &xChange, float &yChange) const {
        if (inputEvents.at(Inputs::MOUSE_MOVE)) {
            xChange = this->xChange;
            yChange = this->yChange;
            xPosition = this->xPos;
            yPosition = this->yPos;
            return true;
        } else {
            return false;
        }
    }

    void setMouseChange(float &xPosition, float &yPosition, float &xChange, float &yChange) {
        this->xPos = xPosition;
        this->yPos = yPosition;
        this->xChange = xChange;
        this->yChange = yChange;
    }

    const bool *getRawKeyStates() const {
        return downKeys;
    }

    void setRawInputState(uint32_t keyState, bool pressed) {
        downKeys[keyState] = pressed;
    }

};


#endif //LIMONENGINE_INPUTSTATES_H
