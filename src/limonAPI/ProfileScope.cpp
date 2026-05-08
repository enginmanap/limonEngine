#include "ProfileScope.h"
#include "LimonAPI.h"

void ProfileScope::endZone() {
    if (context) {
        limonAPI->worldEndProfileZone(context);
        context = 0;
    }
}

ProfileScope::~ProfileScope() {
    endZone();
}
