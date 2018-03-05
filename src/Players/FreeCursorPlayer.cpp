//
// Created by engin on 27.02.2018.
//

#include "FreeCursorPlayer.h"
#include "../Options.h"
#include "../GUI/GUIRenderable.h"

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

void FreeCursorPlayer::rotate(float xChange, float yChange) {
    //FIXME hardcoded 500 is just terrible. Disabling mouse warp should be considered
    cursor->addTranslate(glm::vec2(xChange * 500, yChange * 500 * -1.0f));
    }

void FreeCursorPlayer::getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const {
    fromPosition = this->getPosition();

    // Many thanks to http://antongerdelan.net/opengl/raycasting.html
    glm::vec2 cursorPosition = cursor->getTranslate();
    /* to normalized device coordinates */
    float normalizedDeviceCoordinateX = (2.0f * cursorPosition.x) / options->getScreenWidth() - 1.0f;
    float normalizedDeviceCoordinateY = (2.0f * cursorPosition.y) / options->getScreenHeight() - 1.0f;
    std::cout << "normalized: " << normalizedDeviceCoordinateX << ", " << normalizedDeviceCoordinateY << std::endl;
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

FreeCursorPlayer::FreeCursorPlayer(Options *options, GUIRenderable* cursor):
        options(options),
        dirty(true),
        position(),
        center(glm::vec3(0,0,-1)),
        up(glm::vec3(0,1,0)),
        right(glm::vec3(-1,0,0)),
        view(glm::quat(0,0,0,-1)),
        cursor(cursor)
{

}