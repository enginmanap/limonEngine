//
// Created by engin on 28/12/2025.
//

#ifndef LIMONENGINE_PYTRIGGERINTERFACE_H
#define LIMONENGINE_PYTRIGGERINTERFACE_H
#include "limonAPI/LimonAPI.h"
#include "limonAPI/TriggerInterface.h"
#include "pybind11/cast.h"
#include "pybind11/pytypes.h"


class PyTriggerInterface : public TriggerInterface {
private:
    pybind11::object pyObj;

public:
    PyTriggerInterface(LimonAPI* api, pybind11::object obj)
        : TriggerInterface(api), pyObj(obj) {
        pyObj.attr("_limon_api") = pybind11::cast(api);
    }

    std::vector<LimonTypes::GenericParameter> getParameters() override {
        pybind11::list pyParams = pyObj.attr("get_parameters")();
        std::vector<LimonTypes::GenericParameter> params;
        params.reserve(pybind11::len(pyParams));

        for (auto item : pyParams) {
            params.push_back(item.cast<LimonTypes::GenericParameter>());
        }
        return params;
    }

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override {
        return pyObj.attr("run")(parameters).cast<bool>();
    }

    std::vector<LimonTypes::GenericParameter> getResults() override {
        return pyObj.attr("get_results")().cast<std::vector<LimonTypes::GenericParameter>>();
    }

    std::string getName() const override {
        return pyObj.attr("get_name")().cast<std::string>();
    }
};


#endif //LIMONENGINE_PYTRIGGERINTERFACE_H