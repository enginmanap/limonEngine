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
        pyObj = pybind11::none(); // Release Python reference
    }

    std::string getName() const override {
        return pyObj.attr("get_name")().cast<std::string>();
    }

    void play(long time, ActorInterface::ActorInformation &information) override {
        pyObj.attr("play")(time, information);
    }

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) override {
        pybind11::list param_list;
        for (const auto& param : interactionInformation) {
            param_list.append(param);
        }
        return pyObj.attr("interaction")(param_list).cast<bool>();
    }

    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        try {
            pybind11::object py_result = pyObj.attr("get_parameters")();
            return GenericParameterConverter::convertPythonListToGenericParameterVector(py_result);
        } catch (const std::exception& e) {
            std::cerr << "Error in PyActorInterface::getParameters: " << e.what() << std::endl;
            return std::vector<LimonTypes::GenericParameter>();
        }
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override {
        pybind11::list param_list;
        for (const auto& param : parameters) {
            param_list.append(param);
        }
        pyObj.attr("set_parameters")(param_list);
    }
};

#endif //LIMONENGINE_PYACTORINTERFACE_H
