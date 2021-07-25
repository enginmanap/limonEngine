//
// Created by engin on 3.08.2018.
//

#include "QuitGameOnTrigger.h"

TriggerRegister<QuitGameOnTrigger> QuitGameOnTrigger::reg("QuitGameOnTrigger");


std::vector<LimonTypes::GenericParameter> QuitGameOnTrigger::getParameters() {
    return std::vector<LimonTypes::GenericParameter>();
}

bool QuitGameOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    limonAPI->quitGame();
    return true;
}

std::vector<LimonTypes::GenericParameter> QuitGameOnTrigger::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

QuitGameOnTrigger::QuitGameOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

