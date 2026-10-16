//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_PACKAGEBUILDER_H
#define LIMONENGINE_PACKAGEBUILDER_H

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../Assets/AssetManager.h"

class ScriptManager;
class TextureAsset;
namespace OptionsUtil {
    class Options;
}

//packs what the game reaches from Release.xml: the worlds, what they name, the types they use and the files those types
//declare. Raw models are converted, the package carries .limonmodel files and a manifest instead of them.
//Everything is read through LimonFileSystem, so a file that is only in a mounted Data.pak is packed again
class PackageBuilder {
public:

    PackageBuilder(std::shared_ptr<AssetManager> assetManager, ScriptManager *scriptManager, OptionsUtil::Options *options);

    bool build(const std::string &outputFile);

private:
    std::shared_ptr<AssetManager> assetManager;
    ScriptManager *scriptManager;
    OptionsUtil::Options *options;
    std::vector<std::pair<std::string, AssetManager::AssetTypes>> assetExtensions;

    std::map<std::string, std::vector<std::string>> registeredTypes;//name to its pack file access list
    std::deque<std::string> worldQueue;
    std::set<std::string> reachedWorlds;
    std::set<std::string> reachedTypes;
    std::map<std::string, std::string> packedFiles;//normalized Data/ path to why it is packed
    std::map<std::string, std::string> modelKeys;//model asset key to why it is packed
    std::map<std::string, std::string> convertedModels;//model asset key to the .limonmodel it became, both normalized
    std::vector<std::string> problems;

    void collectRegisteredTypes();

    bool isModelFile(const std::string &normalizedPath) const;

    void addFile(const std::string &path, const std::string &reason);

    void reachType(const std::string &typeName, const std::string &reason);

    void addDeclaredEntry(const std::string &entry, const std::string &reason);

    void addDeclaredFolder(const std::string &normalizedFolder, const std::string &reason);

    bool scanWorld(const std::string &worldFile);

    void scanRenderPipeline();

    bool convertModel(const std::string &modelKey, const std::string &reason, const std::string &stagingDirectory);

    void addTexture(const std::shared_ptr<TextureAsset> &texture, const std::string &modelKey);

    void addPythonModules();

    bool writeManifest(const std::string &manifestFile) const;

    bool writeArchive(const std::string &outputFile, const std::string &stagingDirectory) const;

    void writeReport(const std::string &outputFile) const;
};


#endif //LIMONENGINE_PACKAGEBUILDER_H
