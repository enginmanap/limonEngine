//
// Created by engin on 6.08.2018.
//

#include "ReturnPreviousWorldOnTrigger.h"

TriggerRegister<ReturnPreviousWorldOnTrigger> ReturnPreviousWorldOnTrigger::reg("ReturnPreviousWorldOnTrigger");


std::vector<LimonAPI::ParameterRequest> ReturnPreviousWorldOnTrigger::getParameters() {
    return std::vector<LimonAPI::ParameterRequest>();
}

bool ReturnPreviousWorldOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters __attribute((unused))) {
    limonAPI->returnPreviousWorld();
    return true;
}

std::vector<LimonAPI::ParameterRequest> ReturnPreviousWorldOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

ReturnPreviousWorldOnTrigger::ReturnPreviousWorldOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}