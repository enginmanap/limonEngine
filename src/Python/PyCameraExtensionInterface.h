//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_PYCAMERAEXTENSIONINTERFACE_H
#define LIMONENGINE_PYCAMERAEXTENSIONINTERFACE_H
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "limonAPI/CameraExtensionInterface.h"
#include "GenericParameterConverter.h"
#include "PythonTypeCasters.h"
#include "pybind11/pytypes.h"

/**
 * Trampoline that lets a Python class implement a registered camera rig (CameraExtensionInterface),
 * exactly like PyTriggerInterface / PyActorInterface / PyPlayerExtensionInterface do for their interfaces.
 * It forwards every virtual to the wrapped Python object so Python rigs have full parity with C++ rigs:
 * registration, GenericParameter editing, projection (perspective/orthographic), and the attachment bridge.
 */
class PyCameraExtensionInterface : public CameraExtensionInterface {
private:
    pybind11::object pyObj;

public:
    PyCameraExtensionInterface(LimonAPI* api, pybind11::object obj)
        : CameraExtensionInterface(api), pyObj(obj) {}

    ~PyCameraExtensionInterface() override {
        pyObj = pybind11::none();
    }

    std::string getName() const override {
        try {
            return pyObj.attr("get_name")().cast<std::string>();
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] get_name: " << e.what() << std::endl;
            PyErr_Clear();
            return "<error>";
        }
    }

    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        try {
            return GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_parameters")());
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] get_parameters: " << e.what() << std::endl;
            PyErr_Clear();
            return {};
        }
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override {
        try {
            pybind11::list param_list;
            for (const auto& param : parameters) {
                param_list.append(param);
            }
            pyObj.attr("set_parameters")(param_list);
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] set_parameters: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    bool isDirty() const override {
        try {
            return pyObj.attr("is_dirty")().cast<bool>();
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] is_dirty: " << e.what() << std::endl;
            PyErr_Clear();
            return false;
        }
    }

    void clearDirty() override {
        try {
            pyObj.attr("clear_dirty")();
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] clear_dirty: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    CameraAttachment::ProjectionParameters getProjection() const override {
        try {
            return pyObj.attr("get_projection")().cast<CameraAttachment::ProjectionParameters>();
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] get_projection: " << e.what() << std::endl;
            PyErr_Clear();
            return CameraAttachment::ProjectionParameters{};
        }
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) override {
        try {
            // Pass mutable Vec3 objects (so Python fills them in place), then read them back.
            pybind11::module_ vec3_module = pybind11::module_::import("vec3");
            pybind11::object Vec3Class = vec3_module.attr("Vec3");
            pybind11::object pyPosition = Vec3Class(position.x, position.y, position.z);
            pybind11::object pyCenter   = Vec3Class(center.x, center.y, center.z);
            pybind11::object pyUp       = Vec3Class(up.x, up.y, up.z);
            pybind11::object pyRight    = Vec3Class(right.x, right.y, right.z);

            pyObj.attr("get_camera_variables")(pyPosition, pyCenter, pyUp, pyRight);

            position = pyPosition.cast<glm::vec3>();
            center   = pyCenter.cast<glm::vec3>();
            up       = pyUp.cast<glm::vec3>();
            right    = pyRight.cast<glm::vec3>();
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] get_camera_variables: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    void setAttachmentTransform(const glm::vec3& position, const glm::quat& orientation,
                                const glm::vec3& scale) override {
        try {
            pybind11::module_ vec3_module = pybind11::module_::import("vec3");
            pybind11::object Vec3Class = vec3_module.attr("Vec3");
            pybind11::object pyPosition = Vec3Class(position.x, position.y, position.z);
            pybind11::object pyScale    = Vec3Class(scale.x, scale.y, scale.z);
            // orientation crosses as a quaternion with .x/.y/.z/.w (via the glm::quat type caster).
            pyObj.attr("set_attachment_transform")(pyPosition, orientation, pyScale);
        } catch (const std::exception& e) {
            std::cerr << "[PyCameraRig] set_attachment_transform: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }
};

#endif //LIMONENGINE_PYCAMERAEXTENSIONINTERFACE_H
