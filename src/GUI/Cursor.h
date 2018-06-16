//
// Created by engin on 14.06.2018.
//

#ifndef LIMONENGINE_CURSOR_H
#define LIMONENGINE_CURSOR_H


#include "GUITextBase.h"

// TODO this class in current form is just a place holder
class Cursor: public GUITextBase {
public:
    Cursor(GLHelper *glHelper, Face *font, const std::string &text, const glm::vec3 &color) : GUITextBase(
            glHelper, font, text, color) {}

};


#endif //LIMONENGINE_CURSOR_H
