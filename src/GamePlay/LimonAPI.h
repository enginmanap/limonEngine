//
// Created by engin on 13.05.2018.
//

#ifndef LIMONENGINE_LIMONAPI_H
#define LIMONENGINE_LIMONAPI_H

#include <vector>
#include <string>
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

    uint32_t animateModel(uint32_t modelID, uint32_t animationID, bool looped, const std::string *soundPath);
    uint32_t addGuiText(const std::string &fontFilePath, uint32_t fontSize,
                        const std::string &name, const std::string &text,
                               const glm::vec3 &color,
                               const glm::vec2 &position, float rotation);
    bool updateGuiText(uint32_t guiTextID, const std::string &newText);
    uint32_t removeGuiElement(uint32_t guiElementID);

    uint32_t addObject(const std::string &modelFilePath, float modelWeight, bool physical, const glm::vec3 &position,
                       const glm::vec3 &scale, const glm::quat &orientation);
    bool removeObject(uint32_t objectID);
    bool attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID);//second one is
    bool removeTriggerObject(uint32_t TriggerObjectID);
    bool disconnectObjectFromPhysics(uint32_t modelID);
    bool reconnectObjectToPhysics(uint32_t modelID);


    bool attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath);
    bool detachSoundFromObject(uint32_t objectWorldID);
    uint32_t playSound(const std::string &soundPath, const glm::vec3 &position, bool looped);


    /**
     * * If nothing is hit, returns empty vector
     * returns these values:
     * 1) objectID for what is under the cursor
     * 2,3,4) hit coordinates
     * 5,6,7) hit normal
     */
    std::vector<ParameterRequest> rayCastToCursor();

    bool loadAndSwitchWorld(const std::string& worldFileName);
    bool returnToWorld(const std::string& worldFileName);//if world is not loaded, loads first
    bool LoadAndRemove(const std::string& worldFileName); // removes current world after loading the new one

    void returnPreviousWorld();
    void quitGame();




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

    LimonAPI(std::function<bool (const std::string&)> worldLoadMethod,
             std::function<bool (const std::string&)> worldReturnOrLoadMethod,
             std::function<bool (const std::string&)> worldLoadNewAndRemoveCurrentMethod,
             std::function<void ()> worldExitMethod,
             std::function<void ()> worldReturnPreviousMethod) {
        limonLoadWorld = worldLoadMethod;
        limonReturnOrLoadWorld = worldReturnOrLoadMethod;
        limonLoadNewAndRemoveCurrentWorld = worldLoadNewAndRemoveCurrentMethod;
        limonExitGame = worldExitMethod;
        limonReturnPrevious = worldReturnPreviousMethod;
    }

private:
    friend class WorldLoader;

    std::map<std::string, LimonAPI::ParameterRequest> variableStore;

    std::function<bool(std::vector<LimonAPI::ParameterRequest> &, uint32_t)> worldGenerateEditorElementsForParameters;
    std::function<uint32_t(uint32_t , uint32_t , bool, const std::string* )> worldAddAnimationToObject;
    std::function<uint32_t(const std::string &, uint32_t, const std::string &, const std::string &, const glm::vec3 &, const glm::vec2 &, float)> worldAddGuiText;
    std::function<uint32_t(const std::string &, float, bool, const glm::vec3 &, const glm::vec3 &, const glm::quat &)> worldAddModel;
    std::function<bool(uint32_t, const std::string &)> worldUpdateGuiText;
    std::function<uint32_t (uint32_t)> worldRemoveGuiText;
    std::function<std::vector<LimonAPI::ParameterRequest>(uint32_t , uint32_t )> worldGetResultOfTrigger;
    std::function<bool (uint32_t)> worldRemoveObject;
    std::function<bool (uint32_t, uint32_t)> worldAttachObjectToObject;
    std::function<bool (uint32_t)> worldRemoveTriggerObject;
    std::function<bool (uint32_t)> worldDisconnectObjectFromPhysics;
    std::function<bool (uint32_t)> worldReconnectObjectToPhysics;

    std::function<bool (uint32_t, const std::string&)> worldAttachSoundToObjectAndPlay;
    std::function<bool (uint32_t)> worldDetachSoundFromObject;
    std::function<uint32_t (const std::string&, const glm::vec3&, bool)> worldPlaySound;

    std::function<std::vector<ParameterRequest>()> worldRayCastToCursor;

    /*** Non World API calls *******************************************************/
    std::function<bool (const std::string&)> limonLoadWorld;
    std::function<bool (const std::string&)> limonReturnOrLoadWorld;
    std::function<bool (const std::string&)> limonLoadNewAndRemoveCurrentWorld;

    std::function<void ()> limonExitGame;
    std::function<void ()> limonReturnPrevious;
    /*** Non World API calls *******************************************************/
};


#endif //LIMONENGINE_LIMONAPI_H
