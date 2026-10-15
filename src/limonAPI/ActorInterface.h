//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_ACTOR_H
#define LIMONENGINE_ACTOR_H

#include "LimonAPI.h"


/**
 * On shared library load, void registerActorTypes(std::map<std::string, ActorInterface::Registration>*) function should be callable.
 *
 * This method will be called after object load.
 * For each actor, put its name and ActorInterface::registration<YourActor>() in the map.
 */


class ActorInterface {
public:
    struct Registration {
        ActorInterface*(*factory)(uint32_t, LimonAPI*) = nullptr;
        std::vector<std::string> packFileAccessList;
    };

    struct InformationRequest {
        bool routeToPlayer = false;
        bool routeToCustomPosition = false;
        glm::vec3 customPosition;
        uint32_t maximumRouteNodeCount = 128;//search depth, not cleared by getRequests() so it stays set
    };

private:
    static std::map<std::string, Registration>* typeMap;

protected:
    static std::map<std::string, Registration> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, Registration>();
        }
        return typeMap;
    }

    ActorInterface(uint32_t id, LimonAPI *limonAPI) : worldID(id), limonAPI(limonAPI) {}

    uint32_t worldID;
    uint32_t modelID = 0;
    InformationRequest informationRequest;
    LimonAPI* limonAPI;
    std::vector<LimonTypes::GenericParameter> parameters;
public:
    struct ActorInformation{
        bool canSeePlayerDirectly = false;
        bool isPlayerLeft = false, isPlayerRight = false, isPlayerUp = false, isPlayerDown = false, isPlayerFront = false, isPlayerBack = false;
        float cosineBetweenPlayer = 0.0f;
        glm::vec3 playerDirection;
        float playerDistance = 0.0f;
        float cosineBetweenPlayerForSide;
        std::vector<glm::vec3> routeToRequest;
        bool routeFound = false;
        bool routeReady = false;
        bool playerDead = false;
    };

    // Not virtual
    static std::vector<std::string> getActorNames() {
        std::vector<std::string> names;
        if(typeMap == nullptr) {
            return names;
        }

        for (auto it = typeMap->begin(); it != typeMap->end(); it++) {
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

    virtual std::string getName() const = 0;

    virtual void play(uint32_t time, ActorInterface::ActorInformation &information) = 0;

    virtual bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) = 0;

    /**
     * Returns the parameters held on this actor instance. The base implementation returns the
     * protected ``parameters`` member - seed your defaults into it in the constructor and they are
     * returned here, with configured values written back through setParameters() on edit or load.
     * Override only if the actor builds its parameter list from its own typed members, converting
     * them to GenericParameter here; the bundled actors do this.
     */
    virtual std::vector<LimonTypes::GenericParameter> getParameters() const {
        return this->parameters;
    }

    /**
     * Stores the configured parameter values on the actor instance. The base implementation keeps
     * them in the protected ``parameters`` member. Override only if the actor needs to apply the
     * values onto its own typed members rather than simply hold them.
     */
    virtual void setParameters(std::vector<LimonTypes::GenericParameter> parameters) {
        this->parameters = parameters;
    }

    virtual ~ActorInterface() {};

    uint32_t getWorldID() const {
        return worldID;
    }

    void setModel(uint32_t modelID) {
        this->modelID = modelID;
    }

    uint32_t getModelID() const {
        return modelID;
    }

    glm::vec3 getPosition() const;

    glm::vec3 getFrontVector() const;

    /**
     * Remove requests after this method is called
     *
     * @return
     */
    InformationRequest getRequests() {
        InformationRequest request = this->informationRequest;
        this->informationRequest.routeToCustomPosition = false;
        this->informationRequest.routeToPlayer = false;
        return request;
    }


    static ActorInterface * createActor(std::string const& s, uint32_t id, LimonAPI* apiInstance) {
        auto it = getMap()->find(s);
        if(it == getMap()->end()) {
            return nullptr;
        }
        return it->second.factory(id, apiInstance);
    }
};



template<typename T>
ActorInterface* createActorT(uint32_t id, LimonAPI* limonAPI) {
    return new T(id, limonAPI);
}

template<typename T>
ActorInterface::Registration ActorInterface::registration() {
    Registration result;
    result.factory = &createActorT<T>;
    result.packFileAccessList = T::getPackFileAccessList();
    return result;
}

template<typename T>
class ActorRegister : ActorInterface {

    void play(uint32_t time [[gnu::unused]], ActorInterface::ActorInformation &information [[gnu::unused]]) override {};

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation [[gnu::unused]]) override {return false;};

    std::string getName() const override {
        return "This object is not meant to be used";
    }

public:
    ActorRegister(std::string const& s) : ActorInterface(0, nullptr) {
        getMap()->insert(std::make_pair(s, registration<T>()));
    }

};


#endif //LIMONENGINE_ACTOR_H
