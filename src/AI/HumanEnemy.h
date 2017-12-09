//
// Created by engin on 27.11.2017.
//

#ifndef UBERGAME_HUMANENEMY_H
#define UBERGAME_HUMANENEMY_H


#include "Actor.h"

class HumanEnemy: public Actor {
    void play(long time, ActorInformation &information, Options* options) {
        //check if the player can be seen
        if(information.canSeePlayerDirectly){
            glm::vec3 moveDirection = 0.05f * information.playerDirection;
            moveDirection.y = 0;//for now
            model->addTranslate( moveDirection);
            //Can see the player
            if(information.isPlayerFront) {
                model->setAnimationIndex(0);
            } else {
                model->setAnimationIndex(-1);
            }
            if(information.isPlayerLeft) {
                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateLeft(1.0f, 0.0f, 0.015f, 0.0f);
                    model->addOrientation(rotateLeft);
                }
            }
            if(information.isPlayerRight) {
                //turn just a little bit to right

                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateRight(1.0f, 0.0f, -0.015f, 0.0f);
                    model->addOrientation(rotateRight);
                }
            }
            if(information.isPlayerUp) {
                std::cout << "Up." << std::endl;
            }
            if(information.isPlayerDown) {
                std::cout << "Down." << std::endl;
            }
        } else {
            //Can't see the player
            model->setAnimationIndex(-1);
        }
    }

public:

};


#endif //UBERGAME_HUMANENEMY_H
