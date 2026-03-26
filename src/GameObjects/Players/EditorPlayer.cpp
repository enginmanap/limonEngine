//
// Created by engin on 27.02.2018.
//

#include "EditorPlayer.h"
#include "../../InputHandler.h"
#include "../../GUI/GUIRenderable.h"

EditorPlayer::EditorPlayer(OptionsUtil::Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                           const glm::vec3 &lookDirection, InputHandler* inputHandler) :
        FreeCursorPlayer(options, cursor, position, lookDirection),
        inputHandler(inputHandler) {
    worldSettings.debugMode = DEBUG_NOCHANGE;
    worldSettings.audioPlaying = false;
    worldSettings.worldSimulation = false;
    worldSettings.editorShown = true;
    worldSettings.cursorFree = true;
    worldSettings.resetAnimations = true;
    worldSettings.menuInteraction = false;
}

void EditorPlayer::rotateFree(float xChange, float yChange) {
    dirty = true;
    glm::quat viewChange;
    float lookAroundSpeed = (float)lookAroundSpeedOption.get();
    float lookAroundSpeedX = lookAroundSpeed;
    //scale look around speed with the abs(center.y). for 1 -> look around 0, for 0 -> lookaround 1.
    float lookAroundSpeedY = lookAroundSpeedX * (1.0f - (center.y * center.y));
    viewChange = glm::quat(cos(yChange * lookAroundSpeedY / 2.0f),
                           right.x * sin(yChange * lookAroundSpeedY / 2.0f),
                           right.y * sin(yChange * lookAroundSpeedY / 2.0f),
                           right.z * sin(yChange * lookAroundSpeedY / 2.0f));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * lookAroundSpeedX / 2.0f),
                           up.x * sin(xChange * lookAroundSpeedX / 2.0f),
                           up.y * sin(xChange * lookAroundSpeedX / 2.0f),
                           up.z * sin(xChange * lookAroundSpeedX / 2.0f));
    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    center.x = view.x;
    if (view.y > 0.99f) {
        center.y = 0.99f;
    } else if (view.y < -0.99f) {
        center.y = -0.99f;
    } else {
        center.y = view.y;
    }
    center.z = view.z;
    center = glm::normalize(center);
    right = glm::normalize(glm::cross(center, up));
}

void EditorPlayer::processInput(const InputStates &inputState, long time [[gnu::unused]]) {
    float xPosition, yPosition, xChange, yChange;
    bool hasMouseChange = inputState.getMouseChange(xPosition, yPosition, xChange, yChange);

    // Handle right-click for free look
    if (inputState.getInputEvents(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
        if (inputState.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
            inputHandler->setMouseModeRelative();
        } else {
            inputHandler->setMouseModeFree();
        }
    }

    if (inputState.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
        if (hasMouseChange) {
            rotateFree(xChange, yChange);
        }
    } else {
        if (hasMouseChange) {
            rotate(xPosition, yPosition, xChange, yChange);
        }
    }

    if (inputState.getInputEvents(InputStates::Inputs::RUN)) {
        LimonTypes::Vec4 movementSpeed;
        if(inputState.getInputStatus(InputStates::Inputs::RUN)) {
            movementSpeed = runSpeedOption.get();
            moveSpeedOption.set(movementSpeed);
        } else {
            movementSpeed = walkSpeedOption.get();
            moveSpeedOption.set(movementSpeed);
        }
    }

    EditorPlayer::moveDirections direction = EditorPlayer::NONE;
    //ignore if both are pressed.
    if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD) !=
        inputState.getInputStatus(InputStates::Inputs::MOVE_BACKWARD)) {
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD)) {
            direction = EditorPlayer::FORWARD;
        } else {
            direction = EditorPlayer::BACKWARD;
        }
    }
    if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT) != inputState.getInputStatus(InputStates::Inputs::MOVE_RIGHT)) {
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT)) {
            if (direction == EditorPlayer::FORWARD) {
                direction = EditorPlayer::LEFT_FORWARD;
            } else if (direction == EditorPlayer::BACKWARD) {
                direction = EditorPlayer::LEFT_BACKWARD;
            } else {
                direction = EditorPlayer::LEFT;
            }
        } else if (direction == EditorPlayer::FORWARD) {
            direction = EditorPlayer::RIGHT_FORWARD;
        } else if (direction == EditorPlayer::BACKWARD) {
            direction = EditorPlayer::RIGHT_BACKWARD;
        } else {
            direction = EditorPlayer::RIGHT;
        }
    }

    if (inputState.getInputStatus(InputStates::Inputs::JUMP) && inputState.getInputEvents(InputStates::Inputs::JUMP)) {
        direction = EditorPlayer::UP;
    }

    move(direction);
}
