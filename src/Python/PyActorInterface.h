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

    // The engine assigns the model id through the non-virtual ActorInterface::setModel, which fills
    // this->modelID on this instance but never reaches Python. setModel isn't a Python call, so we
    // can't forward it there; instead the wrapper writes the value straight onto the Python object's
    // model_id attribute at every entry point that crosses into Python. No setter method is exposed,
    // and there is no back-reference from Python to this wrapper.
    void forwardModelID() const {
        pyObj.attr("model_id") = this->modelID;
    }

    void play(long time, ActorInterface::ActorInformation &information) noexcept override {
        try {
            forwardModelID();
            pyObj.attr("play")(time, information);
            // The engine reads route requests through the non-virtual ActorInterface::getRequests(),
            // which inspects this->informationRequest. A C++ actor writes that member directly; a
            // Python actor instead sets flags on its own information_request, so copy them across here.
            pybind11::object request = pyObj.attr("information_request");
            this->informationRequest.routeToPlayer = request.attr("route_to_player").cast<bool>();
            this->informationRequest.routeToCustomPosition = request.attr("route_to_custom_position").cast<bool>();
            if (this->informationRequest.routeToCustomPosition) {
                pybind11::object customPosition = request.attr("custom_position");
                this->informationRequest.customPosition = glm::vec3(
                        customPosition.attr("x").cast<float>(),
                        customPosition.attr("y").cast<float>(),
                        customPosition.attr("z").cast<float>());
            }
            // A request lives for exactly one tick (getRequests() clears the C++ member after the
            // engine consumes it); clear the Python side too so the actor must re-raise it next tick.
            request.attr("route_to_player") = false;
            request.attr("route_to_custom_position") = false;
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] play: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) noexcept override {
        try {
            forwardModelID();
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
            forwardModelID();
            return GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_parameters")());
        } catch (const std::exception& e) {
            std::cerr << "[PyActor] get_parameters: " << e.what() << std::endl;
            PyErr_Clear();
            return {};
        }
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) noexcept override {
        try {
            forwardModelID();
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
