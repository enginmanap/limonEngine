//
// Created by engin on 18.01.2019.
//

#include <cstring>
#include "KillCowboyPlayer.h"

std::vector<LimonAPI::ParameterRequest> KillCowboyPlayer::getParameters() {
    return std::vector<LimonAPI::ParameterRequest>();//no parameters
}

bool KillCowboyPlayer::run(std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) {
    std::vector<LimonAPI::ParameterRequest> prList;
    LimonAPI::ParameterRequest pr;
    pr.valueType = pr.STRING;
    std::strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
    prList.push_back(pr);

    LimonAPI::ParameterRequest pr2;
    pr2.valueType = pr.LONG;
    pr2.value.longValue = 1000;//just to make sure
    prList.push_back(pr2);
    limonAPI->interactWithPlayer(prList);
    return true;
}

std::vector<LimonAPI::ParameterRequest> KillCowboyPlayer::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();//no result set
}

std::string KillCowboyPlayer::getName() const {
    return "KillCowboyPlayer";
}
