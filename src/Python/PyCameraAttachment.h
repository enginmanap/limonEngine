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

    ~PyCameraAttachment() override {
        pyObj = pybind11::none(); // Release Python reference
    }

    bool isDirty() const override {
        return pyObj.attr("isDirty")().cast<bool>();
    }

    void clearDirty() override {
        pyObj.attr("clearDirty")().cast<void>();
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right)  override {
        try {
            // Import Vec3 module and convert glm::vec3 to Vec3 objects
            pybind11::module_ vec3_module = pybind11::module_::import("vec3");
            pybind11::object Vec3Class = vec3_module.attr("Vec3");
            
            // Convert glm::vec3 to Vec3 objects
            pybind11::object pyPosition = Vec3Class(position.x, position.y, position.z);
            pybind11::object pyCenter = Vec3Class(center.x, center.y, center.z);
            pybind11::object pyUp = Vec3Class(up.x, up.y, up.z);
            pybind11::object pyRight = Vec3Class(right.x, right.y, right.z);

            pyObj.attr("getCameraVariables")(pyPosition, pyCenter, pyUp, pyRight);
            
            // Use type caster to convert Vec3 objects back to glm::vec3
            position = pyPosition.cast<glm::vec3>();
            center = pyCenter.cast<glm::vec3>();
            up = pyUp.cast<glm::vec3>();
            right = pyRight.cast<glm::vec3>();
            
        } catch (const std::exception& e) {
            std::cerr << "Error in getCameraVariables: " << e.what() << std::endl;
            throw;
        }
    }
};


#endif //LIMONENGINE_PYCAMERAATTACHMENT_H
