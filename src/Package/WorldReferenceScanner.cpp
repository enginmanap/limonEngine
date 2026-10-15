//
// Created by engin on 15.10.2026.
//

#include "WorldReferenceScanner.h"

#include "../LimonFileSystem.h"

void WorldReferenceScanner::scanDocument(const std::string &worldFile, const tinyxml2::XMLDocument &document, References &references) {
    addIfDataFile(worldFile, references);
    const tinyxml2::XMLElement *worldElement = document.FirstChildElement("World");
    if (worldElement == nullptr) {
        return;
    }
    collectTextReferences(worldElement, references);
    collectBuiltReferences(worldFile, worldElement, references);
}

void WorldReferenceScanner::collectTextReferences(const tinyxml2::XMLElement *element, References &references) {
    for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        const char *text = child->GetText();
        if (text != nullptr) {
            addIfDataFile(text, references);
        }
        collectTextReferences(child, references);
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
                addIfDataFile(std::string(imagesPathElement->GetText()) + "/" + faceElement->GetText(), references);
            }
        }
    }

    const tinyxml2::XMLElement *loadedAnimationsElement = worldElement->FirstChildElement("LoadedAnimations");
    if (loadedAnimationsElement != nullptr) {
        for (const tinyxml2::XMLElement *animationElement = loadedAnimationsElement->FirstChildElement("LoadedAnimation");
             animationElement != nullptr; animationElement = animationElement->NextSiblingElement("LoadedAnimation")) {
            const tinyxml2::XMLElement *nameElement = animationElement->FirstChildElement("Name");
            if (nameElement != nullptr && nameElement->GetText() != nullptr) {
                addIfDataFile("./Data/Animations/" + std::string(nameElement->GetText()) + ".xml", references);//WorldLoader::loadAnimations
            }
        }
    }

    std::string worldStem = worldFile.substr(0, worldFile.find_last_of('.'));//World::name based, as World.cpp builds it
    addIfDataFile(worldStem + ".aiwalk", references);
    addIfDataFile(worldStem + ".aiwalkb", references);
}

void WorldReferenceScanner::addIfDataFile(const std::string &path, References &references) {
    std::string normalizedPath = LimonFileSystem::normalizePath(path);
    if (normalizedPath.compare(0, 5, "Data/") != 0) {
        return;//numbers, names and everything else in the XML stop here, before touching the file system
    }
    if (references.files.count(normalizedPath) != 0 || !LimonFileSystem::fileExists(normalizedPath)) {
        return;
    }
    references.files.insert(normalizedPath);
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
