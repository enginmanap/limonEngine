//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_SCRIPTDEPENDENCYFINDER_H
#define LIMONENGINE_SCRIPTDEPENDENCYFINDER_H

#include <map>
#include <set>
#include <string>
#include <vector>

struct ScriptModuleFiles {
    std::vector<std::string> files;
    std::string failureReason;//empty when its imports could be followed
};

//each module and every module it imports, as paths: the ones from scriptDirectories as LimonFileSystem paths, standard library
//ones as disk paths. Reads the import statements without running them, so a module imported by a string at runtime is not found.
//Runs in an interpreter of its own, created on the calling thread, so it never touches a world's interpreter
std::map<std::string, ScriptModuleFiles> findScriptModuleFiles(const std::set<std::string> &moduleNames, const std::vector<std::string> &scriptDirectories);

#endif //LIMONENGINE_SCRIPTDEPENDENCYFINDER_H
