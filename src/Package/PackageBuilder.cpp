//
// Created by engin on 15.10.2026.
//

#include "PackageBuilder.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <pybind11/embed.h>
#include "EngineDataFiles.h"
#include "WorldReferenceScanner.h"
#include "../LimonFileSystem.h"
#include "../Assets/ModelAsset.h"
#include "../Assets/TextureAsset.h"
#include "../Material.h"
#include "../GameObjects/Model.h"
#include "../Python/ScriptManager.h"
#include "../Python/ScriptDependencyFinder.h"
#include "limonAPI/Options.h"
#include "limonAPI/TriggerInterface.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "limonAPI/CameraExtensionInterface.h"
#include "limonAPI/Graphics/RenderMethodInterface.h"


static bool endsWith(const std::string &text, const std::string &ending) {
    return text.size() >= ending.size() && text.compare(text.size() - ending.size(), ending.size(), ending) == 0;
}

PackageBuilder::PackageBuilder(std::shared_ptr<AssetManager> assetManager, ScriptManager *scriptManager, OptionsUtil::Options *options)
        : assetManager(assetManager), scriptManager(scriptManager), options(options) {}

void PackageBuilder::collectRegisteredTypes() {
    for (const std::string &typeName : TriggerInterface::getTriggerNames()) {
        registeredTypes[typeName] = TriggerInterface::getRegisteredPackFileAccessList(typeName);
    }
    for (const std::string &typeName : ActorInterface::getActorNames()) {
        registeredTypes[typeName] = ActorInterface::getRegisteredPackFileAccessList(typeName);
    }
    for (const std::string &typeName : PlayerExtensionInterface::getExtensionNames()) {
        registeredTypes[typeName] = PlayerExtensionInterface::getRegisteredPackFileAccessList(typeName);
    }
    for (const std::string &typeName : CameraExtensionInterface::getExtensionNames()) {
        registeredTypes[typeName] = CameraExtensionInterface::getRegisteredPackFileAccessList(typeName);
    }
    for (const std::string &typeName : RenderMethodInterface::getRenderMethodNames()) {
        registeredTypes[typeName] = RenderMethodInterface::getRegisteredPackFileAccessList(typeName);
    }
}

bool PackageBuilder::isModelFile(const std::string &normalizedPath) const {
    std::string lowerPath = normalizedPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    for (const std::pair<std::string, AssetManager::AssetTypes> &extension : assetExtensions) {
        if (extension.second == AssetManager::Asset_type_MODEL && endsWith(lowerPath, extension.first)) {
            return true;
        }
    }
    return false;
}

void PackageBuilder::addFile(const std::string &path, const std::string &reason) {
    std::string normalizedPath = LimonFileSystem::normalizePath(path);
    if (normalizedPath.compare(0, 5, "Data/") != 0) {
        return;//Engine/ ships with the engine, anything else is never packed
    }
    if (endsWith(normalizedPath, ".limon")) {
        return;//a sidecar, the converted model carries what it held
    }
    if (isModelFile(normalizedPath)) {
        if (normalizedPath.find("/Mixamo/") != std::string::npos) {
            return;//animations the model next to the Mixamo folder absorbs, loaded alone they have no mesh and stop the engine
        }
        if (!assetManager->isModelAvailable(normalizedPath)) {
            problems.push_back(reason + " names " + normalizedPath + ", which does not exist");
            return;
        }
        std::vector<std::string> packedModelKeys = assetManager->getPackedModelKeys(normalizedPath);
        if (!packedModelKeys.empty()) {
            //listing a folder of a mounted package finds the conversions, the keys they came from are what is asked for
            for (const std::string &packedModelKey : packedModelKeys) {
                modelKeys.emplace(packedModelKey, reason);
            }
            return;
        }
        modelKeys.emplace(normalizedPath, reason);
        return;
    }
    if (!LimonFileSystem::fileExists(normalizedPath)) {
        problems.push_back(reason + " names " + normalizedPath + ", which does not exist");
        return;
    }
    if (!packedFiles.emplace(normalizedPath, reason).second) {
        return;
    }
    if (endsWith(normalizedPath, ".xml")) {
        tinyxml2::XMLDocument document;
        if (LimonFileSystem::loadXML(normalizedPath, document) == tinyxml2::XML_SUCCESS && document.FirstChildElement("World") != nullptr) {
            worldQueue.push_back(normalizedPath);
        }
    }
}

void PackageBuilder::reachType(const std::string &typeName, const std::string &reason) {
    std::map<std::string, std::vector<std::string>>::const_iterator registeredType = registeredTypes.find(typeName);
    if (registeredType == registeredTypes.end() || !reachedTypes.insert(typeName).second) {
        return;
    }
    for (const std::string &entry : registeredType->second) {
        addDeclaredEntry(entry, "declared by " + typeName + ", " + reason);
    }
}

void PackageBuilder::addDeclaredEntry(const std::string &entry, const std::string &reason) {
    if (!entry.empty() && (entry.back() == '/' || entry.back() == '\\')) {
        addDeclaredFolder(LimonFileSystem::normalizePath(entry), reason);
    } else {
        addFile(entry, reason);
    }
}

void PackageBuilder::addDeclaredFolder(const std::string &normalizedFolder, const std::string &reason) {
    std::vector<LimonFileSystem::DirectoryEntry> entries;
    if (!LimonFileSystem::listDirectory(normalizedFolder, entries)) {
        problems.push_back(reason + " names folder " + normalizedFolder + ", which does not exist");
        return;
    }
    for (const LimonFileSystem::DirectoryEntry &entry : entries) {
        if (entry.isDirectory) {
            addDeclaredFolder(normalizedFolder + "/" + entry.name, reason);
        } else {
            addFile(normalizedFolder + "/" + entry.name, reason);
        }
    }
}

bool PackageBuilder::scanWorld(const std::string &worldFile) {
    if (!reachedWorlds.insert(worldFile).second) {
        return true;
    }
    tinyxml2::XMLDocument document;
    if (LimonFileSystem::loadXML(worldFile, document) != tinyxml2::XML_SUCCESS) {
        problems.push_back("world " + worldFile + " could not be read");
        return false;
    }
    std::set<std::string> typeNames;
    for (const std::pair<const std::string, std::vector<std::string>> &registeredType : registeredTypes) {
        typeNames.insert(registeredType.first);
    }
    WorldReferenceScanner::References references;
    WorldReferenceScanner::scanDocument(worldFile, document, typeNames, references);

    std::set<std::string> placedModelFiles;
    for (const std::string &modelKey : references.modelKeys) {
        std::string modelFile = modelKey.substr(0, modelKey.rfind("?flip"));
        if (!assetManager->isModelAvailable(modelKey)) {
            problems.push_back(worldFile + " places " + modelFile + ", which does not exist");
            continue;
        }
        placedModelFiles.insert(modelFile);
        modelKeys.emplace(modelKey, "placed in " + worldFile);
    }
    for (const std::string &file : references.files) {
        if (placedModelFiles.count(file) == 0) {//a model is packed for the flips it is placed with, not unflipped as well
            addFile(file, "referenced by " + worldFile);
        }
    }
    for (const std::string &typeName : references.typeNames) {
        reachType(typeName, "used by " + worldFile);
    }
    return true;
}

void PackageBuilder::scanRenderPipeline() {
    std::string pipelineFile = options->getOption<std::string>(options->getHash("render_pipeline")).getOrDefault("./Engine/renderPipeline.xml");
    tinyxml2::XMLDocument document;
    if (LimonFileSystem::loadXML(pipelineFile, document) != tinyxml2::XML_SUCCESS) {
        problems.push_back("render pipeline " + pipelineFile + " could not be read");
        return;
    }
    addFile(pipelineFile, "the render pipeline");
    std::set<std::string> renderMethodNames;
    for (const std::string &renderMethodName : RenderMethodInterface::getRenderMethodNames()) {
        renderMethodNames.insert(renderMethodName);
    }
    WorldReferenceScanner::References references;
    WorldReferenceScanner::scanTypeNames(document, renderMethodNames, references);
    for (const std::string &typeName : references.typeNames) {
        reachType(typeName, "used by the render pipeline " + pipelineFile);
    }
}

bool PackageBuilder::convertModel(const std::string &modelKey, const std::string &reason, mz_zip_archive &archive) {
    size_t flipPosition = modelKey.rfind("?flip");
    std::string modelFile = modelKey.substr(0, flipPosition);
    std::string loadKey = "./" + modelKey;//the asset tree, and so the Mixamo lookup, knows models by their ./Data/ path

    std::string failureReason;
    std::shared_ptr<ModelAsset> modelAsset = assetManager->loadAssetForConversion<ModelAsset>({loadKey}, failureReason);
    if (modelAsset == nullptr) {
        problems.push_back(modelKey + " could not be loaded, " + reason + ": " + failureReason);
        return false;
    }

    bool converted = true;
    if (endsWith(modelFile, ".limonmodel")) {
        packedFiles.emplace(modelFile, reason);
    } else {
        std::string packedModel = modelFile;
        if (flipPosition != std::string::npos) {
            packedModel += "_flip" + modelKey.substr(flipPosition + 5);//windows file names can not hold the ? of the key
        }
        packedModel += ".limonmodel";
        std::ostringstream modelStream;
        ModelAsset::LimonConversion conversion;
        bool serialized = modelAsset->convertToLimon("./" + packedModel, modelStream, conversion);//the key AssetManager::resolvePackedModel loads it by
        std::string modelBytes = modelStream.str();
        if (serialized && mz_zip_writer_add_mem(&archive, packedModel.substr(5).c_str(), modelBytes.data(), modelBytes.size(), MZ_NO_COMPRESSION)) {
            convertedModels[modelKey] = packedModel;
            for (const std::pair<const size_t, size_t> &materialHash : conversion.materialHashes) {
                if (materialHash.first != materialHash.second) {//only embedded textures change a material, worlds of other models stay byte for byte
                    conversions[modelKey] = conversion;
                    break;
                }
            }
        } else {
            problems.push_back(modelKey + " could not be converted, " + reason);
            converted = false;
        }
    }

    for (const std::pair<const std::string, std::shared_ptr<Material>> &material : modelAsset->getMaterialMap()) {
        addTexture(material.second->getAmbientTexture(), modelKey);
        addTexture(material.second->getDiffuseTexture(), modelKey);
        addTexture(material.second->getSpecularTexture(), modelKey);
        addTexture(material.second->getNormalTexture(), modelKey);
        addTexture(material.second->getOpacityTexture(), modelKey);
    }
    assetManager->freeAsset({loadKey});
    return converted;
}

void PackageBuilder::addTexture(const std::shared_ptr<TextureAsset> &texture, const std::string &modelKey) {
    if (texture == nullptr) {
        return;
    }
    std::vector<std::string> textureName = texture->getName();
    if (textureName.size() != 1) {
        return;//embedded textures are keyed with their model and live inside the .limonmodel
    }
    if (texture->hasLoadFailed()) {
        problems.push_back(modelKey + " asks for texture " + textureName[0] + ", which could not be loaded");
    } else {
        addFile(textureName[0], "texture of " + modelKey);
    }
}

void PackageBuilder::addPythonModules() {
    for (const std::pair<const std::string, std::string> &typeModule : scriptManager->getPythonTypeModules()) {
        if (reachedTypes.count(typeModule.first) == 0) {
            continue;
        }
        std::vector<std::string> moduleFiles;
        try {
            moduleFiles = findScriptModuleFiles(typeModule.second, scriptManager->getScriptDirectories());
        } catch (const pybind11::error_already_set &error) {
            problems.push_back("imports of " + typeModule.second + " could not be followed: " + error.what());
            continue;
        }
        bool moduleFound = false;
        for (const std::string &moduleFile : moduleFiles) {
            std::string normalizedModuleFile = LimonFileSystem::normalizePath(moduleFile);
            if (normalizedModuleFile.compare(0, 5, "Data/") == 0) {
                addFile(normalizedModuleFile, "script of " + typeModule.first);
                moduleFound = true;
            } else if (normalizedModuleFile.compare(0, 7, "Engine/") == 0) {
                moduleFound = true;//Engine/Scripts ships with the engine
            }
        }
        if (!moduleFound) {
            problems.push_back(typeModule.first + " is used, but its module " + typeModule.second + " was found in neither script folder");
        }
    }
}

bool PackageBuilder::addManifest(mz_zip_archive &archive) const {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement *rootElement = document.NewElement("PackManifest");
    document.InsertFirstChild(rootElement);
    for (const std::pair<const std::string, std::string> &convertedModel : convertedModels) {
        tinyxml2::XMLElement *modelElement = document.NewElement("Model");
        modelElement->SetAttribute("key", convertedModel.first.c_str());
        modelElement->SetAttribute("file", convertedModel.second.c_str());
        rootElement->InsertEndChild(modelElement);
    }
    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    std::string manifestEntry = LimonFileSystem::normalizePath(EngineDataFiles::PACKAGE_MANIFEST).substr(5);
    if (!mz_zip_writer_add_mem(&archive, manifestEntry.c_str(), printer.CStr(), printer.CStrSize() - 1, MZ_NO_COMPRESSION)) {//CStrSize counts the terminator
        std::cerr << "Could not add the model manifest: " << mz_zip_get_error_string(mz_zip_get_last_error(&archive)) << std::endl;
        return false;
    }
    return true;
}

bool PackageBuilder::addPackedFiles(mz_zip_archive &archive) const {
    bool written = true;
    //stored, not deflated: most of a game is already compressed images and sound, deflate waits for load time measurements
    for (const std::pair<const std::string, std::string> &packedFile : packedFiles) {
        std::vector<uint8_t> contents;
        if (!LimonFileSystem::readFile(packedFile.first, contents)) {
            std::cerr << "Could not read " << packedFile.first << std::endl;
            written = false;
            continue;
        }
        if (reachedWorlds.count(packedFile.first) != 0 && !conversions.empty()) {
            //the world names raw models, the package loads their conversions, so what it says about their materials has to follow
            tinyxml2::XMLDocument document;
            if (document.Parse(reinterpret_cast<const char *>(contents.data()), contents.size()) != tinyxml2::XML_SUCCESS) {
                std::cerr << "Could not parse " << packedFile.first << " to update its references to converted models" << std::endl;
                written = false;
                continue;
            }
            rewriteWorldReferences(document);
            tinyxml2::XMLPrinter printer;
            document.Print(&printer);
            contents.assign(printer.CStr(), printer.CStr() + printer.CStrSize() - 1);//CStrSize counts the terminator
        }
        if (!mz_zip_writer_add_mem(&archive, packedFile.first.substr(5).c_str(), contents.data(), contents.size(), MZ_NO_COMPRESSION)) {
            std::cerr << "Could not add " << packedFile.first << ": " << mz_zip_get_error_string(mz_zip_get_last_error(&archive)) << std::endl;
            written = false;
        }
    }
    return written;
}

void PackageBuilder::rewriteWorldReferences(tinyxml2::XMLDocument &document) const {
    tinyxml2::XMLElement *worldNode = document.FirstChildElement("World");
    if (worldNode == nullptr) {
        return;
    }
    rewriteObjectReferences(worldNode);
    //world wide overrides name the material they replace by OriginalHash, which model it belonged to is not written
    tinyxml2::XMLElement *materialsNode = worldNode->FirstChildElement("Materials");
    if (materialsNode == nullptr) {
        return;
    }
    std::vector<tinyxml2::XMLElement *> materialNodes;
    for (tinyxml2::XMLElement *materialNode = materialsNode->FirstChildElement("Material"); materialNode != nullptr;
         materialNode = materialNode->NextSiblingElement("Material")) {
        materialNodes.push_back(materialNode);
    }
    //flip variants of one source are one material until converted, then each file has its own, so an entry may become several
    for (tinyxml2::XMLElement *materialNode : materialNodes) {
        tinyxml2::XMLElement *insertAfter = materialNode;
        bool replaced = false;
        for (const std::pair<const std::string, ModelAsset::LimonConversion> &conversion : conversions) {
            tinyxml2::XMLElement *convertedNode = materialNode->DeepClone(&document)->ToElement();
            if (Material::updateSavedReferences(convertedNode, conversion.second.fromOwner, conversion.second.toOwner,
                                                      conversion.second.materialHashes)) {
                materialsNode->InsertAfterChild(insertAfter, convertedNode);
                insertAfter = convertedNode;
                replaced = true;
            } else {
                document.DeleteNode(convertedNode);
            }
        }
        if (replaced) {
            materialsNode->DeleteChild(materialNode);
        }
    }
}

//objects sit at the top level, under parents, in groups and as player attachments, the same walk the scanner does
void PackageBuilder::rewriteObjectReferences(tinyxml2::XMLElement *element) const {
    for (tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (std::string(child->Name()) == "Object") {
            const tinyxml2::XMLElement *fileElement = child->FirstChildElement("File");
            const tinyxml2::XMLElement *flipElement = child->FirstChildElement("Flip");
            if (fileElement != nullptr && fileElement->GetText() != nullptr) {
                std::string modelKey = LimonFileSystem::normalizePath(fileElement->GetText());
                if (flipElement != nullptr && flipElement->GetText() != nullptr && flipElement->GetText()[0] != '\0') {
                    modelKey += std::string("?flip") + flipElement->GetText();
                }
                std::map<std::string, ModelAsset::LimonConversion>::const_iterator conversion = conversions.find(modelKey);
                if (conversion != conversions.end()) {
                    Model::updateSavedReferences(child, conversion->second);
                }
            }
        }
        rewriteObjectReferences(child);
    }
}

void PackageBuilder::writeReport(const std::string &outputFile) const {
    std::ofstream report(outputFile + ".report.txt");
    report << "Package " << outputFile << " from the loose Data/" << (LimonFileSystem::isPackageMounted() ? " over the mounted Data.pak" : "")
           << std::endl << std::endl;
    report << "Worlds (" << reachedWorlds.size() << ")" << std::endl;
    for (const std::string &world : reachedWorlds) {
        report << "  " << world << std::endl;
    }
    report << std::endl << "Types (" << reachedTypes.size() << ")" << std::endl;
    for (const std::string &typeName : reachedTypes) {
        report << "  " << typeName << std::endl;
    }
    report << std::endl << "Converted models (" << convertedModels.size() << ")" << std::endl;
    for (const std::pair<const std::string, std::string> &convertedModel : convertedModels) {
        std::map<std::string, std::string>::const_iterator reason = modelKeys.find(convertedModel.first);
        report << "  " << convertedModel.first << " -> " << convertedModel.second
               << (reason == modelKeys.end() ? "" : "    (" + reason->second + ")") << std::endl;
    }
    report << std::endl << "Files (" << packedFiles.size() << ")" << std::endl;
    for (const std::pair<const std::string, std::string> &packedFile : packedFiles) {
        report << "  " << packedFile.first << "    (" << packedFile.second << ")" << std::endl;
    }
    report << std::endl << "Problems (" << problems.size() << ")" << std::endl;
    for (const std::string &problem : problems) {
        report << "  " << problem << std::endl;
    }
}

bool PackageBuilder::build(const std::string &outputFile) {
    if (!scriptManager->hasActiveSubInterpreter()) {
        std::cerr << "Packing needs an active Python interpreter, its registered types are what worlds are matched against." << std::endl;
        return false;
    }
    assetExtensions = assetManager->getAssetExtensions();
    collectRegisteredTypes();

    for (const std::string &engineDataFile : EngineDataFiles::getAll()) {
        if (LimonFileSystem::fileExists(engineDataFile)) {
            addFile(engineDataFile, "read by the engine");
        }
    }
    tinyxml2::XMLDocument releaseDocument;
    const tinyxml2::XMLElement *startingWorldElement = nullptr;
    if (LimonFileSystem::loadXML(EngineDataFiles::RELEASE, releaseDocument) == tinyxml2::XML_SUCCESS &&
        releaseDocument.FirstChildElement("Release") != nullptr) {
        startingWorldElement = releaseDocument.FirstChildElement("Release")->FirstChildElement("StartingWorld");
    }
    if (startingWorldElement == nullptr || startingWorldElement->GetText() == nullptr) {
        std::cerr << "Packing needs " << EngineDataFiles::RELEASE << " with a StartingWorld, the package is what that world reaches." << std::endl;
        return false;
    }
    addFile(startingWorldElement->GetText(), "the starting world in Release.xml");

    scanRenderPipeline();
    bool succeeded = true;
    while (!worldQueue.empty()) {
        std::string worldFile = worldQueue.front();
        worldQueue.pop_front();
        succeeded = scanWorld(worldFile) && succeeded;
    }

    std::error_code removeError;
    std::filesystem::remove(outputFile, removeError);
    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    if (!mz_zip_writer_init_file(&archive, outputFile.c_str(), 0)) {
        std::cerr << "Could not create " << outputFile << ": " << mz_zip_get_error_string(mz_zip_get_last_error(&archive)) << std::endl;
        return false;
    }
    //each converted model goes into the archive right away, holding them all would keep the whole pack in memory
    std::map<std::string, std::string> modelsToConvert = modelKeys;//texture files found while converting never add models
    for (const std::pair<const std::string, std::string> &modelKey : modelsToConvert) {
        std::cout << "--pack: converting " << modelKey.first << std::endl;
        succeeded = convertModel(modelKey.first, modelKey.second, archive) && succeeded;
    }
    std::cout << "--pack: following the imports of " << reachedTypes.size() << " reached types' scripts" << std::endl;
    addPythonModules();

    if (succeeded) {
        std::cout << "--pack: writing " << outputFile << std::endl;
        succeeded = addPackedFiles(archive) && addManifest(archive);
    }
    if (succeeded && !mz_zip_writer_finalize_archive(&archive)) {
        std::cerr << "Could not finish " << outputFile << ": " << mz_zip_get_error_string(mz_zip_get_last_error(&archive)) << std::endl;
        succeeded = false;
    }
    mz_zip_writer_end(&archive);
    if (!succeeded) {
        std::filesystem::remove(outputFile, removeError);//half an archive must not be left where a game would mount it
    }
    writeReport(outputFile);
    std::cout << "--pack: " << packedFiles.size() << " files and " << convertedModels.size() << " converted models from "
              << reachedWorlds.size() << " worlds, " << problems.size() << " problems, see " << outputFile << ".report.txt" << std::endl;
    return succeeded;
}
