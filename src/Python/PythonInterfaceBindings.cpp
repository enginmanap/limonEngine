#include <Python.h>
#include "PythonInterfaceBindings.h"
#include "PythonTypeCasters.h"
#include <pybind11/embed.h>
#include <limonAPI/LimonAPI.h>
#include <limonAPI/TriggerInterface.h>
#include <limonAPI/ActorInterface.h>
#include <limonAPI/PlayerExtensionInterface.h>
#include <limonAPI/CameraExtensionInterface.h>
#include "PyCameraExtensionInterface.h"
#include "PyPlayerExtensionInterface.h"
#include "PyActorInterface.h"
#include "PyTriggerInterface.h"
#include "SDL2Helper.h"
#include "PythonStdOut.h"
#include "GenericParameterConverter.h"

// Plain struct so pybind11 can bind InputActions constants as static readonly attributes.
struct InputActionsExport {
    static const uint64_t QUIT;
    static const uint64_t MOUSE_MOVE;
    static const uint64_t MOUSE_BUTTON_LEFT;
    static const uint64_t MOUSE_BUTTON_MIDDLE;
    static const uint64_t MOUSE_BUTTON_RIGHT;
    static const uint64_t MOUSE_WHEEL_UP;
    static const uint64_t MOUSE_WHEEL_DOWN;
    static const uint64_t MOVE_FORWARD;
    static const uint64_t MOVE_BACKWARD;
    static const uint64_t MOVE_LEFT;
    static const uint64_t MOVE_RIGHT;
    static const uint64_t JUMP;
    static const uint64_t RUN;
    static const uint64_t DEBUG_MODE;
    static const uint64_t EDITOR;
    static const uint64_t KEY_SHIFT;
    static const uint64_t KEY_CTRL;
    static const uint64_t KEY_ALT;
    static const uint64_t KEY_SUPER;
    static const uint64_t TEXT_INPUT;
    static const uint64_t NUMBER_1;
    static const uint64_t NUMBER_2;
    static const uint64_t F4;
    static const uint64_t F5;
    static const uint64_t LOOK_X;
    static const uint64_t LOOK_Y;
};

const uint64_t InputActionsExport::QUIT               = InputActions::QUIT;
const uint64_t InputActionsExport::MOUSE_MOVE         = InputActions::MOUSE_MOVE;
const uint64_t InputActionsExport::MOUSE_BUTTON_LEFT  = InputActions::MOUSE_BUTTON_LEFT;
const uint64_t InputActionsExport::MOUSE_BUTTON_MIDDLE= InputActions::MOUSE_BUTTON_MIDDLE;
const uint64_t InputActionsExport::MOUSE_BUTTON_RIGHT = InputActions::MOUSE_BUTTON_RIGHT;
const uint64_t InputActionsExport::MOUSE_WHEEL_UP     = InputActions::MOUSE_WHEEL_UP;
const uint64_t InputActionsExport::MOUSE_WHEEL_DOWN   = InputActions::MOUSE_WHEEL_DOWN;
const uint64_t InputActionsExport::MOVE_FORWARD       = InputActions::MOVE_FORWARD;
const uint64_t InputActionsExport::MOVE_BACKWARD      = InputActions::MOVE_BACKWARD;
const uint64_t InputActionsExport::MOVE_LEFT          = InputActions::MOVE_LEFT;
const uint64_t InputActionsExport::MOVE_RIGHT         = InputActions::MOVE_RIGHT;
const uint64_t InputActionsExport::JUMP               = InputActions::JUMP;
const uint64_t InputActionsExport::RUN                = InputActions::RUN;
const uint64_t InputActionsExport::DEBUG_MODE         = InputActions::DEBUG_MODE;
const uint64_t InputActionsExport::EDITOR             = InputActions::EDITOR;
const uint64_t InputActionsExport::KEY_SHIFT          = InputActions::KEY_SHIFT;
const uint64_t InputActionsExport::KEY_CTRL           = InputActions::KEY_CTRL;
const uint64_t InputActionsExport::KEY_ALT            = InputActions::KEY_ALT;
const uint64_t InputActionsExport::KEY_SUPER          = InputActions::KEY_SUPER;
const uint64_t InputActionsExport::TEXT_INPUT         = InputActions::TEXT_INPUT;
const uint64_t InputActionsExport::NUMBER_1           = InputActions::NUMBER_1;
const uint64_t InputActionsExport::NUMBER_2           = InputActions::NUMBER_2;
const uint64_t InputActionsExport::F4                 = InputActions::F4;
const uint64_t InputActionsExport::F5                 = InputActions::F5;
const uint64_t InputActionsExport::LOOK_X             = InputActions::LOOK_X;
const uint64_t InputActionsExport::LOOK_Y             = InputActions::LOOK_Y;

void bindInterfaces(pybind11::module_& m) {
    // Expose InputActions hash constants to Python as a plain class with integer attributes.
    // Python scripts use limon.InputActions.MOVE_FORWARD etc. (uint64_t values).
    pybind11::class_<InputActionsExport>(m, "InputActions")
            .def_readonly_static("QUIT",                &InputActionsExport::QUIT)
            .def_readonly_static("MOUSE_MOVE",          &InputActionsExport::MOUSE_MOVE)
            .def_readonly_static("MOUSE_BUTTON_LEFT",   &InputActionsExport::MOUSE_BUTTON_LEFT)
            .def_readonly_static("MOUSE_BUTTON_MIDDLE", &InputActionsExport::MOUSE_BUTTON_MIDDLE)
            .def_readonly_static("MOUSE_BUTTON_RIGHT",  &InputActionsExport::MOUSE_BUTTON_RIGHT)
            .def_readonly_static("MOUSE_WHEEL_UP",      &InputActionsExport::MOUSE_WHEEL_UP)
            .def_readonly_static("MOUSE_WHEEL_DOWN",    &InputActionsExport::MOUSE_WHEEL_DOWN)
            .def_readonly_static("MOVE_FORWARD",        &InputActionsExport::MOVE_FORWARD)
            .def_readonly_static("MOVE_BACKWARD",       &InputActionsExport::MOVE_BACKWARD)
            .def_readonly_static("MOVE_LEFT",           &InputActionsExport::MOVE_LEFT)
            .def_readonly_static("MOVE_RIGHT",          &InputActionsExport::MOVE_RIGHT)
            .def_readonly_static("JUMP",                &InputActionsExport::JUMP)
            .def_readonly_static("RUN",                 &InputActionsExport::RUN)
            .def_readonly_static("DEBUG_MODE",          &InputActionsExport::DEBUG_MODE)
            .def_readonly_static("EDITOR",              &InputActionsExport::EDITOR)
            .def_readonly_static("KEY_SHIFT",           &InputActionsExport::KEY_SHIFT)
            .def_readonly_static("KEY_CTRL",            &InputActionsExport::KEY_CTRL)
            .def_readonly_static("KEY_ALT",             &InputActionsExport::KEY_ALT)
            .def_readonly_static("KEY_SUPER",           &InputActionsExport::KEY_SUPER)
            .def_readonly_static("TEXT_INPUT",          &InputActionsExport::TEXT_INPUT)
            .def_readonly_static("NUMBER_1",            &InputActionsExport::NUMBER_1)
            .def_readonly_static("NUMBER_2",            &InputActionsExport::NUMBER_2)
            .def_readonly_static("F4",                  &InputActionsExport::F4)
            .def_readonly_static("F5",                  &InputActionsExport::F5)
            .def_readonly_static("LOOK_X",              &InputActionsExport::LOOK_X)
            .def_readonly_static("LOOK_Y",              &InputActionsExport::LOOK_Y);

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
            .def("get_analog_value", &InputStates::getAnalogValue)
            .def("set_analog_value", &InputStates::setAnalogValue);

    // Bind PlayerExtensionInterface::PlayerInformation
    pybind11::class_<PlayerExtensionInterface::PlayerInformation>(m, "PlayerInformation")
            .def(pybind11::init<>())
            .def_readwrite("position", &PlayerExtensionInterface::PlayerInformation::position)
            .def_readwrite("look_direction", &PlayerExtensionInterface::PlayerInformation::lookDirection);

    // Bind CameraAttachment projection descriptor
    pybind11::enum_<CameraAttachment::ProjectionType>(m, "ProjectionType")
            .value("PERSPECTIVE", CameraAttachment::ProjectionType::PERSPECTIVE)
            .value("ORTHOGRAPHIC", CameraAttachment::ProjectionType::ORTHOGRAPHIC);

    pybind11::class_<CameraAttachment::ProjectionParameters>(m, "ProjectionParameters")
            .def(pybind11::init<>())
            .def_readwrite("type", &CameraAttachment::ProjectionParameters::type)
            .def_readwrite("vertical_field_of_view", &CameraAttachment::ProjectionParameters::verticalFieldOfView)
            .def_readwrite("orthographic_half_height", &CameraAttachment::ProjectionParameters::orthographicHalfHeight)
            .def_readwrite("near_plane", &CameraAttachment::ProjectionParameters::nearPlane)
            .def_readwrite("far_plane", &CameraAttachment::ProjectionParameters::farPlane);

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

    // Bind CameraExtensionInterface (registered, configurable camera rig) - full parity with C++.
    pybind11::class_<CameraExtensionInterface, PyCameraExtensionInterface>(m, "CameraExtensionInterface")
            .def(pybind11::init([](LimonAPI *api) {
                pybind11::object pyClass = pybind11::module_::import("camera_extension_interface").attr("CameraExtensionInterface");
                pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
                return new PyCameraExtensionInterface(api, pyInstance);
            }))
            .def("get_name", &CameraExtensionInterface::getName)
            .def("get_parameters", [](CameraExtensionInterface& self) -> std::vector<LimonTypes::GenericParameter> {
                pybind11::object py_result = pybind11::cast(&self).attr("get_parameters")();
                return GenericParameterConverter::convertPythonListToGenericParameterVector(py_result);
            }, "Get configurable parameters for this camera rig")
            .def("set_parameters", [](CameraExtensionInterface& self, pybind11::object pyParams) {
                std::vector<LimonTypes::GenericParameter> params = GenericParameterConverter::convertPythonListToGenericParameterVector(pyParams);
                self.setParameters(params);
            })
            .def("is_dirty", &CameraExtensionInterface::isDirty)
            .def("clear_dirty", &CameraExtensionInterface::clearDirty)
            .def("get_projection", &CameraExtensionInterface::getProjection)
            .def("get_camera_variables", [](CameraExtensionInterface &self) {
                glm::vec3 pos, center, up, right;
                self.getCameraVariables(pos, center, up, right);
                return pybind11::make_tuple(pos, center, up, right);
            })
            .def_static("create_extension", [](const std::string &name, LimonAPI *api) -> CameraExtensionInterface * {
                CameraExtensionInterface* ptr = CameraExtensionInterface::createExtension(name, api);
                if (!ptr) {
                    throw std::runtime_error("Failed to create camera rig: " + name);
                }
                return ptr;
            }, pybind11::return_value_policy::reference);

    // Plugin registration functions
    m.def("register_trigger_type", &TriggerInterface::registerType);
    m.def("register_extension_type", &PlayerExtensionInterface::registerType);
    m.def("get_trigger_names", &TriggerInterface::getTriggerNames);
    m.def("get_extension_names", &PlayerExtensionInterface::getExtensionNames);
    m.def("register_actor_type", &ActorInterface::registerType);
    m.def("get_actor_names", &ActorInterface::getActorNames);
    m.def("register_camera_extension_type", &CameraExtensionInterface::registerType);
    m.def("get_camera_extension_names", &CameraExtensionInterface::getExtensionNames);

    // Binding for std::out
    pybind11::class_<PythonStdOut>(m, "StdOut")
            .def(pybind11::init<>())
            .def("write", &PythonStdOut::write)
            .def("flush", &PythonStdOut::flush);
}
