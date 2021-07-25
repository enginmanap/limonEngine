//
// Created by engin on 6.08.2018.
//

#include "ReturnPreviousWorldOnTrigger.h"

TriggerRegister<ReturnPreviousWorldOnTrigger> ReturnPreviousWorldOnTrigger::reg("ReturnPreviousWorldOnTrigger");


std::vector<LimonTypes::GenericParameter> ReturnPreviousWorldOnTrigger::getParameters() {
    return std::vector<LimonTypes::GenericParameter>();
}

bool ReturnPreviousWorldOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    limonAPI->returnPreviousWorld();
    return true;
}

std::vector<LimonTypes::GenericParameter> ReturnPreviousWorldOnTrigger::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

ReturnPreviousWorldOnTrigger::ReturnPreviousWorldOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}