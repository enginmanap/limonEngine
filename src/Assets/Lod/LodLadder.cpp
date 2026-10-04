//
// Created by engin on 25.09.2026.
//

#include "LodLadder.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>

#include "LodMetadata.h"
#include "consthash/include/consthash/cityhash64.hxx"
#include "limonAPI/Options.h"
#include "limonAPI/util/HashUtil.h"

//bumped by hand whenever the scorer, the search or a generator changes what it produces
static const uint32_t LOD_CALIBRATOR_VERSION = 11;

//the tuned defaults: maps keep about 80, 60 and 40 percent of their triangles at these distances
static const uint32_t LOD_DEFAULT_STEP_COUNT = 3;
static const float LOD_DEFAULT_DISTANCES[LOD_DEFAULT_STEP_COUNT] = {50.0f, 100.0f, 200.0f};
static const float LOD_DEFAULT_TRIANGLE_TARGETS[LOD_DEFAULT_STEP_COUNT] = {80.0f, 60.0f, 40.0f};
static const float LOD_DEFAULT_SURFACE_PIXELS[LOD_DEFAULT_STEP_COUNT] = {4.0f, 3.0f, 4.0f};
static const float LOD_DEFAULT_OUTLINE_PIXELS[LOD_DEFAULT_STEP_COUNT] = {4.0f, 3.0f, 4.0f};
static const float LOD_DEFAULT_HOLE_PIXELS[LOD_DEFAULT_STEP_COUNT] = {2.0f, 6.0f, 8.0f};
static const float LOD_DEFAULT_TEXTURE_PIXELS[LOD_DEFAULT_STEP_COUNT] = {8.0f, 12.0f, 16.0f};
static const float LOD_DEFAULT_NORMAL_PIXELS[LOD_DEFAULT_STEP_COUNT] = {8.0f, 12.0f, 16.0f};
static const uint32_t LOD_MAX_STEPS_FROM_OPTIONS = 7;//LOD0 plus this many, kept under the mesh side ceiling

static const float LOD_SEARCH_LOW_ERROR = 0.0005f;
static const float LOD_SEARCH_HIGH_ERROR = 0.4f;
//a step that saves less than this over the one before it is not worth its own index range and its own bake
static const float LOD_MIN_TRIANGLE_GAIN = 0.05f;
//past eight texels of a 2048 texture a uv change samples another part of the atlas rather than sliding
static const float LOD_UV_JUMP = 8.0f / 2048.0f;
//the surface number is this percentile of how far the pixels moved, so a single stray pixel can't fail a step
static const float LOD_SURFACE_QUANTILE = 0.99f;

static const uint32_t LOD_SCORER_MIN_RESOLUTION = 16;
//a surface that moved stays within a few pixels of where it was. Beyond this the nearest thing at that pixel
//is a different surface showing through a hole, and the size of that gap says nothing about how visible it is
static const float LOD_SCORER_SURFACE_JUMP_PIXELS = 4.0f;
static const uint32_t LOD_SCORER_VIEW_COUNT = 14;
//rendered at twice the screen size and judged in screen pixels, so a pixel holds what a real one would average
static const float LOD_SCORER_SUPERSAMPLING = 2.0f;

void LodLadder::bindAsset(const std::string &newAssetPath, const std::string &newFlipAxes, bool newCanCalibrate,
                          bool newStoreIsBinary) {
    assetPath = newAssetPath;
    flipAxes = newFlipAxes;
    canCalibrate = newCanCalibrate;
    storeIsBinary = newStoreIsBinary;
}

void LodLadder::appendDefaultSteps() {
    steps.clear();
    for (uint32_t step = 0; step < LOD_DEFAULT_STEP_COUNT; ++step) {
        LodStep entry;
        entry.distance = LOD_DEFAULT_DISTANCES[step];
        entry.triangleTarget = LOD_DEFAULT_TRIANGLE_TARGETS[step] / 100.0f;
        entry.limits.surface = LOD_DEFAULT_SURFACE_PIXELS[step];
        entry.limits.outline = LOD_DEFAULT_OUTLINE_PIXELS[step];
        entry.limits.holes = LOD_DEFAULT_HOLE_PIXELS[step];
        entry.limits.texture = LOD_DEFAULT_TEXTURE_PIXELS[step];
        entry.limits.normal = LOD_DEFAULT_NORMAL_PIXELS[step];
        steps.push_back(entry);
    }
}

bool LodLadder::isWeldedStep(size_t stepIndex) const {
    if (!shadowWeldedLevels) {
        return false;
    }
    return stepIndex + 1 >= shadowWeldedFromLevel;
}

//only what changes the meaning of every outcome at once. A step's distance and limits are deliberately not in
//here: a step is matched to a cached outcome by its own, so retargeting one step must not discard the others.
//LOD_calibrate is out too, a cached result was measured either way.
void LodLadder::computeSettingsHash() {
    char packed[192];
    //the reference as configured, never referencePixelsPerMeter: tan differs in its last digit between C runtimes,
    //and a binary copied to another machine recalibrated on every load. %.9g round trips the float options
    snprintf(packed, sizeof(packed), "v%u|s%u|r%u|h%u|f%.9g|w%d,%u|n%.9g", LOD_CALIBRATOR_VERSION, searchSteps,
             maximumResolution, referenceHeight, referenceFov, shadowWeldedLevels ? 1 : 0, shadowWeldedFromLevel,
             normalDeviation);
    settingsHash = consthash::city64(packed, strlen(packed));
}

//one list per field, one entry per step. An empty distance list is how a project switches levels off
void LodLadder::readStepsFromOptions(OptionsUtil::Options *options) {
    steps.clear();
    OptionsUtil::Options::Option<std::vector<float>> distanceOption =
            options->getOption<std::vector<float>>(HASH("LOD_levelDistances"));
    if (!distanceOption.isUsable()) {
        std::cerr << "LOD_levelDistances is missing, no LOD level is generated" << std::endl;
        return;
    }
    std::vector<float> distances = distanceOption.get();
    if (distances.empty()) {
        return;
    }
    const char *listNames[6] = {"LOD_levelTriangleTargets", "LOD_levelSurfacePixels", "LOD_levelOutlinePixels",
                                "LOD_levelHolePixels", "LOD_levelTexturePixels", "LOD_levelNormalPixels"};
    std::vector<float> lists[6];
    for (uint32_t list = 0; list < 6; ++list) {
        OptionsUtil::Options::Option<std::vector<float>> option =
                options->getOption<std::vector<float>>(consthash::city64(listNames[list], strlen(listNames[list])));
        if (option.isUsable()) {
            lists[list] = option.get();
        }
        if (lists[list].size() != distances.size()) {
            //a level judged with another level's limit would be built wrong in silence, so none is built
            std::cerr << listNames[list] << " has " << lists[list].size() << " entries but LOD_levelDistances has "
                      << distances.size() << ", no LOD level is generated" << std::endl;
            return;
        }
    }
    if (distances.size() > LOD_MAX_STEPS_FROM_OPTIONS) {
        std::cerr << "LOD_levelDistances asks for " << distances.size() << " levels, only the first "
                  << LOD_MAX_STEPS_FROM_OPTIONS << " are used" << std::endl;
        distances.resize(LOD_MAX_STEPS_FROM_OPTIONS);
    }
    for (size_t step = 0; step < distances.size(); ++step) {
        LodStep entry;
        entry.distance = distances[step];
        entry.triangleTarget = lists[0][step] / 100.0f;//authored as percent, like the panel reads
        entry.limits.surface = lists[1][step];
        entry.limits.outline = lists[2][step];
        entry.limits.holes = lists[3][step];
        entry.limits.texture = lists[4][step];
        entry.limits.normal = lists[5][step];
        steps.push_back(entry);
    }
}

void LodLadder::readSettings(OptionsUtil::Options *options) {
    if (options != nullptr) {
        calibrationEnabled = options->getOption<bool>(HASH("LOD_calibrate")).getOrDefault(true);
        searchSteps = (uint32_t) options->getOption<long>(HASH("LOD_calibrateSearchSteps")).getOrDefault(7L);
        if (searchSteps < 1) {
            searchSteps = 1;
        }
        maximumResolution = (uint32_t) options->getOption<long>(HASH("LOD_calibrationMaxResolution")).getOrDefault(2048L);
        maximumResolution = std::max(maximumResolution, LOD_SCORER_MIN_RESOLUTION);
        shadowWeldedLevels = options->getOption<bool>(HASH("LOD_shadowWeldedLevels")).getOrDefault(true);
        shadowWeldedFromLevel = (uint32_t) options->getOption<long>(HASH("LOD_shadowWeldedFromLevel")).getOrDefault(2L);
        normalDeviation = (float) options->getOption<double>(HASH("LOD_normalDeviation")).getOrDefault(0.2);
        referenceHeight = (uint32_t) options->getOption<long>(HASH("LOD_referenceHeight")).getOrDefault(1080L);
        referenceFov = (float) options->getOption<double>(HASH("LOD_referenceFov")).getOrDefault(60.0);
        referencePixelsPerMeter = (float) (0.5 * (double) referenceHeight / std::tan(glm::radians(referenceFov * 0.5)));
    }

    //the steps a binary load brought with it carry their own intent, so the project options only fill in an
    //empty ladder. An override read below replaces them either way
    if (steps.empty()) {
        if (options == nullptr) {
            appendDefaultSteps();//a scratch tool with no options, the engine always has them
        } else {
            readStepsFromOptions(options);
        }
    }
    computeSettingsHash();
}

void LodLadder::prepareGenerators(const std::vector<MeshGeometry> &meshes) {
    if (generators.size() == meshes.size()) {
        return;//already prepared for this model, and preparing is the expensive part
    }
    generators.clear();
    generators.reserve(meshes.size());
    sourceIndices.assign(meshes.size(), std::vector<uint16_t>());
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const MeshGeometry &mesh = meshes[meshIndex];
        sourceIndices[meshIndex].assign(mesh.indices, mesh.indices + mesh.indexCount);
        generators.push_back(std::unique_ptr<LodGenerator>(new LodGenerator(*mesh.vertices, *mesh.normals,
                                                                           *mesh.textureCoordinates,
                                                                           sourceIndices[meshIndex].data(),
                                                                           sourceIndices[meshIndex].size())));
    }
}

size_t LodLadder::countTriangles(const std::vector<MeshGeometry> &meshes) {
    size_t triangles = 0;
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        triangles += meshes[meshIndex].indexCount / 3;
    }
    return triangles;
}

void LodLadder::generateCandidate(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                                  float targetError, Candidate &candidate) const {
    candidate.meshIndices.assign(meshes.size(), std::vector<uint16_t>());
    candidate.triangleCount = 0;
    candidate.silhouetteOnly = kind == LodGenerator::GeneratorKind::SILHOUETTE_ONLY;
    candidate.measured = LodPixels();
    candidate.clipped = false;
    for (size_t meshIndex = 0; meshIndex < meshes.size() && meshIndex < generators.size(); ++meshIndex) {
        float ignoredRelativeError = 0.0f;
        generators[meshIndex]->generate(kind, targetError, candidate.meshIndices[meshIndex], ignoredRelativeError);
        candidate.triangleCount += candidate.meshIndices[meshIndex].size() / 3;
    }
}

//geometric bisection, because target error is a multiplicative scale, not an additive one
float LodLadder::bisectForTriangleCount(const std::vector<MeshGeometry> &meshes, float wantedTriangles,
                                        Candidate &outBest) const {
    float low = LOD_SEARCH_LOW_ERROR;
    float high = LOD_SEARCH_HIGH_ERROR;
    float bestError = 0.0f;
    Candidate candidate;
    for (uint32_t step = 0; step < searchSteps; ++step) {
        float middle = std::sqrt(low * high);
        generateCandidate(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING, middle, candidate);
        if ((float) candidate.triangleCount <= wantedTriangles) {
            outBest = candidate;
            bestError = middle;
            high = middle;
        } else {
            low = middle;
        }
    }
    return bestError;
}

void LodLadder::fillOutcome(const Candidate &candidate, float targetError, size_t originalTriangles,
                            LodStep::Outcome &outOutcome) {
    outOutcome.targetError = targetError;
    outOutcome.triangleCount = (uint32_t) candidate.triangleCount;
    outOutcome.achievedRatio = originalTriangles > 0 ? (float) candidate.triangleCount / (float) originalTriangles : 0.0f;
    outOutcome.measured = candidate.measured;
    outOutcome.clipped = candidate.clipped;
}

//the same geometric walk, but every candidate is rendered: what passes moves the error up, what fails moves it
//down, so the walk ends on the coarsest simplification seen to pass
bool LodLadder::searchStep(const std::vector<MeshGeometry> &meshes, const LodStep &step,
                           LodGenerator::GeneratorKind kind, size_t originalTriangles,
                           LodStep::Outcome &outOutcome) const {
    float low = LOD_SEARCH_LOW_ERROR;
    float high = LOD_SEARCH_HIGH_ERROR;
    float bestError = 0.0f;
    Candidate best;
    Candidate candidate;
    for (uint32_t iteration = 0; iteration < searchSteps; ++iteration) {
        float middle = std::sqrt(low * high);
        generateCandidate(meshes, kind, middle, candidate);
        if (measureCandidate(meshes, step.distance, step.limits, candidate)) {
            best = candidate;
            bestError = middle;
            low = middle;
        } else {
            high = middle;
        }
    }
    if (bestError <= 0.0f) {
        //even the smallest simplification breaks a limit at this distance. A coarser step has its own distance
        //and its own limits, so it is still searched
        outOutcome.skipReason = LodSkipReason::NOTHING_FITS;
        return false;
    }
    fillOutcome(best, bestError, originalTriangles, outOutcome);
    return true;
}

bool LodLadder::buildForTriangleShare(const std::vector<MeshGeometry> &meshes, const LodStep &step, float share,
                                      size_t originalTriangles, bool measure, LodStep::Outcome &outOutcome) const {
    if (share <= 0.0f || share >= 1.0f || originalTriangles == 0) {
        return false;
    }
    Candidate best;
    float targetError = bisectForTriangleCount(meshes, std::max(1.0f, (float) originalTriangles * share), best);
    if (targetError <= 0.0f) {
        //hard edges and protected seams put a floor under a mesh. Take the coarsest there is and let the panel
        //report the share it actually reached
        generateCandidate(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING, LOD_SEARCH_HIGH_ERROR, best);
        targetError = LOD_SEARCH_HIGH_ERROR;
    }
    if (best.triangleCount == 0) {
        std::cerr << "could not build a LOD step at " << share * 100.0f << " percent for " << assetPath << std::endl;
        return false;
    }
    if (measure) {
        measureCandidate(meshes, step.distance, step.limits, best);//only for the panel, a typed share is obeyed
    }
    fillOutcome(best, targetError, originalTriangles, outOutcome);
    return true;
}

//the only place that decides whether an outcome is kept. A step the developer asked for is force enabled, we
//don't know the model or the map, so a 2% saving may be exactly the point
bool LodLadder::acceptOutcome(const LodStep &step, size_t previousTriangleCount, LodStep::Outcome &outcome) const {
    if (outcome.triangleCount == 0) {
        outcome.skipReason = LodSkipReason::EMPTY;
        return false;
    }
    if (!step.userSet && previousTriangleCount > 0 &&
        outcome.triangleCount > (uint32_t) (previousTriangleCount * (1.0f - LOD_MIN_TRIANGLE_GAIN))) {
        //a step that barely improves on its predecessor costs an index range, a buffer range and a bake for nothing
        outcome.skipReason = LodSkipReason::NO_GAIN;
        return false;
    }
    outcome.built = true;
    outcome.skipReason = LodSkipReason::NONE;
    return true;
}

//the asked half has to match exactly: a step retargeted to a new share or new limits must not get the mesh it
//was replacing
bool LodLadder::findCachedOutcome(const std::vector<LodStep> &cachedSteps, const LodStep &step, bool welded,
                                  LodStep::Outcome &outOutcome) {
    for (size_t cachedIndex = 0; cachedIndex < cachedSteps.size(); ++cachedIndex) {
        const LodStep &cached = cachedSteps[cachedIndex];
        if (cached.distance != step.distance || !(cached.limits == step.limits) ||
            std::fabs(cached.requestedRatio - step.requestedRatio) > 1e-9f) {
            continue;
        }
        const LodStep::Outcome &cachedOutcome = welded ? cached.welded : cached.structure;
        if (!cachedOutcome.built && cachedOutcome.skipReason == LodSkipReason::NONE) {
            continue;//never searched, says nothing we can reuse
        }
        outOutcome = cachedOutcome;
        return true;
    }
    return false;
}

void LodLadder::buildOneLadder(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                               size_t originalTriangles, const std::vector<LodStep> &cachedSteps,
                               bool generationAllowed, uint32_t &outMeasuredCount, bool &outDerivedAnyTarget) {
    const bool welded = kind == LodGenerator::GeneratorKind::SILHOUETTE_ONLY;
    size_t previousTriangleCount = originalTriangles;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        LodStep &step = steps[stepIndex];
        if (welded && !isWeldedStep(stepIndex)) {
            //depth cameras draw this step's structure outcome, so the first twin has to beat that, not the original
            if (step.structure.built) {
                previousTriangleCount = step.structure.triangleCount;
            }
            continue;
        }
        LodStep::Outcome &outcome = welded ? step.welded : step.structure;
        if (outcome.built) {
            previousTriangleCount = outcome.triangleCount;
            continue;//already taken from the steps this ladder arrived with
        }
        if (outcome.skipReason != LodSkipReason::NONE) {
            continue;//searched and refused before, searching again only finds the same refusal on every load
        }
        if (findCachedOutcome(cachedSteps, step, welded, outcome)) {
            if (outcome.built) {
                previousTriangleCount = outcome.triangleCount;
            }
            continue;
        }
        if (!generationAllowed || step.distance <= 0.0f) {
            continue;//left unbuilt, a later load with generation on searches it
        }
        prepareGenerators(meshes);
        if (!welded && step.userSet && step.requestedRatio > 0.0f) {
            if (buildForTriangleShare(meshes, step, step.requestedRatio, originalTriangles, canCalibrate, outcome)) {
                outMeasuredCount++;
                outDerivedAnyTarget = true;
            } else {
                //the simplifier will not go that far on this model, so the share is dropped rather than kept as
                //an intent nothing can satisfy. The limits take the step from the next build
                step.requestedRatio = 0.0f;
                step.userSet = false;
                continue;
            }
        } else if (!canCalibrate) {
            //a deforming mesh has no pose to render, so the project's hoped for share is all there is to go by.
            //Its welded twin would need a measurement too, so it has none
            if (welded || !buildForTriangleShare(meshes, step, step.triangleTarget, originalTriangles, false, outcome)) {
                continue;
            }
        } else {
            outMeasuredCount++;
            if (!searchStep(meshes, step, kind, originalTriangles, outcome)) {
                continue;
            }
        }
        size_t mustBeat = previousTriangleCount;
        if (welded && step.structure.built) {
            //depth cameras could draw this step's structure outcome instead, a twin with more triangles is waste
            mustBeat = std::min(mustBeat, (size_t) step.structure.triangleCount);
        }
        if (acceptOutcome(step, mustBeat, outcome)) {
            previousTriangleCount = outcome.triangleCount;
        }
    }
}

//index ranges are positional on the mesh side, so the order the plan is emitted in is the order every mesh
//appends, and it is the only thing that maps a step to what the render list draws
void LodLadder::assignMeshLodIndices(std::vector<LevelPlan> &outPlan) {
    outPlan.clear();
    meshLodCount = 1;//index 0 is the original mesh and always exists
    for (size_t pass = 0; pass < 2; ++pass) {
        const bool welded = pass == 1;
        for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
            LodStep::Outcome &outcome = welded ? steps[stepIndex].welded : steps[stepIndex].structure;
            if (!outcome.built) {
                continue;
            }
            outcome.meshLodIndex = meshLodCount;
            LevelPlan plan;
            plan.targetError = outcome.targetError;
            plan.silhouetteOnly = welded;
            outPlan.push_back(plan);
            meshLodCount++;
        }
    }
}

void LodLadder::build(const std::vector<MeshGeometry> &meshes, BuildMode buildMode, std::vector<LevelPlan> &outPlan) {
    outPlan.clear();
    meshLodCount = 1;
    if (meshes.empty() || steps.empty()) {
        return;
    }
    //intent comes off disk when the asset loads and never again: an editor rebuild is running because memory
    //already holds a newer target than the file, and the file is only written once this build has derived it
    if (buildMode == BuildMode::NORMAL) {
        loadIntent();
    }
    //an outcome stays until something drops it, and these two are the only things that drop all of them at once.
    //Never clear them all here and restore the good ones after, that hands a retargeted step its old mesh back
    if (builtSettingsHash != settingsHash || buildMode == BuildMode::FULL_RECALIBRATE) {
        for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
            steps[stepIndex].structure = LodStep::Outcome();
            steps[stepIndex].welded = LodStep::Outcome();
        }
    }

    size_t originalTriangles = countTriangles(meshes);
    if (originalTriangles == 0) {
        return;
    }
    originalTriangleCount = (uint32_t) originalTriangles;

    std::vector<LodStep> cachedSteps;
    //hashing walks every vertex, so it waits until something actually needs the file. A binary whose targets have
    //not moved still holds every outcome, so it never gets here
    uint64_t geometryHash = 0;
    if (!storeIsBinary && !isLadderComplete() && buildMode != BuildMode::FULL_RECALIBRATE) {
        geometryHash = LodMetadata::hashGeometry(meshes);
        LodMetadata::read(assetPath, flipAxes, settingsHash, geometryHash, cachedSteps);
    }

    //LOD_calibrate off means nothing new is made: a model uses what its sidecar or limonmodel holds, or the original
    bool generationAllowed = calibrationEnabled || buildMode != BuildMode::NORMAL;
    bool derivedAnyTarget = false;
    uint32_t measuredCount = 0;
    std::chrono::steady_clock::time_point buildStart = std::chrono::steady_clock::now();
    buildOneLadder(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING, originalTriangles, cachedSteps,
                   generationAllowed, measuredCount, derivedAnyTarget);
    //welded twins for the coarse steps, used by depth only cameras where no texture is ever sampled
    buildOneLadder(meshes, LodGenerator::GeneratorKind::SILHOUETTE_ONLY, originalTriangles, cachedSteps,
                   generationAllowed, measuredCount, derivedAnyTarget);

    if (measuredCount > 0) {
        //only when it actually ran, so a cached load stays quiet and the first load shows what it cost
        std::cout << "calibrated " << measuredCount << " LOD steps for " << assetPath << " in "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count()
                  << " ms" << std::endl;
        if (!storeIsBinary) {//a binary keeps its outcomes in its own steps, they reach disk when it is saved
            if (geometryHash == 0) {
                geometryHash = LodMetadata::hashGeometry(meshes);
            }
            LodMetadata::write(assetPath, flipAxes, settingsHash, geometryHash, steps);
        }
    }
    //never on a plain load: deriving there reproduces what the file already says, and would mark every
    //converted model unsaved
    if (derivedAnyTarget && buildMode != BuildMode::NORMAL) {
        overridesPresent = true;
        unsavedChanges = true;
    }
    builtSettingsHash = settingsHash;
    assignMeshLodIndices(outPlan);
}

bool LodLadder::isLadderComplete() const {
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        if (!steps[stepIndex].structure.built && steps[stepIndex].structure.skipReason == LodSkipReason::NONE) {
            return false;
        }
        if (canCalibrate && isWeldedStep(stepIndex) && !steps[stepIndex].welded.built &&
            steps[stepIndex].welded.skipReason == LodSkipReason::NONE) {
            return false;
        }
    }
    return true;
}

void LodLadder::loadIntent() {
    if (storeIsBinary) {
        return;//the steps came with the archive
    }
    std::vector<LodStep> overriddenSteps;
    if (!LodMetadata::readOverrides(assetPath, flipAxes, overriddenSteps) || overriddenSteps.empty()) {
        return;
    }
    //only the typed shares are stored there, the distances and limits stay the project's
    for (size_t stepIndex = 0; stepIndex < overriddenSteps.size() && stepIndex < steps.size(); ++stepIndex) {
        if (!overriddenSteps[stepIndex].userSet) {
            continue;
        }
        steps[stepIndex].requestedRatio = overriddenSteps[stepIndex].requestedRatio;
        steps[stepIndex].userSet = true;
    }
    overridesPresent = true;
}

bool LodLadder::saveIntent() {
    if (storeIsBinary || !unsavedChanges) {
        return true;
    }
    if (!LodMetadata::writeOverrides(assetPath, flipAxes, overridesPresent ? steps : std::vector<LodStep>())) {
        return false;
    }
    unsavedChanges = false;
    return true;
}

void LodLadder::requestTriangleTarget(size_t stepIndex, float targetRatio) {
    if (stepIndex >= steps.size() || targetRatio <= 0.0f || targetRatio >= 1.0f) {
        return;
    }
    steps[stepIndex].requestedRatio = targetRatio;
    steps[stepIndex].userSet = true;
    //an outcome belongs to what was asked when it was made. Leaving the old one attached is how a retarget
    //silently came back with the mesh it was supposed to replace
    steps[stepIndex].structure = LodStep::Outcome();
    steps[stepIndex].welded = LodStep::Outcome();
}

void LodLadder::clearOverrides() {
    overridesPresent = false;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        steps[stepIndex].userSet = false;
        steps[stepIndex].requestedRatio = 0.0f;
    }
    unsavedChanges = true;
    //the project steps have to come back, and only readSettings knows them
    steps.clear();
}

uint32_t LodLadder::selectLevel(const LodSelectionContext &context, uint32_t previousLevel) const {
    if (meshLodCount <= 1) {
        return 0;
    }
    if (context.forceLevel >= 0) {
        return std::min((uint32_t) context.forceLevel, meshLodCount - 1);
    }
    //the two reaches the dead band allows. Moving up a step needs to be clearly past the switch, moving back
    //down clearly before it
    const float reachGoingCoarser = context.objectDistance * (1.0f - context.hysteresis);
    const float reachGoingFiner = context.objectDistance * (1.0f + context.hysteresis);
    uint32_t selected = 0;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        //welded outcomes drop UVs and normals, so only a depth camera may draw one. Where a step has no twin the
        //depth camera draws the structure outcome
        const LodStep &step = steps[stepIndex];
        const LodStep::Outcome &outcome = (context.isShadowCamera && step.welded.built) ? step.welded : step.structure;
        if (!outcome.built || step.distance <= 0.0f) {
            continue;
        }
        float switchDistance = step.distance * context.objectScale;
        float reach = outcome.meshLodIndex > previousLevel ? reachGoingCoarser : reachGoingFiner;
        if (reach >= switchDistance) {
            selected = outcome.meshLodIndex;//steps run fine to coarse, so the last one that fits is the coarsest
        }
    }
    return selected;
}

void LodLadder::buildViews(std::vector<View> &views) {
    //six axes plus the eight cube corners. The corners are where a player usually stands, and an axis only
    //view lets a face that moved diagonally hide behind its own silhouette
    const glm::vec3 directions[LOD_SCORER_VIEW_COUNT] = {
            glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0),
            glm::vec3(0, 1, 0), glm::vec3(0, -1, 0),
            glm::vec3(0, 0, 1), glm::vec3(0, 0, -1),
            glm::vec3(1, 1, 1), glm::vec3(-1, 1, 1), glm::vec3(1, -1, 1), glm::vec3(1, 1, -1),
            glm::vec3(-1, -1, 1), glm::vec3(-1, 1, -1), glm::vec3(1, -1, -1), glm::vec3(-1, -1, -1)
    };
    views.resize(LOD_SCORER_VIEW_COUNT);
    for (uint32_t viewIndex = 0; viewIndex < LOD_SCORER_VIEW_COUNT; ++viewIndex) {
        glm::vec3 forward = glm::normalize(directions[viewIndex]);
        //picking the helper by the axis we are looking down avoids a degenerate cross product
        glm::vec3 helper = std::fabs(forward.y) > 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        views[viewIndex].forward = forward;
        views[viewIndex].right = glm::normalize(glm::cross(helper, forward));
        views[viewIndex].up = glm::cross(forward, views[viewIndex].right);
    }
}

//the whole mesh, not each triangle: a palette triangle maps to nearly one texel, and its own ratio turned a
//fraction of a texel into tens of units of movement
float LodLadder::averagePositionPerUv(const MeshGeometry &mesh) {
    if (mesh.vertices == nullptr || mesh.textureCoordinates == nullptr || mesh.indices == nullptr ||
        mesh.textureCoordinates->size() != mesh.vertices->size()) {
        return 0.0f;
    }
    const std::vector<glm::vec3> &vertices = *mesh.vertices;
    const std::vector<glm::vec2> &textureCoordinates = *mesh.textureCoordinates;
    double positionLength = 0.0;
    double uvLength = 0.0;
    for (size_t triangle = 0; triangle + 2 < mesh.indexCount; triangle = triangle + 3) {
        for (uint32_t corner = 0; corner < 3; ++corner) {
            uint16_t first = mesh.indices[triangle + corner];
            uint16_t second = mesh.indices[triangle + (corner + 1) % 3];
            if (first >= vertices.size() || second >= vertices.size()) {
                continue;
            }
            positionLength += glm::length(vertices[first] - vertices[second]);
            uvLength += glm::length(textureCoordinates[first] - textureCoordinates[second]);
        }
    }
    return uvLength > 0.0 ? (float) (positionLength / uvLength) : 0.0f;
}

void LodLadder::RasterTarget::reset(uint32_t resolution) {
    size_t pixelCount = (size_t) resolution * resolution;
    depth.assign(pixelCount, FLT_MAX);
    textureCoordinates.assign(pixelCount, glm::vec2(0.0f));
    normals.assign(pixelCount, glm::vec3(0.0f));
    positionPerUv.assign(pixelCount, 0.0f);
    attributeMask.assign(pixelCount, 0);
}

//orthographic, both sides drawn, nearest depth wins. No backface culling, so open and double sided geometry
//like foliage cards doesn't register as damage just for being seen from behind. A null candidate is the original
void LodLadder::rasterize(const std::vector<MeshGeometry> &meshes, const Candidate *candidate, const View &view,
                          const glm::vec3 &center, float scale, uint32_t resolution,
                          const std::vector<float> &meshPositionPerUv, RasterTarget &target) {
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const MeshGeometry &mesh = meshes[meshIndex];
        const uint16_t *indices = mesh.indices;
        size_t indexCount = mesh.indexCount;
        if (candidate != nullptr) {
            if (meshIndex >= candidate->meshIndices.size() || candidate->meshIndices[meshIndex].empty()) {
                continue;
            }
            indices = &candidate->meshIndices[meshIndex][0];
            indexCount = candidate->meshIndices[meshIndex].size();
        }
        if (indices == nullptr || mesh.vertices == nullptr) {
            continue;
        }
        const std::vector<glm::vec3> &vertices = *mesh.vertices;
        const bool hasTextureCoordinates = mesh.textureCoordinates != nullptr &&
                                           mesh.textureCoordinates->size() == vertices.size();
        const bool hasNormals = mesh.normals != nullptr && mesh.normals->size() == vertices.size();
        const uint8_t meshAttributeMask = (uint8_t) ((hasTextureCoordinates ? RASTER_HAS_UV : 0) |
                                                     (hasNormals ? RASTER_HAS_NORMAL : 0));
        for (size_t triangle = 0; triangle + 2 < indexCount; triangle = triangle + 3) {
            glm::vec3 projected[3];
            bool indexOutOfRange = false;
            for (uint32_t corner = 0; corner < 3; ++corner) {
                uint16_t vertexIndex = indices[triangle + corner];
                if (vertexIndex >= vertices.size()) {
                    //an index that doesn't belong to this mesh would read out of bounds. Only this triangle is
                    //skipped: bailing out of the view left the later meshes undrawn and read as damage
                    indexOutOfRange = true;
                    break;
                }
                glm::vec3 relative = vertices[vertexIndex] - center;
                projected[corner] = glm::vec3(glm::dot(relative, view.right) * scale + resolution * 0.5f,
                                              glm::dot(relative, view.up) * scale + resolution * 0.5f,
                                              glm::dot(relative, view.forward) * scale);
            }
            if (indexOutOfRange) {
                continue;
            }
            float area = (projected[1].x - projected[0].x) * (projected[2].y - projected[0].y) -
                         (projected[1].y - projected[0].y) * (projected[2].x - projected[0].x);
            if (std::fabs(area) < 1e-12f) {
                continue;
            }
            const uint16_t corner0 = indices[triangle];
            const uint16_t corner1 = indices[triangle + 1];
            const uint16_t corner2 = indices[triangle + 2];
            glm::vec2 cornerUv[3] = {glm::vec2(0.0f), glm::vec2(0.0f), glm::vec2(0.0f)};
            glm::vec3 cornerNormal[3] = {glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
            if (hasTextureCoordinates) {
                const std::vector<glm::vec2> &textureCoordinates = *mesh.textureCoordinates;
                cornerUv[0] = textureCoordinates[corner0];
                cornerUv[1] = textureCoordinates[corner1];
                cornerUv[2] = textureCoordinates[corner2];
            }
            if (hasNormals) {
                const std::vector<glm::vec3> &normals = *mesh.normals;
                cornerNormal[0] = normals[corner0];
                cornerNormal[1] = normals[corner1];
                cornerNormal[2] = normals[corner2];
            }
            int32_t minX = std::max(0, (int32_t) std::floor(std::min(projected[0].x, std::min(projected[1].x, projected[2].x))));
            int32_t maxX = std::min((int32_t) resolution - 1, (int32_t) std::ceil(std::max(projected[0].x, std::max(projected[1].x, projected[2].x))));
            int32_t minY = std::max(0, (int32_t) std::floor(std::min(projected[0].y, std::min(projected[1].y, projected[2].y))));
            int32_t maxY = std::min((int32_t) resolution - 1, (int32_t) std::ceil(std::max(projected[0].y, std::max(projected[1].y, projected[2].y))));
            for (int32_t y = minY; y <= maxY; ++y) {
                for (int32_t x = minX; x <= maxX; ++x) {
                    float pixelX = x + 0.5f;
                    float pixelY = y + 0.5f;
                    float weight0 = ((projected[1].x - pixelX) * (projected[2].y - pixelY) -
                                     (projected[1].y - pixelY) * (projected[2].x - pixelX)) / area;
                    float weight1 = ((projected[2].x - pixelX) * (projected[0].y - pixelY) -
                                     (projected[2].y - pixelY) * (projected[0].x - pixelX)) / area;
                    float weight2 = 1.0f - weight0 - weight1;
                    if (weight0 < 0 || weight1 < 0 || weight2 < 0) {
                        continue;
                    }
                    float pixelDepth = weight0 * projected[0].z + weight1 * projected[1].z + weight2 * projected[2].z;
                    size_t pixel = (size_t) y * resolution + x;
                    if (pixelDepth < target.depth[pixel]) {
                        target.depth[pixel] = pixelDepth;
                        target.attributeMask[pixel] = meshAttributeMask;
                        target.textureCoordinates[pixel] = weight0 * cornerUv[0] + weight1 * cornerUv[1] + weight2 * cornerUv[2];
                        target.normals[pixel] = weight0 * cornerNormal[0] + weight1 * cornerNormal[1] + weight2 * cornerNormal[2];
                        target.positionPerUv[pixel] = meshPositionPerUv[meshIndex];
                    }
                }
            }
        }
    }
}

//the width of the widest patch, as 4 * area / perimeter: a square reads as its side, a one pixel line as one
//however long it runs. Visited pixels are marked 2, so the mask is spent afterwards
float LodLadder::largestRegionWidth(std::vector<uint8_t> &damaged, uint32_t resolution) {
    float widest = 0.0f;
    std::vector<uint32_t> pending;
    for (uint32_t start = 0; start < damaged.size(); ++start) {
        if (damaged[start] != 1) {
            continue;
        }
        uint64_t area = 0;
        uint64_t perimeter = 0;
        damaged[start] = 2;
        pending.push_back(start);
        while (!pending.empty()) {
            uint32_t pixel = pending.back();
            pending.pop_back();
            area++;
            uint32_t x = pixel % resolution;
            uint32_t y = pixel / resolution;
            const bool hasNeighbour[4] = {x > 0, x + 1 < resolution, y > 0, y + 1 < resolution};
            const uint32_t neighbour[4] = {pixel - 1, pixel + 1, pixel - resolution, pixel + resolution};
            for (uint32_t side = 0; side < 4; ++side) {
                if (!hasNeighbour[side] || damaged[neighbour[side]] == 0) {
                    perimeter++;
                } else if (damaged[neighbour[side]] == 1) {
                    damaged[neighbour[side]] = 2;
                    pending.push_back(neighbour[side]);
                }
            }
        }
        if (perimeter > 0) {
            widest = std::max(widest, (float) (4.0 * (double) area / (double) perimeter));
        }
    }
    return widest;
}

//renders the candidate against the original at the size it has on the reference screen at this distance, from
//fourteen directions, twice that size and judged in screen pixels. Nothing here is relative to the model's size,
//which is what made "one pixel" mean a different thing for every model before.
//Several loader threads run this at once, so it must hold no state between calls
bool LodLadder::measureCandidate(const std::vector<MeshGeometry> &meshes, float distance, const LodPixels &limits,
                                 Candidate &candidate) const {
    candidate.measured = LodPixels();
    candidate.clipped = false;
    if (meshes.empty() || distance <= 0.0f) {
        return false;
    }

    glm::vec3 low(FLT_MAX);
    glm::vec3 high(-FLT_MAX);
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const std::vector<glm::vec3> &vertices = *meshes[meshIndex].vertices;
        for (size_t vertex = 0; vertex < vertices.size(); ++vertex) {
            low = glm::min(low, vertices[vertex]);
            high = glm::max(high, vertices[vertex]);
        }
    }
    if (low.x > high.x) {
        return false;//no vertices at all
    }
    glm::vec3 center = (low + high) * 0.5f;
    glm::vec3 size = high - low;
    float extent = std::max(size.x, std::max(size.y, size.z));
    if (extent <= 0.0f) {
        return false;//a single point, nothing to compare
    }

    //a corner view sees the box across its diagonal, so the image is 1.8 extents wide instead of clipping it
    const float screenScale = referencePixelsPerMeter / distance;
    float scale = LOD_SCORER_SUPERSAMPLING * screenScale;
    uint32_t resolution = (uint32_t) std::ceil(1.8f * extent * scale);
    if (resolution > maximumResolution) {
        resolution = maximumResolution;
        scale = (float) resolution / (1.8f * extent);
        candidate.clipped = true;
    }
    resolution = std::max(resolution, LOD_SCORER_MIN_RESOLUTION);
    //render pixels per screen pixel, the supersampling unless the cap took some of it
    const float perScreenPixel = scale / screenScale;

    std::vector<View> views;
    buildViews(views);
    std::vector<float> meshPositionPerUv(meshes.size(), 0.0f);
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        meshPositionPerUv[meshIndex] = averagePositionPerUv(meshes[meshIndex]);
    }
    RasterTarget reference;
    RasterTarget simplified;
    std::vector<uint8_t> holeMask;
    std::vector<uint8_t> textureMask;
    std::vector<uint8_t> normalMask;
    std::vector<float> movements;//screen pixels
    uint64_t mismatched = 0;
    uint64_t boundary = 0;
    for (size_t viewIndex = 0; viewIndex < views.size(); ++viewIndex) {
        reference.reset(resolution);
        simplified.reset(resolution);
        rasterize(meshes, nullptr, views[viewIndex], center, scale, resolution, meshPositionPerUv, reference);
        rasterize(meshes, &candidate, views[viewIndex], center, scale, resolution, meshPositionPerUv, simplified);
        holeMask.assign(reference.depth.size(), 0);
        textureMask.assign(reference.depth.size(), 0);
        normalMask.assign(reference.depth.size(), 0);
        for (size_t pixel = 0; pixel < reference.depth.size(); ++pixel) {
            bool referenceCovered = reference.depth[pixel] != FLT_MAX;
            bool candidateCovered = simplified.depth[pixel] != FLT_MAX;
            if (referenceCovered) {
                //an edge of the original outline, which is what the mismatched band is spread along
                uint32_t x = (uint32_t) (pixel % resolution);
                uint32_t y = (uint32_t) (pixel / resolution);
                bool onEdge = x == 0 || y == 0 || x + 1 == resolution || y + 1 == resolution ||
                              reference.depth[pixel - 1] == FLT_MAX || reference.depth[pixel + 1] == FLT_MAX ||
                              reference.depth[pixel - resolution] == FLT_MAX ||
                              reference.depth[pixel + resolution] == FLT_MAX;
                if (onEdge) {
                    boundary++;
                }
            }
            if (referenceCovered != candidateCovered) {
                mismatched++;//the outline moved, in or out
                continue;
            }
            if (!referenceCovered) {
                continue;
            }
            float movement = std::fabs(reference.depth[pixel] - simplified.depth[pixel]);
            if (movement > LOD_SCORER_SURFACE_JUMP_PIXELS * perScreenPixel) {
                //not the same surface displaced, something behind it showing through
                holeMask[pixel] = 1;
                continue;
            }
            //a welded level keeps whichever copy's uv and normal it landed on, and only depth cameras draw it
            uint8_t sharedAttributes = candidate.silhouetteOnly ? 0 :
                                       reference.attributeMask[pixel] & simplified.attributeMask[pixel];
            if (sharedAttributes & RASTER_HAS_UV) {
                glm::vec2 uvDifference = reference.textureCoordinates[pixel] - simplified.textureCoordinates[pixel];
                float uvShift = std::max(std::fabs(uvDifference.x), std::fabs(uvDifference.y));
                if (uvShift > LOD_UV_JUMP) {
                    textureMask[pixel] = 1;//another part of the atlas, damage rather than a distance
                } else {
                    //texture sliding across a surface moves what is seen as much as the surface moving would
                    movement = std::max(movement, glm::length(uvDifference) * reference.positionPerUv[pixel] * scale);
                }
            }
            if (sharedAttributes & RASTER_HAS_NORMAL) {
                glm::vec3 referenceNormal = reference.normals[pixel];
                glm::vec3 candidateNormal = simplified.normals[pixel];
                float referenceLength = glm::length(referenceNormal);
                float candidateLength = glm::length(candidateNormal);
                //interpolated normals shrink between corners and the shader normalizes them, so we do too
                if (referenceLength > 0.0f && candidateLength > 0.0f &&
                    glm::length(referenceNormal / referenceLength - candidateNormal / candidateLength) > normalDeviation) {
                    normalMask[pixel] = 1;
                }
            }
            movements.push_back(movement / perScreenPixel);
        }
        candidate.measured.holes = std::max(candidate.measured.holes, largestRegionWidth(holeMask, resolution) / perScreenPixel);
        candidate.measured.texture = std::max(candidate.measured.texture, largestRegionWidth(textureMask, resolution) / perScreenPixel);
        candidate.measured.normal = std::max(candidate.measured.normal, largestRegionWidth(normalMask, resolution) / perScreenPixel);
    }
    //the mismatched pixels form a band along the outline, so its width is the band over the outline length
    candidate.measured.outline = boundary > 0 ? (float) ((double) mismatched / (double) boundary) / perScreenPixel : 0.0f;
    if (!movements.empty()) {
        size_t index = (size_t) (LOD_SURFACE_QUANTILE * (double) (movements.size() - 1));
        std::nth_element(movements.begin(), movements.begin() + index, movements.end());
        candidate.measured.surface = movements[index];
    }

    const LodPixels &measured = candidate.measured;
    if (limits.surface >= 0.0f && measured.surface > limits.surface) {
        return false;
    }
    if (limits.outline >= 0.0f && measured.outline > limits.outline) {
        return false;
    }
    if (limits.holes >= 0.0f && measured.holes > limits.holes) {
        return false;
    }
    if (candidate.silhouetteOnly) {
        return true;//a depth pass shows no texture and no shading
    }
    if (limits.texture >= 0.0f && measured.texture > limits.texture) {
        return false;
    }
    if (limits.normal >= 0.0f && measured.normal > limits.normal) {
        return false;
    }
    return true;
}
