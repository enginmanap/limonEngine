//
// Created by engin on 3.08.2018.
//

#include "QuitGameOnTrigger.h"

TriggerRegister<QuitGameOnTrigger> QuitGameOnTrigger::reg("QuitGameOnTrigger");


std::vector<LimonAPI::ParameterRequest> QuitGameOnTrigger::getParameters() {
    return std::vector<LimonAPI::ParameterRequest>();
}

bool QuitGameOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) {
    limonAPI->quitGame();
    return true;
}

std::vector<LimonAPI::ParameterRequest> QuitGameOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

QuitGameOnTrigger::QuitGameOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

