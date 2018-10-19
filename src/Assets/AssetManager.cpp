//
// Created by engin on 27.07.2016.
//

#include "AssetManager.h"

#include <algorithm>
#include <string>
#include <dirent.h>
#include <sys/stat.h>

void AssetManager::addAssetsRecursively(const std::string &directoryPath, const std::vector<std::string> &fileExtensions)
{
    DIR *directory;
    struct dirent *entry;

    if (!(directory = opendir(directoryPath.c_str()))) {
        return;
    }
    while ((entry = readdir(directory)) != NULL) {
        struct stat fileStat;
        std::string filePath = directoryPath + "/" + std::string(entry->d_name);
        if(stat(filePath.c_str(),&fileStat) < 0) {
            std::cout << "stat failed for entry " << entry->d_name << std::endl;
        } else {
            if(S_ISDIR(fileStat.st_mode)) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                addAssetsRecursively(filePath, fileExtensions);
            } else {
                if(isExtensionInList(entry->d_name, fileExtensions)) {
                    availableAssetsList[filePath] = AssetTypes::Asset_type_MODEL;
                    std::cout << entry->d_name << " added as asset." << std::endl;
                } else {
                    //file found but not added because extension is not in list
                }

            }
        }
    }
    closedir(directory);
}


std::vector<std::string> AssetManager::loadAssetExtensionList() {
    std::vector<std::string> extensionList;
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
    const char* typeName;
    tinyxml2::XMLElement* currentAssetNode =  assetsNode->FirstChildElement("Asset");
    while(currentAssetNode != nullptr) {
        typeName = currentAssetNode->Attribute("type");
        if(!strcmp(typeName, "Model")) {// if type Model
            extensionList.push_back(currentAssetNode->GetText());
            std::cout << "adding available asset extension" << currentAssetNode->GetText() << std::endl;
        } else {
            std::cerr << "Type " << typeName << " Not implemented yet" << std::endl;
            exit(-1);
        }
        currentAssetNode = currentAssetNode->NextSiblingElement("Asset");
    }
    return extensionList;
}

bool AssetManager::loadAssetList() {
    std::vector<std::string> fileExtensions = loadAssetExtensionList();
    addAssetsRecursively("./Data/Models", fileExtensions);
    return true;
}

bool AssetManager::isExtensionInList(const std::string &name, const std::vector<std::string> &vector) {
    //lowercase the string:
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

    for (uint32_t i = 0; i < vector.size(); ++i) {
        if(isEnding(lowerCaseName, vector[i])) {
            return true;
        }
    }
    return false;
}

