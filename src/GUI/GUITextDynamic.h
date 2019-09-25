//
// Created by engin on 14.11.2017.
//

#ifndef LIMONENGINE_GUITEXTDYNAMIC_H
#define LIMONENGINE_GUITEXTDYNAMIC_H


#include "GUITextBase.h"
#include "../Utils/Logger.h"
#include "../Options.h"
#include <list>

struct TextLine {
    std::string text;
    long time;
    int extraLines = 0;
    bool renderedBefore = false;

    TextLine(Logger::LogLine *logLine, long logLineCount) : time(logLine->time) {
        this->text = std::to_string(logLineCount)+ ": " + std::to_string(logLine->level) + ": " + logLine->text;
    }
};

class GUITextDynamic: public GUITextBase {
    int lineHeight;
    int maxCharWidth;
    int totalExtraLines = 0;
    long logLineCount = 1;
    long renderSetupTime = 0;
    std::list<TextLine> textList;
    Logger* source = nullptr;


    long duration = 5000;//default 5 second duration
    bool wordWrap = true;


public:
    GUITextDynamic(OpenGLGraphics *glHelper, Face *font, const glm::vec3 color, int width, int height, Options *options)
            : GUITextBase(glHelper, font, color) {
        lineHeight = face->getLineHeight()/64;
        maxCharWidth = face->getMaxCharWidth()/64;
        this->height = height;
        this->width = width;
        this->source = options->getLogger();

    }

    virtual void setupForTime(long time) {
        renderSetupTime = time;
    };

    void SetSource(Logger *logger){
        this->source = logger;
    }

    void setDuration(long duration) {
        this->duration = duration;
    }

    void renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram) override;

};


#endif //LIMONENGINE_GUITEXTDYNAMIC_H
