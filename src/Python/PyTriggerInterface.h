//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYTRIGGERINTERFACE_H
#define LIMONENGINE_PYTRIGGERINTERFACE_H
#include "limonAPI/LimonAPI.h"
#include "limonAPI/TriggerInterface.h"
#include "pybind11/cast.h"
#include "pybind11/pytypes.h"
#include "GenericParameterConverter.h"


class PyTriggerInterface : public TriggerInterface {
private:
    pybind11::object pyObj;

public:
    PyTriggerInterface(LimonAPI* api, pybind11::object obj)
        : TriggerInterface(api), pyObj(obj) {
        pyObj.attr("_limon_api") = pybind11::cast(api);
        // Seed the default parameters from the Python implementation at create time.
        // After this, the base getParameters()/setParameters() are the source of truth.
        try {
            this->parameters = GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_parameters")());
        } catch (const std::exception& e) {
            std::cerr << "[PyTrigger] get_parameters: " << e.what() << std::endl;
            PyErr_Clear();
        }
    }

    ~PyTriggerInterface() override {
        pyObj = pybind11::none(); // Release Python reference
    }

    bool run(std::vector<LimonTypes::GenericParameter> parameters) noexcept override {
        try {
            return pyObj.attr("run")(GenericParameterConverter::convertGenericParameterVectorToObjects(parameters)).cast<bool>();
        } catch (const std::exception& e) {
            std::cerr << "[PyTrigger] run: " << e.what() << std::endl;
            PyErr_Clear();
            return false;
        }
    }

    std::vector<LimonTypes::GenericParameter> getResults() noexcept override {
        try {
            return GenericParameterConverter::convertPythonListToGenericParameterVector(pyObj.attr("get_results")());
        } catch (const std::exception& e) {
            std::cerr << "[PyTrigger] get_results: " << e.what() << std::endl;
            PyErr_Clear();
            return {};
        }
    }

    [[nodiscard]] std::string getName() const noexcept override {
        try {
            return pyObj.attr("get_name")().cast<std::string>();
        } catch (const std::exception& e) {
            std::cerr << "[PyTrigger] get_name: " << e.what() << std::endl;
            PyErr_Clear();
            return "<error>";
        }
    }
};


#endif //LIMONENGINE_PYTRIGGERINTERFACE_H