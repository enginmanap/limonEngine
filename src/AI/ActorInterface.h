//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_ACTOR_H
#define LIMONENGINE_ACTOR_H

#include "../GamePlay/LimonAPI.h"


/**
 * On shared library load, void registerAsActor(std::map<std::string, ActorInterface*(*)(LimonAPI*)>*) function should be callable.
 *
 * This method will be called after object load.
 * For each actor, the name of actor and a constructor should be put in the map.
 */


class ActorInterface {
public:

    struct InformationRequest {
        bool routeToPlayer = false;
        bool routeToCustomPosition = false;
        glm::vec3 customPosition;
    };

private:
    static std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>* typeMap;

protected:
    static std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)> * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>();
        }
        return typeMap;
    }

    uint32_t worldID;
    uint32_t modelID = 0;
    InformationRequest informationRequest;
    LimonAPI* limonAPI;
public:
    struct ActorInformation{
        bool canSeePlayerDirectly = false;
        bool isPlayerLeft = false, isPlayerRight = false, isPlayerUp = false, isPlayerDown = false, isPlayerFront = false, isPlayerBack = false;
        float cosineBetweenPlayer = 0.0f;
        glm::vec3 playerDirection;
        float playerDistance = 0.0f;
        float cosineBetweenPlayerForSide;
        std::vector<glm::vec3> routeToRequest;
        uint32_t maximumRouteDistance = 128;//in node count
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

    ActorInterface(uint32_t id, LimonAPI *limonAPI) : worldID(id), limonAPI(limonAPI) {}

    static void registerType(const std::string& typeName, ActorInterface*(*constructor)(uint32_t, LimonAPI*)) {
        (*getMap())[typeName] = constructor;
    }

    virtual std::string getName() const = 0;

    virtual void play(long time, ActorInterface::ActorInformation &information) = 0;

    virtual bool interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) = 0;

    virtual std::vector<LimonAPI::ParameterRequest> getParameters() const { return std::vector<LimonAPI::ParameterRequest>();};
    virtual void setParameters(std::vector<LimonAPI::ParameterRequest> parameters __attribute__((unused))) {};

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
        return it->second(id, apiInstance);
    }

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const;

    static ActorInterface *deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI);
};



template<typename T>
ActorInterface* createActorT(uint32_t id, LimonAPI* limonAPI) {
    return new T(id, limonAPI);
}

template<typename T>
class ActorRegister : ActorInterface {

    void play(long time __attribute((unused)), ActorInterface::ActorInformation &information __attribute((unused))) override {};

    bool interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation __attribute((unused))) override {return false;};

    std::string getName() const override {
        return "This object is not meant to be used";
    }

public:
    ActorRegister(std::string const& s) : ActorInterface(0, nullptr) {
        getMap()->insert(std::make_pair(s, &createActorT<T>));
    }

};


#endif //LIMONENGINE_ACTOR_H
