//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_FILEACCESSPOLICY_H
#define LIMONENGINE_FILEACCESSPOLICY_H

#include <string>
#include <vector>
#include <tinyxml2.h>

//Data/ paths plugins and scripts may use: what a loaded world references, or what a registered type declares.
//Checked in loose development too, so a missing declaration fails here instead of only in a packed game
class FileAccessPolicy {
public:
    static void addWorld(const std::string &worldFile, const tinyxml2::XMLDocument &document);

    static void removeWorld(const std::string &worldFile);

    //prints the path and the fix when it refuses
    static bool isAllowed(const std::string &path);

private:
    static bool isDeclared(const std::string &normalizedPath);

    static bool isInList(const std::vector<std::string> &packFileAccessList, const std::string &normalizedPath);
};


#endif //LIMONENGINE_FILEACCESSPOLICY_H
