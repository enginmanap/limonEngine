//
// Created by engin on 18.01.2019.
//

#include <cstring>
#include "KillCowboyPlayer.h"

std::vector<LimonTypes::GenericParameter> KillCowboyPlayer::getParameters() {
    return std::vector<LimonTypes::GenericParameter>();//no parameters
}

bool KillCowboyPlayer::run(std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    std::vector<LimonTypes::GenericParameter> prList;
    LimonTypes::GenericParameter pr;
    pr.valueType = pr.STRING;
    std::strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
    prList.push_back(pr);

    LimonTypes::GenericParameter pr2;
    pr2.valueType = pr.LONG;
    pr2.value.longValue = 1000;//just to make sure
    prList.push_back(pr2);
    limonAPI->interactWithPlayer(prList);
    return true;
}

std::vector<LimonTypes::GenericParameter> KillCowboyPlayer::getResults() {
    return std::vector<LimonTypes::GenericParameter>();//no result set
}

std::string KillCowboyPlayer::getName() const {
    return "KillCowboyPlayer";
}
