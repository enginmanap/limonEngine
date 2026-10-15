//
// Created by engin on 15.10.2026.
//

#include "ScriptImporter.h"

#include <pybind11/eval.h>
#include "../LimonFileSystem.h"

//scripts are imported through LimonFileSystem, so the ones inside Data.pak load too. Appended after the path finder,
//like the script directories used to be appended to sys.path, so a script can't shadow a standard library module
static const char *SCRIPT_IMPORTER_SOURCE = R"PYTHON(
import sys
import importlib.abc
import importlib.util
import limon

class _LimonScriptLoader(importlib.abc.InspectLoader):
    def __init__(self, file_path, is_package):
        self.file_path = file_path
        self.package = is_package

    def is_package(self, fullname):
        return self.package

    def get_filename(self, fullname):
        return self.file_path

    def get_source(self, fullname):
        return limon._read_script_source(self.file_path)

    def exec_module(self, module):
        source = self.get_source(module.__name__)
        if source is None:
            raise ImportError("could not read " + self.file_path, name=module.__name__)
        exec(compile(source, self.file_path, "exec", dont_inherit=True), module.__dict__)

class _LimonScriptFinder(importlib.abc.MetaPathFinder):
    limon_script_finder = True

    def __init__(self, directories):
        self.directories = directories

    def find_spec(self, fullname, path=None, target=None):
        relative_path = fullname.replace(".", "/")
        for directory in self.directories:
            for candidate, is_package in ((relative_path + "/__init__.py", True), (relative_path + ".py", False)):
                file_path = directory + "/" + candidate
                if limon._script_file_exists(file_path):
                    spec = importlib.util.spec_from_loader(fullname, _LimonScriptLoader(file_path, is_package),
                                                           origin=file_path, is_package=is_package)
                    spec.has_location = True
                    if is_package:
                        spec.submodule_search_locations = [directory + "/" + relative_path]
                    return spec
        return None

if not any(getattr(finder, "limon_script_finder", False) for finder in sys.meta_path):
    sys.meta_path.append(_LimonScriptFinder(script_directories))
)PYTHON";

static pybind11::object readScriptSource(const std::string &path) {
    std::string source;
    if (!LimonFileSystem::readFile(path, source)) {
        return pybind11::none();
    }
    return pybind11::str(source);
}

static bool scriptFileExists(const std::string &path) {
    return LimonFileSystem::fileExists(path);
}

void bindScriptImporter(pybind11::module_& m) {
    m.def("_read_script_source", &readScriptSource);
    m.def("_script_file_exists", &scriptFileExists);
}

void installScriptImporter(const std::vector<std::string>& scriptDirectories) {
    pybind11::dict scope;
    scope["__builtins__"] = pybind11::module_::import("builtins");
    pybind11::list directories;//no pybind11/stl.h here, a vector cast would throw "Unregistered type"
    for (const std::string &directory : scriptDirectories) {
        directories.append(directory);
    }
    scope["script_directories"] = directories;
    pybind11::exec(SCRIPT_IMPORTER_SOURCE, scope);
}
