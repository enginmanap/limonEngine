//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"
#include "../World.h"
#include <tinyxml2.h>
//this is because the variable is static
World* LimonAPI::world;

void LimonAPI::setWorld(World *inputWorld) {
        world = inputWorld;
}

void LimonAPI::animateModel(uint32_t modelID, uint32_t animationID, bool looped) {
    world->addAnimationToObject(modelID, animationID, looped);
}

bool LimonAPI::generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters) {
    return world->generateEditorElementsForParameters(runParameters);
}

void LimonAPI::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &text,
                          const glm::vec3 &color, const glm::vec2 &position, float rotation) {
    world->addGuiText(fontFilePath, fontSize, text,color, position,rotation);

}

bool LimonAPI::ParameterRequest::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                                           uint32_t index) const {

    tinyxml2::XMLElement *parameterNode= document.NewElement("Parameter");
    ParametersNode->InsertEndChild(parameterNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("RequestType");
    switch (requestType) {
        case MODEL: {
            currentElement->SetText("Model");
        }
        break;
        case ANIMATION: {
            currentElement->SetText("Animation");
        }
            break;
        case SWITCH: {
            currentElement->SetText("Boolean");
        }
            break;
        case FREE_TEXT: {
            currentElement->SetText("FreeText");
        }
            break;
        }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Description");
    currentElement->SetText(description.c_str());
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("valueType");
    tinyxml2::XMLElement *valueElement = document.NewElement("Value");
    switch (valueType) {
        case STRING: {
            currentElement->SetText("String");
            valueElement->SetText(value.stringValue);
        }
            break;
        case DOUBLE: {
            currentElement->SetText("Double");
            valueElement->SetText(std::to_string(value.doubleValue).c_str());
        }
            break;
        case LONG: {
            currentElement->SetText("Long");
            valueElement->SetText(std::to_string(value.longValue).c_str());
        }
            break;
        case BOOLEAN: {
            currentElement->SetText("Boolean");
            if(value.boolValue) {
                valueElement->SetText("True");
            } else {
                valueElement->SetText("False");
            }
        }
            break;
        default:
            currentElement->SetText("UNKNOWN");
    }
    parameterNode->InsertEndChild(valueElement);
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("IsSet");
    if(isSet) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Index");
    currentElement->SetText(index);
    parameterNode->InsertEndChild(currentElement);
    return true;
}
