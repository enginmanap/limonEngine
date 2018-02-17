//
// Created by engin on 15.02.2018.
//

#include "FreeMovingPlayer.h"
#include "../Options.h"

void FreeMovingPlayer::move(moveDirections direction) {
    if (direction == NONE) {
        return;
    }

    switch (direction) {
        case UP:
            position +=(up * options->getJumpFactor());
            break;
        case LEFT_BACKWARD:
            position +=(-1.0f * (right + center) * options->getMoveSpeed().x);
            break;
        case LEFT_FORWARD:
            position +=((-1.0f * right + center) * options->getMoveSpeed().x);
            break;
        case LEFT:
            position +=(right * -1.0f * options->getMoveSpeed().x);
            break;
        case RIGHT_BACKWARD:
            position +=((right + -1.0f * center) * options->getMoveSpeed().x);
            break;
        case RIGHT_FORWARD:
            position +=((right + center) * options->getMoveSpeed().x);
            break;
        case RIGHT:
            position +=(right * options->getMoveSpeed().x);
            break;
        case BACKWARD:
            position +=(center * -1.0f * options->getMoveSpeed().x);
            break;
        case FORWARD:
            position +=(center * options->getMoveSpeed().x);
            break;
        case NONE:break;//this is here because -Wall complaints if it is not
    }
}

void FreeMovingPlayer::rotate(float xChange, float yChange) {
    glm::quat viewChange;
    viewChange = glm::quat(cos(yChange * options->getLookAroundSpeed() / 2),
                           right.x * sin(yChange * options->getLookAroundSpeed() / 2),
                           right.y * sin(yChange * options->getLookAroundSpeed() / 2),
                           right.z * sin(yChange * options->getLookAroundSpeed() / 2));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * options->getLookAroundSpeed() / 2),
                           up.x * sin(xChange * options->getLookAroundSpeed() / 2),
                           up.y * sin(xChange * options->getLookAroundSpeed() / 2),
                           up.z * sin(xChange * options->getLookAroundSpeed() / 2));
    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    center.x = view.x;
    if (view.y > 1.0f) {
        center.y = 0.9999f;
    } else if (view.y < -1.0f) {
        center.y = -0.9999f;
    } else {
        center.y = view.y;
    }
    center.z = view.z;
    center = glm::normalize(center);
    right = glm::normalize(glm::cross(center, up));
}

FreeMovingPlayer::FreeMovingPlayer(Options *options):
        options(options),
        dirty(true),
        position(),
        center(glm::vec3(0,0,-1)),
        up(glm::vec3(0,1,0)),
        right(glm::vec3(-1,0,0)),
        view(glm::quat(0,0,0,-1))
{}
