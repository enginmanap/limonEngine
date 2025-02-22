//
// Created by engin on 15.02.2018.
//

#include "FreeMovingPlayer.h"
#include "API/Options.h"

void FreeMovingPlayer::move(moveDirections direction) {
    if (direction == NONE) {
        return;
    }
    dirty = true;

    OptionsUtil::Options::Option<LimonTypes::Vec4>movementSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("freeMovementSpeed"));
    LimonTypes::Vec4 movementSpeed = movementSpeedOption.get();
    float jumpFactor = jumpFactorOption.get();

    switch (direction) {
        case UP:
            position +=(up * jumpFactor / 100.0f);
            break;
        case LEFT_BACKWARD:
            position +=(-1.0f * (right + center) * movementSpeed.x);
            break;
        case LEFT_FORWARD:
            position +=((-1.0f * right + center) * movementSpeed.x);
            break;
        case LEFT:
            position +=(right * -1.0f * movementSpeed.x);
            break;
        case RIGHT_BACKWARD:
            position +=((right + -1.0f * center) * movementSpeed.x);
            break;
        case RIGHT_FORWARD:
            position +=((right + center) * movementSpeed.x);
            break;
        case RIGHT:
            position +=(right * movementSpeed.x);
            break;
        case BACKWARD:
            position +=(center * -1.0f * movementSpeed.x);
            break;
        case FORWARD:
            position +=(center * movementSpeed.x);
            break;
        case NONE:break;//this is here because -Wall complaints if it is not
    }
}

void FreeMovingPlayer::rotate(float xPosition [[gnu::unused]], float yPosition [[gnu::unused]], float xChange, float yChange) {
    dirty = true;
    glm::quat viewChange;
    float lookAroundSpeed = lookAroundSpeedOption.get();
    float lookAroundSpeedX = lookAroundSpeed;
    //scale look around speed with the abs(center.y). for 1 -> look around 0, for 0 -> lookaround 1.
    float lookAroundSpeedY = lookAroundSpeedX * (1- (center.y * center.y));
    viewChange = glm::quat(cos(yChange * lookAroundSpeedY / 2),
                           right.x * sin(yChange * lookAroundSpeedY / 2),
                           right.y * sin(yChange * lookAroundSpeedY / 2),
                           right.z * sin(yChange * lookAroundSpeedY / 2));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * lookAroundSpeedX / 2),
                           up.x * sin(xChange * lookAroundSpeedX / 2),
                           up.y * sin(xChange * lookAroundSpeedX / 2),
                           up.z * sin(xChange * lookAroundSpeedX / 2));
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

FreeMovingPlayer::FreeMovingPlayer(OptionsUtil::Options *options, GUIRenderable* cursor, const glm::vec3 &position,
                                   const glm::vec3 &lookDirection):
        Player(cursor, options, position, lookDirection),
        dirty(true),
        position(position),
        center(lookDirection),
        up(glm::vec3(0,1,0)),
        view(glm::quat(0,0,0,-1)) {
    right = glm::normalize(glm::cross(center, up));
    worldSettings.debugMode = DEBUG_ENABLED;
    worldSettings.audioPlaying = true;
    worldSettings.worldSimulation = true;
    worldSettings.editorShown = false;
    worldSettings.cursorFree = false;
    worldSettings.resetAnimations = false;
    worldSettings.menuInteraction = false;

}
