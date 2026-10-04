//
// Created by engin on 25.09.2026.
//

#ifndef LIMONENGINE_LODLADDER_H
#define LIMONENGINE_LODLADDER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "LodGenerator.h"

namespace OptionsUtil {
    class Options;
}

//why a step has no mesh, so the panel can say so instead of hiding the step
enum class LodSkipReason : uint8_t {
    NONE,
    NO_GAIN,        //it barely improved on the step before it, so it would cost an index range for nothing
    NOTHING_FITS,   //even the smallest error damages this model past the budget, so no coarser step can fit either
    EMPTY           //simplified down to no triangles at all
};

//what was asked of one step and what came out. A welded twin is this step's second outcome, not a step of its own
struct LodStep {
    //asked for
    float silhouetteBudget = 0.02f;  //fraction of pixels that may be wrong in outline, uv or normal, the same at any distance
    float requestedRatio = 0.0f;     //triangle share typed in the editor, zero when it came from the options
    bool userSet = false;            //the gain rule steps aside for these: a 2% saving may be exactly the point

    struct Outcome {
        bool built = false;
        LodSkipReason skipReason = LodSkipReason::NONE;
        float targetError = 0.0f;        //what the generator was handed, so this mesh can be rebuilt exactly
        float modelError = 0.0f;         //measured, in model units, what places this level
        uint32_t triangleCount = 0;
        uint32_t meshLodIndex = 0;       //the index range every mesh built for it
        float silhouetteDamage = 0.0f;   //what it cost, for the panel
        float achievedRatio = 0.0f;      //so the panel can flag a target the simplifier would not reach

#ifdef CEREAL_SUPPORT
        template<class Archive>
        void serialize(Archive &archive) {
            archive(built, skipReason, targetError, modelError, triangleCount, meshLodIndex, silhouetteDamage,
                    achievedRatio);
        }
#endif
    };
    Outcome structure;
    Outcome welded;

#ifdef CEREAL_SUPPORT
    template<class Archive>
    void serialize(Archive &archive) {
        archive(silhouetteBudget, requestedRatio, userSet, structure, welded);
    }
#endif
};

struct LodSelectionContext {
    float pixelScale = 0.0f;        //model units to pixels at distance one
    //how much error may show on screen, the shadow scale already folded in. Resolve it once per camera, it was
    //recomputed for every step of every object when the ingredients lived here
    float allowance = 1.0f;
    float hysteresis = 0.0f;
    long forceLevel = -1;
    bool isShadowCamera = false;    //depth only, so welded outcomes may be drawn
    float objectDistance = 1.0f;
    float objectScale = 1.0f;
};

//one model's LOD steps and where each one is used. Never include ModelAsset or MeshAsset here, taking plain
//geometry is what lets a scratch tool drive this against a corpus
class LodLadder {
public:
    //vertices must already carry the node transform, the meshes of a model are scored together in one space
    struct MeshGeometry {
        const std::vector<glm::vec3> *vertices = nullptr;
        const std::vector<glm::vec3> *normals = nullptr;
        const std::vector<glm::vec2> *textureCoordinates = nullptr;
        const uint16_t *indices = nullptr;   //the LOD0 range
        size_t indexCount = 0;
        //what the asset already computed, so telling one version of a mesh from another costs no walk
        glm::vec3 aabbMin = glm::vec3(0.0f);
        glm::vec3 aabbMax = glm::vec3(0.0f);
    };

    //what one mesh has to build, in mesh LOD index order. Index 0 is the original and is never in here
    struct LevelPlan {
        float targetError = 0.0f;
        bool silhouetteOnly = false;

        bool operator==(const LevelPlan &other) const {
            return targetError == other.targetError && silhouetteOnly == other.silhouetteOnly;
        }
#ifdef CEREAL_SUPPORT
        template<class Archive>
        void serialize(Archive &archive) {
            archive(targetError, silhouetteOnly);
        }
#endif
    };

    //EDITOR_CHANGE measures whatever the cache can not supply even with LOD_calibrate off, FULL_RECALIBRATE
    //drops the cache outright
    enum class BuildMode { NORMAL, EDITOR_CHANGE, FULL_RECALIBRATE };

    //storeIsBinary marks an asset whose levels live in its own file, so it never reads or writes a sidecar
    void bindAsset(const std::string &newAssetPath, const std::string &newFlipAxes, bool newCanCalibrate,
                   bool newStoreIsBinary);

    //project defaults and search settings. Read once per build, so an option change is picked up on reload
    void readSettings(OptionsUtil::Options *options);

    void build(const std::vector<MeshGeometry> &meshes, BuildMode buildMode, std::vector<LevelPlan> &outPlan);

    //builds the simplifier inputs if they are not built yet. Preparing them is the expensive part, so a load
    //that changes nothing never gets here
    void prepareGenerators(const std::vector<MeshGeometry> &meshes);

    //records the share only, the budget that reproduces it is measured by the next build. Caller rebuilds after
    void requestTriangleTarget(size_t stepIndex, float targetRatio);

    //back to the project defaults, dropping this model's own targets. Leaves the step list empty, so the caller
    //calls readSettings again before rebuilding
    void clearOverrides();

    //coarsest step this camera admits, as a mesh LOD index. previousLevel is the dead band, so an object sitting
    //on a threshold doesn't flip every frame
    uint32_t selectLevel(const LodSelectionContext &context, uint32_t previousLevel) const;

    //distance at which this outcome's error still projects to no more than the allowance. The one placement rule
    float switchDistanceOf(const LodStep::Outcome &outcome, const LodSelectionContext &context) const;

    //model units to pixels at distance one, for a projection rendered to a target of this height
    static float pixelsPerModelUnit(const glm::mat4 &projectionMatrix, uint32_t targetHeight);

    const std::vector<LodStep> &getSteps() const {
        return steps;
    }

    uint32_t getOriginalTriangleCount() const {
        return originalTriangleCount;
    }

    //how many index ranges the meshes hold, the original included
    uint32_t getMeshLodCount() const {
        return meshLodCount;
    }

    //true when the budgets above came from the metadata file rather than the project options
    bool hasOverrides() const {
        return overridesPresent;
    }

    //edits only change memory until the asset is saved, by the panel or by a world save
    bool hasUnsavedChanges() const {
        return unsavedChanges;
    }

    //a binary is saved by writing the whole asset, so this is cleared from outside
    void clearUnsavedChanges() {
        unsavedChanges = false;
    }

    //writes the targets to a source asset's sidecar. Does nothing for a binary or when nothing changed
    bool saveIntent();

    //the prepared simplifier inputs for one mesh, in the order they were handed to build
    const LodGenerator *getGenerator(size_t meshIndex) const {
        return meshIndex < generators.size() ? generators[meshIndex].get() : nullptr;
    }

#ifdef CEREAL_SUPPORT
    template<class Archive>
    void serialize(Archive &archive) {
        archive(steps, originalTriangleCount, meshLodCount, builtSettingsHash);
    }
#endif

private:
    //indices and what they cost together, so nothing is re-described between generating and measuring
    struct Candidate {
        std::vector<std::vector<uint16_t>> meshIndices;
        size_t triangleCount = 0;
        float silhouette = 0.0f;         //outline changes, holes and wrong uvs or normals together, over covered pixels
        float modelError = 0.0f;         //the worse of the outline displacement and the surface p95
        bool silhouetteOnly = false;     //welded, so its uvs and normals are not what it would be drawn with
    };

    //the two tests the search runs. Target error is a multiplicative scale, so the walk is geometric either way
    enum class SearchGoal {
        DAMAGE_UNDER_BUDGET,    //measures every candidate, which is what costs
        TRIANGLES_UNDER_COUNT   //reads the triangle count, which is free
    };

    struct View {
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 forward;
    };

    //what the nearest surface shows at each pixel. A mesh without uvs or normals leaves its pixels unmarked, so
    //nothing is compared against an attribute that doesn't exist
    struct RasterTarget {
        std::vector<float> depth;
        std::vector<glm::vec2> textureCoordinates;
        std::vector<glm::vec3> normals;
        std::vector<float> positionPerUv;   //model units per uv unit of the triangle drawn there, zero for a flat uv
        std::vector<uint8_t> attributeMask;

        void reset(uint32_t resolution);
    };
    static const uint8_t RASTER_HAS_UV = 1;
    static const uint8_t RASTER_HAS_NORMAL = 2;

    void appendDefaultSteps();
    void computeSettingsHash();
    //welding only pays at the coarse end, and only where no texture is ever sampled
    bool isWeldedStep(size_t stepIndex) const;

    static size_t countTriangles(const std::vector<MeshGeometry> &meshes);
    void generateCandidate(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                           float targetError, bool measure, Candidate &candidate) const;
    float bisectTargetError(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                            SearchGoal goal, float limit, Candidate &outBest) const;
    //fills one outcome from a winning candidate, or records why there is none
    bool searchStep(const std::vector<MeshGeometry> &meshes, const LodStep &step, LodGenerator::GeneratorKind kind,
                    size_t originalTriangles, LodStep::Outcome &outOutcome) const;
    //turns a recorded triangle share into the budget that reproduces it, and fills the outcome it measured
    bool deriveTriangleTarget(const std::vector<MeshGeometry> &meshes, LodStep &step, size_t originalTriangles,
                              LodStep::Outcome &outOutcome);
    //what a step gets when nothing may be measured: the budget read as an error, placed by meshopt's own bound
    void fallbackOutcome(const std::vector<MeshGeometry> &meshes, const LodStep &step,
                         LodGenerator::GeneratorKind kind, size_t originalTriangles,
                         LodStep::Outcome &outOutcome) const;
    //the only place that decides whether an outcome is kept
    bool acceptOutcome(const LodStep &step, size_t previousTriangleCount, LodStep::Outcome &outcome) const;
    void buildOneLadder(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                        size_t originalTriangles, const std::vector<LodStep> &cachedSteps, bool measureAllowed,
                        uint32_t &outMeasuredCount, bool &outDerivedAnyTarget);
    //every step either built or carrying a reason it could not be, so nothing is left to look up
    bool isLadderComplete() const;
    //the developer's targets, which outlive any change to the search
    void loadIntent();
    //a cached step built from the same budget describes the same mesh, so its outcome is taken as it is
    static bool findCachedOutcome(const std::vector<LodStep> &cachedSteps, const LodStep &step,
                                  bool welded, LodStep::Outcome &outOutcome);
    void assignMeshLodIndices(std::vector<LevelPlan> &outPlan);

    //the scorer: renders the candidate against the original from fourteen directions. meshoptimizer's own error
    //is a quadric bound that stops tracking visible damage past about 0.03, so steps are judged against this
    void measureCandidate(const std::vector<MeshGeometry> &meshes, Candidate &candidate) const;
    static void buildViews(std::vector<View> &views);
    static void rasterize(const std::vector<MeshGeometry> &meshes, const Candidate *candidate, const View &view,
                          const glm::vec3 &center, float scale, uint32_t resolution, RasterTarget &target);

    std::vector<LodStep> steps;
    uint32_t originalTriangleCount = 0;
    uint32_t meshLodCount = 1;
    std::vector<std::unique_ptr<LodGenerator>> generators;//prepared inputs, reused by every candidate and mesh
    //our own copy of every mesh's LOD0 range: building a mesh's levels appends to the buffer the caller handed
    //us, and a generator holding a pointer into it would be left reading freed storage
    std::vector<std::vector<uint16_t>> sourceIndices;

    std::string assetPath;
    std::string flipAxes;
    bool canCalibrate = true;       //a skinned mesh deforms, so a bind pose score says nothing about it
    bool overridesPresent = false;
    bool storeIsBinary = false;
    bool unsavedChanges = false;

    bool calibrationEnabled = true;
    uint32_t searchSteps = 7;
    uint32_t scorerResolution = 256;
    bool shadowWeldedLevels = true;
    uint32_t shadowWeldedFromLevel = 3;
    float uvDeviation = 1.0f / 2048.0f;
    float normalDeviation = 0.2f;
    uint64_t settingsHash = 0;
    uint64_t builtSettingsHash = 0;//what the outcomes above were produced under, so a settings change drops them
};

#endif //LIMONENGINE_LODLADDER_H
