//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#define LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#include <iostream>
#include "limonAPI/InputStates.h"
#include "limonAPI/LimonTypes.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "GenericParameterConverter.h"


class PyPlayerExtensionInterface : public PlayerExtensionInterface {
private:
    pybind11::object pyObj;

public:
    PyPlayerExtensionInterface(LimonAPI* api, pybind11::object obj)
        : PlayerExtensionInterface(api), pyObj(obj) {}

    ~PyPlayerExtensionInterface() override {
        pyObj = pybind11::none();
    }

    void processInput(const InputStates& input, const PlayerInformation& playerInfo, uint32_t time) noexcept override {
        try {
            pyObj.attr("process_input")(input, playerInfo, time);
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] process_input: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    void interact(std::vector<LimonTypes::GenericParameter>& interactionData) noexcept override {
        try {
            pyObj.attr("interact")(GenericParameterConverter::convertGenericParameterVectorToObjects(interactionData));
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] interact: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    [[nodiscard]] std::string getName() const noexcept override {
        try {
            return pyObj.attr("get_name")().cast<std::string>();
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] get_name: " << e.what() << std::endl;
            PyErr_Clear();
            return "<error>";
        }
    }

    std::vector<LimonTypes::GenericParameter> getParameters() const noexcept override {
        try {
            return GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_parameters")());
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] get_parameters: " << e.what() << std::endl;
            PyErr_Clear();
            return {};
        }
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) noexcept override {
        try {
            pyObj.attr("set_parameters")(GenericParameterConverter::convertGenericParameterVectorToObjects(parameters));
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] set_parameters: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }
};

#endif //LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
