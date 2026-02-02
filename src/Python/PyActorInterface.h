//
// Created by engin on 02/02/2026.
//

#ifndef LIMONENGINE_PYACTORINTERFACE_H
#define LIMONENGINE_PYACTORINTERFACE_H
#include "limonAPI/LimonTypes.h"
#include "limonAPI/ActorInterface.h"


class PyActorInterface : public ActorInterface {
private:
    pybind11::object pyObj;

public:
    PyActorInterface(uint32_t id, LimonAPI* api, pybind11::object obj)
        : ActorInterface(id, api), pyObj(obj) {}

    std::string getName() const override {
        return pyObj.attr("get_name")().cast<std::string>();
    }

    void play(long time, ActorInterface::ActorInformation &information) override {
        pyObj.attr("play")(time, information);
    }

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) override {
        return pyObj.attr("interaction")(interactionInformation).cast<bool>();
    }

    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        return pyObj.attr("get_parameters")().cast<std::vector<LimonTypes::GenericParameter>>();
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override {
        pyObj.attr("set_parameters")(parameters);
    }
};

#endif //LIMONENGINE_PYACTORINTERFACE_H
