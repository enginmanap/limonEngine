//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#define LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#include <iostream>
#include "limonAPI/InputStates.h"
#include "limonAPI/LimonTypes.h"
#include "limonAPI/PlayerExtensionInterface.h"


class PyPlayerExtensionInterface : public PlayerExtensionInterface {
private:
    pybind11::object pyObj;

public:
    PyPlayerExtensionInterface(LimonAPI* api, pybind11::object obj)
        : PlayerExtensionInterface(api), pyObj(obj) {}

    ~PyPlayerExtensionInterface() override {
        pyObj = pybind11::none();
    }

    void processInput(const InputStates& input, const PlayerInformation& playerInfo, long time) noexcept override {
        try {
            pyObj.attr("process_input")(input, playerInfo, time);
        } catch (const std::exception& e) {
            std::cerr << "[PyPlayer] process_input: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    void interact(std::vector<LimonTypes::GenericParameter>& interactionData) noexcept override {
        try {
            pyObj.attr("interact")(interactionData);
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
};

#endif //LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
