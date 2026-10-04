//
// Created by engin on 23.09.2026.
//

#ifndef LIMONENGINE_LODMETADATA_H
#define LIMONENGINE_LODMETADATA_H

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "LodLadder.h"

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

//the LOD steps of one model, in <asset path>.limon beside it. Two sections because the halves have different
//lifetimes: <LodOverrides> survives a calibrator version bump, <LodCalibration> does not.
//The animation sections share this file and the two flip variants write it from different threads, so every write
//is a read, merge, temp file, atomic replace under one lock
class LodMetadata {
public:
    //flipAxes is empty for the unflipped asset. Mirrored geometry measures differently, so it can not share an entry
    static bool read(const std::string &assetPath, const std::string &flipAxes, uint64_t settingsHash,
                     uint64_t geometryHash, std::vector<LodStep> &outSteps);

    //merges into whatever the file already holds, so the animation sections and the other flip variant survive
    static bool write(const std::string &assetPath, const std::string &flipAxes, uint64_t settingsHash,
                      uint64_t geometryHash, const std::vector<LodStep> &steps);

    //the typed half only: per step, the triangle share and whether the developer set it
    static bool readOverrides(const std::string &assetPath, const std::string &flipAxes,
                              std::vector<LodStep> &outSteps);

    //an empty list removes the overrides, which is how the editor goes back to the project defaults
    static bool writeOverrides(const std::string &assetPath, const std::string &flipAxes,
                               const std::vector<LodStep> &steps);

    //anything writing the metadata file has to hold this and merge, the animation sections share the file
    static std::mutex &getFileMutex();

    //tells one version of a model from another by its counts and bounds, per mesh in order. Deliberately not an
    //exact comparison, see the definition
    static uint64_t hashGeometry(const std::vector<LodLadder::MeshGeometry> &meshes);



private:
    static std::string metadataPathOf(const std::string &assetPath);
    //loads what is already on disk and hands back the root to merge into, creating both if there is no file yet
    static tinyxml2::XMLElement *openForMerge(const std::string &path, tinyxml2::XMLDocument &document);
    static void dropVariant(tinyxml2::XMLElement *sectionNode, const std::string &flipAxes);
    static std::string variantFlipOf(const tinyxml2::XMLElement *variantNode);
    static bool saveOverExisting(tinyxml2::XMLDocument &document, const std::string &path);
    //the variant for this flip inside a named section, after its schema version has been checked
    static tinyxml2::XMLElement *findVariant(tinyxml2::XMLDocument &document, const std::string &assetPath,
                                            const char *sectionName, const std::string &flipAxes);
    //a section's Variant for this flip, created fresh after the old one is dropped
    static tinyxml2::XMLElement *replaceVariant(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *rootNode,
                                                const char *sectionName, const std::string &flipAxes);
    static void writeOutcome(tinyxml2::XMLElement *stepNode, const char *prefix, const LodStep::Outcome &outcome);
    static void readOutcome(const tinyxml2::XMLElement *stepNode, const char *prefix, LodStep::Outcome &outcome);
};

#endif //LIMONENGINE_LODMETADATA_H
