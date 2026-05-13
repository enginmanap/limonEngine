//
// Created by engin on 02/02/2026.
//

#ifndef LIMONENGINE_PYACTORINTERFACE_H
#define LIMONENGINE_PYACTORINTERFACE_H
#include "limonAPI/LimonTypes.h"
#include "limonAPI/ActorInterface.h"
#include "GenericParameterConverter.h"


class PyActorInterface : public ActorInterface {
private:
    pybind11::object pyObj;

public:
    PyActorInterface(uint32_t id, LimonAPI* api, pybind11::object obj)
        : ActorInterface(id, api), pyObj(obj) {}

    ~PyActorInterface() override {
        pyObj = pybind11::none();
    }

    [[nodiscard]] std::string getName() const noexcept override {
        try {
            return pyObj.attr("get_name")().cast<std::string>();
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] get_name: " << e.what() << std::endl;
            PyErr_Clear();
            return "<error>";
        }
    }

    void play(long time, ActorInterface::ActorInformation &information) noexcept override {
        try {
            pyObj.attr("play")(time, information);
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] play: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) noexcept override {
        try {
            pybind11::list param_list;
            for (const auto& param : interactionInformation) {
                param_list.append(param);
            }
            return pyObj.attr("interaction")(param_list).cast<bool>();
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] interaction: " << e.what() << std::endl;
            PyErr_Clear();
            return false;
        }
    }

    std::vector<LimonTypes::GenericParameter> getParameters() const noexcept override {
        try {
            return GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_parameters")());
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] get_parameters: " << e.what() << std::endl;
            PyErr_Clear();
            return {};
        }
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) noexcept override {
        try {
            pybind11::list param_list;
            for (const auto& param : parameters) {
                param_list.append(param);
            }
            pyObj.attr("set_parameters")(param_list);
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] set_parameters: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }
};

#endif //LIMONENGINE_PYACTORINTERFACE_H
