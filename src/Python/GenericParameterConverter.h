#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/cast.h>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "limonAPI/LimonAPI.h"

namespace py = pybind11;

/**
 * This class is used to covert GenericParameter between C++ and Python
 */
class GenericParameterConverter {
public:
    // Convert Python GenericParameter to C++ GenericParameter
    static LimonTypes::GenericParameter convertPythonToGenericParameter(py::object py_param) {
        // Extract properties from Python GenericParameter
        py::object request_type_obj = py_param.attr("request_type");
        py::object value_type_obj = py_param.attr("value_type");
        
        // Handle both enum objects and integers
        int request_type = 0;
        if (py::isinstance<py::int_>(request_type_obj)) {
            request_type = py::cast<int>(request_type_obj);
        } else {
            py::object value_obj = request_type_obj.attr("value");
            request_type = py::cast<int>(value_obj);
        }
        
        int value_type = 0;
        if (py::isinstance<py::int_>(value_type_obj)) {
            value_type = py::cast<int>(value_type_obj);
        } else {
            py::object value_obj = value_type_obj.attr("value");
            value_type = py::cast<int>(value_obj);
        }
        
        std::string description = py::cast<std::string>(py_param.attr("description"));
        py::object value = py_param.attr("value");
        bool is_set = py::cast<bool>(py_param.attr("is_set"));
        
        // Create C++ GenericParameter
        LimonTypes::GenericParameter cpp_param;
        cpp_param.requestType = static_cast<LimonTypes::GenericParameter::RequestParameterTypes>(request_type);
        cpp_param.description = description;
        cpp_param.valueType = static_cast<LimonTypes::GenericParameter::ValueTypes>(value_type);
        cpp_param.isSet = is_set;
        
        // Convert value based on type
        switch (cpp_param.valueType) {
            case LimonTypes::GenericParameter::STRING: {
                std::string str_val = py::cast<std::string>(value);
                strncpy(cpp_param.value.stringValue, str_val.c_str(), sizeof(cpp_param.value.stringValue) - 1);
                cpp_param.value.stringValue[sizeof(cpp_param.value.stringValue) - 1] = '\0';
                break;
            }
            case LimonTypes::GenericParameter::DOUBLE:
                cpp_param.value.doubleValue = py::cast<double>(value);
                break;
            case LimonTypes::GenericParameter::LONG:
                cpp_param.value.longValue = py::cast<long>(value);
                break;
            case LimonTypes::GenericParameter::BOOLEAN:
                cpp_param.value.boolValue = py::cast<bool>(value);
                break;
            case LimonTypes::GenericParameter::VEC4: {
                auto vec = py::cast<std::vector<float>>(value);
                if (vec.size() >= 4) {
                    cpp_param.value.vectorValue.x = vec[0];
                    cpp_param.value.vectorValue.y = vec[1];
                    cpp_param.value.vectorValue.z = vec[2];
                    cpp_param.value.vectorValue.w = vec[3];
                }
                break;
            }
            default:
                cpp_param.value.longValue = 0;
                break;
        }
        
        return cpp_param;
    }

    // Convert Python list to vector of GenericParameter(C++)
    static std::vector<LimonTypes::GenericParameter> convertPythonListToGenericParameterVector(py::object pyList) {
        std::vector<LimonTypes::GenericParameter> result;
        if (pyList.is_none()) {
            return result;
        }
        
        try {
            py::list list = py::cast<py::list>(pyList);
            
            for (const auto& item : list) {
                if (py::isinstance<LimonTypes::GenericParameter>(item)) {
                    // Handle C++ GenericParameter objects
                    result.push_back(py::cast<LimonTypes::GenericParameter>(item));
                } else {
                    // Try to convert Python GenericParameter to C++
                    try {
                        py::object py_param = py::reinterpret_borrow<py::object>(item);
                        LimonTypes::GenericParameter cpp_param = convertPythonToGenericParameter(py_param);
                        result.push_back(cpp_param);
                    } catch (const std::exception& e) {
                        std::cerr << "Error converting Python GenericParameter: " << e.what() << std::endl;
                    }
                }
            }
        } catch (const py::cast_error& e) {
            std::cerr << "Error converting Python list to GenericParameter vector: " << e.what() << std::endl;
        }
        
        return result;
    }

    // Convert C++ GenericParameter to Python object
    static py::object convertGenericParameterToPython(const LimonTypes::GenericParameter& param) {
        // Create the value based on type
        py::object value;
        switch (param.valueType) {
            case LimonTypes::GenericParameter::DOUBLE:
                value = py::cast(param.value.doubleValue);
                break;
            case LimonTypes::GenericParameter::LONG:
                value = py::cast(param.value.longValue);
                break;
            case LimonTypes::GenericParameter::BOOLEAN:
                value = py::cast(param.value.boolValue);
                break;
            case LimonTypes::GenericParameter::STRING:
                value = py::cast(std::string(param.value.stringValue));
                break;
            case LimonTypes::GenericParameter::VEC4: {
                const LimonTypes::Vec4& v = param.value.vectorValue;
                py::list vec_list;
                vec_list.append(v.x);
                vec_list.append(v.y);
                vec_list.append(v.z);
                vec_list.append(v.w);
                value = vec_list;
                break;
            }
            case LimonTypes::GenericParameter::LONG_ARRAY: {
                py::list array_list;
                long size = param.value.longValues[0];
                for (long i = 1; i <= size; ++i) {
                    array_list.append(param.value.longValues[i]);
                }
                value = array_list;
                break;
            }
            case LimonTypes::GenericParameter::MAT4: {
                py::list matrix_list;
                for (int i = 0; i < 4; ++i) {
                    py::list row;
                    for (int j = 0; j < 4; ++j) {
                        row.append(param.value.matrixValue.rows[i][j]);
                    }
                    matrix_list.append(row);
                }
                value = matrix_list;
                break;
            }
            default:
                value = py::none();
                break;
        }

        // Create Python GenericParameter object
        py::module_ generic_param_module = py::module_::import("generic_parameter");
        py::object GenericParameterClass = generic_param_module.attr("GenericParameter");
        return GenericParameterClass(
            static_cast<int>(param.requestType),
            param.description,
            static_cast<int>(param.valueType),
            value,
            param.isSet
        );
    }

    // Helper function to convert vector of GenericParameter(C++) to Python list of GenericParameter objects
    static py::list convertGenericParameterVectorToObjects(const std::vector<LimonTypes::GenericParameter>& params) {
        py::list result;
        
        for (const auto& param : params) {
            try {
                py::object py_param = convertGenericParameterToPython(param);
                result.append(py_param);
            } catch (const std::exception& e) {
                std::cerr << "Error converting parameter: " << e.what() << std::endl;
                // Fallback to raw C++ object
                try {
                    result.append(py::cast(param));
                } catch (const std::exception& e2) {
                    std::cerr << "Even fallback failed: " << e2.what() << std::endl;
                }
            }
        }
        return result;
    }

};
