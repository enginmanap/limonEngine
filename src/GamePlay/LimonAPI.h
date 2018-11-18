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

    struct Vec2 {
        float x,y;

        Vec2() = default;
        Vec2(float x, float y): x(x), y(y){}

        float operator [] (int i) const { switch (i) {
                case 0: return x;
                case 1: return y;
            }}
        float& operator [] (int i) {switch (i) {
                case 0: return x;
                case 1: return y;
            }}
        };


    struct Vec4 {
        float x,y,z,w;

        Vec4() = default;
        Vec4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}
        Vec4(float x, float y, float z): x(x), y(y), z(z), w(0) {}

        float operator [] (int i) const {switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
        }}
        float& operator [] (int i) {switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
        }}

        const Vec4 operator+(const Vec4 &second) {
            Vec4 result;
            result.x = this->x + second.x;
            result.y = this->y + second.y;
            result.z = this->z + second.z;
            result.w = this->w + second.w;
            return result;
        }

        const Vec4 operator-(const Vec4 &second) {
            Vec4 result;
            result.x = this->x - second.x;
            result.y = this->y - second.y;
            result.z = this->z - second.z;
            result.w = this->w - second.w;
            return result;
        }
    };
    struct Mat4 {
        Vec4 rows[4];

        Mat4() = default;
        Mat4(Vec4 row0, Vec4 row1, Vec4 row2, Vec4 row3) {
            rows[0] = row0;
            rows[1] = row1;
            rows[2] = row2;
            rows[3] = row3;
        }

        Vec4 operator [] (int i) const {return rows[i];}
        Vec4& operator [] (int i) {return rows[i];}
    };

    struct ParameterRequest {
        enum RequestParameterTypes { MODEL, ANIMATION, SWITCH, FREE_TEXT, TRIGGER, GUI_TEXT, FREE_NUMBER, COORDINATE, TRANSFORM };
        RequestParameterTypes requestType;
        std::string description;
        enum ValueTypes { STRING, DOUBLE, LONG, LONG_ARRAY, BOOLEAN, VEC4, MAT4 };
        ValueTypes valueType;
        //Up part used for requesting parameter, down part used as values of that request.
        union Value {
            char stringValue[64] = {0};
            long longValue;
            long longValues[16];//first element is the size
            double doubleValue;
            Vec4 vectorValue;
            Mat4 matrixValue;
            bool boolValue;
        };

        Value value;
        bool isSet = false;

        ParameterRequest() {}

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
    uint32_t addGuiImage(const std::string &imageFilePath, const std::string &name, const Vec2 &position,
                             const Vec2 &scale, float rotation);

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

    bool interactWithAI(uint32_t AIID, std::vector<LimonAPI::ParameterRequest> &interactionInformation);



    /**
     * * If nothing is hit, returns empty vector
     * returns these values:
     * 1) objectID for what is under the cursor
     * 2) hit coordinates
     * 3) hit normal
     * 4) If object has AI, id of that AI
     */
    std::vector<ParameterRequest> rayCastToCursor();

    /**
     * If object not found, returns empty vector
     *
     * Returns these values:
     * 1) translate
     * 2) scale
     * 3) orientation
     */
    std::vector<LimonAPI::ParameterRequest> getObjectTransformation(uint32_t objectID);

    bool setObjectTranslate(uint32_t objectID, const LimonAPI::Vec4& position);
    bool setObjectScale(uint32_t objectID, const LimonAPI::Vec4& scale);
    bool setObjectOrientation(uint32_t objectID, const LimonAPI::Vec4& orientation);
    /**
     * Returns mat4 with objects transform
     *
     * It might be required for object that has custom matrix generation
     * @param objectID
     * @return
     */
    std::vector<LimonAPI::ParameterRequest> getObjectTransformationMatrix(uint32_t objectID);

    uint32_t getPlayerAttachedModel();
    LimonAPI::Vec4 getPlayerAttachedModelOffset();
    bool setPlayerAttachedModelOffset(LimonAPI::Vec4 newOffset);
    void killPlayer();

    std::string getModelAnimationName(uint32_t modelID);
    bool getModelAnimationFinished(uint32_t modelID);
    bool setModelAnimation(uint32_t modelID, std::string animationName, bool isLooped = true);
    bool setModelAnimationWithBlend(uint32_t modelID, std::string animationName, bool isLooped = true, long blendTime = 100);

    void interactWithPlayer(std::vector<ParameterRequest>& input);

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

    void addTimedEvent(long waitTime, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall, std::vector<LimonAPI::ParameterRequest> parameters);

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
    std::function<uint32_t(const std::string &, const std::string &, const Vec2 &, const Vec2 &, float)> worldAddGuiImage;
    std::function<uint32_t(const std::string &, float, bool, const glm::vec3 &, const glm::vec3 &, const glm::quat &)> worldAddModel;
    std::function<bool(uint32_t, const std::string &)> worldUpdateGuiText;
    std::function<uint32_t (uint32_t)> worldRemoveGuiText;
    std::function<std::vector<LimonAPI::ParameterRequest>(uint32_t , uint32_t )> worldGetResultOfTrigger;
    std::function<bool (uint32_t)> worldRemoveObject;
    std::function<std::vector<LimonAPI::ParameterRequest>(uint32_t)> worldGetObjectTransformation;
    std::function<bool (uint32_t, const LimonAPI::Vec4&)> worldSetObjectTranslate;
    std::function<bool (uint32_t, const LimonAPI::Vec4&)> worldSetObjectScale;
    std::function<bool (uint32_t, const LimonAPI::Vec4&)> worldSetObjectOrientation;
    std::function<std::vector<LimonAPI::ParameterRequest>(uint32_t)> worldGetObjectTransformationMatrix;
    std::function<bool (uint32_t, uint32_t)> worldAttachObjectToObject;
    std::function<bool (uint32_t)> worldRemoveTriggerObject;
    std::function<bool (uint32_t)> worldDisconnectObjectFromPhysics;
    std::function<bool (uint32_t)> worldReconnectObjectToPhysics;

    std::function<bool (uint32_t, const std::string&)> worldAttachSoundToObjectAndPlay;
    std::function<bool (uint32_t)> worldDetachSoundFromObject;
    std::function<uint32_t (const std::string&, const glm::vec3&, bool)> worldPlaySound;

    std::function<std::vector<ParameterRequest>()> worldRayCastToCursor;
    std::function<bool (uint32_t, std::vector<ParameterRequest>&)> worldInteractWithAI;
    std::function<void (std::vector<ParameterRequest>&)> worldInteractWithPlayer;

    std::function<void (long, std::function<void(const std::vector<LimonAPI::ParameterRequest>&)>, std::vector<LimonAPI::ParameterRequest>)> worldAddTimedEvent;

    std::function<LimonAPI::Vec4 ()> worldGetPlayerAttachmentOffset;
    std::function<bool (LimonAPI::Vec4)> worldSetPlayerAttachmentOffset;
    std::function<uint32_t ()> worldGetPlayerAttachedModel;
    std::function<void ()> worldKillPlayer;

    std::function<std::string(uint32_t)> worldGetModelAnimationName;
    std::function<bool(uint32_t)> worldGetModelAnimationFinished;
    std::function<bool(uint32_t,std::string, bool)> worldSetAnimationOfModel;
    std::function<bool(uint32_t,std::string, bool, long)> worldSetAnimationOfModelWithBlend;

    /*** Non World API calls *******************************************************/
    std::function<bool (const std::string&)> limonLoadWorld;
    std::function<bool (const std::string&)> limonReturnOrLoadWorld;
    std::function<bool (const std::string&)> limonLoadNewAndRemoveCurrentWorld;

    std::function<void ()> limonExitGame;
    std::function<void ()> limonReturnPrevious;
    /*** Non World API calls *******************************************************/
};


#endif //LIMONENGINE_LIMONAPI_H
