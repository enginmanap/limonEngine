#include <Python.h>
#include "PythonGenericParameterBinding.h"
#include "PythonTypeCasters.h"
#include <pybind11/embed.h>
#include "pybind11/stl_bind.h"
#include <limonAPI/LimonAPI.h>
#include <limonAPI/LimonTypes.h>

void bindEnums(pybind11::module_& m) {
    pybind11::enum_<Logger::Subsystem>(m, "LogSubsystem")
        .value("RENDER",    Logger::log_Subsystem_RENDER)
        .value("MODEL",     Logger::log_Subsystem_MODEL)
        .value("INPUT",     Logger::log_Subsystem_INPUT)
        .value("SETTINGS",  Logger::log_Subsystem_SETTINGS)
        .value("AI",        Logger::log_Subsystem_AI)
        .value("LOAD_SAVE", Logger::log_Subsystem_LOAD_SAVE)
        .value("EDITOR",    Logger::log_Subsystem_EDITOR)
        .value("ANIMATION", Logger::log_Subsystem_ANIMATION)
        .export_values();

    pybind11::enum_<Logger::Level>(m, "LogLevel")
        .value("TRACE", Logger::log_level_TRACE)
        .value("DEBUG", Logger::log_level_DEBUG)
        .value("INFO",  Logger::log_level_INFO)
        .value("WARN",  Logger::log_level_WARN)
        .value("ERROR", Logger::log_level_ERROR)
        .export_values();
}

void bindGenericParameter(pybind11::module_& m) {
    // Note: ValueType and RequestParameterType are now defined in Python (generic_parameter.py)
    // We no longer bind the C++ enums to avoid initialization order issues

    pybind11::class_<LimonTypes::GenericParameter>(m, "GenericParameter")
            .def(pybind11::init<>())
            .def_property("request_type",
                          [](const LimonTypes::GenericParameter &self) { return static_cast<int>(self.requestType); },
                          [](LimonTypes::GenericParameter &self, int val) {
                              self.requestType = static_cast<LimonTypes::GenericParameter::RequestParameterTypes>(val);
                          })
            .def_property("description",
                          [](LimonTypes::GenericParameter &self) { return self.description; },
                          [](LimonTypes::GenericParameter &self, const std::string &val) {
                              self.description = val;
                          })
            .def_property("value_type",
                          [](const LimonTypes::GenericParameter &self) { return static_cast<int>(self.valueType); },
                          [](LimonTypes::GenericParameter &self, int val) {
                              self.valueType = static_cast<LimonTypes::GenericParameter::ValueTypes>(val);
                          })
            .def_property("is_set",
                          [](const LimonTypes::GenericParameter &self) { return self.isSet; },
                          [](LimonTypes::GenericParameter &self, bool val) {
                              self.isSet = val;
                          })
            .def("to_string", &LimonTypes::GenericParameter::to_string)
            .def_property("value",
                          [](LimonTypes::GenericParameter &self) -> pybind11::object {
                              switch (self.valueType) {
                                  case LimonTypes::GenericParameter::STRING:
                                      return pybind11::str(std::string(self.value.stringValue));
                                  case LimonTypes::GenericParameter::DOUBLE:
                                      return pybind11::float_(self.value.doubleValue);
                                  case LimonTypes::GenericParameter::LONG:
                                      return pybind11::int_(self.value.longValue);
                                  case LimonTypes::GenericParameter::BOOLEAN:
                                      return pybind11::bool_(self.value.boolValue);
                                  case LimonTypes::GenericParameter::VEC4: {
                                      const LimonTypes::Vec4 &v = self.value.vectorValue;
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
                                  case LimonTypes::GenericParameter::FLOAT_ARRAY: {
                                      pybind11::list array;
                                      long size = static_cast<long>(self.value.floatValues[0]);
                                      for (long i = 0; i < size; ++i) {
                                          array.append(static_cast<float>(self.value.floatValues[i + 1]));
                                      }
                                      return array;
                                  }
                                  default:
                                      return pybind11::none();
                              }
                          },
                          [](LimonTypes::GenericParameter &self, const pybind11::object &value) {
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
                                      try {
                                          self.value.longValue = pybind11::cast<long>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
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
                                      try {
                                          self.value.doubleValue = pybind11::cast<double>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
                                          throw std::runtime_error("Could not convert value to DOUBLE");
                                      }
                                  }
                                  else if (self.valueType == LimonTypes::GenericParameter::BOOLEAN) {
                                      try {
                                          self.value.boolValue = pybind11::cast<bool>(value);
                                          self.isSet = true;
                                      } catch (const pybind11::cast_error&) {
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
                                      pybind11::sequence seq = value.cast<pybind11::sequence>();
                                      if (seq.size() == 4 && self.valueType == LimonTypes::GenericParameter::VEC4) {
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
                                          try {
                                              for (int i = 0; i < 4; ++i) {
                                                  pybind11::sequence row = seq[i].cast<pybind11::sequence>();
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
                                      else if (seq.size() <= 15 &&
                                              self.valueType == LimonTypes::GenericParameter::FLOAT_ARRAY) {
                                          try {
                                              self.value.floatValues[0] = static_cast<float>(seq.size());
                                              for (size_t i = 0; i < seq.size(); ++i) {
                                                  self.value.floatValues[i + 1] = seq[i].cast<float>();
                                              }
                                              self.isSet = true;
                                          } catch (const std::exception& e) {
                                              throw std::runtime_error("Failed to convert sequence to FLOAT_ARRAY. All elements must be numbers");
                                          }
                                      }
                                      else {
                                          throw std::runtime_error("Unsupported sequence type or length for current value type");
                                      }
                                  }
                                  else {
                                      throw std::runtime_error("Unsupported value type or conversion");
                                  }
                              } catch (const std::exception& e) {
                                  throw std::runtime_error(std::string("Error setting value: ") + e.what());
                              }
                          });

    pybind11::bind_vector<std::vector<glm::vec3>>(m, "Vec3Vector");
    pybind11::bind_vector<std::vector<LimonTypes::GenericParameter>>(m, "GenericParameterVector");
}
