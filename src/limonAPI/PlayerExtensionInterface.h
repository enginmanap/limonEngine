//
// Created by engin on 16.11.2018.
//

#ifndef LIMONENGINE_PLAYEREXTENSIONINTERFACE_H
#define LIMONENGINE_PLAYEREXTENSIONINTERFACE_H


#include <string>
#include <map>
#include "LimonAPI.h"
#include "InputStates.h"

/**
 * On shared library load, void registerPlayerExtensionTypes(std::map<std::string, PlayerExtensionInterface::Registration>*) function should be callable.
 *
 * This method will be called after object load.
 * For each extension, put its name and PlayerExtensionInterface::registration<YourExtension>() in the map.
 */

class PlayerExtensionInterface {
public:
    struct Registration {
        PlayerExtensionInterface*(*factory)(LimonAPI*) = nullptr;
        std::vector<std::string> packFileAccessList;
    };
private:
    static std::map<std::string, Registration>* extensionTypesMap;
protected:
    LimonAPI* limonAPI = nullptr;
    std::vector<LimonTypes::GenericParameter> parameters;
    static std::map<std::string, Registration> * getMap() {
        // never deleted. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!extensionTypesMap) {
            extensionTypesMap = new std::map<std::string, Registration>();
        }
        return extensionTypesMap;
    }

    explicit PlayerExtensionInterface(LimonAPI* limonAPI): limonAPI(limonAPI) {}

public:

    struct PlayerInformation {
        LimonTypes::Vec4 position;
        LimonTypes::Vec4 lookDirection; //This is vec4 because we don't have Vec3.
    };

    // Not virtual
    // Renamed from getTriggerNames(): this returns registered player-extension names, not trigger names.
    static std::vector<std::string> getExtensionNames() {
        std::vector<std::string> names;
        for (auto it = getMap()->begin(); it != getMap()->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    //same contract as TriggerInterface::getPackFileAccessList
    static std::vector<std::string> getPackFileAccessList() {
        return {};
    }

    template<typename T>
    static Registration registration();

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

    virtual void processInput(const InputStates &inputHandler, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                                  uint32_t time) = 0;
    virtual void interact(std::vector<LimonTypes::GenericParameter> &interactionData) = 0;

    virtual ~PlayerExtensionInterface() = default;

    virtual std::string getName() const = 0;

    /**
     * Returns the configurable parameters of this extension. The base implementation exposes the
     * values held on the instance; override to expose typed members as descriptors (Actor-style).
     */
    virtual std::vector<LimonTypes::GenericParameter> getParameters() const {
        return this->parameters;
    }

    /**
     * Stores the configured parameter values on the extension instance. The base implementation keeps
     * them in the protected `parameters` member; override to react to value changes.
     */
    virtual void setParameters(std::vector<LimonTypes::GenericParameter> parameters) {
        this->parameters = parameters;
    }

    static PlayerExtensionInterface * createExtension(std::string const& s, LimonAPI* apiInstance) {
        auto it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second.factory(apiInstance);
    }

};



template<typename T>
PlayerExtensionInterface* createPlayerExtension(LimonAPI *limonAPI) {
    return new T(limonAPI);
}

template<typename T>
PlayerExtensionInterface::Registration PlayerExtensionInterface::registration() {
    Registration result;
    result.factory = &createPlayerExtension<T>;
    result.packFileAccessList = T::getPackFileAccessList();
    return result;
}

template<typename T>
class PlayerExtensionRegister : PlayerExtensionInterface {
    std::string getName() const override {
        return "This object is not meant to be used";
    }

    void processInput(const InputStates &inputHandler, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          uint32_t time) override {}
    void interact(std::vector<LimonTypes::GenericParameter> &interactionData [[gnu::unused]]) override {}

public:
    explicit PlayerExtensionRegister(std::string const& s) : PlayerExtensionInterface(nullptr) {
        getMap()->insert(std::make_pair(s, registration<T>()));
    }
};


#endif //LIMONENGINE_PLAYEREXTENSIONINTERFACE_H
