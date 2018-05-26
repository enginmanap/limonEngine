//
// Created by engin on 13.05.2018.
//

#ifndef LIMONENGINE_LIMONAPI_H
#define LIMONENGINE_LIMONAPI_H

#include <vector>
#include <map>
#include <cstdint>


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
        enum RequestParameterTypes { MODEL, ANIMATION, BOOLEAN };
        RequestParameterTypes requestType;
        std::string description;
        //Up part used for requesting parameter, down part used as values of that request.
        enum ValueTypes { STRING, DOUBLE, LONG, BOOL };
        ValueTypes valueType;
        union Value {
            long longValue;
            double doubleValue;
            bool boolValue;
            char* stringValue;
        };

        Value value;
        bool isSet = false;
    };

    static bool generateEditorElementsForParameters(std::vector<ParameterRequest>& runParameters);

    static void animateModel(uint32_t modelID, uint32_t animationID, bool looped);
};


#endif //LIMONENGINE_LIMONAPI_H
