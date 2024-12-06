//
// Created by engin on 27.07.2016.
//

#include "AssetManager.h"
#include "ModelAsset.h"

#include <algorithm>
#include <string>
#include <dirent.h>
#include <sys/stat.h>

void AssetManager::addAssetsRecursively(const std::string &directoryPath, const std::string &fileName,
                                        const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions,
                                        AvailableAssetsNode* nodeToProcess) {
    DIR *directory;
    struct dirent *entry;

    nodeToProcess->name = fileName;
    nodeToProcess->nameLower = nodeToProcess->name;
    std::transform(nodeToProcess->nameLower.begin(), nodeToProcess->nameLower.end(), nodeToProcess->nameLower.begin(), ::tolower);

    nodeToProcess->fullPath = directoryPath;

    if (!(directory = opendir(directoryPath.c_str()))) {
        return;
    }

    nodeToProcess->assetType = Asset_type_DIRECTORY;

    while ((entry = readdir(directory)) != NULL) {
        struct stat fileStat;
        std::string filePath = directoryPath + "/" + std::string(entry->d_name);

        if(stat(filePath.c_str(),&fileStat) < 0) {
            std::cout << "stat failed for entry " << entry->d_name << std::endl;
        } else {
            if(S_ISDIR(fileStat.st_mode)) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                //we found a child, and it is a directory. Create a node and recurse
                AvailableAssetsNode* newNode = new AvailableAssetsNode();
                newNode->parent = nodeToProcess;
                nodeToProcess->children.push_back(newNode);
                addAssetsRecursively(filePath, std::string(entry->d_name), fileExtensions, newNode);

            } else {
                AssetTypes assetType;
                if(isExtensionInList(entry->d_name, fileExtensions, assetType)) {
                    //we found a element that is an asset create and add to children
                    AvailableAssetsNode* newNode = new AvailableAssetsNode;
                    newNode->fullPath = filePath;
                    newNode->name = std::string(entry->d_name);
                    newNode->nameLower = newNode->name;
                    std::transform(newNode->nameLower.begin(), newNode->nameLower.end(), newNode->nameLower.begin(), ::tolower);
                    newNode->assetType = assetType;
                    newNode->parent = nodeToProcess;
                    nodeToProcess->children.push_back(newNode);
                    //std::cout << entry->d_name << " added as asset. with type " << assetType << std::endl;
                } else {
                    //file found but not added because extension is not in list
                }

            }
        }
    }
    closedir(directory);
}


std::vector<std::pair<std::string, AssetManager::AssetTypes>> AssetManager::loadAssetExtensionList() {
    std::vector<std::pair<std::string, AssetTypes>> extensionList;
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(ASSET_EXTENSIONS_FILE.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading Asset file list XML: " <<  xmlDoc.ErrorName() << std::endl;
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
            AssetManager::AssetTypes currentAssetType;
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

bool AssetManager::loadAssetList() {
    std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
    availableAssetsRootNode = new AvailableAssetsNode();
    addAssetsRecursively("./Data", "Data", fileExtensions, availableAssetsRootNode);
    return true;
}

bool AssetManager::isExtensionInList(const std::string &name, const std::vector<std::pair<std::string, AssetTypes>> &vector,
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

const AssetManager::AvailableAssetsNode *
AssetManager::getAvailableAssetsTreeFiltered(AssetManager::AssetTypes type, const std::string &filterText) {
    std::string filterTextLower = filterText;
    std::transform(filterTextLower.begin(), filterTextLower.end(), filterTextLower.begin(), ::tolower);
    std::pair<AssetTypes , std::string> key = std::make_pair(type,filterTextLower);
    if(filteredResults.find(key) != filteredResults.end()) {
        return filteredResults[key];
    }
    //now we should build another tree with given filters
    AssetManager::AvailableAssetsNode *root = getAvailableAssetsTreeFilteredRecursive(availableAssetsRootNode, type, filterTextLower);
    filteredResults[key] = root;
    return root;
}

AssetManager::AvailableAssetsNode *
AssetManager::getAvailableAssetsTreeFilteredRecursive(const AvailableAssetsNode * const assetsNode,
                                                      AssetManager::AssetTypes type,
                                                      const std::string &filterText) {
    AssetManager::AvailableAssetsNode *newAssetsNode = nullptr;
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
        AssetManager::AvailableAssetsNode *childAssetNode = getAvailableAssetsTreeFilteredRecursive(assetsNode->children[i], type, filterText);
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

/**
 * Registers the material if it is not found, and returns the material that should be used.
 * if the material is not found, itself is returned, if it is found, the original one is returned.
 * @param material to register or search for the original
 * @return material that should be used.
 */
std::shared_ptr<Material> AssetManager::registerMaterial(std::shared_ptr<Material> material) {
    const auto foundIt = materials.find(material->getOriginalHash());
    if(foundIt != materials.end()) {
        //std::cerr << "Material found, return "<< material->getName() << std::endl;
        foundIt->second.second++;
        return foundIt->second.first;
    }
    //not found, add
    //std::cerr << "Material not found, create for " << material->getName() << std::endl;
    materials[material->getHash()] = std::make_pair(material, 1);
    return material;

}

/**
 * Registers an overriden material if the material if it is not found, and returns the material that should be used.
 * if the material is not found, itself is returned, if it is found, the original one is returned.
 *
 * for insterting in to possible materials list, original hash is used instead of current, because the material is
 * an override for the original hash, not for the current hash
 *
 * @param material to register or search for the original
 * @return material that should be used.
 */
std::shared_ptr<Material> AssetManager::registerOverriddenMaterial(std::shared_ptr<Material> material) {
    const auto foundIt = materials.find(material->getOriginalHash());
    if(foundIt != materials.end()) {
        //std::cerr << "Material found, return "<< material->getName() << std::endl;
        foundIt->second.second++;
        return foundIt->second.first;
    }
    //not found, add
    //std::cerr << "Material not found, create for " << material->getName() << std::endl;
    materials[material->getOriginalHash()] = std::make_pair(material, 1);
    return material;
}
void AssetManager::unregisterMaterial(std::shared_ptr<const Material> material) {
    auto materialIt = materials.find(material->getOriginalHash());
    if(materialIt == materials.end()) {
        std::cerr << "Unregister for non existent material found!" << std::endl;
    }
    //std::cerr << "Material unregister " << material->getName() << std::endl;
    materialIt->second.second--;
    if(materialIt->second.second == 0) {
        //time to remove the item
        materials.erase(materialIt);
        //std::cerr << "Material getting deleted " << material->getName() << std::endl;
    }
}

const std::map<size_t, std::pair<std::shared_ptr<Material>, uint32_t>>& AssetManager::getMaterials() const {
    return materials;
}
