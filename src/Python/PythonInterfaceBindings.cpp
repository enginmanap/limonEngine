#include <Python.h>
#include "PythonInterfaceBindings.h"
#include "PythonTypeCasters.h"
#include <pybind11/embed.h>
#include <limonAPI/LimonAPI.h>
#include <limonAPI/TriggerInterface.h>
#include <limonAPI/ActorInterface.h>
#include <limonAPI/PlayerExtensionInterface.h>
#include "PyCameraAttachment.h"
#include "PyPlayerExtensionInterface.h"
#include "PyActorInterface.h"
#include "PyTriggerInterface.h"
#include "SDL2Helper.h"
#include "PythonStdOut.h"
#include "GenericParameterConverter.h"

void bindInterfaces(pybind11::module_& m) {
    // Bind InputStates::Inputs enum
    pybind11::enum_<InputStates::Inputs>(m, "Inputs")
            .value("QUIT", InputStates::Inputs::QUIT)
            .value("MOUSE_MOVE", InputStates::Inputs::MOUSE_MOVE)
            .value("MOUSE_BUTTON_LEFT", InputStates::Inputs::MOUSE_BUTTON_LEFT)
            .value("MOUSE_BUTTON_MIDDLE", InputStates::Inputs::MOUSE_BUTTON_MIDDLE)
            .value("MOUSE_BUTTON_RIGHT", InputStates::Inputs::MOUSE_BUTTON_RIGHT)
            .value("MOUSE_WHEEL_UP", InputStates::Inputs::MOUSE_WHEEL_UP)
            .value("MOUSE_WHEEL_DOWN", InputStates::Inputs::MOUSE_WHEEL_DOWN)
            .value("MOVE_FORWARD", InputStates::Inputs::MOVE_FORWARD)
            .value("MOVE_BACKWARD", InputStates::Inputs::MOVE_BACKWARD)
            .value("MOVE_LEFT", InputStates::Inputs::MOVE_LEFT)
            .value("MOVE_RIGHT", InputStates::Inputs::MOVE_RIGHT)
            .value("JUMP", InputStates::Inputs::JUMP)
            .value("RUN", InputStates::Inputs::RUN)
            .value("DEBUG", InputStates::Inputs::DEBUG)
            .value("EDITOR", InputStates::Inputs::EDITOR)
            .value("KEY_SHIFT", InputStates::Inputs::KEY_SHIFT)
            .value("KEY_CTRL", InputStates::Inputs::KEY_CTRL)
            .value("KEY_ALT", InputStates::Inputs::KEY_ALT)
            .value("KEY_SUPER", InputStates::Inputs::KEY_SUPER)
            .value("TEXT_INPUT", InputStates::Inputs::TEXT_INPUT)
            .value("NUMBER_1", InputStates::Inputs::NUMBER_1)
            .value("NUMBER_2", InputStates::Inputs::NUMBER_2)
            .value("F4", InputStates::Inputs::F4)
            .export_values();

    // Bind InputStates class
    pybind11::class_<InputStates>(m, "InputStates")
            .def(pybind11::init<>())
            .def("set_input_status", &InputStates::setInputStatus)
            .def("get_input_status", &InputStates::getInputStatus)
            .def("get_input_events", &InputStates::getInputEvents)
            .def("get_text", &InputStates::getText)
            .def("set_text", &InputStates::setText)
            .def("reset_all_events", &InputStates::resetAllEvents)
            .def("get_mouse_change", [](const InputStates &self) {
                float xPos, yPos, xChange, yChange;
                bool changed = self.getMouseChange(xPos, yPos, xChange, yChange);
                return std::make_tuple(changed, xPos, yPos, xChange, yChange);
            })
            .def("set_mouse_change", &InputStates::setMouseChange)
            .def_readonly_static("KEY_BUFFER_ELEMENTS", &InputStates::keyBufferElements);

    // Bind PlayerExtensionInterface::PlayerInformation
    pybind11::class_<PlayerExtensionInterface::PlayerInformation>(m, "PlayerInformation")
            .def(pybind11::init<>())
            .def_readwrite("position", &PlayerExtensionInterface::PlayerInformation::position)
            .def_readwrite("look_direction", &PlayerExtensionInterface::PlayerInformation::lookDirection);

    // Bind CameraAttachment
    pybind11::class_<CameraAttachment, PyCameraAttachment>(m, "CameraAttachment")
            .def(pybind11::init([](LimonAPI *api) {
                pybind11::object pyClass = pybind11::module_::import("camera_attachment").attr("CameraAttachment");
                pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
                return new PyCameraAttachment(api, pyInstance);
            }))
            .def("is_dirty", &CameraAttachment::isDirty)
            .def("clear_dirty", &CameraAttachment::clearDirty)
            .def("get_camera_variables", [](CameraAttachment &self) {
                glm::vec3 pos, center, up, right;
                self.getCameraVariables(pos, center, up, right);
                return pybind11::make_tuple(pos, center, up, right);
            });

    pybind11::class_<TriggerInterface, PyTriggerInterface>(m, "TriggerInterface")
            .def(pybind11::init([](LimonAPI *api) {
                pybind11::object pyClass = pybind11::module_::import("trigger_interface").attr("TriggerInterface");
                pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
                return new PyTriggerInterface(api, pyInstance);
            }))
            .def("get_parameters", &TriggerInterface::getParameters)
            .def("run", &TriggerInterface::run)
            .def("get_results", &TriggerInterface::getResults)
            .def("get_name", &TriggerInterface::getName);

    pybind11::class_<PlayerExtensionInterface, PyPlayerExtensionInterface>(m, "PlayerExtensionInterface")
        .def(pybind11::init([](LimonAPI *api) {
            pybind11::object pyClass = pybind11::module_::import("player_extension_interface").attr("PlayerExtensionInterface");
            pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
            return new PyPlayerExtensionInterface(api, pyInstance);
        }))
            .def("process_input", &PlayerExtensionInterface::processInput)
            .def("interact", &PlayerExtensionInterface::interact)
            .def("get_name", &PlayerExtensionInterface::getName)
            .def("get_custom_camera_attachment", &PlayerExtensionInterface::getCustomCameraAttachment,
                 pybind11::return_value_policy::reference)
            .def_static("create_extension", [](const std::string &name, LimonAPI *api) -> PlayerExtensionInterface * {
                PlayerExtensionInterface* ptr = PlayerExtensionInterface::createExtension(name, api);
                if (!ptr) {
                    throw std::runtime_error("Failed to create extension: " + name);
                }
                return ptr;
            }, pybind11::return_value_policy::reference);

    // Bind ActorInterface::ActorInformation
    pybind11::class_<ActorInterface::ActorInformation>(m, "ActorInformation")
            .def(pybind11::init<>())
            .def_readwrite("can_see_player_directly", &ActorInterface::ActorInformation::canSeePlayerDirectly)
            .def_readwrite("is_player_left", &ActorInterface::ActorInformation::isPlayerLeft)
            .def_readwrite("is_player_right", &ActorInterface::ActorInformation::isPlayerRight)
            .def_readwrite("is_player_up", &ActorInterface::ActorInformation::isPlayerUp)
            .def_readwrite("is_player_down", &ActorInterface::ActorInformation::isPlayerDown)
            .def_readwrite("is_player_front", &ActorInterface::ActorInformation::isPlayerFront)
            .def_readwrite("is_player_back", &ActorInterface::ActorInformation::isPlayerBack)
            .def_readwrite("cosine_between_player", &ActorInterface::ActorInformation::cosineBetweenPlayer)
            .def_readwrite("player_direction", &ActorInterface::ActorInformation::playerDirection)
            .def_readwrite("player_distance", &ActorInterface::ActorInformation::playerDistance)
            .def_readwrite("cosine_between_player_for_side", &ActorInterface::ActorInformation::cosineBetweenPlayerForSide)
            .def_readwrite("route_to_request", &ActorInterface::ActorInformation::routeToRequest)
            .def_readwrite("maximum_route_distance", &ActorInterface::ActorInformation::maximumRouteDistance)
            .def_readwrite("route_found", &ActorInterface::ActorInformation::routeFound)
            .def_readwrite("route_ready", &ActorInterface::ActorInformation::routeReady)
            .def_readwrite("player_dead", &ActorInterface::ActorInformation::playerDead);

    pybind11::class_<ActorInterface, PyActorInterface>(m, "ActorInterface")
        .def(pybind11::init([](uint32_t id, LimonAPI *api) {
            pybind11::object pyClass = pybind11::module_::import("actor_interface").attr("ActorInterface");
            pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
            return new PyActorInterface(id, api, pyInstance);
        }))
            .def("get_name", &ActorInterface::getName)
            .def("play", &ActorInterface::play)
            .def("interaction", &ActorInterface::interaction)
            .def("get_parameters", [](ActorInterface& self) -> std::vector<LimonTypes::GenericParameter> {
                pybind11::object py_result = pybind11::cast(&self).attr("get_parameters")();
                return GenericParameterConverter::convertPythonListToGenericParameterVector(py_result);
            }, "Get configurable parameters for this actor")
            .def("set_parameters", [](ActorInterface& self, pybind11::object pyParams) {
                std::vector<LimonTypes::GenericParameter> params = GenericParameterConverter::convertPythonListToGenericParameterVector(pyParams);
                self.setParameters(params);
            })
            .def_static("create_actor", [](const std::string &name, uint32_t id, LimonAPI *api) -> ActorInterface * {
                ActorInterface* ptr = ActorInterface::createActor(name, id, api);
                if (!ptr) {
                    throw std::runtime_error("Failed to create actor: " + name);
                }
                return ptr;
            }, pybind11::return_value_policy::reference);

    // Plugin registration functions
    m.def("register_trigger_type", &TriggerInterface::registerType);
    m.def("register_extension_type", &PlayerExtensionInterface::registerType);
    m.def("get_trigger_names", &TriggerInterface::getTriggerNames);
    m.def("get_extension_names", &PlayerExtensionInterface::getTriggerNames);
    m.def("register_actor_type", &ActorInterface::registerType);
    m.def("get_actor_names", &ActorInterface::getActorNames);

    // Binding for std::out
    pybind11::class_<PythonStdOut>(m, "StdOut")
            .def(pybind11::init<>())
            .def("write", &PythonStdOut::write)
            .def("flush", &PythonStdOut::flush);
}
