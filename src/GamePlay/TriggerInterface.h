//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_TRIGGERINTERFACE_H
#define LIMONENGINE_TRIGGERINTERFACE_H

#include "LimonAPI.h"

class TriggerInterface {
public:
    virtual std::vector<LimonAPI::ParameterRequest> getParameters() = 0;
    virtual bool run(std::vector<LimonAPI::ParameterRequest> parameters) = 0;

    virtual ~TriggerInterface() = default;
};

#endif //LIMONENGINE_TRIGGERINTERFACE_H
