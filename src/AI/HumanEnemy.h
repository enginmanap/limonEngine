//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_HUMANENEMY_H
#define LIMONENGINE_HUMANENEMY_H


#include "Actor.h"
#include "../Utils/GLMUtils.h"

class HumanEnemy: public Actor {
    long playerPursuitStartTime = 0L;
    long playerPursuitTimeout = 500000L; //if not see player for this amount, return.
    bool returnToPosition = false;
    glm::vec3 initialPosition;
    glm::vec3 lastWalkDirection;

public:
    HumanEnemy(uint32_t id) : Actor(id) {}

    void play(long time, ActorInformation &information, Options* options __attribute__((unused))) {//FIXME unused attribute is temporary
        //check if the player can be seen
        if(information.canSeePlayerDirectly && information.isPlayerFront) {
            if (playerPursuitStartTime == 0) {
                model->setAnimation("Walk");
                //means we will just start pursuit, mark the position so we can return.
                initialPosition = GLMConverter::BltToGLM(model->getRigidBody()->getCenterOfMassPosition());
                returnToPosition = true;
            }
            playerPursuitStartTime = time;
        }

        if(time - playerPursuitStartTime >= playerPursuitTimeout) {
            playerPursuitStartTime = 0;
        }

        if(playerPursuitStartTime == 0) {
            //if not in player pursuit
            if(returnToPosition) {
                //TODO search for route to initial position and return
            }
        } else {
            //if player pursuit mode
            if (information.canGoToPlayer) {
                //keep the last known direction, if player is at a unknown place.
                //FIXME this is a hack, normally this should not be necessary but sometimes even player is a valid place,
                //actor might not be for current implementation.
                lastWalkDirection = information.toPlayerRoute;
            }
            glm::vec3 moveDirection = 0.1f * lastWalkDirection;
            model->getTransformation()->addTranslate(moveDirection);
            if(information.isPlayerLeft) {
                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateLeft(1.0f, 0.0f, 0.015f, 0.0f);
                    model->getTransformation()->addOrientation(rotateLeft);
                }
            }
            if(information.isPlayerRight) {
                //turn just a little bit to right
                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateRight(1.0f, 0.0f, -0.015f, 0.0f);
                    model->getTransformation()->addOrientation(rotateRight);
                }
            }
            if(information.isPlayerUp) {
                //std::cout << "Up." << std::endl;
            }
            if(information.isPlayerDown) {
                //std::cout << "Down." << std::endl;
            }
        }
    }

    virtual void IMGuiEditorView() {
        ImGui::Text("Human Enemy AI");
        if(playerPursuitStartTime == 0) {
            ImGui::Text("Status: Awaiting Player detection");
        } else {
            ImGui::Text("Status: Player pursuit");
            if(ImGui::Button("Stop pursuit")) {
                playerPursuitStartTime = 0;
                model->setAnimation("Idle");
            }
        }

    }
};


#endif //LIMONENGINE_HUMANENEMY_H
