//
// Created by engin on 9.06.2026.
//

#include "OptionsDisplayAction.h"

OptionsDisplayAction::OptionsDisplayAction(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {
    LimonTypes::GenericParameter fontPathParam;
    fontPathParam.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    fontPathParam.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    fontPathParam.description = "Font file path";
    strncpy(fontPathParam.value.stringValue, "./Data/Fonts/Helvetica-Normal.ttf", sizeof(fontPathParam.value.stringValue) - 1);
    fontPathParam.isSet = true;
    this->parameters.push_back(fontPathParam);
}

bool OptionsDisplayAction::run(std::vector<LimonTypes::GenericParameter> parameters) {
    if (!limonAPI->getOptions()) {
        return false;
    }

    std::string fontPath = "./Data/Fonts/Helvetica-Normal.ttf";
    if (!parameters.empty() && parameters[0].isSet) {
        fontPath = parameters[0].value.stringValue;
    }

    const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> allOptions = limonAPI->getOptions()->getAllOptions();

    const uint32_t fontSize = 24;
    const float startY = 0.05f;
    const float rowHeight = 0.03f;
    const float nameX = 0.05f;
    const float valueX = 0.55f;
    const glm::vec3 nameColor(220, 220, 100);
    const glm::vec3 valueColor(200, 200, 200);

    uint32_t row = 0;
    for (auto &optionEntry : allOptions) {
        float yPos = startY + row * rowHeight;

        uint32_t nameID = limonAPI->addGuiText(
            fontPath, fontSize,
            "optName_" + optionEntry.first,
            optionEntry.first,
            nameColor,
            glm::vec2(nameX, yPos),
            0.0f);
        guiElementIDs.push_back(nameID);

        uint32_t valueID = limonAPI->addGuiText(
            fontPath, fontSize,
            "optValue_" + optionEntry.first,
            optionEntry.second->to_string(),
            valueColor,
            glm::vec2(valueX, yPos),
            0.0f);
        guiElementIDs.push_back(valueID);

        ++row;
    }

    return true;
}

std::vector<LimonTypes::GenericParameter> OptionsDisplayAction::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}
