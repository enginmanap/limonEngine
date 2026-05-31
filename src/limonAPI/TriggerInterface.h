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
    std::vector<LimonTypes::GenericParameter> parameters;
    static std::map<std::string, TriggerInterface*(*)(LimonAPI*)> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, TriggerInterface*(*)(LimonAPI*)>();
        }
        return typeMap;
    }
    explicit TriggerInterface(LimonAPI* limonAPI): limonAPI(limonAPI) {}

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

    /**
     * Returns the parameters held on this trigger instance. Right after construction these are the
     * defaults the trigger seeded in its constructor; after an editor edit or a load they are the
     * configured values. This single member is the source of truth for editing, serialization and
     * run(). Override only if a trigger builds its parameter list from typed members (Actor-style).
     */
    virtual std::vector<LimonTypes::GenericParameter> getParameters() const {
        return this->parameters;
    }

    /**
     * Stores the configured parameter values on the trigger instance. Override only if a trigger
     * needs to react to value changes.
     */
    virtual void setParameters(std::vector<LimonTypes::GenericParameter> parameters) {
        this->parameters = parameters;
    }

    virtual bool run(std::vector<LimonTypes::GenericParameter> parameters) = 0;
    virtual std::vector<LimonTypes::GenericParameter> getResults() = 0;

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
    virtual bool run(std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) override {
        return false;
    };
    std::string getName() const override {
        return "This object is not meant to be used";
    }

    std::vector<LimonTypes::GenericParameter> getResults() override {
        return std::vector<LimonTypes::GenericParameter>();
    }

public:
    TriggerRegister(std::string const& s) : TriggerInterface(nullptr) {
        getMap()->insert(std::make_pair(s, &createT<T>));
    }

};

#endif //LIMONENGINE_TRIGGERINTERFACE_H
