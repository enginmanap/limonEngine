//
// Created by engin on 6.06.2018.
//

#include "CoinPickUpOnTrigger.h"
#include "UpdateGuiTextOnTrigger.h"

CoinPickUpOnTrigger::CoinPickUpOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

std::vector<LimonAPI::ParameterRequest> CoinPickUpOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT;
    pr.description = "Counter GUI Element";
    parameters.push_back(pr);

    LimonAPI::ParameterRequest pr2;
    pr2.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    pr2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    pr2.description = "Model to remove";
    parameters.push_back(pr2);

    return parameters;}

bool CoinPickUpOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    //FIXME there is no enum value for variable, there should be
    LimonAPI::ParameterRequest& coinPickupCount = limonAPI->getVariable("coinPickupCount");

    coinPickupCount.value.longValue++;
    bool result = limonAPI->updateGuiText(parameters[0].value.longValue, std::to_string(coinPickupCount.value.longValue)) == 0; //set gui text to 1
    result = limonAPI->removeObject(parameters[1].value.longValue) == 0 && result;
    return result;
}

std::vector<LimonAPI::ParameterRequest> CoinPickUpOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();//not feeding other triggers the result
}

void registerAsTrigger(std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* triggerMap) {
    (*triggerMap)["CoinPickUpOnTrigger"] = &createT<CoinPickUpOnTrigger>;
    (*triggerMap)["UpdateGuiTextOnTrigger"] = &createT<UpdateGuiTextOnTrigger>;
}
