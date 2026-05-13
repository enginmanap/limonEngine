#pragma once
#include <cstdint>
#include <functional>

/**
 * RAII profiling zone. Obtain one via LimonAPI::profileScope(name); the zone
 * begins on construction and ends when the object is destroyed.
 *
 * C++ usage — let the destructor close the zone automatically:
 *
 *   void MyActor::play(long time, ActorInformation& info) {
 *       ProfileScope scope = limonAPI->profileScope(playZoneName);
 *       // ... work ...
 *   }  // zone ends here
 *
 * Python usage — use as a context manager so the zone ends at a deterministic
 * point rather than waiting for the garbage collector:
 *
 *   def play(self, time, actor_information):
 *       with self.limon_api.profile_scope(self._play_zone_name):
 *           # ... work ...
 *       # zone ends here, before GC
 *
 * endZone() exists solely to support the Python context manager __exit__
 * callback. C++ code should never call it directly — rely on RAII instead.
 *
 * The end-zone function is stored by value (copied at construction time) so
 * that ProfileScope remains valid even if the originating LimonAPI instance is
 * destroyed before the zone closes — e.g. when a Python script triggers a
 * world switch while inside a profile_scope() context manager.
 */
class ProfileScope {
    friend class LimonAPI;
    ProfileScope(std::function<void(uint64_t)> endZoneFunc, uint64_t context)
        : endZoneFunc(std::move(endZoneFunc)), context(context) {}
public:
    ~ProfileScope();

    // Called by the Python binding's __exit__. C++ callers should use RAII.
    void endZone();

    ProfileScope(ProfileScope&& other) noexcept
        : endZoneFunc(std::move(other.endZoneFunc)), context(other.context) {
        other.context = 0;
    }

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    std::function<void(uint64_t)> endZoneFunc;
    uint64_t context;
};
