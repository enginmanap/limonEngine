//
// Created by engin on 17.01.2019.
//

#ifndef LIMONENGINE_APISERIALIZER_H
#define LIMONENGINE_APISERIALIZER_H

#include <tinyxml2.h>
#include <memory>

#include "limonAPI/LimonAPI.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/TriggerInterface.h"

class APISerializer {
public:
    static bool serializeParameterRequest(const LimonTypes::GenericParameter& parameterRequest, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                                          uint32_t index);

    static std::shared_ptr<LimonTypes::GenericParameter> deserializeParameterRequest(tinyxml2::XMLElement *parameterNode, uint32_t &index);

    static bool serializeTriggerCode(const TriggerInterface &trigger, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggerNode,
                                     const std::string &triggerCodeNodeName,
                                     const std::vector<LimonTypes::GenericParameter> &parameters, bool enabled);

    static TriggerInterface* deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
                                                    const std::string &nodeName, LimonAPI *limonAPI,
                                                    std::vector<LimonTypes::GenericParameter> &parameters, bool &enabled);

    static void serializeActorInterface(const ActorInterface& actor, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode);

    static ActorInterface *deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI);

    static void loadVec4(tinyxml2::XMLNode *vectorNode, LimonTypes::Vec4 &vector);

};


#endif //LIMONENGINE_APISERIALIZER_H
