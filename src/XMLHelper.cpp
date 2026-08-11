//
// Created by engin on 10.08.2026.
//

#include "XMLHelper.h"
#include <iostream>

namespace XMLHelper {

    bool readRequiredText(tinyxml2::XMLNode *parent, const char *tag, std::string &out, const char *errorMessage) {
        tinyxml2::XMLElement *element = parent == nullptr ? nullptr : parent->FirstChildElement(tag);
        if (element == nullptr || element->GetText() == nullptr) {
            std::cerr << errorMessage << std::endl;
            return false;
        }
        out = element->GetText();
        return true;
    }

    bool readOptionalText(tinyxml2::XMLNode *parent, const char *tag, std::string &out) {
        tinyxml2::XMLElement *element = parent == nullptr ? nullptr : parent->FirstChildElement(tag);
        if (element == nullptr || element->GetText() == nullptr) {
            return false;
        }
        out = element->GetText();
        return true;
    }

    bool readRequiredFloat(tinyxml2::XMLNode *parent, const char *tag, float &out, const char *errorMessage) {
        tinyxml2::XMLElement *element = parent == nullptr ? nullptr : parent->FirstChildElement(tag);
        if (element == nullptr || element->GetText() == nullptr) {
            std::cerr << errorMessage << std::endl;
            return false;
        }
        out = std::stof(element->GetText());
        return true;
    }

    float readFloatOrDefault(tinyxml2::XMLNode *parent, const char *tag, float defaultValue) {
        tinyxml2::XMLElement *element = parent == nullptr ? nullptr : parent->FirstChildElement(tag);
        if (element == nullptr || element->GetText() == nullptr) {
            return defaultValue;
        }
        return std::stof(element->GetText());
    }

    bool readVec3(tinyxml2::XMLNode *parent, glm::vec3 &vector) {
        if (parent == nullptr) {
            return false;
        }
        if (!readRequiredFloat(parent, "X", vector.x, "Vector is missing x.")) return false;
        if (!readRequiredFloat(parent, "Y", vector.y, "Vector is missing y.")) return false;
        if (!readRequiredFloat(parent, "Z", vector.z, "Vector is missing z.")) return false;
        return true;
    }

    void writeVec3(tinyxml2::XMLDocument &document, tinyxml2::XMLNode *parent, const glm::vec3 &vector) {
        writeElement(document, parent, "X", vector.x);
        writeElement(document, parent, "Y", vector.y);
        writeElement(document, parent, "Z", vector.z);
    }
}
