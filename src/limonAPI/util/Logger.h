//
// Created by engin on 22.11.2017.
//

#ifndef LIMONENGINE_LOGGER_H
#define LIMONENGINE_LOGGER_H

//THIS FILE SHOULD NOT INCLUDE ANY LOCAL CLASSES
#include <string>
#include <deque>
#include <glm/glm.hpp>

struct Line {
    glm::vec3 from;
    glm::vec3 fromColor;
    int needsCameraTransform;//FIXME This variable is repeated, because it must be per vertex. Maybe we can share int as bytes.
    glm::vec3 to;
    glm::vec3 toColor;
    int needsCameraTransform2;//this is the other one

    Line(const glm::vec3 &from,
         const glm::vec3 &to,
         const glm::vec3 &fromColor,
         const glm::vec3 &toColor,
         const bool needsCameraTransform): from(from), fromColor(fromColor), needsCameraTransform(needsCameraTransform), to(to), toColor(toColor), needsCameraTransform2(needsCameraTransform){};
};

class Logger {
public:
    enum Subsystem {log_Subsystem_RENDER, log_Subsystem_MODEL, log_Subsystem_INPUT, log_Subsystem_SETTINGS, log_Subsystem_AI, log_Subsystem_LOAD_SAVE, log_Subsystem_EDITOR, log_Subsystem_ANIMATION};
    enum Level {log_level_TRACE, log_level_DEBUG, log_level_INFO, log_level_WARN, log_level_ERROR };

    struct LogLine {
        Subsystem subsystem;
        Level level;
        std::string text;
        long time;

        LogLine(Subsystem subsystem, Level level, const std::string &text, long time) : subsystem(subsystem), level(level), text(text), time(time) {};
    };

private:
    std::deque<LogLine*> logQueue;
    std::map<uint32_t, std::vector<Line>> userManagedLineBuffer;
    uint32_t lineBufferIndex = 0;
public:

    uint32_t drawLine(glm::vec3 from, glm::vec3 to, glm::vec3 fromColor, glm::vec3 toColor, bool requireCameraTransform) {
        lineBufferIndex++;
        std::vector<Line> lineBuffer;
        lineBuffer.push_back(Line(from, to, fromColor, toColor, requireCameraTransform));
        userManagedLineBuffer[lineBufferIndex] = lineBuffer;
        return lineBufferIndex;
    }

    bool drawLine(uint32_t bufferIndex, glm::vec3 from, glm::vec3 to, glm::vec3 fromColor, glm::vec3 toColor, bool requireCameraTransform) {
        std::map<uint32_t, std::vector<Line>>::iterator bufferToAdd = userManagedLineBuffer.find(bufferIndex);
        if(bufferToAdd == userManagedLineBuffer.end()) {
            return false;
        }
        bufferToAdd->second.push_back(Line(from, to, fromColor,toColor, requireCameraTransform));
        return true;
    }

    bool clearLineBuffer(uint32_t bufferIndex) {
        std::map<uint32_t, std::vector<Line>>::iterator bufferToDelete = userManagedLineBuffer.find(bufferIndex);
        if(bufferToDelete == userManagedLineBuffer.end()) {
            return false;
        }
        userManagedLineBuffer.erase(bufferToDelete);
        return true;
    }

    const std::map<uint32_t, std::vector<Line>>& getDrawLines() {
        return userManagedLineBuffer;
    }

    void log(Subsystem subsystem, Level level, const std::string &text) {
        logQueue.push_back(new LogLine(subsystem, level, text, gameTimeProvider()));
        //FIXME this gameTimeProvider is SDL_GetTicks, and causing issues since we switch to game time usage. Requires fixing
    };

    /**
     * This method returns a pointer to a single line of log, in order of it
     * received. Removing the object is the callers responsibility.
     *
     * @return single line of log
     */
    LogLine* getLog() {
        if(logQueue.empty() ) {
            return nullptr;
        }
        LogLine* temp = logQueue.front();
        logQueue.pop_front();
        return temp;
    }

    std::function<uint32_t()> gameTimeProvider;

    explicit Logger(const std::function<uint32_t ()> &gameTimeProvider) {
        this->gameTimeProvider = gameTimeProvider;
    }
};



#endif //LIMONENGINE_LOGGER_H
