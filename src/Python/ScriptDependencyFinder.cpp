//
// Created by engin on 15.10.2026.
//

#include "ScriptDependencyFinder.h"

#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/subinterpreter.h>
#include "ScriptImporter.h"

//script folders are searched through LimonFileSystem like ScriptImporter does, so a script that is only inside Data.pak
//is followed too. Standard library lookups go first, as in the importer
static const char *SCRIPT_DEPENDENCY_FINDER_SOURCE = R"PYTHON(
import io
import modulefinder
import limon

class _LimonModuleFinder(modulefinder.ModuleFinder):
    def __init__(self, script_directories):
        super().__init__()
        self.script_directories = script_directories

    def find_module(self, name, path, parent=None):
        if path is None:
            try:
                return super().find_module(name, path, parent)
            except ImportError:
                directories = self.script_directories
        else:
            directories = [directory for directory in path if any(directory.startswith(script_directory) for script_directory in self.script_directories)]
            if not directories:
                return super().find_module(name, path, parent)
        for directory in directories:
            package_directory = directory + "/" + name
            if limon._script_file_exists(package_directory + "/__init__.py"):
                return None, package_directory, ("", "", modulefinder._PKG_DIRECTORY)
            file_path = directory + "/" + name + ".py"
            source = limon._read_script_source(file_path)
            if source is not None:
                return io.BytesIO(source.encode("utf-8")), file_path, (".py", "rb", modulefinder._PY_SOURCE)
        raise ImportError("No module named " + name, name=name)

finder = _LimonModuleFinder(script_directories)
finder.import_hook(module_name)
module_files = [module.__file__ for module in finder.modules.values() if module.__file__]
)PYTHON";

std::map<std::string, ScriptModuleFiles> findScriptModuleFiles(const std::set<std::string> &moduleNames, const std::vector<std::string> &scriptDirectories) {
    std::map<std::string, ScriptModuleFiles> moduleFiles;
    try {
        //create() fetches the error state before it switches to the main interpreter, a thread that never ran Python has none to fetch
        pybind11::subinterpreter_scoped_activate mainActivation(pybind11::subinterpreter::main());
        pybind11::subinterpreter interpreter = pybind11::subinterpreter::create();
        //every Python object below has to be gone before the interpreter is
        pybind11::subinterpreter_scoped_activate activation(interpreter);
        installScriptImporter(scriptDirectories);
        pybind11::list directories;//no pybind11/stl.h here, a vector cast would throw "Unregistered type"
        for (const std::string &directory : scriptDirectories) {
            directories.append(directory);
        }
        for (const std::string &moduleName : moduleNames) {
            try {
                pybind11::dict scope;
                scope["__builtins__"] = pybind11::module_::import("builtins");
                scope["module_name"] = moduleName;
                scope["script_directories"] = directories;
                pybind11::exec(SCRIPT_DEPENDENCY_FINDER_SOURCE, scope);
                for (pybind11::handle moduleFile : pybind11::list(scope["module_files"])) {
                    moduleFiles[moduleName].files.push_back(moduleFile.cast<std::string>());
                }
            } catch (const pybind11::error_already_set &error) {
                moduleFiles[moduleName].failureReason = error.what();
            }
        }
    } catch (const std::exception &error) {
        for (const std::string &moduleName : moduleNames) {
            moduleFiles[moduleName].failureReason = std::string("no interpreter to read imports in: ") + error.what();
        }
    }
    return moduleFiles;
}
