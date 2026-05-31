//
// Created by engin on 21.06.2018.
//

#include "MayanLever.h"



MayanLever::MayanLever(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {

    /*
     * parameters:
        0) object to move as lever
        1) object movement animation
        2-3-4) parts to animate
        5) animation to move the door
     */

    LimonTypes::GenericParameter leverModelParameter;
    leverModelParameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    leverModelParameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    leverModelParameter.description = "Model to move as lever";
    this->parameters.push_back(leverModelParameter);

    LimonTypes::GenericParameter leverAnimation;
    leverAnimation.requestType = LimonTypes::GenericParameter::RequestParameterTypes::ANIMATION;
    leverAnimation.description = "Lever activation animation";
    this->parameters.push_back(leverAnimation);

    LimonTypes::GenericParameter animateModelParameter1;
    animateModelParameter1.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    animateModelParameter1.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    animateModelParameter1.description = "Door part 1";
    this->parameters.push_back(animateModelParameter1);

    LimonTypes::GenericParameter animateModelParameter2;
    animateModelParameter2.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    animateModelParameter2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    animateModelParameter2.description = "Door part 2";
    this->parameters.push_back(animateModelParameter2);

    LimonTypes::GenericParameter animateModelParameter3;
    animateModelParameter3.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    animateModelParameter3.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    animateModelParameter3.description = "Door part 3";
    this->parameters.push_back(animateModelParameter3);

    LimonTypes::GenericParameter stairAnimation;
    leverAnimation.requestType = LimonTypes::GenericParameter::RequestParameterTypes::ANIMATION;
    leverAnimation.description = "Door opening animation";
    this->parameters.push_back(leverAnimation);
}

bool MayanLever::run(std::vector<LimonTypes::GenericParameter> parameters) {
    bool result;

    //animate the lever
    result = limonAPI->animateModel(parameters[0].value.longValue, parameters[1].value.longValue, false) == 0;

    std::string coinPickupSound = "./Data/Sounds/castleDoor.wav";
    //move the door
    result = limonAPI->animateModel(parameters[2].value.longValue, parameters[5].value.longValue, false, coinPickupSound) == 0 && result;
    result = limonAPI->animateModel(parameters[3].value.longValue, parameters[5].value.longValue, false) == 0 && result;
    result = limonAPI->animateModel(parameters[4].value.longValue, parameters[5].value.longValue, false) == 0 && result;

    return result;
}

std::vector<LimonTypes::GenericParameter> MayanLever::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}
