//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_WORLDREFERENCESCANNER_H
#define LIMONENGINE_WORLDREFERENCESCANNER_H

#include <set>
#include <string>
#include <tinyxml2.h>

//what a world names, read from its XML without loading it. The packer ships these, and plugin and script file access is allowed to them
class WorldReferenceScanner {
public:
    struct References {
        std::set<std::string> files;//normalized, existing files under Data/
        std::set<std::string> worlds;//the ones among files whose root is <World>
        std::set<std::string> typeNames;//text that equals a registered type name, a coincidental match only ships something extra
    };

    //typeNames is what a match is looked for in, the access policy needs files only and passes none
    static void scanDocument(const std::string &worldFile, const tinyxml2::XMLDocument &document,
                             const std::set<std::string> &typeNames, References &references);

    //the same rules over any XML, render pipelines name their render methods this way
    static void scanTypeNames(const tinyxml2::XMLDocument &document, const std::set<std::string> &typeNames, References &references);

private:
    static void collectTextReferences(const tinyxml2::XMLElement *element, const std::set<std::string> &typeNames, References &references);

    static void collectBuiltReferences(const std::string &worldFile, const tinyxml2::XMLElement *worldElement, References &references);

    static void addIfDataFile(const std::string &path, References &references);

    static bool isWorldFile(const std::string &normalizedPath);
};


#endif //LIMONENGINE_WORLDREFERENCESCANNER_H
