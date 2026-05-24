#ifndef LIMONENGINE_PYTHONGENERICPARAMETERBINDING_H
#define LIMONENGINE_PYTHONGENERICPARAMETERBINDING_H

#include <pybind11/pybind11.h>

void bindEnums(pybind11::module_& m);
void bindGenericParameter(pybind11::module_& m);

#endif //LIMONENGINE_PYTHONGENERICPARAMETERBINDING_H
