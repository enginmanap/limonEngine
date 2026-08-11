//
// Created by engin on 10.08.2026.
//

#ifndef LIMONENGINE_XMLHELPER_H
#define LIMONENGINE_XMLHELPER_H

#include <string>
#include <glm/vec3.hpp>
#include <tinyxml2.h>

namespace XMLHelper {

    template<typename T>
    void writeElement(tinyxml2::XMLDocument &document, tinyxml2::XMLNode *parent, const char *tag, T value) {
        tinyxml2::XMLElement *element = document.NewElement(tag);
        element->SetText(value);
        parent->InsertEndChild(element);
    }

    inline void writeElement(tinyxml2::XMLDocument &document, tinyxml2::XMLNode *parent, const char *tag, const std::string &value) {
        writeElement(document, parent, tag, value.c_str());
    }

    bool readRequiredText(tinyxml2::XMLNode *parent, const char *tag, std::string &out, const char *errorMessage);

    // leaves out untouched if not found, caller keeps its own default
    bool readOptionalText(tinyxml2::XMLNode *parent, const char *tag, std::string &out);

    bool readRequiredFloat(tinyxml2::XMLNode *parent, const char *tag, float &out, const char *errorMessage);

    float readFloatOrDefault(tinyxml2::XMLNode *parent, const char *tag, float defaultValue);

    bool readVec3(tinyxml2::XMLNode *parent, glm::vec3 &vector); // same contract as the old WorldLoader::loadVec3

    void writeVec3(tinyxml2::XMLDocument &document, tinyxml2::XMLNode *parent, const glm::vec3 &vector);
}

#endif //LIMONENGINE_XMLHELPER_H
