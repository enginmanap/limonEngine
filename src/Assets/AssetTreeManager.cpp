//
// Created by engin on 18.10.2026.
//

#include "AssetTreeManager.h"

#include <algorithm>
#include <iostream>
#include <tinyxml2.h>
#include "../LimonFileSystem.h"

AssetTreeManager::AssetTreeManager() {
    loadAssetList();
}

AssetTreeManager::~AssetTreeManager() {
    if (assetReloadThread.joinable()) {
        assetReloadThread.join();
    }
    delete pendingAssetsRootNode;
    delete availableAssetsRootNode;

    for (auto tree_iterator = filteredResults.begin(); tree_iterator != filteredResults.end(); ++tree_iterator) {
        delete tree_iterator->second;
    }
}

void AssetTreeManager::addAssetsRecursively(const std::string &directoryPath, const std::string &fileName,
                                        const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions,
                                        AvailableAssetsNode* nodeToProcess) {
    nodeToProcess->name = fileName;
    nodeToProcess->nameLower = nodeToProcess->name;
    std::transform(nodeToProcess->nameLower.begin(), nodeToProcess->nameLower.end(), nodeToProcess->nameLower.begin(), ::tolower);

    nodeToProcess->fullPath = directoryPath;

    std::vector<LimonFileSystem::DirectoryEntry> entries;
    if (!LimonFileSystem::listDirectory(directoryPath, entries)) {
        return;
    }

    nodeToProcess->assetType = Asset_type_DIRECTORY;

    for (const LimonFileSystem::DirectoryEntry &entry : entries) {
        std::string filePath = directoryPath + "/" + entry.name;//this is the asset name the editor writes to maps, keep the "./" form
        if (entry.isDirectory) {
            //we found a child, and it is a directory. Create a node and recurse
            AvailableAssetsNode* newNode = new AvailableAssetsNode();
            newNode->parent = nodeToProcess;
            nodeToProcess->children.push_back(newNode);
            addAssetsRecursively(filePath, entry.name, fileExtensions, newNode);

        } else {
            AssetTypes assetType;
            if(isExtensionInList(entry.name, fileExtensions, assetType)) {
                //we found a element that is an asset create and add to children
                AvailableAssetsNode* newNode = new AvailableAssetsNode;
                newNode->fullPath = filePath;
                newNode->name = entry.name;
                newNode->nameLower = newNode->name;
                std::transform(newNode->nameLower.begin(), newNode->nameLower.end(), newNode->nameLower.begin(), ::tolower);
                newNode->assetType = assetType;
                newNode->parent = nodeToProcess;
                nodeToProcess->children.push_back(newNode);
                //std::cout << entry.name << " added as asset. with type " << assetType << std::endl;
            } else {
                //file found but not added because extension is not in list
            }

        }
    }
}


std::vector<std::pair<std::string, AssetTreeManager::AssetTypes>> AssetTreeManager::loadAssetExtensionList() {
    std::vector<std::pair<std::string, AssetTypes>> extensionList;
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = LimonFileSystem::loadXML(ASSET_EXTENSIONS_FILE, xmlDoc);
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading Asset file list XML: " <<  tinyxml2::XMLDocument::ErrorIDToName(eResult) << std::endl;
        return extensionList;
    }
    tinyxml2::XMLNode * assetsNode = xmlDoc.FirstChild();
    if (assetsNode == nullptr) {
        std::cerr << "Asset list xml is not a valid XML." << std::endl;
        return extensionList;
    }
    const char* typeNameRaw;
    tinyxml2::XMLElement* currentAssetNode =  assetsNode->FirstChildElement("Asset");
    while(currentAssetNode != nullptr) {
        typeNameRaw = currentAssetNode->Attribute("type");

        if (typeNameRaw != nullptr) {
            std::string typeName = typeNameRaw;
            AssetTreeManager::AssetTypes currentAssetType;
            if (typeName == "Model") {
                currentAssetType = Asset_type_MODEL;
            } else if (typeName == "Texture") {
                currentAssetType = Asset_type_TEXTURE;
            } else if(typeName == "Sound") {
                currentAssetType = Asset_type_SOUND;
            } else if(typeName == "Graphics Program") {
                currentAssetType = Asset_type_GRAPHICSPROGRAM;
            } else {
                std::cerr << "Requested asset type is not implemented yet. exiting.." << std::endl;
                exit(1);
            }
            extensionList.push_back(std::pair<std::string, AssetTypes>(currentAssetNode->GetText(), currentAssetType));
            //std::cout << "adding available asset extension " << currentAssetNode->GetText() << std::endl;
        } else {
            std::cerr << "Asset extension without a type is invalid, skipping" << std::endl;
        }
        currentAssetNode = currentAssetNode->NextSiblingElement("Asset");
    }
    return extensionList;
}

bool AssetTreeManager::loadAssetList() {
    std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
    availableAssetsRootNode = new AvailableAssetsNode();
    availableAssetsRootNode->assetType = Asset_type_DIRECTORY;
    availableAssetsRootNode->fullPath = ".";
    availableAssetsRootNode->nameLower = "";
    availableAssetsRootNode->name = "Assets";
    AvailableAssetsNode* data = new AvailableAssetsNode;
    addAssetsRecursively("./Data", "Data", fileExtensions, data);
    availableAssetsRootNode->children.push_back(data);
    AvailableAssetsNode* engine = new AvailableAssetsNode;
    addAssetsRecursively("./Engine", "Engine", fileExtensions, engine);
    availableAssetsRootNode->children.push_back(engine);
    return true;
}

bool AssetTreeManager::isExtensionInList(const std::string &name, const std::vector<std::pair<std::string, AssetTypes>> &vector,
                                     AssetTypes &elementAssetType) {
    //lowercase the string:
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

    for (uint32_t i = 0; i < vector.size(); ++i) {
        if(isEnding(lowerCaseName, vector[i].first)) {
            elementAssetType = vector[i].second;
            return true;
        }
    }
    return false;
}

std::string AssetTreeManager::findFirstAssetPathByName(AssetTreeManager::AssetTypes type, const std::string &name) {
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    std::lock_guard<std::mutex> lock(availableAssetsMutex);
    if (availableAssetsRootNode == nullptr) {
        return "";
    }
    return findFirstAssetPathByNameRecursive(availableAssetsRootNode, type, nameLower);
}

std::string AssetTreeManager::findFirstAssetPathByNameRecursive(const AvailableAssetsNode *assetsNode, AssetTreeManager::AssetTypes type, const std::string &nameLower) {
    if (assetsNode->assetType == type) {
        return assetsNode->nameLower.find(nameLower) != std::string::npos ? assetsNode->fullPath : "";
    }
    if (assetsNode->assetType != Asset_type_DIRECTORY) {
        return "";
    }
    for (const AvailableAssetsNode *child : assetsNode->children) {
        std::string found = findFirstAssetPathByNameRecursive(child, type, nameLower);
        if (!found.empty()) {
            return found;
        }
    }
    return "";
}

const AssetTreeManager::AvailableAssetsNode *
AssetTreeManager::getAvailableAssetsTreeFiltered(AssetTreeManager::AssetTypes type, const std::string &filterText) {
    std::string filterTextLower = filterText;
    std::transform(filterTextLower.begin(), filterTextLower.end(), filterTextLower.begin(), ::tolower);
    std::pair<AssetTypes , std::string> key = std::make_pair(type,filterTextLower);
    std::lock_guard<std::mutex> lock(availableAssetsMutex);
    if(filteredResults.find(key) != filteredResults.end()) {
        return filteredResults[key];
    }
    //now we should build another tree with given filters
    AssetTreeManager::AvailableAssetsNode *root = getAvailableAssetsTreeFilteredRecursive(availableAssetsRootNode, type, filterTextLower);
    filteredResults[key] = root;
    return root;
}

AssetTreeManager::AvailableAssetsNode *
AssetTreeManager::getAvailableAssetsTreeFilteredRecursive(const AvailableAssetsNode * const assetsNode,
                                                      AssetTreeManager::AssetTypes type,
                                                      const std::string &filterText) {
    AssetTreeManager::AvailableAssetsNode *newAssetsNode = nullptr;
    if(assetsNode->assetType == type) {
        if(assetsNode->nameLower.find(filterText) != std::string::npos) {
            AvailableAssetsNode* newAssetNode = new AvailableAssetsNode();
            newAssetNode->name = assetsNode->name;
            newAssetNode->fullPath = assetsNode->fullPath;
            newAssetNode->nameLower = assetsNode->nameLower;
            newAssetNode->assetType = assetsNode->assetType;
            //not setting parent, because if this was called recursively, caller sets it, if root it should be null
            return newAssetNode;
        } else {
            return nullptr;
        }
    } else if(assetsNode->assetType != Asset_type_DIRECTORY) {
        return nullptr;
    }
    //at this point, we made sure the asset node type is directory, and any child that doesn't match filters will return null.
    for (size_t i = 0; i < assetsNode->children.size(); ++i) {
        AssetTreeManager::AvailableAssetsNode *childAssetNode = getAvailableAssetsTreeFilteredRecursive(assetsNode->children[i], type, filterText);
        if(childAssetNode != nullptr) {
            if(newAssetsNode == nullptr) {
                newAssetsNode = new AvailableAssetsNode();
                newAssetsNode->name = assetsNode->name;
                newAssetsNode->nameLower = assetsNode->nameLower;
                newAssetsNode->fullPath = assetsNode->fullPath;
                newAssetsNode->assetType = assetsNode->assetType;//directory known
                //parent = nullptr;
            }
            childAssetNode->parent = newAssetsNode;
            newAssetsNode->children.push_back(childAssetNode);
        }
    }
    return newAssetsNode;
}

std::vector<std::string> AssetTreeManager::getMixamoAnimationFilePaths(const std::string &modelAssetPath) {
    std::vector<std::string> mixamoFilePaths;
    std::lock_guard<std::mutex> lock(availableAssetsMutex);
    const AvailableAssetsNode* thisAssetsNode = availableAssetsRootNode->findNode(modelAssetPath);
    if (thisAssetsNode == nullptr) {
        std::cerr << "The asset " << modelAssetPath << " is not in asset tree, skipping Mixamo search" << std::endl;
        return mixamoFilePaths;
    }
    if (thisAssetsNode->parent == nullptr) {
        return mixamoFilePaths;
    }
    const AvailableAssetsNode* parentNode = thisAssetsNode->parent;
    for (size_t i = 0; i < parentNode->children.size(); ++i) {
        const AvailableAssetsNode* sibling = parentNode->children[i];
        if (sibling->name == "Mixamo") {
            for (size_t j = 0; j < sibling->children.size(); ++j) {
                const AvailableAssetsNode* mixamoFile = sibling->children[j];
                if (mixamoFile->assetType == AssetTypes::Asset_type_MODEL) {
                    mixamoFilePaths.push_back(mixamoFile->fullPath);
                }
            }
        }
    }
    return mixamoFilePaths;
}

void AssetTreeManager::beginReloadAssetList() {
    if (assetListReloadInProgress.load()) {
        return;
    }
    assetListReloadInProgress.store(true);
    if (assetReloadThread.joinable()) {
        assetReloadThread.join();
    }
    assetReloadThread = std::thread([this]() {
        std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
        AvailableAssetsNode* newRoot = new AvailableAssetsNode();
        newRoot->assetType = Asset_type_DIRECTORY;
        newRoot->fullPath = ".";
        newRoot->nameLower = "";
        newRoot->name = "Assets";
        AvailableAssetsNode* data = new AvailableAssetsNode();
        addAssetsRecursively("./Data", "Data", fileExtensions, data);
        newRoot->children.push_back(data);
        AvailableAssetsNode* engine = new AvailableAssetsNode();
        addAssetsRecursively("./Engine", "Engine", fileExtensions, engine);
        newRoot->children.push_back(engine);
        pendingAssetsRootNode = newRoot;
        assetListReloadInProgress.store(false);
    });
}

bool AssetTreeManager::applyPendingAssetListReload() {
    if (assetListReloadInProgress.load()) {
        return false;
    }
    if (pendingAssetsRootNode == nullptr) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(availableAssetsMutex);
        for (auto treeIterator = filteredResults.begin(); treeIterator != filteredResults.end(); ++treeIterator) {
            delete treeIterator->second;
        }
        filteredResults.clear();
        delete availableAssetsRootNode;
        availableAssetsRootNode = pendingAssetsRootNode;
    }
    pendingAssetsRootNode = nullptr;
    assetListVersion++;
    return true;
}
