#pragma once
#include <cstdint>

class LimonAPI;

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
 */
class ProfileScope {
    friend class LimonAPI;
    ProfileScope(LimonAPI* api, uint64_t context) : limonAPI(api), context(context) {}
public:
    ~ProfileScope();

    // Called by the Python binding's __exit__. C++ callers should use RAII.
    void endZone();

    ProfileScope(ProfileScope&& other) noexcept : limonAPI(other.limonAPI), context(other.context) {
        other.context = 0;
    }

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    LimonAPI* limonAPI;
    uint64_t context;
};
