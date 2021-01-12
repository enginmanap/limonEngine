//
// Created by engin on 21.06.2018.
//

#include "MayanCoinPickup.h"
#include <iostream>

MayanCoinPickup::MayanCoinPickup(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

std::vector<LimonAPI::ParameterRequest> MayanCoinPickup::getParameters() {

    /*
     * parameters:
        0) object to remove
        1) text to decrease
        2) Counter start from
        3-4-5) part 1 2 3 to animate
        6) animation itself
     */

    std::vector<LimonAPI::ParameterRequest> parameters;

    LimonAPI::ParameterRequest removeModelParameter;
    removeModelParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    removeModelParameter.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    removeModelParameter.description = "Model to remove";
    parameters.push_back(removeModelParameter);

    LimonAPI::ParameterRequest counterParameter;
    counterParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    counterParameter.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT;
    counterParameter.description = "Counter GUI Element";
    parameters.push_back(counterParameter);

    LimonAPI::ParameterRequest counterStartParameter;
    counterStartParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    counterStartParameter.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_NUMBER;
    counterStartParameter.description = "Count start from";
    parameters.push_back(counterStartParameter);

    LimonAPI::ParameterRequest animateModelParameter1;
    animateModelParameter1.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    animateModelParameter1.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    animateModelParameter1.description = "stairs part 1";
    parameters.push_back(animateModelParameter1);

    LimonAPI::ParameterRequest animateModelParameter2;
    animateModelParameter2.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    animateModelParameter2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    animateModelParameter2.description = "stairs part 2";
    parameters.push_back(animateModelParameter2);

    LimonAPI::ParameterRequest animateModelParameter3;
    animateModelParameter3.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    animateModelParameter3.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    animateModelParameter3.description = "stairs part 3";
    parameters.push_back(animateModelParameter3);

    LimonAPI::ParameterRequest stairAnimation;
    stairAnimation.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::ANIMATION;
    stairAnimation.description = "Animation to apply";
    parameters.push_back(stairAnimation);

    LimonAPI::ParameterRequest textToAdd;
    textToAdd.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    textToAdd.description = "Text to show when coin picked up.(Optional)";
    textToAdd.isSet = true;
    parameters.push_back(textToAdd);

    return parameters;
}

bool MayanCoinPickup::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    bool result;
    //FIXME there is no enum value for variable, there should be
    LimonAPI::ParameterRequest& coinPickupCount = limonAPI->getVariable("coinPickupCount");
    if(coinPickupCount.value.longValue == 0) {
        //means this is the first coin that gets picked up
        coinPickupCount.value.longValue = parameters[2].value.longValue;
    }
    if(coinPickupCount.value.longValue > 1) {
        coinPickupCount.value.longValue--;
        result = limonAPI->updateGuiText(parameters[1].value.longValue, std::to_string(coinPickupCount.value.longValue)) == 0;
    } else {
        //means this is the last Coin to be picked up. Remove the counter
        result = limonAPI->removeGuiElement(parameters[1].value.longValue);
    }

    //remove the coin
    result = limonAPI->removeObject(parameters[0].value.longValue) == 0 && result;

    //Start particle emitter
    if(!limonAPI->enableParticleEmitter(575)){
        std::cerr << "Particle Emitter not found!" << std::endl;
    } else {
        std::vector<LimonAPI::ParameterRequest> empty;
        limonAPI->addTimedEvent(7000, [=](const std::vector<LimonAPI::ParameterRequest>&) { limonAPI->disableParticleEmitter(575);}  , empty);
    }


    //move the stairs
    std::string stairsMoveSound = "./Data/Sounds/rumble.wav";
    limonAPI->animateModel(parameters[3].value.longValue, parameters[6].value.longValue, false, &stairsMoveSound);
    limonAPI->animateModel(parameters[4].value.longValue, parameters[6].value.longValue, false, nullptr);
    limonAPI->animateModel(parameters[5].value.longValue, parameters[6].value.longValue, false, nullptr);

    std::string coinPickupSound = "./Data/Sounds/coinPickup.wav";
    limonAPI->playSound(coinPickupSound, glm::vec3(0, 0, 0), false, false);

    if(parameters.size() == 8) {
        std::string text = parameters[7].value.stringValue;
        if(text != "") {
            addedTextId = limonAPI->addGuiText("./Data/Fonts/Helvetica-Normal.ttf", 32, "Coin pick text",
                                               text, glm::vec3(150, 150, 255), glm::vec2(0.9f, 0.3f), 0.0f);
        }
        result = result && addedTextId != 0;
    }
    return result;
}

std::vector<LimonAPI::ParameterRequest> MayanCoinPickup::getResults() {
    std::vector<LimonAPI::ParameterRequest> result;
    LimonAPI::ParameterRequest resultValue;
    resultValue.isSet = true;
    resultValue.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT;
    resultValue.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    resultValue.value.longValue = addedTextId;
    result.push_back(resultValue);
    return result;
}
