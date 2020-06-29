//
// Created by engin on 27.02.2018.
//

#include "FreeCursorPlayer.h"
#include "../../Options.h"
#include "../../GUI/GUIRenderable.h"

void FreeCursorPlayer::move(moveDirections direction) {
    if (direction == NONE) {
        return;
    }

    switch (direction) {
        case UP:
            position +=(up * options->getJumpFactor() / 100.0f);
            break;
        case LEFT_BACKWARD:
            position +=(-1.0f * (right + center) * options->getMoveSpeed().x / 100.0f);
            break;
        case LEFT_FORWARD:
            position +=((-1.0f * right + center) * options->getMoveSpeed().x / 100.0f);
            break;
        case LEFT:
            position +=(right * -1.0f * options->getMoveSpeed().x / 100.0f);
            break;
        case RIGHT_BACKWARD:
            position +=((right + -1.0f * center) * options->getMoveSpeed().x / 100.0f);
            break;
        case RIGHT_FORWARD:
            position +=((right + center) * options->getMoveSpeed().x / 100.0f);
            break;
        case RIGHT:
            position +=(right * options->getMoveSpeed().x / 100.0f);
            break;
        case BACKWARD:
            position +=(center * -1.0f * options->getMoveSpeed().x / 100.0f);
            break;
        case FORWARD:
            position +=(center * options->getMoveSpeed().x / 100.0f);
            break;
        case NONE:break;//this is here because -Wall complaints if it is not
    }
}

void FreeCursorPlayer::rotate(float xPosition, float yPosition, float xChange [[gnu::unused]], float yChange [[gnu::unused]]) {
    glm::vec2 cursorPosition((options->getScreenWidth()/2.0f)  + xPosition * options->getScreenWidth() /2.0f,
                             (options->getScreenHeight()/2.0f) - yPosition * options->getScreenHeight()/2.0f);//y is negative, because sdl reports opposite of OpenGL

    // FIXME this look around code is repeated in each player. I believe it should have been part of player class.
    // It can't be used directly because that would eliminate possibilities like 3rd person cameras.

    float lookAroundSpeed = options->getLookAroundSpeed();
    //scale look around speed with the abs(center.y). for 1 -> look around 0, for 0 -> lookaround 1.
    lookAroundSpeed = lookAroundSpeed * (1- (center.y * center.y));

    //if cursor is in the edge, rotate player look at
    if(cursorPosition.x == 0 || cursorPosition.x == options->getScreenWidth() - 1) {
        float xSpeed = 0.02;
        if(cursorPosition.x == 0) {
            xSpeed = -0.02f;
        }
        glm::quat viewChange;
        viewChange = glm::quat(cos(0.02f * lookAroundSpeed / 2),
                               up.x * sin(xSpeed * lookAroundSpeed / 2),
                               up.y * sin(xSpeed * lookAroundSpeed / 2),
                               up.z * sin(xSpeed * lookAroundSpeed / 2));
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

    if(cursorPosition.y == 1 || cursorPosition.y == options->getScreenHeight() ) {//since y was negative, the 1 changes places
        float ySpeed = -0.02f;
        if(cursorPosition.y == 1) {
            ySpeed = 0.02f;
        }
        glm::quat viewChange;
        viewChange = glm::quat(cos(0.02f * lookAroundSpeed / 2),
                               right.x * sin(ySpeed * lookAroundSpeed / 2),
                               right.y * sin(ySpeed * lookAroundSpeed / 2),
                               right.z * sin(ySpeed * lookAroundSpeed / 2));

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

    cursor->setTranslate(cursorPosition);
    }

void FreeCursorPlayer::getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const {
    fromPosition = this->getPosition();

    // Many thanks to http://antongerdelan.net/opengl/raycasting.html
    glm::vec2 cursorPosition = cursor->getTranslate();
    /* to normalized device coordinates */
    float normalizedDeviceCoordinateX = (2.0f * cursorPosition.x) / options->getScreenWidth() - 1.0f;
    float normalizedDeviceCoordinateY = (2.0f * cursorPosition.y) / options->getScreenHeight() - 1.0f;
    /* homogeneous clip coordinates */
    glm::vec4 clipSpaceRay = glm::vec4(normalizedDeviceCoordinateX, normalizedDeviceCoordinateY, -1.0, 1.0);
    /* eye coordinates */
    float aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
    glm::mat4 perspectiveProjectionMatrix = glm::perspective(options->PI/3.0f, 1.0f / aspect, 0.1f, 1000.0f);
    glm::vec4 cameraSpaceRay = glm::inverse(perspectiveProjectionMatrix) * clipSpaceRay;
    cameraSpaceRay = glm::vec4(cameraSpaceRay.x, cameraSpaceRay.y, -1.0, 0.0);

    /* world coordinate */
    glm::mat4 cameraTransformMatrix = glm::lookAt(position, center + position, up);
    glm::vec4 worldSpaceRay = glm::inverse(cameraTransformMatrix) * cameraSpaceRay;
    // switch to vec3 and normalize
    toPosition = glm::normalize(glm::vec3(worldSpaceRay.x, worldSpaceRay.y, worldSpaceRay.z));
}

FreeCursorPlayer::FreeCursorPlayer(Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                                   const glm::vec3 &lookDirection) :
        Player(cursor, options, position, lookDirection),
        dirty(true),
        position(position),
        center(lookDirection),
        up(glm::vec3(0,1,0)),
        view(glm::quat(0,0,0,-1)) {
    right = glm::normalize(glm::cross(center, up));
    worldSettings.debugMode = DEBUG_NOCHANGE;
    worldSettings.audioPlaying = false;
    worldSettings.worldSimulation = false;
    worldSettings.editorShown = true;
    worldSettings.cursorFree = true;
    worldSettings.resetAnimations = true;
    worldSettings.menuInteraction = false;
}