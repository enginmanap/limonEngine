//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYCAMERAATTACHMENT_H
#define LIMONENGINE_PYCAMERAATTACHMENT_H
#include <glm/vec3.hpp>

#include "limonAPI/CameraAttachment.h"
#include "pybind11/pytypes.h"


class PyCameraAttachment : public CameraAttachment {
private:
    pybind11::object pyObj;
    LimonAPI* limonAPI;  // We'll add this
public:
    explicit PyCameraAttachment(LimonAPI* limonAPI, pybind11::object obj)
        : pyObj(obj), limonAPI(limonAPI) {}

    bool isDirty() const override {
        return pyObj.attr("isDirty")().cast<bool>();
    }

    void clearDirty() override {
        pyObj.attr("clearDirty")().cast<void>();
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right)  override {
        //std::cout << "get parameters as " << position.x << ", " << position.y << ", " << position.z << " " << std::endl;
        try {
            // Create dictionaries to hold the values
            pybind11::dict pyPosition;
            pyPosition["x"] = position.x;
            pyPosition["y"] = position.y;
            pyPosition["z"] = position.z;

            pybind11::dict pyCenter;
            pyCenter["x"] = center.x;
            pyCenter["y"] = center.y;
            pyCenter["z"] = center.z;

            pybind11::dict pyUp;
            pyUp["x"] = up.x;
            pyUp["y"] = up.y;
            pyUp["z"] = up.z;

            pybind11::dict pyRight;
            pyRight["x"] = right.x;
            pyRight["y"] = right.y;
            pyRight["z"] = right.z;

            // Call the Python function
            pyObj.attr("getCameraVariables")(pyPosition, pyCenter, pyUp, pyRight);

            // Update the C++ references with the modified values
            position.x = pyPosition["x"].cast<float>();
            position.y = pyPosition["y"].cast<float>();
            position.z = pyPosition["z"].cast<float>();

            center.x = pyCenter["x"].cast<float>();
            center.y = pyCenter["y"].cast<float>();
            center.z = pyCenter["z"].cast<float>();

            up.x = pyUp["x"].cast<float>();
            up.y = pyUp["y"].cast<float>();
            up.z = pyUp["z"].cast<float>();

            right.x = pyRight["x"].cast<float>();
            right.y = pyRight["y"].cast<float>();
            right.z = pyRight["z"].cast<float>();
        } catch (const std::exception& e) {
            std::cerr << "Error in getCameraVariables: " << e.what() << std::endl;
            throw;
        }
        //std::cout << "returning as parameters as " << position.x << ", " << position.y << ", " << position.z << " " << std::endl;
        return;
    }
};


#endif //LIMONENGINE_PYCAMERAATTACHMENT_H