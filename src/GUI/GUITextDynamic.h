//
// Created by engin on 14.11.2017.
//

#ifndef UBERGAME_GUITEXTDYNAMIC_H
#define UBERGAME_GUITEXTDYNAMIC_H


#include "GUIText.h"
#include <list>

struct TimedText {
    std::string text;
    long time;
    int extraLines = 0;
    bool renderedBefore = false;

    TimedText(const std::string &text, long time, long logLineCount) : time(time) {
        this->text = std::to_string(logLineCount)+ ": " + text;
    }
};

class GUITextDynamic: public GUIText {
    int lineHeight;
    int maxCharWidth;
    int totalExtraLines = 0;
    long logLineCount = 1;
    std::list<TimedText> textList;

    long duration = 5000;//default 10 second duration
    bool wordWrap = true;


public:
    GUITextDynamic(GLHelper *glHelper, Face *font, const glm::vec3 color, int width, int height) : GUIText(glHelper, font, color) {
        lineHeight = face->getLineHeight()/64;
        maxCharWidth = face->getMaxCharWidth()/64;
        this->height = height;
        this->width = width;
    }

    void setDuration(long duration) {
        this->duration = duration;
    }

    void addText(const std::string &text, long time) {
        textList.push_back(TimedText(text,time, logLineCount++));
    };

    void render();
};


#endif //UBERGAME_GUITEXTDYNAMIC_H
