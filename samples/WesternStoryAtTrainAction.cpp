//
// Created by engin on 25.01.2019.
//

#include <iostream>
#include "WesternStoryAtTrainAction.h"

bool WesternStoryAtTrainAction::run(std::vector<LimonTypes::GenericParameter> parameters) {
    if(parameters.size() != 3) {
        std::cerr << this->getName() << " didn't get the parameters, it won't run. " << std::endl;
        return false;
    }

    std::vector<LimonTypes::GenericParameter>triggerResult = limonAPI->getResultOfTrigger(
            static_cast<uint32_t>(parameters[0].value.longValues[1]),
            static_cast<uint32_t>(parameters[0].value.longValues[2]));

    bool shouldBeRun = parameters[1].value.boolValue;
    if(triggerResult.size() == 0) {
        std::cerr << "trigger result was empty, it should have contained a bool" << std::endl;
        return false;
    }
    bool isRun = triggerResult[0].value.boolValue;
    if(shouldBeRun != isRun) {
        //skip because the requirement didn't fulfilled.
        return false;
    }

    limonAPI->loadAndSwitchWorld(parameters[2].value.stringValue);

    return true;
}

std::vector<LimonTypes::GenericParameter> WesternStoryAtTrainAction::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

std::string WesternStoryAtTrainAction::getName() const {
    return "WesternStoryAtTrain";
}
