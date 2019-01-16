//
// Created by engin on 17.01.2019.
//

#ifndef LIMONENGINE_SDKSERIALIZER_H
#define LIMONENGINE_SDKSERIALIZER_H

#include <tinyxml2.h>
#include <memory>

#include "LimonAPI.h"
#include "../AI/ActorInterface.h"
#include "TriggerInterface.h"

class SDKSerializer {
public:
    static bool serializeParameterRequest(const LimonAPI::ParameterRequest& parameterRequest, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                       uint32_t index);

    static std::shared_ptr<LimonAPI::ParameterRequest> deserializeParameterRequest(tinyxml2::XMLElement *parameterNode, uint32_t &index);

    static bool serializeTriggerCode(const TriggerInterface &trigger, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggerNode,
                              const std::string &triggerCodeNodeName,
                              const std::vector<LimonAPI::ParameterRequest> &parameters, bool enabled);

    static TriggerInterface* deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
                                                    const std::string &nodeName, LimonAPI *limonAPI,
                                                    std::vector<LimonAPI::ParameterRequest> &parameters, bool &enabled);

    static void serializeActorInterface(const ActorInterface& actor, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode);

    static ActorInterface *deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI);

    static void loadVec4(tinyxml2::XMLNode *vectorNode, LimonAPI::Vec4 &vector);

};


#endif //LIMONENGINE_SDKSERIALIZER_H
