//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_TRIGGERINTERFACE_H
#define LIMONENGINE_TRIGGERINTERFACE_H

#include "LimonAPI.h"


class TriggerInterface {
    static std::map<std::string, TriggerInterface*(*)()>* typeMap;
protected:
    static std::map<std::string, TriggerInterface*(*)()> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, TriggerInterface*(*)()>();
        }
        return typeMap;
    }

public:
    virtual std::vector<LimonAPI::ParameterRequest> getParameters() = 0;
    virtual bool run(std::vector<LimonAPI::ParameterRequest> parameters) = 0;

    virtual ~TriggerInterface() = default;

    static TriggerInterface * createTrigger(std::string const& s) {
        std::map<std::string, TriggerInterface*(*)()>::iterator it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second();
    }

};

template<typename T>
TriggerInterface* createT() {
    return new T;
}

template<typename T>
class TriggerRegister : TriggerInterface {
    virtual std::vector<LimonAPI::ParameterRequest> getParameters() {
        return std::vector<LimonAPI::ParameterRequest>();
    };
    virtual bool run(std::vector<LimonAPI::ParameterRequest> parameters __attribute((unused))) {
        return false;
    };
public:
    TriggerRegister(std::string const& s) {
        getMap()->insert(std::make_pair(s, &createT<T>));
    }
};

#endif //LIMONENGINE_TRIGGERINTERFACE_H
