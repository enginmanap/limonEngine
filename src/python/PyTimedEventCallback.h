//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYTIMEDEVENTCALLBACK_H
#define LIMONENGINE_PYTIMEDEVENTCALLBACK_H
#include "limonAPI/LimonTypes.h"
#include "pybind11/gil.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"

// Wrapper class to handle Python callbacks for timed events
class PyTimedEventCallback {
public:
    PyTimedEventCallback(pybind11::function callback) : callback_(std::move(callback)) {}

    void operator()(const std::vector<LimonTypes::GenericParameter>& params) const {
        pybind11::gil_scoped_acquire acquire;
        try {
            // ReSharper disable once CppExpressionWithoutSideEffects
            callback_(params);
        } catch (const std::exception& e) {
            pybind11::print("Error in Python callback:", e.what());
        }
    }

private:
    pybind11::function callback_;
};

#endif //LIMONENGINE_PYTIMEDEVENTCALLBACK_H