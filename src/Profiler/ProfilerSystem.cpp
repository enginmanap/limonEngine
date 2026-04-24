#include "ProfilerSystem.h"
#include <numeric>
#include <algorithm>
#include <mutex>
#include <array>
#include <unordered_map>
#include <chrono>
#include "Profiler/ProfilerState.h"
#include "limonAPI/Options.h"
#include "Utils/HashUtil.h"

#ifdef TRACY_ENABLE
#include "../../libs/tracy/server/TracyWorker.hpp"

int16_t ProfilerSystem::getSourceLocation(const char* name) {
    auto it = zoneNameCache.find(name);
    if (it != zoneNameCache.end()) {
        return it->second;
    }

    if (tracyWorker && tracyWorker->AreSourceLocationZonesReady()) {
        const auto& sourceLocationZones = tracyWorker->GetSourceLocationZones();
        for (const auto& sourceLocationIdZonePair : sourceLocationZones) {
            const auto& sourceLocation = tracyWorker->GetSourceLocation(sourceLocationIdZonePair.first);
            const char* zoneName = tracyWorker->GetZoneName(sourceLocation);
            if (zoneName && strcmp(zoneName, name) == 0) {
                zoneNameCache[name] = sourceLocationIdZonePair.first;
                return sourceLocationIdZonePair.first;
            }
        }
    }
    return -1;
}

#endif

ProfilerSystem::ProfilerSystem(const OptionsUtil::Options* options) {
#ifdef TRACY_ENABLE
    enableTracingServerOption = options->getOption<bool>(HASH("Profiler.EnableServer"));
#endif
}

ProfilerSystem::~ProfilerSystem() {
#ifdef TRACY_ENABLE
    if (tracyWorker) {
        delete tracyWorker;
        tracyWorker = nullptr;
    }
#endif
}

tracy::Worker* ProfilerSystem::GetWorker() const {
#ifdef TRACY_ENABLE
    return tracyWorker;
#else
    return nullptr;
#endif
}

OptionsUtil::Options::Option<bool>* ProfilerSystem::getTracingServerOption() {
#ifdef TRACY_ENABLE
    return &enableTracingServerOption;
#else
    return nullptr;
#endif
}

void ProfilerSystem::Update() {
#ifdef TRACY_ENABLE

    if (enableTracingServerOption.getOrDefault(true)) {
        if (ProfilerState::traceOverallFrameTime || ProfilerState::traceSimulation || ProfilerState::traceVisibility || ProfilerState::traceRendering) {
            if (!tracyWorker) {
                tracyWorker = new tracy::Worker("127.0.0.1", 8086, -1);
            }
        }
    } else {
        if (tracyWorker) {
            delete tracyWorker;
            tracyWorker = nullptr;

            std::lock_guard<std::mutex> lock(frameTimeMutex);
            frameTimeHistoryArray.fill(0);
            frameTimeHead = 0;
            frameTimeCount = 0;
            frameLastProcessedIndex = 0;

            zoneTimeHistory.clear();
            zoneTimeHead.clear();
            zoneTimeCount.clear();
            zoneLastProcessedIndex.clear();
            zoneNameCache.clear();
        }
    }

    if (!tracyWorker || !tracyWorker->IsConnected() || !tracyWorker->HasData()) {
        return;
    }
    auto collectZoneTime = [&](const std::string& zoneName, bool enabled) {
        std::lock_guard<std::mutex> lock(frameTimeMutex);
        if (!enabled) {
            zoneTimeHistory.erase(zoneName);
            zoneTimeHead.erase(zoneName);
            zoneTimeCount.erase(zoneName);
            zoneLastProcessedIndex.erase(zoneName);
            return;
        }

        int16_t sourceLocationId = getSourceLocation(zoneName.c_str());
        if (sourceLocationId == -1) {
            return;
        }
        if (!tracyWorker->AreSourceLocationZonesReady()) {
            return;
        }
        const auto& zonesData = tracyWorker->GetZonesForSourceLocation(sourceLocationId);
        const auto& zones = zonesData.zones;
        size_t zoneCount = zones.size();
        if (zoneCount == 0) {
            return;
        }
        size_t lastProcessed = zoneLastProcessedIndex[zoneName];

        for (size_t i = lastProcessed; i < zoneCount; ++i) {
            const auto& zone = zones[i];
            if (!zone.Zone()->IsEndValid()) {
                continue;
            }
            int64_t zoneTimeNs = zone.Zone()->End() - zone.Zone()->Start();

            auto& history = zoneTimeHistory[zoneName];
            auto& head = zoneTimeHead[zoneName];
            auto& count = zoneTimeCount[zoneName];

            history[head] = static_cast<float>(zoneTimeNs) / 1000000.0f;
            head = (head + 1) % frameTimeHistorySize;
            if (count < frameTimeHistorySize) {
                count++;
            }

            zoneLastProcessedIndex[zoneName] = i + 1;
        }
    };

    collectZoneTime("Frame", ProfilerState::traceOverallFrameTime);
    collectZoneTime("World::play", ProfilerState::traceSimulation);
    collectZoneTime("VisibilityManager::update", ProfilerState::traceVisibility);
    collectZoneTime("Render", ProfilerState::traceRendering);
#endif
}

std::vector<float> ProfilerSystem::GetFrameTimeHistory() const {
    return GetZoneTimeHistory("Frame");
}

float ProfilerSystem::GetAverageFrameTime() const {
    return GetZoneAverageFrameTime("Frame");
}

float ProfilerSystem::GetMinFrameTime() const {
    return GetZoneMinFrameTime("Frame");
}

float ProfilerSystem::GetMaxFrameTime() const {
    return GetZoneMaxFrameTime("Frame");
}

float ProfilerSystem::GetPercentileFrameTime(float percentile) const {
    return GetZonePercentileFrameTime("Frame", percentile);
}

std::vector<float> ProfilerSystem::GetZoneTimeHistory(const std::string& name) const {
    std::vector<float> history;
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    if (!zoneTimeHistory.contains(name)) {
        return history;
    }
    const auto& zone_history = zoneTimeHistory.at(name);
    size_t count = std::min(zoneTimeCount.at(name), static_cast<size_t>(100));
    size_t head = zoneTimeCount.at(name) >= 100 ? (zoneTimeHead.at(name) + frameTimeHistorySize - 100) % frameTimeHistorySize : 0;

    history.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        history.push_back(zone_history[(head + i) % frameTimeHistorySize]);
    }
#endif
    return history;
}

float ProfilerSystem::GetZoneAverageFrameTime(const std::string& name) const {
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    if (!zoneTimeHistory.contains(name)) {
        return 0.0f;
    }
    const auto& zone_history = zoneTimeHistory.at(name);
    size_t count = std::min(zoneTimeCount.at(name), static_cast<size_t>(100));
    if (count == 0) {
        return 0.0f;
    }
    float sum = 0.0f;
    size_t head = zoneTimeCount.at(name) >= 100 ? (zoneTimeHead.at(name) + frameTimeHistorySize - 100) % frameTimeHistorySize : 0;
    for (size_t i = 0; i < count; ++i) {
        sum += zone_history[(head + i) % frameTimeHistorySize];
    }
    return sum / count;
#else
    return 0.0f;
#endif
}

float ProfilerSystem::GetZoneMinFrameTime(const std::string& name) const {
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    if (!zoneTimeHistory.contains(name)) {
        return 0.0f;
    }
    const auto& zone_history = zoneTimeHistory.at(name);
    size_t count = std::min(zoneTimeCount.at(name), static_cast<size_t>(100));
    if (count == 0) return 0.0f;
    size_t head = zoneTimeCount.at(name) >= 100 ? (zoneTimeHead.at(name) + frameTimeHistorySize - 100) % frameTimeHistorySize : 0;
    float min_val = zone_history[head];
    for (size_t i = 1; i < count; ++i) {
        if (zone_history[(head + i) % frameTimeHistorySize] < min_val) {
            min_val = zone_history[(head + i) % frameTimeHistorySize];
        }
    }
    return min_val;
#else
    return 0.0f;
#endif
}

float ProfilerSystem::GetZoneMaxFrameTime(const std::string& name) const {
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    if (!zoneTimeHistory.contains(name)) {
        return 0.0f;
    }
    const auto& zone_history = zoneTimeHistory.at(name);
    size_t count = std::min(zoneTimeCount.at(name), static_cast<size_t>(100));
    if (count == 0) return 0.0f;
    size_t head = zoneTimeCount.at(name) >= 100 ? (zoneTimeHead.at(name) + frameTimeHistorySize - 100) % frameTimeHistorySize : 0;
    float maxValue = zone_history[head];
    for (size_t i = 1; i < count; ++i) {
        if (zone_history[(head + i) % frameTimeHistorySize] > maxValue) {
            maxValue = zone_history[(head + i) % frameTimeHistorySize];
        }
    }
    return maxValue;
#else
    return 0.0f;
#endif
}

float ProfilerSystem::GetZonePercentileFrameTime(const std::string& name, float percentile) const {
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    if (!zoneTimeHistory.contains(name)) {
        return 0.0f;
    }
    const auto& zone_history = zoneTimeHistory.at(name);
    size_t count = std::min(zoneTimeCount.at(name), static_cast<size_t>(100));
    if (count == 0) return 0.0f;
    std::vector<float> sorted;
    sorted.reserve(count);
    size_t head = zoneTimeCount.at(name) >= 100 ? (zoneTimeHead.at(name) + frameTimeHistorySize - 100) % frameTimeHistorySize : 0;
    for(size_t i = 0; i < count; ++i) {
        sorted.push_back(zone_history[(head + i) % frameTimeHistorySize]);
    }
    std::ranges::sort(sorted, std::greater<float>()); // Sort descending to get the slowest frames

    // We want the average of the slowest `percentile` fraction of frames
    size_t numSlowest = static_cast<size_t>(std::ceil(sorted.size() * percentile));
    if (numSlowest == 0) {
        return sorted[0]; // If percentile is very small, just return the absolute slowest
    }

    float sum = 0.0f;
    for (size_t i = 0; i < numSlowest; ++i) {
        sum += sorted[i];
    }

    return sum / numSlowest;
#else
    return 0.0f;
#endif
}