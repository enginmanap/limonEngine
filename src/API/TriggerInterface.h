//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_TRIGGERINTERFACE_H
#define LIMONENGINE_TRIGGERINTERFACE_H

#include "LimonAPI.h"

/**
 * On shared library load, void registerAsTrigger(std::map<std::string, TriggerInterface*(*)(LimonAPI*)>*) function should be callable.
 *
 * This method will be called after object load.
 * For each trigger, the name of trigger and a constructor should be put in the map.
 */


class TriggerInterface {
    static std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* typeMap;
protected:
    LimonAPI* limonAPI = nullptr;
    static std::map<std::string, TriggerInterface*(*)(LimonAPI*)> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, TriggerInterface*(*)(LimonAPI*)>();
        }
        return typeMap;
    }

    TriggerInterface(LimonAPI* limonAPI): limonAPI(limonAPI) {}

public:
    // Not virtual
    static std::vector<std::string> getTriggerNames() {
        std::vector<std::string> names;
        for (auto it = typeMap->begin(); it != typeMap->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    static void registerType(const std::string& typeName, TriggerInterface*(*constructor)(LimonAPI*)) {
        (*typeMap)[typeName] = constructor;
    }

    virtual std::vector<LimonAPI::ParameterRequest> getParameters() = 0;
    virtual bool run(std::vector<LimonAPI::ParameterRequest> parameters) = 0;
    virtual std::vector<LimonAPI::ParameterRequest> getResults() = 0;

    virtual ~TriggerInterface() = default;

    virtual std::string getName() const = 0;


    static TriggerInterface * createTrigger(std::string const& s, LimonAPI* apiInstance) {
        std::map<std::string, TriggerInterface*(*)(LimonAPI*)>::iterator it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second(apiInstance);
    }

};

template<typename T>
TriggerInterface* createT(LimonAPI* limonAPI) {
    return new T(limonAPI);
}

template<typename T>
class TriggerRegister : TriggerInterface {
    virtual std::vector<LimonAPI::ParameterRequest> getParameters() {
        return std::vector<LimonAPI::ParameterRequest>();
    };
    virtual bool run(std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) override {
        return false;
    };
    std::string getName() const override {
        return "This object is not meant to be used";
    }

    std::vector<LimonAPI::ParameterRequest> getResults() override {
        return std::vector<LimonAPI::ParameterRequest>();
    }

public:
    TriggerRegister(std::string const& s) : TriggerInterface(nullptr) {
        getMap()->insert(std::make_pair(s, &createT<T>));
    }

};

#endif //LIMONENGINE_TRIGGERINTERFACE_H
