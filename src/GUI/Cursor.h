//
// Created by engin on 14.06.2018.
//

#ifndef LIMONENGINE_CURSOR_H
#define LIMONENGINE_CURSOR_H


#include "GUIText.h"

// TODO this class in current form is just a place holder
class Cursor: public GUIText {
public:
    Cursor(GLHelper *glHelper, uint32_t id, Face *font, const std::string &text, const glm::vec3 &color) : GUIText(
            glHelper, id, font, text, color) {}

    bool serialize(tinyxml2::XMLDocument &document __attribute((unused)), tinyxml2::XMLElement *parentNode __attribute((unused))) override { return true;};//This object is created by world itself, it should not be serialized

};


#endif //LIMONENGINE_CURSOR_H
