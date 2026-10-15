//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_SCRIPTIMPORTER_H
#define LIMONENGINE_SCRIPTIMPORTER_H

#include <string>
#include <vector>
#include <pybind11/pybind11.h>

//limon._read_script_source and limon._script_file_exists, the importer reads scripts through them
void bindScriptImporter(pybind11::module_& m);

//once per interpreter, after the limon module can be imported. Directories are searched in order, built-ins first
void installScriptImporter(const std::vector<std::string>& scriptDirectories);

#endif //LIMONENGINE_SCRIPTIMPORTER_H
