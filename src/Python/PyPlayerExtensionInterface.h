//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#define LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H
#include "PyCameraAttachment.h"
#include "limonAPI/InputStates.h"
#include "limonAPI/LimonTypes.h"
#include "limonAPI/PlayerExtensionInterface.h"


class PyPlayerExtensionInterface : public PlayerExtensionInterface {
private:
    pybind11::object pyObj;

public:
    PyPlayerExtensionInterface(LimonAPI* api, pybind11::object obj)
        : PlayerExtensionInterface(api), pyObj(obj) {}

    void processInput(const InputStates& input, const PlayerInformation& playerInfo, long time) override {
        pyObj.attr("process_input")(input, playerInfo, time);
    }

    void interact(std::vector<LimonTypes::GenericParameter>& interactionData) override {
        pyObj.attr("interact")(interactionData);
    }

    std::string getName() const override {
        return pyObj.attr("get_name")().cast<std::string>();
    }

    CameraAttachment* getCustomCameraAttachment() override {
        pybind11::object pyCam = pyObj.attr("get_custom_camera_attachment")();
        if (pyCam.is_none()) {
            return nullptr;
        }
        // You'll need to implement PyCameraAttachment similarly
    return new PyCameraAttachment(limonAPI, pyCam);
    }
};

#endif //LIMONENGINE_PYPLAYEREXTENSIONINTERFACE_H