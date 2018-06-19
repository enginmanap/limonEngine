//
// Created by engin on 13.05.2018.
//

#ifndef LIMONENGINE_LIMONAPI_H
#define LIMONENGINE_LIMONAPI_H

#include <vector>
#include <map>
#include <cstdint>
#include <glm/glm.hpp>
#include <tinyxml2.h>
#include <functional>


class Model;
class AnimationCustom;
class World;
class WorldLoader;
class PhysicalRenderable;



class LimonAPI {
public:
    struct ParameterRequest {
        enum RequestParameterTypes { MODEL, ANIMATION, SWITCH, FREE_TEXT, TRIGGER, GUI_TEXT, FREE_NUMBER};
        RequestParameterTypes requestType;
        std::string description;
        enum ValueTypes { STRING, DOUBLE, LONG, LONG_ARRAY, BOOLEAN };
        ValueTypes valueType;
        //Up part used for requesting parameter, down part used as values of that request.
        union Value {
            char stringValue[64] = {0};
            long longValue;
            long longValues[16];//first element is the size
            double doubleValue;
            bool boolValue;
        };

        Value value;
        bool isSet = false;

        bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                       uint32_t index) const;

        bool deserialize(tinyxml2::XMLElement *parameterNode, uint32_t &index);
    };

    bool generateEditorElementsForParameters(std::vector<ParameterRequest> &runParameters, uint32_t index);

    uint32_t animateModel(uint32_t modelID, uint32_t animationID, bool looped);
    uint32_t addGuiText(const std::string &fontFilePath, uint32_t fontSize,
                        const std::string &name, const std::string &text,
                               const glm::vec3 &color,
                               const glm::vec2 &position, float rotation);
    uint32_t updateGuiText(uint32_t guiTextID, const std::string &newText);
    uint32_t removeGuiElement(uint32_t guiElementID);
    uint32_t removeObject(uint32_t guiElementID);
    uint32_t removeTriggerObject(uint32_t TriggerObjectID);
    bool disconnectObjectFromPhysics(uint32_ modelID);
    bool reconnectObjectToPhysics(uint32_t modelID);

    std::vector<ParameterRequest> getResultOfTrigger(uint32_t TriggerObjectID, uint32_t TriggerCodeID);

    /**
     * This method Returns a parameter request reference that you can update. If the variable was never set,
     * it creates one with the default values. There are no safety checks, user is fully responsible for the variables.
     *
     * Don't forget, these variables are not saved in world save, so they should be considered temporary.
     *
     * @param variableName
     * @return variable itself
     */
    LimonAPI::ParameterRequest& getVariable(const std::string& variableName) {
        if(variableStore.find(variableName) == variableStore.end()) {
            variableStore[variableName] = LimonAPI::ParameterRequest();
        }
        return variableStore[variableName];
    }


private:
    friend class WorldLoader;

    std::map<std::string, LimonAPI::ParameterRequest> variableStore;

    std::function<bool(std::vector<LimonAPI::ParameterRequest> &, uint32_t)> worldGenerateEditorElementsForParameters;
    std::function<uint32_t(uint32_t , uint32_t , bool )> worldAddAnimationToObject;
    std::function<uint32_t(const std::string &, uint32_t, const std::string &, const std::string &, const glm::vec3 &, const glm::vec2 &, float)> worldAddGuiText;
    std::function<uint32_t(uint32_t, const std::string &)> worldUpdateGuiText;
    std::function<uint32_t (uint32_t)> worldRemoveGuiText;
    std::function<std::vector<LimonAPI::ParameterRequest>(uint32_t , uint32_t )> worldGetResultOfTrigger;
    std::function<uint32_t (uint32_t)> worldRemoveObject;
    std::function<uint32_t (uint32_t)> worldRemoveTriggerObject;
    std::function<bool (uint32_t)> worldDisconnectObjectFromPhysics;
    std::function<bool (uint32_t)> worldReconnectObjectToPhysics;
};


#endif //LIMONENGINE_LIMONAPI_H
