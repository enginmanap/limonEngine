//
// Created by engin on 25.09.2026.
//

#include "LodLadder.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "LodMetadata.h"
#include "consthash/include/consthash/cityhash64.hxx"
#include "limonAPI/Options.h"
#include "limonAPI/util/HashUtil.h"

//bumped by hand whenever the scorer, the search or a generator changes what it produces
static const uint32_t LOD_CALIBRATOR_VERSION = 7;

//four steps, each allowed more wrong pixels than the one before it
static const float LOD_DEFAULT_SILHOUETTE_BUDGETS[4] = {0.005f, 0.02f, 0.05f, 0.10f};
static const uint32_t LOD_MAX_STEPS_FROM_OPTIONS = 7;//LOD0 plus this many, kept under the mesh side ceiling

static const float LOD_SEARCH_LOW_ERROR = 0.0005f;
static const float LOD_SEARCH_HIGH_ERROR = 0.4f;
//a step that saves less than this over the one before it is not worth its own index range and its own bake
static const float LOD_MIN_TRIANGLE_GAIN = 0.05f;
//the smallest damage the scorer can resolve. A mild triangle target can measure zero, and a budget of zero is
//a configuration nothing can meet, so it is floored to this instead of refusing the step
static const float LOD_MIN_BUDGET = 1e-6f;
//the surface percentile a step is placed by, so a handful of stray pixels cannot set the distance
static const float LOD_PLACEMENT_QUANTILE = 0.95f;

static const uint32_t LOD_SCORER_MIN_RESOLUTION = 32;
//a surface that moved stays within a few pixels of where it was. Beyond this the nearest thing at that pixel
//is a different surface showing through a hole, and the size of that gap says nothing about how visible it is
static const float LOD_SCORER_SURFACE_JUMP_PIXELS = 4.0f;
static const uint32_t LOD_SCORER_VIEW_COUNT = 14;

void LodLadder::bindAsset(const std::string &newAssetPath, const std::string &newFlipAxes, bool newCanCalibrate,
                          bool newStoreIsBinary) {
    assetPath = newAssetPath;
    flipAxes = newFlipAxes;
    canCalibrate = newCanCalibrate;
    storeIsBinary = newStoreIsBinary;
}

void LodLadder::appendDefaultSteps() {
    steps.clear();
    for (uint32_t step = 0; step < 4; ++step) {
        LodStep entry;
        entry.silhouetteBudget = LOD_DEFAULT_SILHOUETTE_BUDGETS[step];
        steps.push_back(entry);
    }
}

bool LodLadder::isWeldedStep(size_t stepIndex) const {
    if (!shadowWeldedLevels) {
        return false;
    }
    //the corpus puts welding at about 1.3x the saving around 4% outline damage and 3x at 27%, so below the
    //coarse end it costs damage for nothing
    return stepIndex + 1 >= shadowWeldedFromLevel;
}

//only what changes the meaning of every outcome at once. The budgets are deliberately not in here: a step is
//matched to a cached outcome by its own budget, so retargeting one step must not discard the other five.
//LOD_calibrate is out too, a cached result was measured either way.
void LodLadder::computeSettingsHash() {
    std::string packed = "v" + std::to_string(LOD_CALIBRATOR_VERSION) + "|s" + std::to_string(searchSteps) +
                         "|r" + std::to_string(scorerResolution) + "|w" + std::to_string(shadowWeldedLevels ? 1 : 0) +
                         "," + std::to_string(shadowWeldedFromLevel);
    char deviations[64];
    //to_string keeps six decimals, so 1/4096 and 1/4100 would share a cache entry
    snprintf(deviations, sizeof(deviations), "|u%.9g|n%.9g", uvDeviation, normalDeviation);
    packed += deviations;
    settingsHash = consthash::city64(packed.c_str(), packed.length());
}

void LodLadder::readSettings(OptionsUtil::Options *options) {
    if (options != nullptr) {
        calibrationEnabled = options->getOption<bool>(HASH("LOD_calibrate")).getOrDefault(true);
        searchSteps = (uint32_t) options->getOption<long>(HASH("LOD_calibrateSearchSteps")).getOrDefault(7L);
        if (searchSteps < 1) {
            searchSteps = 1;
        }
        scorerResolution = (uint32_t) options->getOption<long>(HASH("LOD_scorerResolution")).getOrDefault(256L);
        shadowWeldedLevels = options->getOption<bool>(HASH("LOD_shadowWeldedLevels")).getOrDefault(true);
        shadowWeldedFromLevel = (uint32_t) options->getOption<long>(HASH("LOD_shadowWeldedFromLevel")).getOrDefault(3L);
        uvDeviation = (float) options->getOption<double>(HASH("LOD_uvDeviation")).getOrDefault(1.0 / 2048.0);
        normalDeviation = (float) options->getOption<double>(HASH("LOD_normalDeviation")).getOrDefault(0.2);
    }

    //the steps a binary load brought with it carry their own intent, so the project defaults only fill in an
    //empty ladder. An override read below replaces them either way
    if (steps.empty()) {
        std::vector<float> budgets;
        if (options != nullptr) {
            OptionsUtil::Options::Option<std::vector<float>> budgetOption =
                    options->getOption<std::vector<float>>(HASH("LOD_levelSilhouetteBudgetList"));
            if (budgetOption.isUsable()) {
                budgets = budgetOption.get();
            }
        }
        if (budgets.empty()) {
            if (options != nullptr) {
                std::cerr << "LOD_levelSilhouetteBudgetList is missing or empty, using the measured defaults" << std::endl;
            }
            appendDefaultSteps();
        } else {
            if (budgets.size() > LOD_MAX_STEPS_FROM_OPTIONS) {
                std::cerr << "LOD_levelSilhouetteBudgetList asks for " << budgets.size() << " steps, only the first "
                          << LOD_MAX_STEPS_FROM_OPTIONS << " are used" << std::endl;
                budgets.resize(LOD_MAX_STEPS_FROM_OPTIONS);
            }
            steps.clear();
            for (size_t step = 0; step < budgets.size(); ++step) {
                LodStep entry;
                //budgets are authored as percent, because that is how the report reads
                entry.silhouetteBudget = budgets[step] / 100.0f;
                steps.push_back(entry);
            }
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
                                  float targetError, bool measure, Candidate &candidate) const {
    candidate.meshIndices.assign(meshes.size(), std::vector<uint16_t>());
    candidate.triangleCount = 0;
    candidate.silhouetteOnly = kind == LodGenerator::GeneratorKind::SILHOUETTE_ONLY;
    candidate.silhouette = 0.0f;
    candidate.modelError = 0.0f;
    for (size_t meshIndex = 0; meshIndex < meshes.size() && meshIndex < generators.size(); ++meshIndex) {
        float ignoredRelativeError = 0.0f;
        generators[meshIndex]->generate(kind, targetError, candidate.meshIndices[meshIndex], ignoredRelativeError);
        candidate.triangleCount += candidate.meshIndices[meshIndex].size() / 3;
    }
    if (measure) {
        measureCandidate(meshes, candidate);
    }
}

//geometric bisection, because target error is a multiplicative scale, not an additive one. The two goals differ
//only in the test and in whether the candidate has to be rendered to apply it
float LodLadder::bisectTargetError(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                                   SearchGoal goal, float limit, Candidate &outBest) const {
    const bool measure = goal == SearchGoal::DAMAGE_UNDER_BUDGET;
    float low = LOD_SEARCH_LOW_ERROR;
    float high = LOD_SEARCH_HIGH_ERROR;
    float bestError = 0.0f;
    bool foundAny = false;
    Candidate candidate;
    for (uint32_t step = 0; step < searchSteps; ++step) {
        float middle = std::sqrt(low * high);
        generateCandidate(meshes, kind, middle, measure, candidate);
        bool fits = measure ? candidate.silhouette <= limit : (float) candidate.triangleCount <= limit;
        if (fits) {
            outBest = candidate;
            bestError = middle;
            foundAny = true;
            //damage grows with error and triangle count falls with it, so a fit means opposite things to move
            if (measure) {
                low = middle;
            } else {
                high = middle;
            }
        } else if (measure) {
            high = middle;
        } else {
            low = middle;
        }
    }
    return foundAny ? bestError : 0.0f;
}

bool LodLadder::searchStep(const std::vector<MeshGeometry> &meshes, const LodStep &step,
                           LodGenerator::GeneratorKind kind, size_t originalTriangles,
                           LodStep::Outcome &outOutcome) const {
    //a budget of zero is a broken configuration, not a strict one, so it is repaired rather than obeyed
    float budget = std::max(step.silhouetteBudget, LOD_MIN_BUDGET);
    Candidate best;
    float bestError = bisectTargetError(meshes, kind, SearchGoal::DAMAGE_UNDER_BUDGET, budget, best);
    if (bestError <= 0.0f) {
        //even the smallest error damages this model past its budget, so this step and every coarser one would
        //be worse. Saying so shortens the ladder rather than shipping something visibly wrong
        outOutcome.skipReason = LodSkipReason::NOTHING_FITS;
        return false;
    }
    outOutcome.targetError = bestError;
    outOutcome.modelError = best.modelError;
    outOutcome.triangleCount = (uint32_t) best.triangleCount;
    outOutcome.silhouetteDamage = best.silhouette;
    outOutcome.achievedRatio = originalTriangles > 0 ? (float) best.triangleCount / (float) originalTriangles : 0.0f;
    return true;
}

//nothing was measured, so meshopt's quadric bound is all there is. It places steps far too far out, which is
//what switching calibration off without a cached result costs
void LodLadder::fallbackOutcome(const std::vector<MeshGeometry> &meshes, const LodStep &step,
                                LodGenerator::GeneratorKind kind, size_t originalTriangles,
                                LodStep::Outcome &outOutcome) const {
    float meshScale = 0.0f;
    float worstRelativeError = 0.0f;
    size_t triangles = 0;
    std::vector<uint16_t> indices;
    for (size_t meshIndex = 0; meshIndex < meshes.size() && meshIndex < generators.size(); ++meshIndex) {
        float relativeError = 0.0f;
        generators[meshIndex]->generate(kind, step.silhouetteBudget, indices, relativeError);
        triangles += indices.size() / 3;
        worstRelativeError = std::max(worstRelativeError, relativeError);
        meshScale = std::max(meshScale, generators[meshIndex]->getMeshScale());
    }
    outOutcome.targetError = step.silhouetteBudget;
    outOutcome.modelError = worstRelativeError * meshScale;
    outOutcome.triangleCount = (uint32_t) triangles;
    outOutcome.achievedRatio = originalTriangles > 0 ? (float) triangles / (float) originalTriangles : 0.0f;
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

//budget alone is not enough to match on: a retargeted step still carries the old budget until the build derives
//the new one, so we would hand back the mesh the retarget was replacing
bool LodLadder::findCachedOutcome(const std::vector<LodStep> &cachedSteps, const LodStep &step, bool welded,
                                  LodStep::Outcome &outOutcome) {
    for (size_t cachedIndex = 0; cachedIndex < cachedSteps.size(); ++cachedIndex) {
        const LodStep &cached = cachedSteps[cachedIndex];
        if (cached.silhouetteBudget <= 0.0f ||
            std::fabs(cached.silhouetteBudget - step.silhouetteBudget) > 1e-9f ||
            std::fabs(cached.requestedRatio - step.requestedRatio) > 1e-9f) {
            continue;
        }
        const LodStep::Outcome &cachedOutcome = welded ? cached.welded : cached.structure;
        if (!cachedOutcome.built) {
            continue;//a step the cache also could not build says nothing we can reuse
        }
        outOutcome = cachedOutcome;
        return true;
    }
    return false;
}

void LodLadder::buildOneLadder(const std::vector<MeshGeometry> &meshes, LodGenerator::GeneratorKind kind,
                               size_t originalTriangles, const std::vector<LodStep> &cachedSteps,
                               bool measureAllowed, uint32_t &outMeasuredCount, bool &outDerivedAnyTarget) {
    const bool welded = kind == LodGenerator::GeneratorKind::SILHOUETTE_ONLY;
    size_t previousTriangleCount = originalTriangles;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        if (welded && !isWeldedStep(stepIndex)) {
            continue;//welding only pays at the coarse end, so the twin ladder is the same walk over fewer steps
        }
        LodStep &step = steps[stepIndex];
        LodStep::Outcome &outcome = welded ? step.welded : step.structure;
        if (outcome.built) {
            previousTriangleCount = outcome.triangleCount;
            continue;//already taken from the steps this ladder arrived with
        }
        if (outcome.skipReason != LodSkipReason::NONE) {
            continue;//measured and refused before, searching again only finds the same refusal on every load
        }
        bool derived = false;
        if (!findCachedOutcome(cachedSteps, step, welded, outcome)) {
            //nothing cached, so derive the budget from the share. The welded twin follows the budget this sets
            if (!welded && step.requestedRatio > 0.0f && measureAllowed) {
                prepareGenerators(meshes);
                derived = deriveTriangleTarget(meshes, step, originalTriangles, outcome);
                if (derived) {
                    outMeasuredCount++;
                    outDerivedAnyTarget = true;
                } else {
                    //the simplifier will not go that far on this model, so the share is dropped rather than kept
                    //as an intent nothing can satisfy. The budget it had is left alone
                    step.requestedRatio = 0.0f;
                    step.userSet = false;
                }
            }
        }
        if (!derived && !outcome.built && !findCachedOutcome(cachedSteps, step, welded, outcome)) {
            if (!measureAllowed) {
                prepareGenerators(meshes);
                fallbackOutcome(meshes, step, kind, originalTriangles, outcome);
            } else {
                prepareGenerators(meshes);
                if (!searchStep(meshes, step, kind, originalTriangles, outcome)) {
                    //no error met this budget, and a coarser step can only be worse
                    for (size_t coarser = stepIndex; coarser < steps.size(); ++coarser) {
                        LodStep::Outcome &rest = welded ? steps[coarser].welded : steps[coarser].structure;
                        rest.skipReason = LodSkipReason::NOTHING_FITS;
                    }
                    return;
                }
                outMeasuredCount++;
            }
        }
        if (acceptOutcome(step, previousTriangleCount, outcome)) {
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
    if (!storeIsBinary &&  !isLadderComplete() && canCalibrate && buildMode != BuildMode::FULL_RECALIBRATE) {
        geometryHash = LodMetadata::hashGeometry(meshes);
        LodMetadata::read(assetPath, flipAxes, settingsHash, geometryHash, cachedSteps);
    }

    bool measureAllowed = canCalibrate && (calibrationEnabled || buildMode != BuildMode::NORMAL);
    bool derivedAnyTarget = false;
    uint32_t measuredCount = 0;
    std::chrono::steady_clock::time_point buildStart = std::chrono::steady_clock::now();
    buildOneLadder(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING, originalTriangles, cachedSteps,
                   measureAllowed, measuredCount, derivedAnyTarget);
    //welded twins for the coarse steps, used by depth only cameras where no texture is ever sampled
    buildOneLadder(meshes, LodGenerator::GeneratorKind::SILHOUETTE_ONLY, originalTriangles, cachedSteps,
                   measureAllowed, measuredCount, derivedAnyTarget);

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
        if (isWeldedStep(stepIndex) && !steps[stepIndex].welded.built &&
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
    //only the asked for half is stored there, so it replaces the budgets without claiming any measurement
    std::vector<LodStep> merged;
    for (size_t stepIndex = 0; stepIndex < overriddenSteps.size(); ++stepIndex) {
        LodStep entry = overriddenSteps[stepIndex];
        //only when the budget still matches: an outcome measured under another budget describes another mesh,
        //which is exactly the case a binary whose targets moved since export has to rebuild
        if (stepIndex < steps.size() &&
            std::fabs(steps[stepIndex].silhouetteBudget - entry.silhouetteBudget) <= 1e-9f) {
            entry.structure = steps[stepIndex].structure;
            entry.welded = steps[stepIndex].welded;
        }
        merged.push_back(entry);
    }
    steps = merged;
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
    //an outcome belongs to the budget it was measured under. Leaving the old one attached is how a retarget
    //silently came back with the mesh it was supposed to replace
    steps[stepIndex].structure = LodStep::Outcome();
    steps[stepIndex].welded = LodStep::Outcome();
}

//bisect on triangle count, which needs no rendering, then score the winner once. That damage is the budget that
//finds the same mesh again later, for one measurement instead of the scored search a budget would need
bool LodLadder::deriveTriangleTarget(const std::vector<MeshGeometry> &meshes, LodStep &step,
                                     size_t originalTriangles, LodStep::Outcome &outOutcome) {
    if (step.requestedRatio <= 0.0f || step.requestedRatio >= 1.0f || originalTriangles == 0) {
        return false;
    }
    float wantedTriangles = std::max(1.0f, (float) originalTriangles * step.requestedRatio);
    Candidate best;
    float targetError = bisectTargetError(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING,
                                          SearchGoal::TRIANGLES_UNDER_COUNT, wantedTriangles, best);
    if (targetError <= 0.0f) {
        //hard edges and protected seams put a floor under a mesh. Take the coarsest there is and let the panel
        //report the share it actually reached
        generateCandidate(meshes, LodGenerator::GeneratorKind::STRUCTURE_PRESERVING, LOD_SEARCH_HIGH_ERROR, false, best);
        targetError = LOD_SEARCH_HIGH_ERROR;
    }
    if (best.triangleCount == 0) {
        std::cerr << "could not build a LOD step at " << step.requestedRatio * 100.0f << " percent for "
                  << assetPath << std::endl;
        return false;
    }
    measureCandidate(meshes, best);
    //zero measured damage floors rather than refuses: the search then lands on the coarsest mesh that shows nothing
    step.silhouetteBudget = std::max(best.silhouette, LOD_MIN_BUDGET);
    outOutcome.targetError = targetError;
    outOutcome.modelError = best.modelError;
    outOutcome.triangleCount = (uint32_t) best.triangleCount;
    outOutcome.silhouetteDamage = best.silhouette;
    outOutcome.achievedRatio = (float) best.triangleCount / (float) originalTriangles;
    return true;
}

void LodLadder::clearOverrides() {
    overridesPresent = false;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        steps[stepIndex].userSet = false;
        steps[stepIndex].requestedRatio = 0.0f;
    }
    unsavedChanges = true;
    //the project budgets have to come back, and only readSettings knows them
    steps.clear();
}

float LodLadder::pixelsPerModelUnit(const glm::mat4 &projectionMatrix, uint32_t targetHeight) {
    return projectionMatrix[1][1] * (float) targetHeight * 0.5f;
}

float LodLadder::switchDistanceOf(const LodStep::Outcome &outcome, const LodSelectionContext &context) const {
    if (context.allowance <= 0.0f || outcome.modelError <= 0.0f) {
        return 0.0f;
    }
    return (outcome.modelError * context.objectScale * context.pixelScale) / context.allowance;
}

uint32_t LodLadder::selectLevel(const LodSelectionContext &context, uint32_t previousLevel) const {
    if (meshLodCount <= 1) {
        return 0;
    }
    if (context.forceLevel >= 0) {
        return std::min((uint32_t) context.forceLevel, meshLodCount - 1);
    }
    //the two reaches the dead band allows, so the step loop compares instead of dividing. Moving up a step needs
    //to be clearly past the switch, moving back down clearly before it
    const float reachGoingCoarser = context.objectDistance * (1.0f - context.hysteresis);
    const float reachGoingFiner = context.objectDistance * (1.0f + context.hysteresis);
    uint32_t selected = 0;
    for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
        //welded outcomes drop UVs and normals, so only a depth camera may draw one. Where a step has no twin the
        //depth camera draws the structure outcome, under the same allowance
        const LodStep &step = steps[stepIndex];
        const LodStep::Outcome &outcome = (context.isShadowCamera && step.welded.built) ? step.welded : step.structure;
        if (!outcome.built) {
            continue;
        }
        float switchDistance = switchDistanceOf(outcome, context);
        if (switchDistance <= 0.0f) {
            continue;
        }
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
                          const glm::vec3 &center, float scale, uint32_t resolution, RasterTarget &target) {
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
            float trianglePositionPerUv = 0.0f;
            if (hasTextureCoordinates) {
                const std::vector<glm::vec2> &textureCoordinates = *mesh.textureCoordinates;
                cornerUv[0] = textureCoordinates[corner0];
                cornerUv[1] = textureCoordinates[corner1];
                cornerUv[2] = textureCoordinates[corner2];
                float positionLength = glm::length(vertices[corner1] - vertices[corner0]) +
                                       glm::length(vertices[corner2] - vertices[corner1]) +
                                       glm::length(vertices[corner0] - vertices[corner2]);
                float uvLength = glm::length(cornerUv[1] - cornerUv[0]) + glm::length(cornerUv[2] - cornerUv[1]) +
                                 glm::length(cornerUv[0] - cornerUv[2]);
                //a palette triangle maps to a single texel, a uv change there is a different colour, not a distance
                trianglePositionPerUv = uvLength > 0.0f ? positionLength / uvLength : 0.0f;
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
                        target.positionPerUv[pixel] = trianglePositionPerUv;
                    }
                }
            }
        }
    }
}

//scores what the candidate would show against the original, from fourteen directions. Nothing here takes an on
//screen size, damage is the same fraction at every distance.
//Several loader threads run this at once, so it must hold no state between calls
void LodLadder::measureCandidate(const std::vector<MeshGeometry> &meshes, Candidate &candidate) const {
    candidate.silhouette = 0.0f;
    candidate.modelError = 0.0f;
    if (meshes.empty()) {
        return;
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
        return;//no vertices at all
    }
    glm::vec3 center = (low + high) * 0.5f;
    glm::vec3 size = high - low;
    float extent = std::max(size.x, std::max(size.y, size.z));
    if (extent <= 0.0f) {
        return;//a single point, nothing to compare
    }

    uint32_t resolution = std::max(LOD_SCORER_MIN_RESOLUTION, scorerResolution);
    //a corner view sees the box across its diagonal, so the fit leaves room for it instead of clipping
    float scale = (resolution * 0.5f) / (extent * 0.9f);

    std::vector<View> views;
    buildViews(views);

    RasterTarget reference;
    RasterTarget simplified;
    //pixels here, turned into model units with the same scale once every view has been walked
    std::vector<float> deviations;
    uint64_t mismatched = 0;
    uint64_t covered = 0;
    uint64_t boundary = 0;
    uint64_t holes = 0;
    uint64_t wrongAttributes = 0;
    for (size_t viewIndex = 0; viewIndex < views.size(); ++viewIndex) {
        reference.reset(resolution);
        simplified.reset(resolution);
        rasterize(meshes, nullptr, views[viewIndex], center, scale, resolution, reference);
        rasterize(meshes, &candidate, views[viewIndex], center, scale, resolution, simplified);
        const std::vector<float> &referenceDepth = reference.depth;
        const std::vector<float> &candidateDepth = simplified.depth;
        for (size_t pixel = 0; pixel < referenceDepth.size(); ++pixel) {
            bool referenceCovered = referenceDepth[pixel] != FLT_MAX;
            bool candidateCovered = candidateDepth[pixel] != FLT_MAX;
            if (referenceCovered) {
                covered++;
                //an edge of the original outline, which is what the mismatched band is spread along
                uint32_t x = (uint32_t) (pixel % resolution);
                uint32_t y = (uint32_t) (pixel / resolution);
                bool onEdge = x == 0 || y == 0 || x + 1 == resolution || y + 1 == resolution ||
                              referenceDepth[pixel - 1] == FLT_MAX || referenceDepth[pixel + 1] == FLT_MAX ||
                              referenceDepth[pixel - resolution] == FLT_MAX ||
                              referenceDepth[pixel + resolution] == FLT_MAX;
                if (onEdge) {
                    boundary++;
                }
            }
            if (referenceCovered != candidateCovered) {
                mismatched++;//the outline moved, in or out
            } else if (referenceCovered) {
                float deviation = std::fabs(referenceDepth[pixel] - candidateDepth[pixel]);
                if (deviation > LOD_SCORER_SURFACE_JUMP_PIXELS) {
                    //not the same surface displaced, something behind it showing through. Its depth is the
                    //distance to whatever happens to be back there, so it would only poison the distribution
                    holes++;
                } else {
                    //the same surface, but a collapse along a seam can still show another part of the texture or
                    //shade it differently without moving it. A window sliding off a flat facade is only seen here
                    uint8_t sharedAttributes = candidate.silhouetteOnly ? 0 ://welded keeps arbitrary copies, depth only draws it
                                               reference.attributeMask[pixel] & simplified.attributeMask[pixel];
                    bool attributeWrong = false;
                    if (sharedAttributes & RASTER_HAS_UV) {
                        glm::vec2 uvDifference = reference.textureCoordinates[pixel] - simplified.textureCoordinates[pixel];
                        //per axis, a texel is a square
                        if (std::max(std::fabs(uvDifference.x), std::fabs(uvDifference.y)) > uvDeviation) {
                            attributeWrong = true;
                        }
                        //texture sliding across a surface moves what is seen as much as the surface moving would
                        deviation = std::max(deviation, glm::length(uvDifference) * reference.positionPerUv[pixel] * scale);
                    }
                    if (sharedAttributes & RASTER_HAS_NORMAL) {
                        glm::vec3 referenceNormal = reference.normals[pixel];
                        glm::vec3 candidateNormal = simplified.normals[pixel];
                        float referenceLength = glm::length(referenceNormal);
                        float candidateLength = glm::length(candidateNormal);
                        //interpolated normals shrink between corners and the shader normalizes them, so we do too
                        if (referenceLength > 0.0f && candidateLength > 0.0f &&
                            glm::length(referenceNormal / referenceLength - candidateNormal / candidateLength) > normalDeviation) {
                            attributeWrong = true;
                        }
                    }
                    if (attributeWrong) {
                        wrongAttributes++;
                    }
                    deviations.push_back(deviation);
                }
            }
        }
    }
    if (covered == 0) {
        //nothing of the original was visible, so an empty candidate is not an improvement
        candidate.silhouette = mismatched > 0 ? 1.0f : 0.0f;
        return;
    }
    //holes and wrong attributes count with the outline: all are places where the wrong thing is on screen, and
    //all are what the budget has to bound
    candidate.silhouette = (float) ((double) (mismatched + holes + wrongAttributes) / (double) covered);

    //the mismatched pixels form a band along the outline, so its width is the band over the outline length.
    //Counting the outline instead of guessing it from the bounding box keeps every model shape honest
    float outlineDisplacement = 0.0f;
    if (boundary > 0) {
        outlineDisplacement = (float) ((double) mismatched / (double) boundary) / scale;
    }
    float surfaceDeviation = 0.0f;
    if (!deviations.empty()) {
        //placed against the samples, not against every covered pixel: a pixel whose coverage changed is already
        //counted as outline damage, and counting it here too would make one number move twice
        std::sort(deviations.begin(), deviations.end());
        size_t index = (size_t) (LOD_PLACEMENT_QUANTILE * (double) (deviations.size() - 1));
        surfaceDeviation = deviations[index] / scale;
    }
    //the worse of the two things that actually moved, both measured, both in model units. meshopt's own error
    //places nothing: on the western buildings it reads a hundred times high
    candidate.modelError = std::max(outlineDisplacement, surfaceDeviation);
}
