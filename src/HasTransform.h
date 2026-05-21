//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_HASTRANSFORM_H
#define LIMONENGINE_HASTRANSFORM_H

#include "Transformation.h"

class HasTransform {
public:
    virtual Transformation* getTransformation() = 0;
    virtual const Transformation* getTransformation() const = 0;
    virtual ~HasTransform() = default;
};

#endif //LIMONENGINE_HASTRANSFORM_H
