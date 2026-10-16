//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_SCRIPTDEPENDENCYFINDER_H
#define LIMONENGINE_SCRIPTDEPENDENCYFINDER_H

#include <string>
#include <vector>

//the module and every module it imports, as paths: the ones from scriptDirectories as LimonFileSystem paths, standard library
//ones as disk paths. Reads the import statements without running them, so a module imported by a string at runtime is not found.
//Needs an active interpreter
std::vector<std::string> findScriptModuleFiles(const std::string &moduleName, const std::vector<std::string> &scriptDirectories);

#endif //LIMONENGINE_SCRIPTDEPENDENCYFINDER_H
