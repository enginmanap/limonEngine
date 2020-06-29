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
 * On shared library load, void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*) function should be callable.
 *
 * This method will be called after object load.
 * For each trigger, the name of trigger and a constructor should be put in the map.
 */

class PlayerExtensionInterface {
    static std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>* extensionTypesMap;
protected:
    LimonAPI* limonAPI = nullptr;
    static std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)> * getMap() {
        // never deleted. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!extensionTypesMap) {
            extensionTypesMap = new std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>();
        }
        return extensionTypesMap;
    }

    PlayerExtensionInterface(LimonAPI* limonAPI): limonAPI(limonAPI) {}

public:

    struct PlayerInformation {
        LimonAPI::Vec4 position;
        LimonAPI::Vec4 lookDirection;
    };

    // Not virtual
    static std::vector<std::string> getTriggerNames() {
        std::vector<std::string> names;
        for (auto it = extensionTypesMap->begin(); it != extensionTypesMap->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    static void registerType(const std::string& typeName, PlayerExtensionInterface*(*constructor)(LimonAPI*)) {
        (*getMap())[typeName] = constructor;
    }

    virtual void processInput(const InputStates &inputHandler, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                                  long time) = 0;
    virtual void interact(std::vector<LimonAPI::ParameterRequest> &interactionData) = 0;

    virtual ~PlayerExtensionInterface() = default;

    virtual std::string getName() const = 0;


    static PlayerExtensionInterface * createExtension(std::string const& s, LimonAPI* apiInstance) {
        auto it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second(apiInstance);
    }

};



template<typename T>
PlayerExtensionInterface* createPlayerExtension(LimonAPI *limonAPI) {
    return new T(limonAPI);
}

template<typename T>
class PlayerExtensionRegister : PlayerExtensionInterface {
    std::string getName() const override {
        return "This object is not meant to be used";
    }

    void processInput(const InputStates &inputHandler, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          long time) override {}
    void interact(std::vector<LimonAPI::ParameterRequest> &interactionData [[gnu::unused]]) override {}

public:
    explicit PlayerExtensionRegister(std::string const& s) : PlayerExtensionInterface(nullptr) {
        getMap()->insert(std::make_pair(s, &createPlayerExtension<T>));
    }

};


#endif //LIMONENGINE_PLAYEREXTENSIONINTERFACE_H
