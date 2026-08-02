//
// Created by engin on 02.08.2026.
//

#ifndef LIMONENGINE_MATERIALREGISTRY_H
#define LIMONENGINE_MATERIALREGISTRY_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <unordered_map>
#include <vector>

class GraphicsInterface;
class Material;

/**
 * Owns every registered Material and dedups them by content.
 *
 * Storage is keyed by Material::getRegistrationID(), which never changes. Content is only an index used to
 * answer "does an identical material already exist". Editing a material can therefore never invalidate
 * where it lives, which is what removes the need for edit windows, re-keying and collision nudging.
 */
class MaterialRegistry {
public:
    explicit MaterialRegistry(GraphicsInterface* graphicsWrapper) : graphicsWrapper(graphicsWrapper) { }

    //returns the material to use, which is the already registered one if an identical material exists
    std::shared_ptr<Material> registerMaterial(std::shared_ptr<Material> material);

    void unregisterMaterial(std::shared_ptr<const Material> material);

    /**
     * A copy, taken under the lock, because registerMaterial runs on the background asset threads while the
     * editor walks this. Handing out a reference to the live map was a data race whatever container it used:
     * a reader following node pointers during an insert's rebalance is undefined, and iterator invalidation
     * rules say nothing about concurrent access.
     *
     * The shared_ptrs also keep every material alive for as long as the caller holds the snapshot, so an
     * unregister on another thread can not pull one out from under it.
     */
    void fillMaterialsSnapshot(std::vector<std::pair<uint32_t, std::shared_ptr<Material>>> &outSnapshot) const;

    /**
     * Bumped whenever a material joins or leaves. Read it to decide whether a snapshot needs rebuilding, so
     * a caller pays only when something actually changed rather than once a frame - asset loading during an
     * open material list is rare, and a fresh heap vector every frame costs more than the race it guards.
     */
    uint32_t getMaterialsVersion() const {
        return materialsVersion.load();
    }

    //call while a material is being edited. O(1), so it costs nothing to do every frame, and it keeps dedup
    //correct without needing to know when an edit ended
    void refreshContentIndex(const std::shared_ptr<Material> &material);

    //a saved override names the base it replaces by content, since registrationID is runtime only
    std::shared_ptr<Material> findByContentHash(size_t contentHash);

    //a registered copy that deliberately skips content dedup, so the caller gets its own instance
    std::shared_ptr<Material> createScratchCopy(const std::shared_ptr<const Material> &source);

    /**
     * Splits an edited material in two: the edit moves onto a new material that keeps the plain name, and
     * the original is put back to valuesBeforeEdit. Returns the new one, registered.
     *
     * The original's name is left alone - it is usually ModelAsset's, shared by every world and cached to
     * disk, so the editor derives a base_ prefix for display instead of anything being renamed here.
     *
     * Lives here rather than in the Editor because it is all identity work - naming, originalHash, the GPU
     * slot - and this is the only class Material grants that access to.
     */
    std::shared_ptr<Material> splitOffOverride(const std::shared_ptr<Material> &material, const std::shared_ptr<const Material> &valuesBeforeEdit);


private:
    mutable std::mutex materialsMutex;//mutable so the const snapshot accessors can guard themselves
    //atomic so getMaterialsVersion can be read without taking the lock every frame
    std::atomic<uint32_t> materialsVersion{0};
    //only ever touched under materialsMutex, so it does not need to be atomic
    uint32_t nextMaterialIndex = 1;

    /*
     * Keyed by registrationID. The pair is {the material, how many holders asked for it} - so `second.first`
     * is the material and `second.second` is the reference count, which reads badly at every use site but is
     * the same shape AssetManager::assets uses for assets.
     *
     * A holder is anything that called registerMaterial: a ModelAsset's materialMap, a Model's mesh, a
     * world's override table. At zero the entry is erased and its GPU slot goes back to the free list.
     */
    std::map<uint32_t, std::pair<std::shared_ptr<Material>, uint32_t>> materials;

    //content hash -> registrationID. An index, not storage. A stale entry costs a missed dedup, nothing more
    std::unordered_map<size_t, uint32_t> contentIndex;

    //materialIndex is a slot in a fixed size GPU buffer, so freed slots have to come back or a long editing
    //session walks past NR_MAX_MATERIALS and never recovers
    std::vector<uint32_t> freeMaterialIndices;

    GraphicsInterface* graphicsWrapper;

    //assume materialsMutex is already held
    uint32_t acquireMaterialIndexInternal();
    void releaseMaterialIndexInternal(uint32_t materialIndex);
    void dropContentIndexEntriesForInternal(uint32_t registrationID);
};

#endif //LIMONENGINE_MATERIALREGISTRY_H
