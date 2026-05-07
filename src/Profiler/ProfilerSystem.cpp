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
        if (ProfilerState::traceOverallFrameTime || ProfilerState::traceSimulation || ProfilerState::traceVisibility || ProfilerState::traceRendering || ProfilerState::traceGpuRendering) {
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
            perThreadZoneLastProcessed.clear();
            gpuZoneLastProcessed.clear();
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
    collectPerThreadZoneTime("fillVisibleObjectPerCamera", ProfilerState::traceVisibility);
    collectZoneTime("Render", ProfilerState::traceRendering);
    collectAllGpuZones(ProfilerState::traceGpuRendering);
#endif
}

#ifdef TRACY_ENABLE
void ProfilerSystem::collectPerThreadZoneTime(const std::string& zoneName, bool enabled) {
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    const std::string prefix = zoneName + "::";

    if (!enabled) {
        for (auto it = zoneTimeHistory.begin(); it != zoneTimeHistory.end(); ) {
            if (it->first.compare(0, prefix.size(), prefix) == 0) {
                zoneTimeHead.erase(it->first);
                zoneTimeCount.erase(it->first);
                it = zoneTimeHistory.erase(it);
            } else {
                ++it;
            }
        }
        perThreadZoneLastProcessed.erase(zoneName);
        zoneNameCache.erase(zoneName);
        return;
    }

    int16_t srcLocId = getSourceLocation(zoneName.c_str());
    if (srcLocId == -1) return;
    if (!tracyWorker->AreSourceLocationZonesReady()) return;

    const auto& zonesData = tracyWorker->GetZonesForSourceLocation(srcLocId);
    const auto& zones = zonesData.zones;
    size_t zoneCount = zones.size();
    if (zoneCount == 0) return;

    const auto& threadList = tracyWorker->GetThreadData();
    size_t lastProcessed = perThreadZoneLastProcessed[zoneName];

    for (size_t i = lastProcessed; i < zoneCount; ++i) {
        const auto& ztd = zones[i];
        if (!ztd.Zone()->IsEndValid()) continue;

        uint16_t threadIdx = ztd.Thread();
        if (threadIdx >= threadList.size()) continue;

        const char* threadName = tracyWorker->GetThreadName(threadList[threadIdx]->id);
        std::string threadKey = prefix + threadName;

        int64_t durationNs = ztd.Zone()->End() - ztd.Zone()->Start();

        auto& history = zoneTimeHistory[threadKey];
        auto& head = zoneTimeHead[threadKey];
        auto& count = zoneTimeCount[threadKey];

        history[head] = static_cast<float>(durationNs) / 1000000.0f;
        head = (head + 1) % frameTimeHistorySize;
        if (count < frameTimeHistorySize) count++;

        perThreadZoneLastProcessed[zoneName] = i + 1;
    }
}
#endif

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

std::vector<std::string> ProfilerSystem::GetZoneThreadNames(const std::string& zoneName) const {
    std::vector<std::string> names;
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    const std::string prefix = zoneName + "::";
    for (const auto& entry : zoneTimeHistory) {
        if (entry.first.compare(0, prefix.size(), prefix) == 0) {
            names.push_back(entry.first.substr(prefix.size()));
        }
    }
#endif
    return names;
}

#ifdef TRACY_ENABLE
void ProfilerSystem::collectAllGpuZones(bool enabled) {
    std::lock_guard<std::mutex> lock(frameTimeMutex);
    constexpr std::string_view gpuPrefix = "GPU::";

    if (!enabled) {
        for (auto it = zoneTimeHistory.begin(); it != zoneTimeHistory.end(); ) {
            if (it->first.compare(0, gpuPrefix.size(), gpuPrefix) == 0) {
                zoneTimeHead.erase(it->first);
                zoneTimeCount.erase(it->first);
                it = zoneTimeHistory.erase(it);
            } else {
                ++it;
            }
        }
        gpuZoneLastProcessed.clear();
        return;
    }

    if (!tracyWorker->AreGpuSourceLocationZonesReady()) return;

    const auto& gpuSourceLocZones = tracyWorker->GetGpuSourceLocationZones();
    for (const auto& [srcLocId, gpuZoneData] : gpuSourceLocZones) {
        const auto& srcLoc = tracyWorker->GetSourceLocation(srcLocId);
        const char* zoneName = tracyWorker->GetZoneName(srcLoc);
        if (!zoneName) continue;

        const std::string key = std::string(gpuPrefix) + zoneName;
        const auto& zones = gpuZoneData.zones;
        const size_t zoneCount = zones.size();
        if (zoneCount == 0) continue;

        size_t lastProcessed = gpuZoneLastProcessed[key];
        for (size_t i = lastProcessed; i < zoneCount; ++i) {
            const auto* ev = zones[i].Zone();
            if (ev->GpuEnd() < 0) continue; // query result not yet available

            const int64_t durationNs = ev->GpuEnd() - ev->GpuStart();
            if (durationNs < 0) continue;

            auto& history = zoneTimeHistory[key];
            auto& head = zoneTimeHead[key];
            auto& count = zoneTimeCount[key];

            history[head] = static_cast<float>(durationNs) / 1000000.0f;
            head = (head + 1) % frameTimeHistorySize;
            if (count < frameTimeHistorySize) count++;

            gpuZoneLastProcessed[key] = i + 1;
        }
    }
}
#endif

std::vector<std::string> ProfilerSystem::GetGpuZoneNames() const {
    return GetZoneThreadNames("GPU");
}

std::vector<ProfileEvent> ProfilerSystem::GetLastFrameEvents(FrameFilter filter) const {
    std::vector<ProfileEvent> result;
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);

    if (!tracyWorker || !tracyWorker->IsConnected() || !tracyWorker->HasData()) {
        return result;
    }
    if (!tracyWorker->AreSourceLocationZonesReady()) {
        return result;
    }

    const auto& allSrcLocs = tracyWorker->GetSourceLocationZones();

    // Single pass: locate Frame and World::play zone data.
    using ZoneDataT = std::remove_reference_t<decltype(allSrcLocs.begin()->second)>;
    const ZoneDataT* frameZoneData = nullptr;
    const ZoneDataT* worldPlayData = nullptr;
    for (const auto& [id, zoneData] : allSrcLocs) {
        const char* name = tracyWorker->GetZoneName(tracyWorker->GetSourceLocation(id));
        if (!name) continue;
        if (!frameZoneData && strcmp(name, "Frame") == 0)        frameZoneData = &zoneData;
        if (!worldPlayData && strcmp(name, "World::play") == 0)  worldPlayData = &zoneData;
        if (frameZoneData && worldPlayData) break;
    }
    if (!frameZoneData) return result;

    // Search Frame zones newest to oldest for the first one matching the filter.
    int64_t frameStart = -1, frameEnd = -1;
    uint16_t mainThread = 0;
    for (int i = (int)frameZoneData->zones.size() - 1; i >= 0; --i) {
        if (!frameZoneData->zones[i].Zone()->IsEndValid()) continue;

        const int64_t fStart = frameZoneData->zones[i].Zone()->Start();
        const int64_t fEnd   = frameZoneData->zones[i].Zone()->End();

        if (filter != FrameFilter::All) {
            bool hasGameTick = false;
            if (worldPlayData) {
                const auto& wpZones = worldPlayData->zones;
                for (int wi = (int)wpZones.size() - 1; wi >= 0; --wi) {
                    const int64_t wStart = wpZones[wi].Zone()->Start();
                    if (wStart < fStart) break;
                    if (wStart < fEnd) { hasGameTick = true; break; }
                }
            }
            if (filter == FrameFilter::GameTick  && !hasGameTick) continue;
            if (filter == FrameFilter::RenderOnly &&  hasGameTick) continue;
        }

        frameStart = fStart;
        frameEnd   = fEnd;
        mainThread = frameZoneData->zones[i].Thread();
        break;
    }

    if (frameStart == -1) return result;

    constexpr double nsToMs = 1.0 / 1000000.0;

    // Collect all zones from ALL threads that fall within [frameStart, frameEnd].
    // Group by thread index so depths can be computed independently per thread.
    std::unordered_map<uint16_t, std::vector<ProfileEvent>> perThread;

    for (const auto& [srcLocId, zoneData] : allSrcLocs) {
        const auto& zones = zoneData.zones;
        for (int i = (int)zones.size() - 1; i >= 0; --i) {
            const auto& ztd = zones[i];
            int64_t start = ztd.Zone()->Start();
            if (start < frameStart) break;

            if (!ztd.Zone()->IsEndValid()) continue;
            int64_t end = ztd.Zone()->End();
            if (end > frameEnd) continue;

            // Use GetZoneName(ZoneEvent) so per-zone name overrides (ZoneNameV) are respected.
            const char* zoneName = tracyWorker->GetZoneName(*ztd.Zone());
            if (!zoneName) continue;

            ProfileEvent ev;
            ev.Name      = zoneName;
            ev.StartTime = (start - frameStart) * nsToMs;
            ev.EndTime   = (end   - frameStart) * nsToMs;
            ev.Depth     = 0;
            perThread[ztd.Thread()].push_back(std::move(ev));
        }
    }

    // Helper: sort events then assign depths via an end-time stack, offset by baseDepth.
    // Returns the number of depth rows consumed by this thread's events.
    auto assignDepths = [&](std::vector<ProfileEvent>& events, uint32_t baseDepth) -> uint32_t {
        std::sort(events.begin(), events.end(), [](const ProfileEvent& a, const ProfileEvent& b) {
            if (a.StartTime != b.StartTime) return a.StartTime < b.StartTime;
            return (a.EndTime - a.StartTime) > (b.EndTime - b.StartTime);
        });

        std::vector<double> depthStack;
        uint32_t maxLocal = 0;
        for (auto& e : events) {
            while (!depthStack.empty() && depthStack.back() <= e.StartTime)
                depthStack.pop_back();
            uint32_t local = static_cast<uint32_t>(depthStack.size());
            maxLocal = std::max(maxLocal, local);
            e.Depth  = baseDepth + local;
            depthStack.push_back(e.EndTime);
        }
        return maxLocal + 1;
    };

    // Collect GPU zones. Tracy calibrates GpuStart/GpuEnd to the same CPU nanosecond epoch,
    // so (GpuStart - frameStart) gives the correct frame-relative position directly.
    // Forward scan with continue (not break) because GPU zones are sorted by GpuStart,
    // not CpuStart, so a break on CpuStart would terminate early.
    if (tracyWorker->AreGpuSourceLocationZonesReady()) {
        const auto& gpuSrcLocs = tracyWorker->GetGpuSourceLocationZones();
        for (const auto& [srcLocId, gpuZoneData] : gpuSrcLocs) {
            const auto& srcLoc   = tracyWorker->GetSourceLocation(srcLocId);
            const char* zoneName = tracyWorker->GetZoneName(srcLoc);
            if (!zoneName) continue;

            for (const auto& ztd : gpuZoneData.zones) {
                const auto* ev = ztd.Zone();
                if (ev->CpuStart() < frameStart || ev->CpuStart() > frameEnd) continue;
                if (ev->GpuEnd() < 0)               continue;
                if (ev->GpuEnd() <= ev->GpuStart()) continue;

                const uint16_t gpuKey = static_cast<uint16_t>(0x8000u + ztd.Thread());
                ProfileEvent gev;
                gev.Name      = zoneName;
                // GpuStart calibration has a systematic epoch offset vs the CPU clock,
                // so use CpuStart (a real CPU timestamp) for position and GPU duration for width.
                const double gpuDuration = (ev->GpuEnd() - ev->GpuStart()) * nsToMs;
                gev.StartTime = (ev->CpuStart() - frameStart) * nsToMs;
                gev.EndTime   = gev.StartTime + gpuDuration;
                gev.Depth     = 0;
                perThread[gpuKey].push_back(std::move(gev));
            }
        }
    }

    // Main thread first, then other CPU threads, then GPU context bands (keys ≥ 0x8000).
    // Each band is separated by one blank row.
    uint32_t base = 0;
    if (auto it = perThread.find(mainThread); it != perThread.end()) {
        base += assignDepths(it->second, base);
        for (auto& e : it->second) result.push_back(std::move(e));
    }
    for (auto& [tid, events] : perThread) {
        if (tid == mainThread) continue;
        base += 1;
        base += assignDepths(events, base);
        for (auto& e : events) result.push_back(std::move(e));
    }
#endif
    return result;
}

std::vector<std::string> ProfilerSystem::GetGpuDebugInfo(FrameFilter filter) const {
    std::vector<std::string> lines;
#ifdef TRACY_ENABLE
    std::lock_guard<std::mutex> lock(frameTimeMutex);

    if (!tracyWorker || !tracyWorker->IsConnected() || !tracyWorker->HasData()) {
        lines.push_back("Tracy worker not ready.");
        return lines;
    }
    if (!tracyWorker->AreSourceLocationZonesReady()) {
        lines.push_back("Source location zones not ready.");
        return lines;
    }

    const auto& allSrcLocs = tracyWorker->GetSourceLocationZones();

    using ZoneDataT = std::remove_reference_t<decltype(allSrcLocs.begin()->second)>;
    const ZoneDataT* frameZoneData = nullptr;
    const ZoneDataT* worldPlayData = nullptr;
    for (const auto& [id, zoneData] : allSrcLocs) {
        const char* name = tracyWorker->GetZoneName(tracyWorker->GetSourceLocation(id));
        if (!name) continue;
        if (!frameZoneData && strcmp(name, "Frame") == 0)        frameZoneData = &zoneData;
        if (!worldPlayData && strcmp(name, "World::play") == 0)  worldPlayData = &zoneData;
        if (frameZoneData && worldPlayData) break;
    }

    int64_t frameStart = -1, frameEnd = -1;
    if (frameZoneData) {
        for (int i = (int)frameZoneData->zones.size() - 1; i >= 0; --i) {
            if (!frameZoneData->zones[i].Zone()->IsEndValid()) continue;

            const int64_t fStart = frameZoneData->zones[i].Zone()->Start();
            const int64_t fEnd   = frameZoneData->zones[i].Zone()->End();

            if (filter != FrameFilter::All) {
                bool hasGameTick = false;
                if (worldPlayData) {
                    const auto& wpZones = worldPlayData->zones;
                    for (int wi = (int)wpZones.size() - 1; wi >= 0; --wi) {
                        const int64_t wStart = wpZones[wi].Zone()->Start();
                        if (wStart < fStart) break;
                        if (wStart < fEnd) { hasGameTick = true; break; }
                    }
                }
                if (filter == FrameFilter::GameTick  && !hasGameTick) continue;
                if (filter == FrameFilter::RenderOnly &&  hasGameTick) continue;
            }

            frameStart = fStart;
            frameEnd   = fEnd;
            break;
        }
    }

    char buf[512];
    if (frameStart == -1) {
        lines.push_back("No completed Frame zone found for the selected filter.");
        return lines;
    }

    constexpr double nsToMs = 1.0 / 1000000.0;
    snprintf(buf, sizeof(buf), "Frame window: 0.000 to %.3f ms", (frameEnd - frameStart) * nsToMs);
    lines.push_back(buf);

    if (!tracyWorker->AreGpuSourceLocationZonesReady()) {
        lines.push_back("GPU source location zones not ready.");
        return lines;
    }

    const auto& gpuSrcLocs = tracyWorker->GetGpuSourceLocationZones();
    snprintf(buf, sizeof(buf), "GPU source locations: %zu", gpuSrcLocs.size());
    lines.push_back(buf);

    for (const auto& [srcLocId, gpuZoneData] : gpuSrcLocs) {
        const auto& srcLoc = tracyWorker->GetSourceLocation(srcLocId);
        const char* zoneName = tracyWorker->GetZoneName(srcLoc);
        if (!zoneName) {
            lines.push_back("  [null name] skipped");
            continue;
        }

        const auto& zones = gpuZoneData.zones;
        int total = (int)zones.size();
        int cpuOob = 0, gpuEndNeg = 0, gpuEndLeStart = 0, accepted = 0;
        bool foundFirstAccepted = false;
        bool foundFirstAllRejected = false;
        double firstAccCpuStart = 0, firstAccGpuStart = 0, firstAccGpuEnd = 0;
        double firstRejCpuStart = 0, firstRejGpuStart = 0, firstRejGpuEnd = 0;
        std::string firstRejReason;

        for (const auto& ztd : zones) {
            const auto* ev = ztd.Zone();
            if (ev->CpuStart() < frameStart || ev->CpuStart() > frameEnd) {
                cpuOob++;
                if (!foundFirstAllRejected) {
                    foundFirstAllRejected = true;
                    firstRejReason    = "CpuStart OOB";
                    firstRejCpuStart  = (ev->CpuStart() - frameStart) * nsToMs;
                    firstRejGpuStart  = (ev->GpuStart() - frameStart) * nsToMs;
                    firstRejGpuEnd    = (ev->GpuEnd() >= 0) ? (ev->GpuEnd() - frameStart) * nsToMs : (double)ev->GpuEnd();
                }
                continue;
            }
            if (ev->GpuEnd() < 0) {
                gpuEndNeg++;
                if (!foundFirstAllRejected) {
                    foundFirstAllRejected = true;
                    firstRejReason    = "GpuEnd<0";
                    firstRejCpuStart  = (ev->CpuStart() - frameStart) * nsToMs;
                    firstRejGpuStart  = (ev->GpuStart() - frameStart) * nsToMs;
                    firstRejGpuEnd    = (double)ev->GpuEnd();
                }
                continue;
            }
            if (ev->GpuEnd() <= ev->GpuStart()) {
                gpuEndLeStart++;
                if (!foundFirstAllRejected) {
                    foundFirstAllRejected = true;
                    firstRejReason    = "GpuEnd<=GpuStart";
                    firstRejCpuStart  = (ev->CpuStart() - frameStart) * nsToMs;
                    firstRejGpuStart  = (ev->GpuStart() - frameStart) * nsToMs;
                    firstRejGpuEnd    = (ev->GpuEnd() - frameStart) * nsToMs;
                }
                continue;
            }
            accepted++;
            if (!foundFirstAccepted) {
                foundFirstAccepted = true;
                firstAccCpuStart = (ev->CpuStart() - frameStart) * nsToMs;
                firstAccGpuStart = (ev->GpuStart() - frameStart) * nsToMs;
                firstAccGpuEnd   = (ev->GpuEnd()   - frameStart) * nsToMs;
            }
        }

        snprintf(buf, sizeof(buf), "  [%s] total=%d cpuOOB=%d gpuNeg=%d gpuLe=%d accepted=%d",
            zoneName, total, cpuOob, gpuEndNeg, gpuEndLeStart, accepted);
        lines.push_back(buf);

        if (foundFirstAccepted) {
            snprintf(buf, sizeof(buf), "    first-acc: cpu=%.3fms gpu=[%.3f, %.3f]ms dur=%.3fms",
                firstAccCpuStart, firstAccGpuStart, firstAccGpuEnd, firstAccGpuEnd - firstAccGpuStart);
            lines.push_back(buf);
        }
        if (accepted == 0 && foundFirstAllRejected) {
            snprintf(buf, sizeof(buf), "    first-rej(%s): cpu=%.3fms gpu=[%.3f, %.3f]ms",
                firstRejReason.c_str(), firstRejCpuStart, firstRejGpuStart, firstRejGpuEnd);
            lines.push_back(buf);
        }
    }
#endif
    return lines;
}