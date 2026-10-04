//
// Created by engin on 23.09.2026.
//

#include "LodMetadata.h"

#include <cstdio>
#include <iostream>
#include <mutex>
#include <tinyxml2.h>
#include "consthash/include/consthash/cityhash64.hxx"
#include <SDL3/SDL_filesystem.h>

static const uint32_t LOD_METADATA_SCHEMA_VERSION = 10;
//two flip variants of one model load in parallel and share this file, and the animation sections live in it too
static std::mutex lodMetadataWriteMutex;

std::mutex &LodMetadata::getFileMutex() {
    return lodMetadataWriteMutex;
}

std::string LodMetadata::metadataPathOf(const std::string &assetPath) {
    return assetPath + ".limon";
}

//re-read rather than write from memory, so a variant or an animation section written meanwhile survives
tinyxml2::XMLElement *LodMetadata::openForMerge(const std::string &path, tinyxml2::XMLDocument &document) {
    tinyxml2::XMLElement *rootNode = nullptr;
    if (document.LoadFile(path.c_str()) == tinyxml2::XML_SUCCESS) {
        rootNode = document.FirstChildElement("ModelCustomizations");
    }
    if (rootNode == nullptr) {
        document.Clear();
        rootNode = document.NewElement("ModelCustomizations");
        document.InsertFirstChild(rootNode);
    }
    return rootNode;
}

std::string LodMetadata::variantFlipOf(const tinyxml2::XMLElement *variantNode) {
    const char *flipAttribute = variantNode->Attribute("flip");
    return flipAttribute == nullptr ? "" : flipAttribute;
}

//the variant being written replaces the one already there, the other flip variant is left alone
void LodMetadata::dropVariant(tinyxml2::XMLElement *sectionNode, const std::string &flipAxes) {
    for (tinyxml2::XMLElement *variantNode = sectionNode->FirstChildElement("Variant");
         variantNode != nullptr; variantNode = variantNode->NextSiblingElement("Variant")) {
        if (variantFlipOf(variantNode) == flipAxes) {
            sectionNode->DeleteChild(variantNode);
            return;
        }
    }
}

tinyxml2::XMLElement *LodMetadata::replaceVariant(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *rootNode,
                                                 const char *sectionName, const std::string &flipAxes) {
    tinyxml2::XMLElement *sectionNode = rootNode->FirstChildElement(sectionName);
    if (sectionNode != nullptr && sectionNode->UnsignedAttribute("schemaVersion") != LOD_METADATA_SCHEMA_VERSION) {
        //everything in there was written against another layout, keeping parts of it would mix two definitions
        rootNode->DeleteChild(sectionNode);
        sectionNode = nullptr;
    }
    if (sectionNode == nullptr) {
        sectionNode = document.NewElement(sectionName);
        sectionNode->SetAttribute("schemaVersion", LOD_METADATA_SCHEMA_VERSION);
        rootNode->InsertEndChild(sectionNode);
    }
    dropVariant(sectionNode, flipAxes);
    tinyxml2::XMLElement *variantNode = document.NewElement("Variant");
    variantNode->SetAttribute("flip", flipAxes.c_str());
    sectionNode->InsertEndChild(variantNode);
    return variantNode;
}

bool LodMetadata::saveOverExisting(tinyxml2::XMLDocument &document, const std::string &path) {
    std::string temporaryPath = path + ".tmp";
    if (document.SaveFile(temporaryPath.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Could not write LOD metadata " << temporaryPath << std::endl;
        return false;
    }
    //SDL replaces the target in one step, so a reader never sees the file missing or half written. Doing it
    //with remove then rename left exactly that window, and the two flip variants of one model share this path
    if (!SDL_RenamePath(temporaryPath.c_str(), path.c_str())) {
        std::cerr << "Could not replace LOD metadata " << path << std::endl;
        return false;
    }
    return true;
}

//the variant for this flip inside a named section, or nullptr. Both readers walk to the same place
tinyxml2::XMLElement *LodMetadata::findVariant(tinyxml2::XMLDocument &document, const std::string &assetPath,
                                              const char *sectionName, const std::string &flipAxes) {
    if (document.LoadFile(metadataPathOf(assetPath).c_str()) != tinyxml2::XML_SUCCESS) {
        return nullptr;//no metadata yet, which is the normal first load
    }
    tinyxml2::XMLElement *rootNode = document.FirstChildElement("ModelCustomizations");
    if (rootNode == nullptr) {
        return nullptr;
    }
    tinyxml2::XMLElement *sectionNode = rootNode->FirstChildElement(sectionName);
    if (sectionNode == nullptr) {
        return nullptr;
    }
    //a block written against another layout would be read field by field into zeros, and a zero distance silently
    //produces an empty ladder. Discarding it is the only safe reading
    if (sectionNode->UnsignedAttribute("schemaVersion") != LOD_METADATA_SCHEMA_VERSION) {
        return nullptr;
    }
    for (tinyxml2::XMLElement *variantNode = sectionNode->FirstChildElement("Variant");
         variantNode != nullptr; variantNode = variantNode->NextSiblingElement("Variant")) {
        if (variantFlipOf(variantNode) == flipAxes) {
            return variantNode;
        }
    }
    return nullptr;
}

//identity, not precision: a stale entry can not corrupt a mesh, targetError is relative so meshopt regenerates
//from whatever is there. It costs a level used at the wrong distance, and any real edit moves a count or the bounds
uint64_t LodMetadata::hashGeometry(const std::vector<LodLadder::MeshGeometry> &meshes) {
    std::string packed;
    char buffer[192];
    //order matters, so the meshes are printed in the order the model holds them
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const LodLadder::MeshGeometry &mesh = meshes[meshIndex];
        //%.9g round trips a float, so two different bounds never print the same
        snprintf(buffer, sizeof(buffer), "%zu,%zu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g;", mesh.indexCount / 3,
                 mesh.vertices == nullptr ? (size_t) 0 : mesh.vertices->size(),
                 mesh.aabbMin.x, mesh.aabbMin.y, mesh.aabbMin.z, mesh.aabbMax.x, mesh.aabbMax.y, mesh.aabbMax.z);
        packed += buffer;
    }
    return consthash::city64(packed.c_str(), packed.length());
}

//%.9g everywhere, because a rounded error would regenerate a different index buffer than the one measured
static void setFloatAttribute(tinyxml2::XMLElement *node, const std::string &name, float value) {
    char numberBuffer[64];
    snprintf(numberBuffer, sizeof(numberBuffer), "%.9g", value);
    node->SetAttribute(name.c_str(), numberBuffer);
}

//one attribute per kind, prefixed, so a limit and its measurement read side by side in the file
static void writePixels(tinyxml2::XMLElement *node, const std::string &prefix, const LodPixels &pixels) {
    setFloatAttribute(node, prefix + "Surface", pixels.surface);
    setFloatAttribute(node, prefix + "Outline", pixels.outline);
    setFloatAttribute(node, prefix + "Holes", pixels.holes);
    setFloatAttribute(node, prefix + "Texture", pixels.texture);
    setFloatAttribute(node, prefix + "Normal", pixels.normal);
}

static void readPixels(const tinyxml2::XMLElement *node, const std::string &prefix, LodPixels &pixels) {
    pixels.surface = node->FloatAttribute((prefix + "Surface").c_str());
    pixels.outline = node->FloatAttribute((prefix + "Outline").c_str());
    pixels.holes = node->FloatAttribute((prefix + "Holes").c_str());
    pixels.texture = node->FloatAttribute((prefix + "Texture").c_str());
    pixels.normal = node->FloatAttribute((prefix + "Normal").c_str());
}

void LodMetadata::writeOutcome(tinyxml2::XMLElement *stepNode, const char *prefix, const LodStep::Outcome &outcome) {
    std::string base(prefix);
    stepNode->SetAttribute((base + "Built").c_str(), outcome.built);
    stepNode->SetAttribute((base + "Skip").c_str(), (uint32_t) outcome.skipReason);
    setFloatAttribute(stepNode, base + "TargetError", outcome.targetError);
    stepNode->SetAttribute((base + "Triangles").c_str(), outcome.triangleCount);
    setFloatAttribute(stepNode, base + "Achieved", outcome.achievedRatio);
    writePixels(stepNode, base + "Measured", outcome.measured);
    stepNode->SetAttribute((base + "Clipped").c_str(), outcome.clipped);
}

void LodMetadata::readOutcome(const tinyxml2::XMLElement *stepNode, const char *prefix, LodStep::Outcome &outcome) {
    std::string base(prefix);
    outcome.built = stepNode->BoolAttribute((base + "Built").c_str());
    outcome.skipReason = (LodSkipReason) stepNode->UnsignedAttribute((base + "Skip").c_str());
    outcome.targetError = stepNode->FloatAttribute((base + "TargetError").c_str());
    outcome.triangleCount = stepNode->UnsignedAttribute((base + "Triangles").c_str());
    outcome.achievedRatio = stepNode->FloatAttribute((base + "Achieved").c_str());
    readPixels(stepNode, base + "Measured", outcome.measured);
    outcome.clipped = stepNode->BoolAttribute((base + "Clipped").c_str());
    //the mesh index is assigned fresh on every build, since which steps are built decides it
    outcome.meshLodIndex = 0;
}

bool LodMetadata::read(const std::string &assetPath, const std::string &flipAxes, uint64_t settingsHash,
                       uint64_t geometryHash, std::vector<LodStep> &outSteps) {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement *variantNode = findVariant(document, assetPath, "LodCalibration", flipAxes);
    if (variantNode == nullptr) {
        return false;
    }
    if (variantNode->Unsigned64Attribute("geometryHash") != geometryHash) {
        return false;//the model itself changed, nothing measured against the old one describes it
    }
    if (variantNode->Unsigned64Attribute("settingsHash") != settingsHash) {
        return false;//a different scorer, search or reference produced these, so they are not ours to reuse
    }
    outSteps.clear();
    for (tinyxml2::XMLElement *stepNode = variantNode->FirstChildElement("Step");
         stepNode != nullptr; stepNode = stepNode->NextSiblingElement("Step")) {
        LodStep step;
        step.distance = stepNode->FloatAttribute("distance");
        readPixels(stepNode, "limit", step.limits);
        step.triangleTarget = stepNode->FloatAttribute("triangleTarget");
        step.requestedRatio = stepNode->FloatAttribute("requestedRatio");
        step.userSet = stepNode->BoolAttribute("userSet");
        readOutcome(stepNode, "structure", step.structure);
        readOutcome(stepNode, "welded", step.welded);
        outSteps.push_back(step);
    }
    return !outSteps.empty();
}

bool LodMetadata::write(const std::string &assetPath, const std::string &flipAxes, uint64_t settingsHash,
                        uint64_t geometryHash, const std::vector<LodStep> &steps) {
    if (steps.empty()) {
        //an empty ladder is a failure to report, not a result to cache. Writing it would also make the next load
        //re-measure and fail the same way in silence
        std::cerr << "no LOD step could be built for " << assetPath << ", nothing cached" << std::endl;
        return false;
    }
    const std::lock_guard<std::mutex> lock(lodMetadataWriteMutex);
    std::string path = metadataPathOf(assetPath);
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement *rootNode = openForMerge(path, document);
    tinyxml2::XMLElement *variantNode = replaceVariant(document, rootNode, "LodCalibration", flipAxes);
    variantNode->SetAttribute("settingsHash", settingsHash);
    variantNode->SetAttribute("geometryHash", geometryHash);
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        tinyxml2::XMLElement *stepNode = document.NewElement("Step");
        stepNode->SetAttribute("index", (uint32_t) (stepIndex + 1));
        setFloatAttribute(stepNode, "distance", steps[stepIndex].distance);
        writePixels(stepNode, "limit", steps[stepIndex].limits);
        setFloatAttribute(stepNode, "triangleTarget", steps[stepIndex].triangleTarget);
        setFloatAttribute(stepNode, "requestedRatio", steps[stepIndex].requestedRatio);
        stepNode->SetAttribute("userSet", steps[stepIndex].userSet);
        writeOutcome(stepNode, "structure", steps[stepIndex].structure);
        writeOutcome(stepNode, "welded", steps[stepIndex].welded);
        variantNode->InsertEndChild(stepNode);
    }
    return saveOverExisting(document, path);
}

bool LodMetadata::readOverrides(const std::string &assetPath, const std::string &flipAxes,
                                std::vector<LodStep> &outSteps) {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement *variantNode = findVariant(document, assetPath, "LodOverrides", flipAxes);
    if (variantNode == nullptr) {
        return false;
    }
    outSteps.clear();
    for (tinyxml2::XMLElement *stepNode = variantNode->FirstChildElement("Step");
         stepNode != nullptr; stepNode = stepNode->NextSiblingElement("Step")) {
        LodStep step;
        step.userSet = stepNode->BoolAttribute("userSet");
        step.requestedRatio = stepNode->FloatAttribute("requestedRatio");
        if (step.userSet && (step.requestedRatio <= 0.0f || step.requestedRatio >= 1.0f)) {
            //a share outside zero to one can never be built, so it is a broken record rather than a strict one. Only
            //this step is dropped: the list is positional, so it keeps its place without a target
            std::cerr << "LOD override step " << outSteps.size() + 1 << " for " << assetPath
                      << " has no usable triangle share, ignoring it" << std::endl;
            step.userSet = false;
            step.requestedRatio = 0.0f;
        }
        outSteps.push_back(step);
    }
    return !outSteps.empty();
}

bool LodMetadata::writeOverrides(const std::string &assetPath, const std::string &flipAxes,
                                 const std::vector<LodStep> &steps) {
    const std::lock_guard<std::mutex> lock(lodMetadataWriteMutex);
    std::string path = metadataPathOf(assetPath);
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement *rootNode = openForMerge(path, document);
    tinyxml2::XMLElement *variantNode = replaceVariant(document, rootNode, "LodOverrides", flipAxes);
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        tinyxml2::XMLElement *stepNode = document.NewElement("Step");
        stepNode->SetAttribute("index", (uint32_t) (stepIndex + 1));
        stepNode->SetAttribute("userSet", steps[stepIndex].userSet);
        setFloatAttribute(stepNode, "requestedRatio", steps[stepIndex].requestedRatio);
        variantNode->InsertEndChild(stepNode);
    }
    return saveOverExisting(document, path);
}
