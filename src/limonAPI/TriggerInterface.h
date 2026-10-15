//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_TRIGGERINTERFACE_H
#define LIMONENGINE_TRIGGERINTERFACE_H

#include "LimonAPI.h"

/**
 * On shared library load, void registerTriggerTypes(std::map<std::string, TriggerInterface::Registration>*) function should be callable.
 *
 * This method will be called after object load.
 * For each trigger, put its name and TriggerInterface::registration<YourTrigger>() in the map.
 */


class TriggerInterface {
public:
    struct Registration {
        TriggerInterface*(*factory)(LimonAPI*) = nullptr;
        std::vector<std::string> packFileAccessList;
    };
private:
    static std::map<std::string, Registration>* typeMap;
protected:
    LimonAPI* limonAPI = nullptr;
    std::vector<LimonTypes::GenericParameter> parameters;
    static std::map<std::string, Registration> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, Registration>();
        }
        return typeMap;
    }
    explicit TriggerInterface(LimonAPI* limonAPI): limonAPI(limonAPI) {}

public:
    /**
     * Every ./Data/ file this type reads or passes to LimonAPI, a trailing / meaning the whole folder. Hide this with
     * your own static. A path no world references and no type lists is refused, and the packer ships only what is listed.
     */
    static std::vector<std::string> getPackFileAccessList() {
        return {};
    }

    template<typename T>
    static Registration registration();

    // Not virtual
    static std::vector<std::string> getTriggerNames() {
        std::vector<std::string> names;
        for (auto it = getMap()->begin(); it != getMap()->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    static void registerType(const std::string& typeName, const Registration& registration) {
        (*getMap())[typeName] = registration;
    }

    static std::vector<std::string> getRegisteredPackFileAccessList(const std::string& typeName) {
        auto it = getMap()->find(typeName);
        if (it == getMap()->end()) {
            return {};
        }
        return it->second.packFileAccessList;
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
        std::map<std::string, Registration>::iterator it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second.factory(apiInstance);
    }

};

template<typename T>
TriggerInterface* createT(LimonAPI* limonAPI) {
    return new T(limonAPI);
}

template<typename T>
TriggerInterface::Registration TriggerInterface::registration() {
    Registration result;
    result.factory = &createT<T>;
    result.packFileAccessList = T::getPackFileAccessList();
    return result;
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
        getMap()->insert(std::make_pair(s, registration<T>()));
    }

};

#endif //LIMONENGINE_TRIGGERINTERFACE_H
