#include "ProfileScope.h"

void ProfileScope::endZone() {
    if (context) {
        endZoneFunc(context);
        context = 0;
    }
}

ProfileScope::~ProfileScope() {
    endZone();
}
