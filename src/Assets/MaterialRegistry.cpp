//
// Created by engin on 02.08.2026.
//

#include "MaterialRegistry.h"

#include <iostream>

#include "../Material.h"
#include "limonAPI/Graphics/GraphicsInterface.h"

std::shared_ptr<Material> MaterialRegistry::registerMaterial(std::shared_ptr<Material> material) {
    std::lock_guard<std::mutex> lock(materialsMutex);
    /*
     * This exact material is already ours, so this is another holder taking a reference. Do not fall through
     * to the content lookup: an edited copy that still matches something else would be swapped out from
     * under whoever already holds it.
     */
    std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>>::iterator ownIt = materials.find(material->getRegistrationID());
    if (ownIt != materials.end()) {
        ownIt->second.second++;
        return ownIt->second.first;
    }

    size_t materialHash = material->getHash();

    std::unordered_map<size_t, uint32_t>::iterator indexIt = contentIndex.find(materialHash);
    if (indexIt != contentIndex.end()) {
        std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>>::iterator foundIt = materials.find(indexIt->second);
        //the index is a hint. Verify it still describes the material it points at, a stale entry just means
        //we register a new material instead, which is a defined outcome rather than a recovery
        if (foundIt != materials.end() && foundIt->second.first->getHash() == materialHash) {
            foundIt->second.second++;
            return foundIt->second.first;
        }
        contentIndex.erase(indexIt);
    }

    material->materialIndex = acquireMaterialIndexInternal();
    materials[material->getRegistrationID()] = std::make_pair(material, 1);
    contentIndex[materialHash] = material->getRegistrationID();
    materialsVersion++;
    return material;
}

void MaterialRegistry::unregisterMaterial(std::shared_ptr<const Material> material) {
    std::lock_guard<std::mutex> lock(materialsMutex);
    //keyed by identity, so this can not miss because someone edited the material. A miss here is a real
    //double release and worth the noise
    std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>>::iterator materialIt = materials.find(material->getRegistrationID());
    if (materialIt == materials.end()) {
        std::cerr << "Unregister for non existent material found! " << material->getName() << std::endl;
        return;
    }
    materialIt->second.second--;
    if (materialIt->second.second == 0) {
        releaseMaterialIndexInternal(materialIt->second.first->getMaterialIndex());
        dropContentIndexEntriesForInternal(material->getRegistrationID());
        materials.erase(materialIt);
        materialsVersion++;
    }
}

void MaterialRegistry::fillMaterialsSnapshot(std::vector<std::pair<uint32_t, std::shared_ptr<Material>>> &outSnapshot) const {
    std::lock_guard<std::mutex> lock(materialsMutex);
    outSnapshot.clear();//keeps its capacity, so a rebuild does not go back to the heap
    outSnapshot.reserve(materials.size());
    for (std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>>::const_iterator it = materials.begin(); it != materials.end(); ++it) {
        outSnapshot.emplace_back(it->first, it->second.first);
    }
}

void MaterialRegistry::refreshContentIndex(const std::shared_ptr<Material> &material) {
    if (material == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(materialsMutex);
    if (materials.count(material->getRegistrationID()) == 0) {
        return;//not ours, a scratch copy the editor has not handed over yet
    }
    /*
     * Insert only, and never displace an entry that is already there - it got the content first.
     *
     * Entries left behind pointing at content this material no longer has are harmless: every reader
     * verifies against the live hash and erases what does not match, and unregisterMaterial sweeps whatever
     * is left when the material goes. So there is no bookkeeping to keep in step and no transition to miss,
     * which is exactly what made the old edit windows fragile.
     */
    contentIndex.emplace(material->getHash(), material->getRegistrationID());
}

std::shared_ptr<Material> MaterialRegistry::findByContentHash(size_t contentHash) {
    std::lock_guard<std::mutex> lock(materialsMutex);
    std::unordered_map<size_t, uint32_t>::iterator indexIt = contentIndex.find(contentHash);
    if (indexIt == contentIndex.end()) {
        return nullptr;
    }
    std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>>::const_iterator foundIt = materials.find(indexIt->second);
    if (foundIt == materials.end() || foundIt->second.first->getHash() != contentHash) {
        //the index is a hint. Drop what no longer describes its material, same as registerMaterial does,
        //so a stale entry is cleaned by whichever read path reaches it first
        contentIndex.erase(indexIt);
        return nullptr;
    }
    return foundIt->second.first;
}

/*
 * A fresh copy has the source's content, so going through registerMaterial would dedup it straight back
 * into the source and the caller would end up sharing the very thing it wanted its own version of. It is
 * inserted directly instead: storage is keyed by registrationID, so two materials with identical content
 * coexist fine.
 *
 * Deliberately left out of the content index - the source owns that content until this copy is actually
 * edited, and refreshContentIndex adds it then.
 */
std::shared_ptr<Material> MaterialRegistry::createScratchCopy(const std::shared_ptr<const Material> &source) {
    std::shared_ptr<Material> copiedMaterial = std::make_shared<Material>(*source);
    //record what it replaces, so it reads as world owned later and is not split a second time
    copiedMaterial->setOriginalHash(source->getHash());
    std::lock_guard<std::mutex> lock(materialsMutex);
    copiedMaterial->materialIndex = acquireMaterialIndexInternal();
    materials[copiedMaterial->getRegistrationID()] = std::make_pair(copiedMaterial, 1);
    materialsVersion++;
    return copiedMaterial;
}

std::shared_ptr<Material> MaterialRegistry::splitOffOverride(const std::shared_ptr<Material> &material, const std::shared_ptr<const Material> &valuesBeforeEdit) {
    std::string editedName = material->getName();
    std::shared_ptr<Material> overrideMaterial = std::make_shared<Material>(*material);//carries the edit
    overrideMaterial->setName(editedName);//the copy constructor prefixed it, this is the material now

    /*
     * The base keeps its name. It is usually ModelAsset's own material, shared by every world and written
     * into the cereal cache, so renaming it here would show up in worlds that never overrode anything and
     * could be baked into the .limonmodel on disk. The editor derives the base_ prefix for display instead.
     */
    material->restoreValuesFrom(*valuesBeforeEdit);
    graphicsWrapper->setMaterial(*material);//its slot still holds the edited values

    overrideMaterial = registerMaterial(overrideMaterial);
    /*
     * originalHash goes on what came back, not on the candidate above: registerMaterial can dedup and hand
     * back a material that already existed, and setting it on the discarded candidate would save the override
     * with 0, which resolveMaterialOverrides then drops on the next load.
     *
     * A non-zero one that names a different base means the edit landed on exactly the content of another
     * override. Overwriting it would break that one, so say so and leave it alone - getting two materials to
     * byte identical content by dragging float sliders is not a case worth designing around, but it is worth
     * knowing about if it ever happens.
     */
    if (overrideMaterial->getOriginalHash() == 0) {
        overrideMaterial->setOriginalHash(valuesBeforeEdit->getHash());
    } else if (overrideMaterial->getOriginalHash() != valuesBeforeEdit->getHash()) {
        std::cerr << "Material " << overrideMaterial->getName() << " already overrides a different material, "
                  << "leaving its base alone. This edit matched its content exactly." << std::endl;
    }
    graphicsWrapper->setMaterial(*overrideMaterial);
    return overrideMaterial;
}

void MaterialRegistry::dropContentIndexEntriesForInternal(uint32_t registrationID) {
    for (std::unordered_map<size_t, uint32_t>::iterator indexIt = contentIndex.begin(); indexIt != contentIndex.end(); ) {
        if (indexIt->second == registrationID) {
            indexIt = contentIndex.erase(indexIt);
        } else {
            ++indexIt;
        }
    }
}

uint32_t MaterialRegistry::acquireMaterialIndexInternal() {
    if (!freeMaterialIndices.empty()) {
        uint32_t reusedIndex = freeMaterialIndices.back();
        freeMaterialIndices.pop_back();
        return reusedIndex;
    }
    return nextMaterialIndex++;
}

void MaterialRegistry::releaseMaterialIndexInternal(uint32_t materialIndex) {
    if (materialIndex == 0) {
        return;//0 is what an unregistered material carries, never a real slot
    }
    freeMaterialIndices.push_back(materialIndex);
}
