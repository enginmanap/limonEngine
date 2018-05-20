//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "TriggerInterface.h"

class AnimationCustom;
class Model;

class AnimateOnTrigger : public TriggerInterface {
    const AnimationCustom* animation = nullptr;
    Model* model;
    bool loop;
public:
    bool addEditorElements() override;

    bool run() override;

};


#endif //LIMONENGINE_ANIMATONTRIGGER_H
