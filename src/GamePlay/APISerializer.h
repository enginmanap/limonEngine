//
// Created by engin on 17.01.2019.
//

#ifndef LIMONENGINE_APISERIALIZER_H
#define LIMONENGINE_APISERIALIZER_H

#include <tinyxml2.h>
#include <memory>
#include <unordered_map>

#include "limonAPI/LimonAPI.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/TriggerInterface.h"

class APISerializer {
public:
    // Rewrites any GenericParameter that references another world object's ID (MODEL, GUI_TEXT,
    // LIGHT, SOUND, CAMERA_RIG, TRIGGER) to idRemap's mapped ID, if that object is present in the
    // map. Used when cloning an AI/trigger/camera-rig extension whose parameters may point at
    // objects being copied in the same operation, so the clone targets the copies instead of the
    // originals. References to objects not in idRemap (e.g. some other, un-copied object elsewhere
    // in the world) are left untouched. ANIMATION holds an index, not an object ID, and is skipped.
    static void remapObjectReferenceParameters(std::vector<LimonTypes::GenericParameter>& parameters,
                                                const std::unordered_map<uint32_t, uint32_t>& idRemap);

    static bool serializeParameterRequest(const LimonTypes::GenericParameter& parameterRequest, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                                          uint32_t index);

    static std::shared_ptr<LimonTypes::GenericParameter> deserializeParameterRequest(tinyxml2::XMLElement *parameterNode, uint32_t &index);

    static bool serializeTriggerCode(const TriggerInterface &trigger, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggerNode,
                                     const std::string &triggerCodeNodeName, bool enabled);

    static TriggerInterface* deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
                                                    const std::string &nodeName, LimonAPI *limonAPI, bool &enabled);

    static void serializeActorInterface(const ActorInterface& actor, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode);

    static ActorInterface *deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI, uint32_t modelID);

    static void loadVec4(tinyxml2::XMLNode *vectorNode, LimonTypes::Vec4 &vector);

};


#endif //LIMONENGINE_APISERIALIZER_H
