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


class Model;
class AnimationCustom;
class World;
class WorldLoader;
class PhysicalRenderable;



class LimonAPI {
    friend class WorldLoader;
    static World* world;

    static void setWorld(World* inputWorld);
public:
    struct ParameterRequest {
        enum RequestParameterTypes { MODEL, ANIMATION, SWITCH, FREE_TEXT };
        RequestParameterTypes requestType;
        std::string description;
        enum ValueTypes { STRING, DOUBLE, LONG, BOOLEAN };
        ValueTypes valueType;
        //Up part used for requesting parameter, down part used as values of that request.
        union Value {
            char stringValue[64] = {0};
            long longValue;
            double doubleValue;
            bool boolValue;
        };

        Value value;
        bool isSet = false;

        bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                       uint32_t index) const;

        bool deserialize(tinyxml2::XMLElement *parameterNode, uint32_t &index);
    };

    static bool generateEditorElementsForParameters(std::vector<ParameterRequest> &runParameters, uint32_t index);

    static void animateModel(uint32_t modelID, uint32_t animationID, bool looped);
    static void addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &text, const glm::vec3 &color,
                                              const glm::vec2 &position, float rotation);
};


#endif //LIMONENGINE_LIMONAPI_H
