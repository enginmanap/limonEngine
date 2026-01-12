//
// Created by engin on 25/12/2025.
//

#include <Python.h>
#include "ScriptManager.h"

#include "PythonStdOut.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>

#include "PyCameraAttachment.h"
#include "PyPlayerExtensionInterface.h"
#include "PyTimedEventCallback.h"
#include "PyTriggerInterface.h"
#include "SDL2Helper.h"

#ifndef PYTHON_BUNDLE_ZIP
#error "PYTHON_BUNDLE_ZIP must be defined by the build system"
#endif

// Conversion for Vec3
template<>
class pybind11::detail::type_caster<glm::vec3> {
public:
    PYBIND11_TYPE_CASTER(glm::vec3, _("glm::vec3"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::object>(src))
            return false;

        auto obj = pybind11::reinterpret_borrow<pybind11::object>(src);
        if (!pybind11::hasattr(obj, "x") || !pybind11::hasattr(obj, "y") || !pybind11::hasattr(obj, "z"))
            return false;

        value.x = obj.attr("x").cast<float>();
        value.y = obj.attr("y").cast<float>();
        value.z = obj.attr("z").cast<float>();
        return true;
    }

    static handle cast(const glm::vec3 &src, return_value_policy, handle) {
        // Return a dictionary instead of creating a Python object
        pybind11::dict d;
        d["x"] = src.x;
        d["y"] = src.y;
        d["z"] = src.z;
        return d.release();
    }
};

// Conversion for quaternion
template<>
class pybind11::detail::type_caster<glm::quat> {
public:
    PYBIND11_TYPE_CASTER(glm::quat, _("glm::quat"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::object>(src))
            return false;

        auto obj = pybind11::reinterpret_borrow<pybind11::object>(src);
        if (!pybind11::hasattr(obj, "x") || !pybind11::hasattr(obj, "y") ||
            !pybind11::hasattr(obj, "z") || !pybind11::hasattr(obj, "w"))
            return false;

        float x = obj.attr("x").cast<float>();
        float y = obj.attr("y").cast<float>();
        float z = obj.attr("z").cast<float>();
        float w = obj.attr("w").cast<float>();

        value = glm::quat(w, x, y, z);
        return true;
    }

    static handle cast(const glm::quat &src, return_value_policy, handle) {
        // Return a dictionary instead of creating a Python object
        pybind11::dict d;
        d["x"] = src.x;
        d["y"] = src.y;
        d["z"] = src.z;
        d["w"] = src.w;
        return d.release();
    }
};

// Add the type caster for std::vector<unsigned int>
template<>
class pybind11::detail::type_caster<std::vector<unsigned int>> {
public:
    PYBIND11_TYPE_CASTER(std::vector<unsigned int>, _("List[int]"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::sequence>(src))
            return false;

        auto seq = pybind11::reinterpret_borrow<pybind11::sequence>(src);
        value.clear();
        value.reserve(seq.size());

        for (const auto& item : seq) {
            value.push_back(item.cast<unsigned int>());
        }
        return true;
    }

    static handle cast(const std::vector<unsigned int>& src, return_value_policy, handle) {
        pybind11::list list;
        for (const auto& item : src) {
            list.append(item);
        }
        return list.release();
    }
};


namespace {
    // since TriggerInterface and PlayerExtensionInterface have protected constructors, we need to instantiate them in a different way.
    // I tried to use factory pattern but it didn't work. This is some crazy template magic that creates MAX_PYTHON_SCRIPT_COUNT number creators,
    // so you can instantiate them

    template<size_t N>
    TriggerInterface* CreateTriggerByIndex(LimonAPI* api) {
        return ScriptManager::CreateTriggerWrapper(api, N);
    }

    template<size_t N>
    PlayerExtensionInterface* CreatePlayerExtensionByIndex(LimonAPI* api) {
        return ScriptManager::CreatePlayerExtensionWrapper(api, N);
    }
    // Force instantiation of template functions for a specific N
    template<int N>
    struct template_instantiator {
        static void instantiate() {
            // These declarations will force instantiation of the template functions
            TriggerInterface* (*trigger_func)(LimonAPI*) = &CreateTriggerByIndex<N>;
            PlayerExtensionInterface* (*extension_func)(LimonAPI*) = &CreatePlayerExtensionByIndex<N>;
            (void)trigger_func;  // Suppress unused variable warning
            (void)extension_func; // Suppress unused variable warning
        }
    };

    // Helper to generate instantiations from START to END (inclusive)
    template<int START, int END>
    struct generate_range {
        static_assert(START <= END, "Invalid range");
        static void generate() {
            template_instantiator<START>::instantiate();
            generate_range<START + 1, END>::generate();
        }
    };

    // Base case for recursion
    template<int N>
    struct generate_range<N, N> {
        static void generate() {
            template_instantiator<N>::instantiate();
        }
    };

    // Macro to generate instantiations from 0 to MAX-1
#define GENERATE_UP_TO(MAX) \
static_assert(MAX > 0, "MAX must be greater than 0"); \
namespace { \
const struct _template_initializer { \
_template_initializer() { \
generate_range<0, (MAX)-1>::generate(); \
} \
} _initializer; \
}

    // Generate instantiations from 0 to 63
    GENERATE_UP_TO(MAX_PYTHON_SCRIPT_COUNT);

    // Clean up macros
#undef GENERATE_UP_TO

}

template<size_t... Is>
static constexpr auto GetTriggerFactoryHelper(std::index_sequence<Is...>, size_t index) -> TriggerInterface* (*)(LimonAPI*) {
    constexpr std::array<TriggerInterface* (*)(LimonAPI*), sizeof...(Is)> factories = {
        &CreateTriggerByIndex<Is>...
    };
    return (index < factories.size()) ? factories[index] : nullptr;
}

template<size_t... Is>
static constexpr auto GetPlayerExtensionFactoryHelper(std::index_sequence<Is...>, size_t index) -> PlayerExtensionInterface* (*)(LimonAPI*) {
    constexpr std::array<PlayerExtensionInterface* (*)(LimonAPI*), sizeof...(Is)> factories = {
        &CreatePlayerExtensionByIndex<Is>...
    };
    return (index < factories.size()) ? factories[index] : nullptr;
}

// ---------------------------------------------------------
// 1. EXPOSE C++ TO PYTHON (The Embedded Module)
// ---------------------------------------------------------
// Unlike a .so plugin, this module is built directly into your .exe.
// Python scripts can simply 'import limon' without needing a limon.so file.
PYBIND11_EMBEDDED_MODULE(limon, m) {
    m.doc() = "Python bindings for Limon Engine";

    // First, bind the enums
    pybind11::enum_<LimonTypes::GenericParameter::RequestParameterTypes>(m, "RequestParameterType")
            .value("MODEL", LimonTypes::GenericParameter::MODEL)
            .value("ANIMATION", LimonTypes::GenericParameter::ANIMATION)
            .value("SWITCH", LimonTypes::GenericParameter::SWITCH)
            .value("FREE_TEXT", LimonTypes::GenericParameter::FREE_TEXT)
            .value("TRIGGER", LimonTypes::GenericParameter::TRIGGER)
            .value("GUI_TEXT", LimonTypes::GenericParameter::GUI_TEXT)
            .value("FREE_NUMBER", LimonTypes::GenericParameter::FREE_NUMBER)
            .value("COORDINATE", LimonTypes::GenericParameter::COORDINATE)
            .value("TRANSFORM", LimonTypes::GenericParameter::TRANSFORM)
            .value("MULTI_SELECT", LimonTypes::GenericParameter::MULTI_SELECT)
            .export_values();

    pybind11::enum_<LimonTypes::GenericParameter::ValueTypes>(m, "ValueType")
            .value("STRING", LimonTypes::GenericParameter::STRING)
            .value("DOUBLE", LimonTypes::GenericParameter::DOUBLE)
            .value("LONG", LimonTypes::GenericParameter::LONG)
            .value("LONG_ARRAY", LimonTypes::GenericParameter::LONG_ARRAY)
            .value("BOOLEAN", LimonTypes::GenericParameter::BOOLEAN)
            .value("VEC4", LimonTypes::GenericParameter::VEC4)
            .value("MAT4", LimonTypes::GenericParameter::MAT4)
            .export_values();

    pybind11::class_<LimonTypes::GenericParameter>(m, "GenericParameter")
            .def(pybind11::init<>())
            .def_property("request_type",
                          [](const LimonTypes::GenericParameter &self) { return self.requestType; },
                          [](LimonTypes::GenericParameter &self, LimonTypes::GenericParameter::RequestParameterTypes val) {
                              self.requestType = val;
                          })
            .def_property("description",
                          [](LimonTypes::GenericParameter &self) { return self.description; },
                          [](LimonTypes::GenericParameter &self, const std::string &val) {
                              self.description = val;
                          })
            .def_property("value_type",
                          [](const LimonTypes::GenericParameter &self) { return self.valueType; },
                          [](LimonTypes::GenericParameter &self, LimonTypes::GenericParameter::ValueTypes val) {
                              self.valueType = val;
                          })
            .def_property("is_set",
                          [](const LimonTypes::GenericParameter &self) { return self.isSet; },
                          [](LimonTypes::GenericParameter &self, bool val) {
                              self.isSet = val;
                          })
            .def("to_string", &LimonTypes::GenericParameter::to_string)
            .def_property("value",
                          [](LimonTypes::GenericParameter &self) -> pybind11::object {
                              // Getter implementation
                              switch (self.valueType) {
                                  case LimonTypes::GenericParameter::STRING:
                                      return pybind11::str(std::string(self.value.stringValue));
                                  case LimonTypes::GenericParameter::DOUBLE:
                                      return pybind11::float_(self.value.doubleValue);
                                  case LimonTypes::GenericParameter::LONG:
                                      return pybind11::int_(self.value.longValue);  // Ensure we return a Python int
                                  case LimonTypes::GenericParameter::BOOLEAN:
                                      return pybind11::bool_(self.value.boolValue);
                                  case LimonTypes::GenericParameter::VEC4: {
                                      auto &v = self.value.vectorValue;
                                      return pybind11::make_tuple(v.x, v.y, v.z, v.w);
                                  }
                                  case LimonTypes::GenericParameter::MAT4: {
                                      pybind11::list matrix;
                                      for (int i = 0; i < 4; ++i) {
                                          pybind11::list row;
                                          for (int j = 0; j < 4; ++j) {
                                              row.append(self.value.matrixValue[i][j]);
                                          }
                                          matrix.append(row);
                                      }
                                      return matrix;
                                  }
                                  case LimonTypes::GenericParameter::LONG_ARRAY: {
                                      pybind11::list array;
                                      for (long i = 0; i < self.value.longValues[0]; ++i) {
                                          array.append(static_cast<long>(self.value.longValues[i + 1]));
                                      }
                                      return array;
                                  }
                                  default:
                                      return pybind11::none();
                              }
                          },
                          [](LimonTypes::GenericParameter &self, const pybind11::object &value) {
                              // Setter implementation - handle conversion based on valueType
                              try {
                                  if (pybind11::isinstance<pybind11::str>(value)) {
                                      std::string str = value.cast<std::string>();
                                      if (str.size() >= 64) {
                                          throw std::runtime_error("String value too long, max 63 characters");
                                      }
                                      strncpy(self.value.stringValue, str.c_str(), 63);
                                      self.value.stringValue[63] = '\0';
                                      self.valueType = LimonTypes::GenericParameter::STRING;
                                      self.isSet = true;
                                  }
                                  else if (self.valueType == LimonTypes::GenericParameter::LONG) {
                                      // Handle LONG type specifically
                                      try {
                                          self.value.longValue = pybind11::cast<long>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
                                          // Try converting from float if direct cast fails
                                          try {
                                              double d = pybind11::cast<double>(value);
                                              self.value.longValue = static_cast<long>(d);
                                              self.isSet = true;
                                          } catch (const pybind11::cast_error&) {
                                              throw std::runtime_error("Could not convert value to LONG");
                                          }
                                      }
                                  }
                                  else if (self.valueType == LimonTypes::GenericParameter::DOUBLE) {
                                      // Handle DOUBLE type
                                      try {
                                          self.value.doubleValue = pybind11::cast<double>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
                                          throw std::runtime_error("Could not convert value to DOUBLE");
                                      }
                                  }
                                  else if (self.valueType == LimonTypes::GenericParameter::BOOLEAN) {
                                      // Handle BOOLEAN type
                                      try {
                                          self.value.boolValue = pybind11::cast<bool>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
                                          // Try converting from int/float
                                          try {
                                              double d = pybind11::cast<double>(value);
                                              self.value.boolValue = d != 0.0;
                                              self.isSet = true;
                                          } catch (const pybind11::cast_error&) {
                                              throw std::runtime_error("Could not convert value to BOOLEAN");
                                          }
                                      }
                                  }
                                  else if (pybind11::isinstance<pybind11::tuple>(value) || pybind11::isinstance<pybind11::list>(value)) {
                                      auto seq = value.cast<pybind11::sequence>();
                                      if (seq.size() == 4 && self.valueType == LimonTypes::GenericParameter::VEC4) {
                                          // Handle VEC4
                                          try {
                                              self.value.vectorValue.x = seq[0].cast<float>();
                                              self.value.vectorValue.y = seq[1].cast<float>();
                                              self.value.vectorValue.z = seq[2].cast<float>();
                                              self.value.vectorValue.w = seq[3].cast<float>();
                                              self.isSet = true;
                                          } catch (const std::exception& e) {
                                              throw std::runtime_error("Failed to convert sequence to VEC4. All elements must be numbers");
                                          }
                                      }
                                      else if (seq.size() == 4 && pybind11::isinstance<pybind11::sequence>(seq[0]) && 
                                              self.valueType == LimonTypes::GenericParameter::MAT4) {
                                          // Handle MAT4 (4x4 matrix)
                                          try {
                                              for (int i = 0; i < 4; ++i) {
                                                  auto row = seq[i].cast<pybind11::sequence>();
                                                  if (row.size() != 4) {
                                                      throw std::runtime_error("Each row must have exactly 4 elements for MAT4");
                                                  }
                                                  for (int j = 0; j < 4; ++j) {
                                                      self.value.matrixValue[i][j] = row[j].cast<float>();
                                                  }
                                              }
                                              self.isSet = true;
                                          } catch (const std::exception& e) {
                                              throw std::runtime_error("Failed to convert sequence to MAT4. All elements must be numbers");
                                          }
                                      }
                                      else if (seq.size() <= 32 &&
                                              self.valueType == LimonTypes::GenericParameter::LONG_ARRAY) {
                                          // Handle LONG_ARRAY (up to 32 elements)
                                          try {
                                              self.value.longValues[0] = seq.size(); // NOLINT(*-narrowing-conversions)
                                              for (size_t i = 0; i < seq.size(); ++i) {
                                                  self.value.longValues[i + 1] = seq[i].cast<long>();
                                              }
                                              self.isSet = true;
                                          } catch (const std::exception& e) {
                                              throw std::runtime_error("Failed to convert sequence to LONG_ARRAY. All elements must be integers");
                                          }
                                      }
                                      else {
                                          throw std::runtime_error("Unsupported sequence type or length for current value type");
                                      }
                                  }
                                  else {
                                      // Default handling for unknown types
                                      throw std::runtime_error("Unsupported value type or conversion");
                                  }
                              } catch (const std::exception& e) {
                                  throw std::runtime_error(std::string("Error setting value: ") + e.what());
                              }
                          });

    // Then bind the vector of GenericParameter
    pybind11::bind_vector<std::vector<LimonTypes::GenericParameter> >(m, "GenericParameterVector");

    // Bind LimonTypes::Vec4
    pybind11::class_<LimonTypes::Vec4>(m, "Vec4")
            .def(pybind11::init<>())
            .def(pybind11::init<float, float, float, float>(),
                 pybind11::arg("x") = 0.0f, pybind11::arg("y") = 0.0f,
                 pybind11::arg("z") = 0.0f, pybind11::arg("w") = 0.0f)
            .def_readwrite("x", &LimonTypes::Vec4::x)
            .def_readwrite("y", &LimonTypes::Vec4::y)
            .def_readwrite("z", &LimonTypes::Vec4::z)
            .def_readwrite("w", &LimonTypes::Vec4::w);

    // Bind LimonAPI
    pybind11::class_<LimonAPI> limon(m, "LimonAPI");

    // Bind methods
    limon.def("get_options", &LimonAPI::getOptions, "Get engine options")

            // GUI Methods
            .def("add_gui_text",
                 [](LimonAPI &self,
                    const std::string &font_file_path,
                    unsigned int font_size,
                    const std::string &name,
                    const std::string &text,
                    const pybind11::object &color,
                    const pybind11::object &position,
                    float rotation) -> unsigned int {
                     // Convert Python color to glm::vec3
                     glm::vec3 color_vec(1.0f, 1.0f, 1.0f); // Default white
                     if (!color.is_none()) {
                         color_vec = pybind11::cast<glm::vec3>(color);
                     }

                     // Convert Python position to glm::vec2
                     glm::vec2 pos_vec(0.0f, 0.0f); // Default position
                     if (!position.is_none()) {
                         pos_vec = pybind11::cast<glm::vec2>(position);
                     }

                     return self.addGuiText(font_file_path, font_size, name, text,
                                            color_vec, pos_vec, rotation);
                 },
                 "Add a text element to the GUI\n\
             Args:\n\
             font_file_path (str): Path to the font file\n\
             font_size (int): Size of the font\n\
             name (str): Name of the text element\n\
             text (str): The text to display\n\
             color (tuple, optional): RGB color as (r, g, b). Defaults to white (1.0, 1.0, 1.0).\n\
             position (tuple, optional): Position as (x, y). Defaults to (0.0, 0.0).\n\
             rotation (float, optional): Rotation in degrees. Defaults to 0.0.",
                 pybind11::arg("font_file_path"),
                 pybind11::arg("font_size"),
                 pybind11::arg("name"),
                 pybind11::arg("text"),
                 pybind11::arg("color") = pybind11::none(),
                 pybind11::arg("position") = pybind11::none(),
                 pybind11::arg("rotation") = 0.0f)
            .def("add_gui_image",
                 [](LimonAPI &self,
                    const std::string &image_file_path,
                    const std::string &name,
                    const pybind11::object &position,
                    const pybind11::object &scale,
                    float rotation) -> unsigned int {
                     // Convert Python position to LimonTypes::Vec2
                     LimonTypes::Vec2 pos_vec{0.0f, 0.0f}; // Default position
                     if (!position.is_none()) {
                         auto pos = pybind11::cast<glm::vec2>(position);
                         pos_vec.x = pos.x;
                         pos_vec.y = pos.y;
                     }

                     // Convert Python scale to LimonTypes::Vec2
                     LimonTypes::Vec2 scale_vec{1.0f, 1.0f}; // Default scale
                     if (!scale.is_none()) {
                         auto scl = pybind11::cast<glm::vec2>(scale);
                         scale_vec.x = scl.x;
                         scale_vec.y = scl.y;
                     }

                     return self.addGuiImage(image_file_path, name, pos_vec, scale_vec, rotation);
                 },
                 "Add an image to the GUI\n\
             Args:\n\
             image_file_path (str): Path to the image file\n\
             name (str): Name of the image element\n\
             position (tuple, optional): Position as (x, y). Defaults to (0.0, 0.0).\n\
             scale (tuple, optional): Scale as (x, y). Defaults to (1.0, 1.0).\n\
             rotation (float, optional): Rotation in degrees. Defaults to 0.0.",
                 pybind11::arg("image_file_path"),
                 pybind11::arg("name"),
                 pybind11::arg("position") = pybind11::none(),
                 pybind11::arg("scale") = pybind11::none(),
                 pybind11::arg("rotation") = 0.0f)
            .def("update_gui_text", &LimonAPI::updateGuiText,
                 "Update GUI text element",
                 pybind11::arg("gui_text_id"), pybind11::arg("new_text"))
            .def("remove_gui_element", &LimonAPI::removeGuiElement,
                 "Remove a GUI element",
                 pybind11::arg("gui_element_id"))

            // Object Manipulation
            .def("set_object_temporary", &LimonAPI::setObjectTemporary,
                 "Set if an object is temporary",
                 pybind11::arg("object_id"), pybind11::arg("temporary"))
            .def("attach_object_to_object", &LimonAPI::attachObjectToObject,
                 "Attach one object to another",
                 pybind11::arg("object_id"), pybind11::arg("object_to_attach_to_id"))
            .def("remove_trigger_object", &LimonAPI::removeTriggerObject,
                 "Remove a trigger object",
                 pybind11::arg("trigger_object_id"))
            .def("disconnect_object_from_physics", &LimonAPI::disconnectObjectFromPhysics,
                 "Disable physics for an object",
                 pybind11::arg("object_id"))
            .def("reconnect_object_to_physics", &LimonAPI::reconnectObjectToPhysics,
                 "Re-enable physics for an object",
                 pybind11::arg("object_id"))
            .def("apply_force", &LimonAPI::applyForce,
                 "Apply force to an object",
                 pybind11::arg("object_id"), pybind11::arg("force_position"), pybind11::arg("force_amount"))
            .def("apply_force_to_player", &LimonAPI::applyForceToPlayer,
                 "Apply force to the player",
                 pybind11::arg("force_amount"))
            .def("add_object_translate", &LimonAPI::addObjectTranslate,
                 "Add to an object's position",
                 pybind11::arg("object_id"), pybind11::arg("translation"))
            .def("add_object_scale", &LimonAPI::addObjectScale,
                 "Add to an object's scale",
                 pybind11::arg("object_id"), pybind11::arg("scale"))
            .def("add_object_orientation", &LimonAPI::addObjectOrientation,
                 "Add to an object's orientation",
                 pybind11::arg("object_id"), pybind11::arg("orientation"))
            .def("get_object_transformation_matrix", &LimonAPI::getObjectTransformationMatrix,
                 "Get an object's transformation matrix",
                 pybind11::arg("object_id"))
            .def("get_model_children", &LimonAPI::getModelChildren,
                 "Get children of a model",
                 pybind11::arg("model_id"))

            // Sound
            .def("attach_sound_to_object", &LimonAPI::attachSoundToObjectAndPlay,
                 "Attach and play a sound on an object",
                 pybind11::arg("object_id"), pybind11::arg("sound_path"))
            .def("detach_sound_from_object", &LimonAPI::detachSoundFromObject,
                 "Detach sound from an object",
                 pybind11::arg("object_id"))
            .def("play_sound", &LimonAPI::playSound,
                 "Play a sound at a position",
                 pybind11::arg("sound_path"), pybind11::arg("position"),
                 pybind11::arg("position_relative") = false, pybind11::arg("looped") = false)

            // AI Interaction
            .def("interact_with_ai", &LimonAPI::interactWithAI,
                 "Interact with an AI",
                 pybind11::arg("ai_id"), pybind11::arg("interaction_information"))

            // Particle Systems
            .def("disable_particle_emitter", &LimonAPI::disableParticleEmitter,
                 "Disable a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("enable_particle_emitter", &LimonAPI::enableParticleEmitter,
                 "Enable a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("add_particle_emitter", &LimonAPI::addParticleEmitter,
                 "Add a new particle emitter",
                 pybind11::arg("name"), pybind11::arg("texture_file"),
                 pybind11::arg("start_position"), pybind11::arg("max_start_distances"),
                 pybind11::arg("size"), pybind11::arg("count"),
                 pybind11::arg("life_time"), pybind11::arg("particles_per_ms"),
                 pybind11::arg("continuously_emit"))
            .def("remove_particle_emitter", &LimonAPI::removeParticleEmitter,
                 "Remove a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("set_emitter_particle_speed", &LimonAPI::setEmitterParticleSpeed,
                 "Set particle speed for an emitter",
                 pybind11::arg("emitter_id"), pybind11::arg("speed_multiplier"),
                 pybind11::arg("speed_offset"))
            .def("set_emitter_particle_gravity", &LimonAPI::setEmitterParticleGravity,
                 "Set particle gravity for an emitter",
                 pybind11::arg("emitter_id"), pybind11::arg("gravity"))

            // Ray Casting
            .def("ray_cast_to_cursor", &LimonAPI::rayCastToCursor,
                 "Cast a ray from camera to cursor position")
            .def("ray_cast", &LimonAPI::rayCastFirstHit,
                 "Cast a ray from start point in direction",
                 pybind11::arg("start"), pybind11::arg("direction"))

            // Lighting
            .def("add_light_translate", &LimonAPI::addLightTranslate,
                 "Translate a light",
                 pybind11::arg("light_id"), pybind11::arg("translation"))
            .def("set_light_color", &LimonAPI::setLightColor,
                 "Set a light's color",
                 pybind11::arg("light_id"), pybind11::arg("color"))

            // World Management
            .def("load_and_remove_world", &LimonAPI::LoadAndRemove,
                 "Load a new world and remove the current one",
                 pybind11::arg("world_file_name"))
            .def("return_to_previous_world", &LimonAPI::returnPreviousWorld,
                 "Return to the previously loaded world")
            .def("quit_game", &LimonAPI::quitGame,
                 "Quit the game")
            .def("change_render_pipeline", &LimonAPI::changeRenderPipeline,
                 "Change the render pipeline",
                 pybind11::arg("pipeline_file_name"))

            // Player Related
            .def("kill_player", &LimonAPI::killPlayer,
                 "Kill the player")
            .def("set_model_animation", &LimonAPI::setModelAnimation,
                 "Set an animation for a model",
                 pybind11::arg("model_id"), pybind11::arg("animation_name"),
                 pybind11::arg("looped") = true)
            .def("set_model_animation_with_blend", &LimonAPI::setModelAnimationWithBlend,
                 "Set an animation for a model with blending",
                 pybind11::arg("model_id"), pybind11::arg("animation_name"),
                 pybind11::arg("looped") = true, pybind11::arg("blend_time") = 100)
            .def("set_model_animation_speed", &LimonAPI::setModelAnimationSpeed,
                 "Set the animation speed for a model",
                 pybind11::arg("model_id"), pybind11::arg("speed"))

            // Variable Management
            .def("get_variable", &LimonAPI::getVariable,
                 "Get a script variable by name",
                 pybind11::arg("variable_name"),
                 pybind11::return_value_policy::reference)

            // Input
            .def("simulate_input", &LimonAPI::simulateInput,
                 "Simulate input events",
                 pybind11::arg("input_states"))

            // Trigger System
            .def("get_result_of_trigger", &LimonAPI::getResultOfTrigger,
                 "Get result of a trigger",
                 pybind11::arg("trigger_object_id"), pybind11::arg("trigger_code_id"));

    // Animation methods
    limon.def("animate_model", &LimonAPI::animateModel,
              "Animate a model",
              pybind11::arg("model_id"), pybind11::arg("animation_id"),
              pybind11::arg("looped") = false, pybind11::arg("sound_path") = nullptr);

    limon.def("get_model_animation_name", &LimonAPI::getModelAnimationName,
              "Get the name of the current animation for a model",
              pybind11::arg("model_id"));

    limon.def("get_model_animation_finished", &LimonAPI::getModelAnimationFinished,
              "Check if model's animation has finished",
              pybind11::arg("model_id"));

    // Object manipulation methods
    limon.def("add_object",
              [](LimonAPI &self,
                 const std::string &model_file_path,
                 float model_weight,
                 bool physical,
                 const glm::vec3 &position,
                 const glm::vec3 &scale,
                 const glm::quat &orientation) {
                  return self.addObject(
                      model_file_path,
                      model_weight,
                      physical,
                      position,
                      scale,
                      orientation
                  );
              },
              "Add a new object to the scene",
              pybind11::arg("model_file_path"),
              pybind11::arg("model_weight") = 1.0f,
              pybind11::arg("physical") = true,
              pybind11::arg("position") = glm::vec3(0, 0, 0),
              pybind11::arg("scale") = glm::vec3(1, 1, 1),
              pybind11::arg("orientation") = glm::quat(1.0f, 0.0f, 0.0f, 0.0f) // w, x, y, z
    );

    limon.def("remove_object", &LimonAPI::removeObject,
              "Remove an object from the scene",
              pybind11::arg("object_id"), pybind11::arg("remove_children") = true);

    // Player methods

    limon.def("get_player_position", [](LimonAPI &self) {
        glm::vec3 position, center, up, right;
        self.getPlayerPosition(position, center, up, right);
        return pybind11::make_tuple(
            pybind11::dict(pybind11::arg("x")=position.x, pybind11::arg("y")=position.y, pybind11::arg("z")=position.z),
            pybind11::dict(pybind11::arg("x")=center.x, pybind11::arg("y")=center.y, pybind11::arg("z")=center.z),
            pybind11::dict(pybind11::arg("x")=up.x, pybind11::arg("y")=up.y, pybind11::arg("z")=up.z),
            pybind11::dict(pybind11::arg("x")=right.x, pybind11::arg("y")=right.y, pybind11::arg("z")=right.z)
        );
    }, "Get player position, view direction, up and right vectors");

    limon.def("get_player_attached_model", &LimonAPI::getPlayerAttachedModel,
              "Get the ID of the model attached to the player");

    limon.def("get_player_attached_model_offset", &LimonAPI::getPlayerAttachedModelOffset,
              "Get the offset of the model attached to the player");

    limon.def("set_player_attached_model_offset", &LimonAPI::setPlayerAttachedModelOffset,
              "Set the offset of the model attached to the player",
              pybind11::arg("new_offset"));

    // World management
    limon.def("load_and_switch_world", &LimonAPI::loadAndSwitchWorld,
              "Load and switch to a new world",
              pybind11::arg("world_file_name"));

    limon.def("return_to_world", &LimonAPI::returnToWorld,
              "Return to a previously loaded world",
              pybind11::arg("world_file_name"));

    // Timed events with Python callback support
    limon.def("add_timed_event",
              [](LimonAPI &self, uint64_t wait_time, bool use_wall_time,
                 const pybind11::function &callback,
                 const std::vector<LimonTypes::GenericParameter> &params) {
                  auto cb = PyTimedEventCallback(callback);
                  return self.addTimedEvent(wait_time, use_wall_time, cb, params);
              },
              "Add a timed event with a Python callback",
              pybind11::arg("wait_time"),
              pybind11::arg("use_wall_time"),
              pybind11::arg("callback"),
              pybind11::arg("parameters") = std::vector<LimonTypes::GenericParameter>{});

    // Add more methods as needed...

    // Utility function to create a Vec4 from Python
    m.def("create_vec4", [](float x, float y, float z, float w) {
              return LimonTypes::Vec4{x, y, z, w};
          }, "Create a new Vec4",
          pybind11::arg("x") = 0.0f, pybind11::arg("y") = 0.0f,
          pybind11::arg("z") = 0.0f, pybind11::arg("w") = 0.0f);
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
                // This is a factory function that creates both the Python and C++ objects
                pybind11::object pyClass = pybind11::module_::import("limon_interface").attr("CameraAttachment");
                pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
                // Don't call __init__ here - let Python handle it
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
                // This is a factory function that creates both the Python and C++ objects
                pybind11::object pyClass = pybind11::module_::import("limon_interface").attr("TriggerInterface");
                pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
                // Don't call __init__ here - let Python handle it
                return new PyTriggerInterface(api, pyInstance);
            }))
            .def("get_parameters", &TriggerInterface::getParameters)
            .def("run", &TriggerInterface::run)
            .def("get_results", &TriggerInterface::getResults)
            .def("get_name", &TriggerInterface::getName);

    pybind11::class_<PlayerExtensionInterface, PyPlayerExtensionInterface>(m, "PlayerExtensionInterface")
        .def(pybind11::init([](LimonAPI *api) {
            // This is a factory function that creates both the Python and C++ objects
            pybind11::object pyClass = pybind11::module_::import("limon_interface").attr("PlayerExtensionInterface");
            pybind11::object pyInstance = pyClass.attr("__new__")(pyClass);
            // Don't call __init__ here - let Python handle it
            return new PyPlayerExtensionInterface(api, pyInstance);
        }))
            .def("process_input", &PlayerExtensionInterface::processInput)
            .def("interact", &PlayerExtensionInterface::interact)
            .def("get_name", &PlayerExtensionInterface::getName)
            .def("get_custom_camera_attachment", &PlayerExtensionInterface::getCustomCameraAttachment,
                 pybind11::return_value_policy::reference)
            .def_static("create_extension", [](const std::string &name, LimonAPI *api) -> PlayerExtensionInterface * {
                auto ptr = PlayerExtensionInterface::createExtension(name, api);
                if (!ptr) {
                    throw std::runtime_error("Failed to create extension: " + name);
                }
                return ptr;
            }, pybind11::return_value_policy::reference);

    // Register the trigger and extension registration functions
    m.def("register_trigger_type", &TriggerInterface::registerType);
    m.def("register_extension_type", &PlayerExtensionInterface::registerType);
    m.def("get_trigger_names", &TriggerInterface::getTriggerNames);
    m.def("get_extension_names", &PlayerExtensionInterface::getTriggerNames);

    //Binding for std::out
    pybind11::class_<PythonStdOut>(m, "StdOut")
            .def(pybind11::init<>())
            .def("write", &PythonStdOut::write)
            .def("flush", &PythonStdOut::flush);
}

ScriptManager::ScriptManager(const std::string& directoryPath) : directoryPath(directoryPath) {
    try {
        // Get the Python bundle path from the compile definition
        std::filesystem::path bundlePath(PYTHON_BUNDLE_ZIP);
        std::filesystem::path localPython = bundlePath.parent_path().parent_path(); // Go up two levels from site-packages.zip

        // 2. Validate it exists (Good for debugging)
        if (!std::filesystem::exists(localPython)) {
            std::cerr << "CRITICAL ERROR: Bundled Python environment not found at: "
                      << localPython << std::endl;
            std::cerr << "Did you copy the 'python' folder into the build directory?" << std::endl;
            return;
        }

        // Set Python path to include the bundle and its lib-dynload directory
        std::filesystem::path dynLoadPath = bundlePath.parent_path() / "lib-dynload";
        std::filesystem::path stdLibPath = localPython / "Lib";
        std::filesystem::path sitePackagesPath = localPython / "Lib" / "site-packages";

        if (!std::filesystem::exists(bundlePath) || !std::filesystem::exists(dynLoadPath)) {
            std::cerr << "CRITICAL ERROR: Required Python files not found:\n"
                     << "  " << bundlePath << "\n"
                     << "  " << dynLoadPath << std::endl;
            return;
        }

        // Initialize Python with the new configuration system
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        
        // Set Python home
        std::wstring homePath = std::filesystem::absolute(localPython).wstring();
        PyConfig_SetString(&config, &config.home, homePath.c_str());
        
        // Set module search paths
        config.module_search_paths_set = 1;
        PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(bundlePath).wstring().c_str());
        PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(dynLoadPath).wstring().c_str());
        PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(stdLibPath).wstring().c_str());
        PyWideStringList_Append(&config.module_search_paths, std::filesystem::absolute(sitePackagesPath).wstring().c_str());
        
        // Initialize Python with the new config
        Py_InitializeFromConfig(&config);
        PyConfig_Clear(&config);
        try {
            guard = new pybind11::scoped_interpreter{};
            std::cout << "[Limon] Python initialized from bundle at: " << bundlePath << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Limon] Python Init Failed: " << e.what() << std::endl;
        }
        auto sys = pybind11::module::import("sys");
        auto limon = pybind11::module::import("limon");
        sys.attr("path").attr("append")(directoryPath);

        // 2. Redirect Python's stdout to our C++ object.
        // This object is just horribly inefficient, but I assume only usecase for print in python would be troubleshooting,
        // and batching would make that harder by adding delay
        auto redirector = limon.attr("StdOut")();
        sys.attr("stdout") = redirector;
        sys.attr("stderr") = redirector; // Catch errors
    } catch (const std::exception& e) {
        std::cerr << "[Scripting] Error initializing Python: " << e.what() << std::endl;
    }
}

void ScriptManager::LoadScript(const std::string &moduleName) {
    try {
        std::cout << "[ScriptManager] trying to load extension: " << moduleName << std::endl;

        pybind11::module_ module;
        try {
            module = pybind11::module_::import(moduleName.c_str());
        } catch (const pybind11::error_already_set& e) {
            std::cerr << "[ScriptManager] Failed to import module " << moduleName << ": " << e.what() << std::endl;
            PyErr_Print();
            return;
        }        auto triggerClasses = pybind11::list(module.attr("__dict__").attr("items")());
        for (auto item: triggerClasses) {
            auto [name, obj] = item.cast<std::pair<std::string, pybind11::object> >();
        }

        for (auto item: triggerClasses) {
            auto [name, obj] = item.cast<std::pair<std::string, pybind11::object> >();

            if (!pybind11::isinstance<pybind11::type>(obj) ||
                !pybind11::hasattr(obj, "__bases__")) {
                continue;
            }

            // Function pointer type for TriggerInterface
            using TriggerFactory = TriggerInterface* (*)(LimonAPI *);
            // Function pointer type for PlayerExtensionInterface
            using ExtensionFactory = PlayerExtensionInterface* (*)(LimonAPI *);

            // Check for TriggerInterface subclasses
            if (IsSubclassOf(obj, "TriggerInterface")) {
                std::cout << "[ScriptManager] found TriggerInterface: " << moduleName << std::endl;
                size_t callbackIndex = GetCallbacks().size();
                GetCallbacks().push_back({obj, CallBackTypes::TRIGGER});

                constexpr size_t MAX_TRIGGERS = MAX_PYTHON_SCRIPT_COUNT; // Match the value from GENERATE_UP_TO macro
                TriggerFactory factory = GetTriggerFactoryHelper(std::make_index_sequence<MAX_TRIGGERS>{}, callbackIndex);
                
                if (!factory) {
                    std::cerr << "[ScriptManager] Too many trigger types registered (max " << MAX_TRIGGERS << ")" << std::endl;
                    continue;
                }

                TriggerInterface::registerType(name, factory);
                std::cout << "[ScriptManager] Registered trigger: " << name << std::endl;
            }
            // Similar for PlayerExtensionInterface
            else if (IsSubclassOf(obj, "PlayerExtensionInterface")) {
                std::cout << "[ScriptManager] found PlayerExtensionInterface: " << moduleName << std::endl;
                size_t callbackIndex = GetCallbacks().size();
                GetCallbacks().push_back({obj, CallBackTypes::PLAYER_EXTENSION});

                constexpr size_t MAX_EXTENSIONS = MAX_PYTHON_SCRIPT_COUNT; // Match the value from GENERATE_UP_TO macro
                ExtensionFactory factory = GetPlayerExtensionFactoryHelper(std::make_index_sequence<MAX_EXTENSIONS>{}, callbackIndex);
                
                if (!factory) {
                    std::cerr << "[ScriptManager] Too many extension types registered (max " << MAX_EXTENSIONS << ")" << std::endl;
                    continue;
                }

                PlayerExtensionInterface::registerType(name, factory);
                std::cout << "[ScriptManager] Registered extension: " << name << std::endl;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[ScriptManager] Error loading script " << moduleName
                << ": " << e.what() << std::endl;
    }
}

TriggerInterface *ScriptManager::CreateTriggerWrapper(LimonAPI *api, size_t index) {
    try {
        auto &callbacks = GetCallbacks();
        if (index < callbacks.size() && callbacks[index].callBackType == CallBackTypes::TRIGGER) {
            pybind11::object pyClass = callbacks[index].pyClass;

            // Create the Python object with proper initialization
            pybind11::object instance = pyClass.attr("__new__")(pyClass);
            instance.attr("__init__")(api); // Only pass the API, not the second parameter

            // Create a new PyTriggerInterface that wraps the Python object
            return new PyTriggerInterface(api, instance);
        }
    } catch (const pybind11::error_already_set &e) {
        std::cerr << "[ScriptManager] Python error in CreateTriggerWrapper: " << e.what() << std::endl;
        PyErr_Print();
    } catch (const std::exception &e) {
        std::cerr << "[ScriptManager] Error in CreateTriggerWrapper: " << e.what() << std::endl;
    }
    return nullptr;
}

PlayerExtensionInterface* ScriptManager::CreatePlayerExtensionWrapper(LimonAPI* api, size_t index) {
    try {
        auto& callbacks = GetCallbacks();
        if (index < callbacks.size() && callbacks[index].callBackType == CallBackTypes::PLAYER_EXTENSION) {
            pybind11::gil_scoped_acquire acquire;  // Ensure GIL is held

            try {
                // Get the Python class
                pybind11::object pyClass = callbacks[index].pyClass;

                // Create the instance using __new__ and __init__ separately
                pybind11::object instance = pyClass.attr("__new__")(pyClass);

                // Now call __init__ with the API
                instance.attr("__init__")(pybind11::cast(api, pybind11::return_value_policy::reference));

                // Verify the object has required methods
                if (!pybind11::hasattr(instance, "process_input") ||
                    !pybind11::hasattr(instance, "interact") ||
                    !pybind11::hasattr(instance, "get_name") ||
                    !pybind11::hasattr(instance, "get_custom_camera_attachment")) {
                    std::cerr << "[ScriptManager] Python extension class is missing required methods" << std::endl;
                    return nullptr;
                }

                std::cout << "Successfully created Python PlayerExtension instance" << std::endl;
                return new PyPlayerExtensionInterface(api, instance);

            } catch (const pybind11::error_already_set& e) {
                std::cerr << "[ScriptManager] Python error during extension creation: " << e.what() << std::endl;
                PyErr_Print();
                return nullptr;
            } catch (const std::exception& e) {
                std::cerr << "[ScriptManager] C++ error during extension creation: " << e.what() << std::endl;
                return nullptr;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScriptManager] Error in CreateExtensionWrapper: " << e.what() << std::endl;
    }
    return nullptr;
}