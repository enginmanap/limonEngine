//
// Defines the embedded 'limon' Python module.
// All binding logic lives in the four helpers below; this file is intentionally thin.
//
// IMPORTANT: This TU has no exported symbols that other TUs reference, so some linkers
// (MinGW/ld) will discard it via dead-code elimination, silently dropping the
// PYBIND11_EMBEDDED_MODULE static registration. ensurePythonModuleBindingsLinked() is
// an anchor symbol that ScriptManager.cpp calls to prevent this.
//
#include <Python.h>
#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>

#include "PythonGenericParameterBinding.h"
#include "PythonLimonAPIBinding.h"
#include "PythonInterfaceBindings.h"

void ensurePythonModuleBindingsLinked() {}

// ---------------------------------------------------------
// EXPOSE C++ TO PYTHON (The Embedded Module)
// ---------------------------------------------------------
PYBIND11_EMBEDDED_MODULE(limon, m, pybind11::multiple_interpreters::per_interpreter_gil()) {
#ifdef PYTHON_DEBUGGING
    std::cout << "[limon module] Current interpreter: " << PyThreadState_GetInterpreter(PyThreadState_Get()) << std::endl;
    std::cout << "[limon module] Thread state: " << PyThreadState_Get() << std::endl;
#endif

    bool alreadyRegistered = false;
    try {
        pybind11::module_ testModule = pybind11::module_::import("limon");
        if (pybind11::hasattr(testModule, "TriggerInterface")) {
            alreadyRegistered = true;
        }
    } catch (...) {
        // Module not yet imported, so not registered
    }

    if (alreadyRegistered) {
#ifdef PYTHON_DEBUGGING
        std::cout << "[limon module] Classes already registered in this interpreter, skipping bindings" << std::endl;
#endif
        return;
    }

    m.doc() = "Python bindings for Limon Engine";
#ifdef PYTHON_DEBUGGING
    std::cout << "[limon module] Module initialization started" << std::endl;
#endif

    bindEnums(m);
    bindGenericParameter(m);
    bindLimonAPI(m);
    bindInterfaces(m);
}
