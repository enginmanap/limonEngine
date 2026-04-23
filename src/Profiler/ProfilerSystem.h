#pragma once

#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <mutex>
#include "limonAPI/Options.h"

namespace tracy {
    class Worker;
}

class ProfilerSystem {
public:
    explicit ProfilerSystem(const OptionsUtil::Options* options);
    ~ProfilerSystem();

    tracy::Worker* GetWorker() const;
    OptionsUtil::Options::Option<bool>* getTracingServerOption();

    void Update();

    std::vector<float> GetFrameTimeHistory() const;
    float GetAverageFrameTime() const;
    float GetMinFrameTime() const;
    float GetMaxFrameTime() const;
    float GetPercentileFrameTime(float percentile) const;

    std::vector<float> GetZoneTimeHistory(const std::string& name) const;
    float GetZoneAverageFrameTime(const std::string& name) const;
    float GetZoneMinFrameTime(const std::string& name) const;
    float GetZoneMaxFrameTime(const std::string& name) const;
    float GetZonePercentileFrameTime(const std::string& name, float percentile) const;

private:
#ifdef TRACY_ENABLE
    tracy::Worker* tracyWorker = nullptr;
    static constexpr size_t frameTimeHistorySize = 10000;
    std::array<float, frameTimeHistorySize> frameTimeHistoryArray{};
    size_t frameTimeHead = 0;
    size_t frameTimeCount = 0;
    size_t frameLastProcessedIndex = 0;
    mutable std::mutex frameTimeMutex;

    std::unordered_map<std::string, std::array<float, frameTimeHistorySize>> zoneTimeHistory;
    std::unordered_map<std::string, size_t> zoneTimeHead;
    std::unordered_map<std::string, size_t> zoneTimeCount;
    std::unordered_map<std::string, size_t> zoneLastProcessedIndex;
    std::unordered_map<std::string, int16_t> zoneNameCache;

    OptionsUtil::Options::Option<bool> enableTracingServerOption;

    int16_t getSourceLocation(const char* name);
#endif
};
