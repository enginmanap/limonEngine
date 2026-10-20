//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_PACKAGEBUILDER_H
#define LIMONENGINE_PACKAGEBUILDER_H

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include <miniz.h>
#include "../Assets/AssetManager.h"
#include "../Assets/AssetTreeManager.h"
#include "../Assets/ModelAsset.h"
#include "WorldReferenceScanner.h"

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
    enum class Step { NOT_STARTED, READING_WORLDS, CONVERTING_MODELS, FOLLOWING_SCRIPTS, WRITING, DONE };

    struct Progress {
        Step step = Step::NOT_STARTED;
        uint32_t convertedModelCount = 0;
        uint32_t modelCount = 0;//known once the worlds are read, before any model loads
        std::string currentModel;
        uint32_t problemCount = 0;
        bool succeeded = false;
    };

    PackageBuilder(std::shared_ptr<AssetManager> assetManager, ScriptManager *scriptManager, OptionsUtil::Options *options);

    //main thread. Refuses what can't be packed, with the reason, and copies everything run reads that the main thread could change under it.
    //An empty startingWorld packs what Release.xml starts from, any other world becomes the packed Release.xml's StartingWorld
    bool prepare(const std::string &startingWorld, std::string &refusalReason);

    //any thread, after prepare. The result is in getProgress().succeeded too
    bool run(const std::string &outputFile);

    Progress getProgress() const;

    //sorted, a texture's users grouped under it. Once the progress says DONE, the worker adds none after that
    std::vector<std::string> getProblems() const;

private:
    std::shared_ptr<AssetManager> assetManager;
    ScriptManager *scriptManager;
    OptionsUtil::Options *options;
    std::vector<std::pair<std::string, AssetTreeManager::AssetTypes>> assetExtensions;

    std::map<std::string, std::vector<std::string>> registeredTypes;//name to its pack file access list
    std::deque<std::string> worldQueue;
    std::set<std::string> reachedWorlds;
    std::set<std::string> reachedTypes;
    std::map<std::string, std::string> packedFiles;//normalized Data/ path to why it is packed
    std::map<std::string, std::string> modelKeys;//model asset key to why it is packed
    std::map<std::string, std::string> convertedModels;//model asset key to the .limonmodel it became, both normalized
    std::map<std::string, ModelAsset::LimonConversion> conversions;//model asset key to what converting it renamed, for the worlds referring to it
    //model asset key to every object placing it, and the world it is in
    std::map<std::string, std::vector<std::pair<std::string, WorldReferenceScanner::References::PlacedObject>>> placedObjects;
    std::map<std::string, std::set<uint64_t>> worldOverriddenBases;//world file to the base hashes its world overrides replace
    std::set<std::string> modelsReachedUnplaced;//a script can spawn these anywhere, so every base of theirs is packed
    //models convert in parallel and finish in any order, so their findings are kept in sets and grouped when read
    std::vector<std::string> problems;
    std::map<std::string, std::set<std::string>> missingTextureUsers;//texture to the models asking for it
    std::map<std::string, std::set<std::string>> textureUsers;//packed texture to the models using it, report only
    std::map<std::pair<std::string, std::string>, std::set<std::string>> repairedTextureUsers;//named texture and the file found for it, to "model material", report only
    //copied by prepare
    std::map<std::string, std::string> pythonTypeModules;
    std::vector<std::string> scriptDirectories;
    uint32_t maxModelsInFlight = 10;
    std::string startingWorldOverride;

    //the Release.xml on disk with StartingWorld set to startingWorldOverride, or a new one if there is none
    std::vector<uint8_t> buildOverriddenRelease() const;

    mutable std::mutex progressMutex;
    Progress progress;

    void setStep(Step step);

    void addProblem(const std::string &problem);

    void addMissingTexture(const std::string &texture, const std::string &modelKey);

    //caller holds progressMutex
    uint32_t countProblems() const;

    void collectRegisteredTypes();

    bool isModelFile(const std::string &normalizedPath) const;

    void addFile(const std::string &path, const std::string &reason);

    void reachType(const std::string &typeName, const std::string &reason);

    void addDeclaredEntry(const std::string &entry, const std::string &reason);

    void addDeclaredFolder(const std::string &normalizedFolder, const std::string &reason);

    bool scanWorld(const std::string &worldFile);

    void scanRenderPipeline();

    bool convertModel(const LoadRequester::LoadResult &loaded, const std::string &modelKey, const std::string &reason, mz_zip_archive &archive);

    //the rest still go into the .limonmodel, only their textures stay out
    std::vector<std::shared_ptr<Material>> getDrawnBaseMaterials(const std::shared_ptr<ModelAsset> &modelAsset, const std::string &modelKey) const;

    //packer thread, no GL context
    void addTextures(const std::vector<std::shared_ptr<Material>> &materials, const std::string &modelKey);

    void addPythonModules();

    bool addManifest(mz_zip_archive &archive) const;

    bool addPackedFiles(mz_zip_archive &archive) const;

    void rewriteWorldReferences(tinyxml2::XMLDocument &document) const;

    void collectReferencedMaterialKeys(const tinyxml2::XMLElement *element, std::set<std::string> &keys) const;

    //changedKeys: model asset key to the material pool keys its conversion renamed
    void rewriteObjectReferences(tinyxml2::XMLElement *element, const std::map<std::string, std::unordered_set<std::string>> &changedKeys) const;

    void writeReport(const std::string &outputFile) const;
};


#endif //LIMONENGINE_PACKAGEBUILDER_H
