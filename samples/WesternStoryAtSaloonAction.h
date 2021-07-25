//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYATSALOON_H
#define LIMONENGINE_WESTERNSTORYATSALOON_H


#include <API/TriggerInterface.h>

class WesternStoryAtSaloonAction : public TriggerInterface {
    uint32_t textID1,textID2;
    bool hasRun = false;
public:
    WesternStoryAtSaloonAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {}
    void showMessages1(const std::vector<LimonTypes::GenericParameter> &emptyParamList);
    void showMessages2(const std::vector<LimonTypes::GenericParameter> &emptyParamList);
    void showMessages3(const std::vector<LimonTypes::GenericParameter> &emptyParamList);


    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override;

};


#endif //LIMONENGINE_WESTERNSTORYATSALOON_H
