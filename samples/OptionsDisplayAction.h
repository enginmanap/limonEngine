//
// Created by engin on 9.06.2026.
//

#ifndef LIMONENGINE_OPTIONSDISPLAYACTION_H
#define LIMONENGINE_OPTIONSDISPLAYACTION_H

#include <vector>
#include <cstdint>
#include "limonAPI/TriggerInterface.h"

class OptionsDisplayAction : public TriggerInterface {
    std::vector<uint32_t> guiElementIDs;
public:
    explicit OptionsDisplayAction(LimonAPI *limonAPI);

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "OptionsDisplayAction";
    }
};

#endif //LIMONENGINE_OPTIONSDISPLAYACTION_H
