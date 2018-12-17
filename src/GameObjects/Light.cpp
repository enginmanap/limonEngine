//
// Created by engin on 18.06.2016.
//

#include <glm/ext.hpp>
#include "Light.h"

void Light::calculateActiveDistance() {
    /*
     * to cut off at 0.1,
     * for a = const, b = linear, c = exp attenuation
     * c*d^2 + b*d + a = 1000;
     *
     * since we want 10, we should calculate for (a - 1000)
     */

    //calculate discriminant
    //b^2 - 4*a*c

    if(attenuation.z == 0) {
        if(attenuation.y == 0) {
            activeDistance = 500;//max
        } else {
            //z = 0 means this is not a second degree equation. handle it
            // mx + n = y
            // when y < sqrt(1000) is active limit
            activeDistance = (sqrt(1000) - attenuation.x) / attenuation.y;
        }
    } else {
        std::cout << "for " << glm::to_string(attenuation);
        float discriminant = attenuation.y * attenuation.y - (4 * (attenuation.x - 1000) * attenuation.z);
        if (discriminant < 0) {
            activeDistance = 0;
        } else {
            activeDistance = (-1 * attenuation.y);
            if (activeDistance > discriminant) {
                activeDistance = activeDistance - std::sqrt(discriminant);
            } else {
                activeDistance = activeDistance + std::sqrt(discriminant);
            }

            activeDistance = activeDistance / (2 * attenuation.z);
        }
    }
}

