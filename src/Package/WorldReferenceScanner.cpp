//
// Created by engin on 15.10.2026.
//

#include "WorldReferenceScanner.h"

#include <sstream>

#include "../LimonFileSystem.h"

void WorldReferenceScanner::scanDocument(const std::string &worldFile, const tinyxml2::XMLDocument &document,
                                         const std::set<std::string> &typeNames, References &references) {
    addDataPath(worldFile, references);
    const tinyxml2::XMLElement *worldElement = document.FirstChildElement("World");
    if (worldElement == nullptr) {
        return;
    }
    collectTextReferences(worldElement, typeNames, references);
    collectBuiltReferences(worldFile, worldElement, references);
    std::set<std::string> materialKeys;
    collectWorldOverrides(worldElement, materialKeys, references);
    collectModelKeys(worldElement, materialKeys, references);
}

//the pool's keys, and the bases its WorldOverride entries replace, the same entries WorldLoader::loadMaterials reads
void WorldReferenceScanner::collectWorldOverrides(const tinyxml2::XMLElement *worldElement, std::set<std::string> &materialKeys, References &references) {
    const tinyxml2::XMLElement *materialsElement = worldElement->FirstChildElement("Materials");
    if (materialsElement == nullptr) {
        return;
    }
    for (const tinyxml2::XMLElement *materialElement = materialsElement->FirstChildElement("Material"); materialElement != nullptr;
         materialElement = materialElement->NextSiblingElement("Material")) {
        if (materialElement->Attribute("Key") == nullptr) {
            continue;
        }
        materialKeys.insert(materialElement->Attribute("Key"));
        const tinyxml2::XMLElement *originalHashElement = materialElement->FirstChildElement("OriginalHash");
        if (materialElement->IntAttribute("WorldOverride", 0) == 0 || originalHashElement == nullptr || originalHashElement->GetText() == nullptr) {
            continue;
        }
        uint64_t originalHash = 0;
        std::stringstream stream(originalHashElement->GetText());//Material::deserialize reads it the same way
        stream >> originalHash;
        references.overriddenBaseHashes.insert(originalHash);
    }
}

void WorldReferenceScanner::scanTypeNames(const tinyxml2::XMLDocument &document, const std::set<std::string> &typeNames, References &references) {
    if (document.RootElement() != nullptr) {
        collectTextReferences(document.RootElement(), typeNames, references);
    }
}

void WorldReferenceScanner::collectTextReferences(const tinyxml2::XMLElement *element, const std::set<std::string> &typeNames, References &references) {
    for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        const char *text = child->GetText();
        if (text != nullptr) {
            addDataPath(text, references);
            if (typeNames.count(text) != 0) {
                references.typeNames.insert(text);
            }
        }
        collectTextReferences(child, typeNames, references);
    }
}

//objects sit at the top level, under parents, in groups and as player attachments, so every <Object> anywhere counts
void WorldReferenceScanner::collectModelKeys(const tinyxml2::XMLElement *element, const std::set<std::string> &materialKeys, References &references) {
    for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (std::string(child->Name()) == "Object") {
            const tinyxml2::XMLElement *fileElement = child->FirstChildElement("File");
            const tinyxml2::XMLElement *flipElement = child->FirstChildElement("Flip");
            if (fileElement != nullptr && fileElement->GetText() != nullptr) {
                std::string modelKey = LimonFileSystem::normalizePath(fileElement->GetText());
                if (flipElement != nullptr && flipElement->GetText() != nullptr && flipElement->GetText()[0] != '\0') {
                    modelKey += std::string("?flip") + flipElement->GetText();//Model's asset key, Model.cpp builds the same
                }
                references.modelKeys.insert(modelKey);
                References::PlacedObject placedObject;
                placedObject.modelKey = modelKey;
                const tinyxml2::XMLElement *meshMaterialList = child->FirstChildElement("MeshMaterialList");
                for (const tinyxml2::XMLElement *meshMaterial = meshMaterialList == nullptr ? nullptr : meshMaterialList->FirstChildElement("MeshMaterial");
                     meshMaterial != nullptr; meshMaterial = meshMaterial->NextSiblingElement("MeshMaterial")) {
                    //an entry the loader skips leaves the mesh on its base
                    const char *materialKey = meshMaterial->Attribute("MaterialKey");
                    if (meshMaterial->Attribute("MeshName") == nullptr || materialKey == nullptr || materialKeys.count(materialKey) == 0) {
                        continue;
                    }
                    References::MeshMaterialEntry entry;
                    entry.meshName = meshMaterial->Attribute("MeshName");
                    uint64_t baseMaterialHash = 0;
                    if (meshMaterial->QueryUnsigned64Attribute("BaseHash", &baseMaterialHash) == tinyxml2::XML_SUCCESS) {
                        entry.baseMaterialHash = baseMaterialHash;
                        meshMaterial->QueryUnsignedAttribute("Ordinal", &entry.ordinal);
                    }
                    placedObject.meshMaterials.push_back(entry);
                }
                references.placedObjects.push_back(placedObject);
            }
        }
        collectModelKeys(child, materialKeys, references);
    }
}

//paths the loader builds instead of reading them whole from an element
void WorldReferenceScanner::collectBuiltReferences(const std::string &worldFile, const tinyxml2::XMLElement *worldElement, References &references) {
    const tinyxml2::XMLElement *skyElement = worldElement->FirstChildElement("Sky");
    const tinyxml2::XMLElement *imagesPathElement = skyElement == nullptr ? nullptr : skyElement->FirstChildElement("ImagesPath");
    if (imagesPathElement != nullptr && imagesPathElement->GetText() != nullptr) {
        for (const char *faceName : {"Right", "Left", "Top", "Bottom", "Back", "Front"}) {
            const tinyxml2::XMLElement *faceElement = skyElement->FirstChildElement(faceName);
            if (faceElement != nullptr && faceElement->GetText() != nullptr) {
                addDataPath(std::string(imagesPathElement->GetText()) + "/" + faceElement->GetText(), references);
            }
        }
    }

    const tinyxml2::XMLElement *loadedAnimationsElement = worldElement->FirstChildElement("LoadedAnimations");
    if (loadedAnimationsElement != nullptr) {
        for (const tinyxml2::XMLElement *animationElement = loadedAnimationsElement->FirstChildElement("LoadedAnimation");
             animationElement != nullptr; animationElement = animationElement->NextSiblingElement("LoadedAnimation")) {
            const tinyxml2::XMLElement *nameElement = animationElement->FirstChildElement("Name");
            if (nameElement != nullptr && nameElement->GetText() != nullptr) {
                addDataPath("./Data/Animations/" + std::string(nameElement->GetText()) + ".xml", references);//WorldLoader::loadAnimations
            }
        }
    }

    std::string worldStem = worldFile.substr(0, worldFile.find_last_of('.'));//World::name based, as World.cpp builds it
    //the loader reads these only when present, so they are guesses, not names
    if (LimonFileSystem::fileExists(worldStem + ".aiwalk")) {
        addDataPath(worldStem + ".aiwalk", references);
    }
    if (LimonFileSystem::fileExists(worldStem + ".aiwalkb")) {
        addDataPath(worldStem + ".aiwalkb", references);
    }
}

//not checked for existence: a packed raw model is only its .limonmodel, and the packer reports what is missing
void WorldReferenceScanner::addDataPath(const std::string &path, References &references) {
    std::string normalizedPath = LimonFileSystem::normalizePath(path);
    if (normalizedPath.compare(0, 5, "Data/") != 0) {
        return;//numbers, names and everything else in the XML stop here, before touching the file system
    }
    if (LimonFileSystem::isDirectory(normalizedPath)) {
        return;//the sky's ImagesPath, its faces are built references
    }
    if (!references.files.insert(normalizedPath).second) {
        return;
    }
    if (isWorldFile(normalizedPath)) {
        references.worlds.insert(normalizedPath);
    }
}

bool WorldReferenceScanner::isWorldFile(const std::string &normalizedPath) {
    if (normalizedPath.size() < 4 || normalizedPath.compare(normalizedPath.size() - 4, 4, ".xml") != 0) {
        return false;
    }
    tinyxml2::XMLDocument document;
    if (LimonFileSystem::loadXML(normalizedPath, document) != tinyxml2::XML_SUCCESS) {
        return false;
    }
    return document.FirstChildElement("World") != nullptr;
}
