---
layout: post
title: "How to use the Pipeline Editor"
description: "How to use the Pipeline Editor"
date: "2026-03-24 15:46:00 +0100"
author: enginmanap
version: 0.7.0
categories: [technical]
github_comments_issueid: 157
---

When the work to have a configurable render pipeline started, it had 2 goals.
1. More flexibility for rendering, by allowing different objects to be rendered by their own rules
2. Better optimization through automatic calculations/heuristics

In this blog post I will try to explain both.


## Table of Contents

- [Part 1: High-Level Rendering Pipeline - How to customize rendering](#part-1-high-level-rendering-pipeline---how-to-customize-rendering)
  - [The Visual Pipeline Editor](#the-visual-pipeline-editor)
    - [Setting Up Render Stages](#setting-up-render-stages)
    - [Adding full screen post-processing effects](#adding-full-screen-post-processing-effects)
    - [3D World Rendering, and Camera-Object Tag Matching](#3d-world-rendering-and-camera-object-tag-matching)
    - [Practical Examples](#practical-examples)
      - [Example 1: Basic Scene Setup](#example-1-basic-scene-setup)
      - [Example 2: Selective Rendering](#example-2-selective-rendering)
      - [Example 3: Different Shaders for Opaque and Transparent objects](#example-3-different-shaders-for-opaque-and-transparent-objects)
    - [Runtime behaviour](#runtime-behaviour)
    - [Understanding the Filtering Pipeline](#understanding-the-filtering-pipeline)
    - [Light-Based Filtering System](#light-based-filtering-system)
    - [Configuration Options for Filtering](#configuration-options-for-filtering)
    - [Recommended Settings for Different Scenarios](#recommended-settings-for-different-scenarios)
    - [Performance Considerations](#performance-considerations)
- [Part 2: Technical Deep-Dive - Implementation Details](#part-2-technical-deep-dive---implementation-details)
  - [Architecture Overview](#architecture-overview)
  - [The Tag-Based Rendering Pipeline](#the-tag-based-rendering-pipeline)
    - [Camera Identification and Object Type Tags](#1-camera-identification-and-object-type-tags)
    - [Visibility Request Structure](#2-visibility-request-structure)
    - [The fillVisibleObjectPerCamera Method](#3-the-fillvisibleobjectpercamera-method)
    - [Level of Detail (LOD) Filtering](#4-level-of-detail-lod-filtering)
    - [Mesh-Level Processing](#5-mesh-level-processing)
    - [RenderList](#6-renderlist)
    - [The renderCameraByTag Method](#7-the-rendercamerabytag-method)
  - [Configuration Options and Parameters](#configuration-options-and-parameters)
  - [Performance Optimizations](#performance-optimizations)
  - [Data Structures](#data-structures)
    - [VisibilityRequest Class](#visibilityrequest-class)
    - [RenderList Class](#renderlist-class)
    - [Tag Matching Implementation](#tag-matching-implementation)
  - [Integration with Node Editor System](#integration-with-node-editor-system)


# Part 1: High-Level Rendering Pipeline - How to customize rendering

## The Visual Pipeline Editor

### Setting Up Render Stages

The Limon Engine uses a visual node-based editor to configure rendering pipelines, explained in a previous blog post. As a short reminder, this GUI automatically scans available shaders and exposes them as nodes, with their inputs and outputs ready for connecting to render pipeline as the user sees fit. After the node setup is done, it can be activated in real-time and see the results.

Each node also represents a rendering stage (which might merge with others as an optimization in real-time). We have many render methods that might be active on a stage, which determines what rendering calls would be made in the given stage. 

### Adding full screen post-processing effects

Most likely use case for the pipeline editor is adding post-processing effects, like color grading based on user taking damage etc. Because of this, Limon comes with a method for full screen, screen space pass method. If user wants to run their shader as a post-processing step, adding the node in appropriate step, connecting the inputs and outputs, then choosing `Render quad` method is enough to have the effect.

### 3D World Rendering, and Camera-Object Tag Matching

Rendering specific game objects as part of the pipeline, from a specific camera is a more involved endeavor, but it is not much complicated, once the pipeline is understood.
For each node, it is possible to set both camera and object tags. At the heart of the system is the tag matching mechanism:

1. **Camera Tags**: Each camera has tags that identify itself. Player camera itself is tagged with "player_camera", while light cameras have their own tags (e.g., "point_camera", "directional_camera", "debug")
2. **Object Tags**: Every object in the scene has tags that define what type of object it is. These tags can be set in the editor, "Object Details" pane. (e.g., "static", "transparent", "animated")
3. **Render Stages**: Pipeline stages define which cameras should be used to render this stage, and which objects should be rendered based on tags.
4. **Filtering Pipeline**: Each stage applies multiple filtering layers - tag matching, frustum/Occlusion/Distance culling, or skip based on objects size on screen
5. **LOD selection**: For objects that pass the filters, an LOD is selected based on the size on screen.

### Practical Examples

#### Example 1: Basic Scene Setup
- **Player Camera** (PerspectiveCamera, tagged as "player_camera") renders objects with tags "static_model_object", "physical_model_object", "transparent_model_object"
- **Debug Camera** (PerspectiveCamera, tagged as "player_camera") renders objects tagged with "debug_object"
- **Shadow Cascade Camera** (OrthographicCamera, tagged as "directional_camera") only renders objects tagged as "shadow-caster"
- **Point Light Camera** (CubeCamera, tagged as "point_camera") renders objects within light radius

#### Example 2: Selective Rendering
To make an object invisible to a specific camera:

1. Select the object in the editor
2. Remove or modify its tags in the property panel
3. The object will no longer match the render stage criteria for that camera
4. The pipeline automatically excludes it from that camera's render list

#### Example 3: Different Shaders for Opaque and Transparent objects
1. Create a new node with shader for Transparent objects
2. Set the camera tag to "player_camera"
3. Set the object tags to "transparent_model_object"
4. Set its input and output accordingly. For outputs, select the texture to use.
5. Set the stage properties, like depth read/write. Make sure blending is enabled, back face culling is disabled
6. Create another node with the shader for opaque objects
7. Set the camera tag to "player_camera"
8. Set the object tags to "basic_model_object"
9. Set its input and output accordingly. For outputs, select the texture to use. 
10. Set the stage properties, like depth read/write. Make sure blending is disabled
11. When you save the pipeline, the interface will show the suggested render order. It will have transparent rendering after opaque as an optimization.

### Runtime behaviour

1. **Load Pipeline**: The engine loads `renderPipeline.xml` which contains your node graph configuration
2. **Camera Setup**: Each camera registers its identification tags during initialization
3. **Object Registration**: Objects register their type tags when added to the scene
4. **Multi-Layer Filtering**: The engine applies tag matching, frustum culling, and camera-specific occlusion rules
5. **Render Execution**: Each render stage executes with its pre-filtered object list

### Understanding the Filtering Pipeline

The rendering system uses a sophisticated multi-layer filtering approach:

#### Layer 1: Tag-Based Filtering
- Matches camera identification tags with render stage requirements
- Filters objects based on their type tags against stage criteria
- This is the first and fastest filtering layer, as the tags are converted to hashes, and reused 

#### Layer 2: Frustum Culling
- Removes objects outside the camera's view frustum
- Applied after tag filtering to reduce processing overhead
- Works for all camera types

#### Layer 3: Camera-Specific Occlusion
- **PerspectiveCamera**: Full software occlusion culling with depth testing
- **OrthographicCamera**: No extra culling (Used for directional/Sun shadows)
- **CubeCamera**: Distance-based culling (used for point light shadows)

#### Layer 4: Level of Detail (LOD) Filtering
- **Distance-based LOD**: Objects are assigned different detail levels based on distance from player
- **Screen Size and Distance Filtering**: For objects that are small in world space, but also small on the screen (so distant from camera) can be skipped, based on settings

### Light-Based Filtering System

Limon also automatically filters or alters the lights before rendering starts:

#### Point Light Selection
Point lights are automatically filtered based on their distance from the player and their effective radius:

- **Distance Culling**: Only point lights within the player's view frustum and active radius are considered
- **Priority System**: If there are too many point lights in the scene, only the closest ones to the player are used

#### Directional Light Shadow Cascades
Directional lights create shadow maps that follow the player's view:

- **Player-Following Cascades**: Shadow cascades are automatically recalculated based on the player's current position and viewing direction
- **Adaptive Coverage**: Each cascade covers a specific distance range from the player, ensuring detailed shadows where needed
- **Dynamic Updates**: As the player moves through the world, the shadow cascades smoothly follow to maintain consistent quality

#### Light Camera System
Each light type creates its own specialized cameras for shadow rendering:

- **Point Light Shadows**: Use a special 6-faced camera system to capture shadows in all directions
- **Directional Light Shadows**: Use multiple orthographic cameras for cascaded shadow mapping
- **Pipeline Integration**: These shadow cameras participate in the same tag-based rendering pipeline as regular cameras

### Configuration Options for Filtering

The system provides several configuration options to control filtering behavior:

#### LOD and Distance Options
- **LodDistanceList**: Array of distances for LOD level transitions (e.g., [10, 25, 50, 100])
- **SkipRenderDistance**: Minimum distance for considering to skip rendering small objects. Objects closer would not be skipped, no matter how small.
- **SkipRenderSize**: Maximum screen size (0-1 range) below which objects are skipped
- **MaxSkipRenderSize**: Maximum object size in world units that can be skipped. Used to prevent skipping Big objects like Mountains, that were too far away.

#### Occlusion Options
- **SoftwareOcclusionOccluderSize**: Minimum screen size (0-1 range) for objects to be treated as occluders. These objects' silhouettes are rendered in CPU for occlusion checks. As the number gets smaller, more objects are software rendered, and CPU usage increases. 
- **SoftwareOcclusionRenderDump**: Enable/disable occlusion depth buffer debugging logs and dumps software depth buffer to file.
- **SoftwareOcclusionRenderFrequency**: Frame interval for software occlusion depth buffer dumps. Only useful for debugging occlusion purposes.

#### Performance Options
- **SplitModelToMeshCount**: Threshold for per-mesh frustum culling (models with more meshes get individual culling) This is useful if importing big chunks of game map as single model, which is not recommended.
- **multiThreadedCulling**: Enable/disable multi-threaded visibility processing. Disabling is only useful for debugging purposes.

### Recommended Settings for Different Scenarios

#### High-Performance Gaming

LodDistanceList: [25, 50, 100, 200]
SkipRenderDistance: 150
SkipRenderSize: 0.01
MaxSkipRenderSize: 5.0
SoftwareOcclusionOccluderSize: 0.1
SplitModelToMeshCount: 10

#### High-Quality Rendering

LodDistanceList: [50, 100, 200, 400]
SkipRenderDistance: 500
SkipRenderSize: 0.005
MaxSkipRenderSize: 2.0
SoftwareOcclusionOccluderSize: 0.05
SplitModelToMeshCount: 5


#### Debug/Development

LodDistanceList: [100, 200, 400]
SkipRenderDistance: 0
SkipRenderSize: 0
MaxSkipRenderSize: 0
SoftwareOcclusionOccluderSize: 0.25
SplitModelToMeshCount: 1
SoftwareOcclusionRenderDump: True
multiThreadedCulling: False
SoftwareOcclusionRenderFrequency: 300

### Performance Considerations

The visual editor automatically optimizes your pipeline:

- **Dirty State Tracking**: Only processes objects or cameras that have changed
- **Multi-threading**: Visibility processing runs in parallel for multiple cameras
- **LOD Integration**: Automatically applies level-of-detail based on distance
- **Camera-Specific Optimization**: Different occlusion rules for different camera types

---

# Part 2: Technical Deep-Dive - Implementation Details

## Architecture Overview

The rendering system is built around several key components that work together to create efficient render lists:

- **RenderList**: A container that organizes meshes by material and depth for optimal rendering
- **VisibilityRequest**: Encapsulates all data needed for visibility determination per camera
- **Tag System**: Uses hashed strings for efficient tag matching between cameras and objects
- **Multi-threaded Culling**: Parallel processing of visibility for multiple cameras

## The Tag-Based Rendering Pipeline

### 1. Camera Identification and Object Type Tags

The system uses two distinct types of tags:

**Camera Identification Tags**: These identify what type of camera it is:
```cpp
// Camera setup with identification tags
playerCamera->addTag(HardCodedTags::CAMERA_PLAYER);
directional_light_camera->addTag(HardCodedTags::CAMERA_LIGHT_DIRECTIONAL);
point_light_camera_->addTag(HardCodedTags::CAMERA_LIGHT_POINT);
```

**Object Type Tags**: These define what type of object it is:
```cpp
// Object setup with type tags
staticObject->addTag(HardCodedTags::OBJECT_MODEL_STATIC);
animatedObject->addTag(HardCodedTags::OBJECT_MODEL_ANIMATED);
transparentObject->addTag(HardCodedTags::OBJECT_MODEL_TRANSPARENT);
```

**Render Stage Configuration**: Each pipeline stage defines which camera identification tags should render which object type tags. As the render pipeline is loaded, it creates mappings between cameras and object tags as an optimization.

### 2. Visibility Request Structure

For each camera, the engine creates a `VisibilityRequest` that contains:

- **Camera reference**: The camera being processed
- **Object collection**: All objects in the world
- **Visibility map**: Maps tag combinations to RenderLists
- **Culling options**: LOD distances, occlusion settings, etc.
- **Player position**: Used for LOD calculations

```cpp
std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>*> cullingResults;
```

### 3. The fillVisibleObjectPerCamera Method

This is the core method that builds render lists. Let's break down its operation:

#### Initialization Phase

The method first extracts configuration options and sets up camera-specific parameters:

```cpp
void fillVisibleObjectPerCamera(const void* visibilityRequestRaw) {
    const VisibilityRequest* visibilityRequest = static_cast<const VisibilityRequest *>(visibilityRequestRaw);
    std::vector<long> lodDistances = visibilityRequest->lodDistancesOption.get();
    
    // Setup view matrices and culling parameters
    if(visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE ||
       visibilityRequest->camera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
        viewMatrix = visibilityRequest->camera->getProjectionMatrix() * 
                    visibilityRequest->camera->getCameraMatrixConst();
    }
}
```

#### Occlusion Culling Setup

Different camera types have different occlusion behaviors:

```cpp
bool skipOcclusionCulling = false;
if (visibilityRequest->camera->getType() != Camera::CameraTypes::PERSPECTIVE) {
    // OrthographicCamera and CubeCamera skip complex software occlusion
    skipOcclusionCulling = true;
} else {
    // PerspectiveCamera gets full software occlusion culling
    glm::mat4 invertedView = glm::inverse(visibilityRequest->camera->getCameraMatrixConst());
    viewDirection = -glm::vec3(invertedView[2]);
    cameraPos = glm::vec3(invertedView[3]);
    
    visibilityRequest->occlusionCuller.newFrame(cameraPos, viewDirection, 
                                              visibilityRequest->camera->getCameraMatrixConst(), 
                                              visibilityRequest->camera->getProjectionMatrix());
}
```

#### Object Processing Loop

The method iterates through all objects in the world, performing several filtering steps:

1. **Dirty State Optimization**: Skip processing if neither camera nor object has changed
2. **Tag Filtering**: Check if object type tags match the render stage's object tag requirements
3. **Frustum Culling**: Determine if object is within camera's view frustum
4. **LOD Selection**: Choose appropriate level of detail based on distance
5. **Camera-Specific Occlusion**: Apply occlusion culling based on camera type

```cpp
for (auto objectIt = visibilityRequest->objects->begin(); 
     objectIt != visibilityRequest->objects->end(); ++objectIt) {
    
    // Skip if nothing changed
    if(!visibilityRequest->camera->isDirty() && 
       !objectIt->second->isDirtyForFrustum() && 
       skipOcclusionCulling) {
        continue;
    }
    
    Model *currentModel = dynamic_cast<Model *>(objectIt->second);
    
    // Check if object tags match any render stage criteria for this camera
    for (auto& visibilityEntry: *visibilityRequest->visibility) {
        if (VisibilityRequest::isAnyTagMatch(visibilityEntry.first, currentModel->getTags())) {
            bool isVisible = visibilityRequest->camera->isVisible(*currentModel);
            
            if(isVisible) {
                // Add to render list with camera-specific occlusion handling
                processVisibleObject(currentModel, visibilityEntry);
            } else {
                // Remove from render list
                processInvisibleObject(currentModel, visibilityEntry);
            }
        }
    }
}
```

### 4. Level of Detail (LOD) Filtering

The `getLodLevel` method calculates rough screen space size of the object using AABB, and uses this to determine an LOD level, or skip rendering. Then returns the estimated screen space size for Occlusion culling use. It also provides estimated average depth for ordering the render.

```cpp
static uint32_t getLodLevel(const std::vector<long>& lodDistances, 
                           float skipRenderDistance, 
                           float skipRenderSize, 
                           float maxSkipRenderSize, 
                           const glm::mat4 &viewMatrix, 
                           const glm::vec3& playerPosition, 
                           glm::vec3 minAABB, 
                           glm::vec3 maxAABB, 
                           float &objectAverageDepth, 
                           float &objectScreenSize) {
    
    // Calculate object's screen size in normalized device coordinates
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(minAABB, maxAABB, viewMatrix, ndcMin, ndcMax);
    const float screenSizeX = (ndcMax.x - ndcMin.x) / 2.0f;
    const float screenSizeY = (ndcMax.y - ndcMin.y) / 2.0f;
    objectScreenSize = (screenSizeX * screenSizeY);
    
    // Calculate average depth for sorting
    objectAverageDepth = (ndcMax.z + ndcMin.z) / -2.0f;
    
    // Calculate distance from player
    const float dx = std::max(minAABB.x - playerPosition.x, std::max(0.0f, playerPosition.x - maxAABB.x));
    const float dy = std::max(minAABB.y - playerPosition.y, std::max(0.0f, playerPosition.y - maxAABB.y));
    const float dz = std::max(minAABB.z - playerPosition.z, std::max(0.0f, playerPosition.z - maxAABB.z));
    const float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Complete skip rendering if object meets all criteria:
    // 1. Beyond skip distance AND small enough in world units
    // 2. Small enough on screen
    if(skipRenderDistance != 0 && distance > skipRenderDistance) {
        if ((maxAABB.x - minAABB.x) < maxSkipRenderSize &&
            (maxAABB.y - minAABB.y) < maxSkipRenderSize) {
            if(screenSizeX < skipRenderSize && screenSizeY < skipRenderSize) {
                return SKIP_LOD_LEVEL; // Completely skip this object
            }
        }
    }
    
    // Return appropriate LOD level based on distance
    for (size_t i = 0; i < lodDistances.size(); ++i) {
        if(distance < static_cast<float>(lodDistances[i])) {
            return i;
        }
    }
    return lodDistances.size()-1; // Farthest LOD
}
```

### 5. Mesh-Level Processing

For visible models, all meshes of the model would be processed. 

If software occlusion culling is enabled for the camera, the behavior changes.  If the object is selected as Occluder, it is rendered to the software depth map, otherwise it is put in a list to check for occlusion. Only after all objects are scanned and all Occluders are rendered, then the rest is queried against the depth buffer and added to the render list.

If software occlusion culling is not enabled, all meshes of the model will be put in to the render list.

```cpp
const std::vector<Model::MeshMeta *> &meshMetas = currentModel->getMeshMetaData();

for (auto& meshMeta:meshMetas) {
    uint32_t lod = World::getLodLevel(lodDistances, skipRenderDistance, 
                                      skipRenderSize, maxSkipRenderSize, 
                                      viewMatrix, visibilityRequest->playerPosition, 
                                      meshMeta->mesh->getAabbMin(), 
                                      meshMeta->mesh->getAabbMax(), 
                                      objectAverageDepth, objectScreenSize);
    
    if (lod != SKIP_LOD_LEVEL) {
        if (objectScreenSize > softwareOcclusionOccluderSize || skipOcclusionCulling) {
            // Object is large enough to be an occluder OR camera skips occlusion
            if (!skipOcclusionCulling) {
                visibilityRequest->occlusionCuller.renderOccluder(meshMeta, 
                                                                currentModel->getTransformation()->getWorldTransform());
            }
            visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, 
                                                  currentModel, lod, objectAverageDepth);
        } else {
            // Object is small - treat as potential occludee (only for perspective cameras)
            if (!skipOcclusionCulling) {
                visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, 
                                                              lod, objectAverageDepth, 
                                                              &visibilityEntry.second);
            } else {
                // For non-perspective cameras, add directly without occlusion testing
                visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, 
                                                      currentModel, lod, objectAverageDepth);
            }
        }
    }
}
```

### 6. RenderList 

RenderList itself has internal ordering and grouping. Objects are grouped by their asset for instanced rendering. Then grouped by materials to minimize material changes. Then sorted by their average depth to minimize overdraw.

### 7. The renderCameraByTag Method

When it's time to render, the `renderCameraByTag` method uses the pre-built render lists. Note that this method receives the camera identification tag, not render tags:

```cpp
void World::renderCameraByTag(const std::shared_ptr<GraphicsProgram> &renderProgram, 
                             const std::string &cameraName, 
                             const std::vector<HashUtil::HashedString> &tags) const {
    uint64_t hashedCameraTag = HashUtil::hashString(cameraName); // Camera identification tag
    
    for (const auto &visibilityEntry: cullingResults) {
        if (visibilityEntry.first->hasTag(hashedCameraTag)) { // Match camera by identification tag
            std::unordered_map<std::vector<uint64_t>, RenderList, 
                              VisibilityRequest::uint64_vector_hasher>& renderLists = *visibilityEntry.second;
            
            // Render player attachments first
            if (!currentPlayer->isDead() && startingPlayer.attachedModel != nullptr) {
                std::vector<uint32_t> alreadyRenderedModelIds;
                for (const auto &renderTag: tags) {
                    renderPlayerAttachmentsRecursiveByTag(startingPlayer.attachedModel, 
                                                          renderTag.hash, renderProgram, 
                                                          alreadyRenderedModelIds);
                }
            }
            
            // Render each matching render list (filtered by object type tags)
            for (auto& renderListEntry: renderLists) {
                if (!VisibilityRequest::vectorComparator(renderListEntry.first, tags)) {
                    continue;
                }
                const RenderList& renderList = renderListEntry.second;
                renderList.render(graphicsWrapper, renderProgram);
            }
        }
    }
}
```

## Configuration Options and Parameters

The filtering system is controlled by several configuration options that are loaded from the engine's options system:

### Core Filtering Options

#### LOD and Distance Parameters
- **LodDistanceList** (`LodDistanceList`): Array of distances for LOD level transitions
  - Example: `4, 10, 25, 50, 100` means LOD 0 at <10 units, LOD 1 at 10-25, etc.
  - More values = more LOD levels, better performance but more memory usage

- **SkipRenderDistance** (`SkipRenderDistance`): Maximum distance before considering to skip small objects
  - Set to 0 to disable distance-based skipping
  - Typical values: 100-500 units depending on scene scale

- **SkipRenderSize** (`SkipRenderSize`): Minimum screen size (0-1 range) below which objects are skipped
  - 0.01 = 1% of screen, 0.001 = 0.1% of screen
  - Smaller values = more objects rendered, higher quality

- **MaxSkipRenderSize** (`MaxSkipRenderSize`): Maximum object size in world units that can be skipped
  - Prevents skipping large objects like buildings even if far away
  - Typical values: 2.0-10.0 units

#### Occlusion Parameters
- **SoftwareOcclusionOccluderSize** (`SoftwareOcclusionOccluderSize`): Minimum screen size for occluders
  - Objects bigger than this will not be Occlusion culled, but will be used to cull other objects 
  - Default: 0.25 (25% of screen)
  - Smaller values improves occlusion culling precision but also increases CPU usage.

- **SoftwareOcclusionRenderDump** (`SoftwareOcclusionRenderDump`): Enable occlusion depth buffer to be written to file. For debugging only.
  - Set to `true` to save depth buffer images for debugging
  - Performance impact: use only for debugging occlusion issues, it causes visible stutter.

- **SoftwareOcclusionRenderFrequency** (`SoftwareOcclusionRenderDumpFrequency`): Frame interval for debug dumps
  - Default: 500 (every 500 game ticks ~8 seconds with default ticks of 60hz)
  - Lower values rarely provide useful information, use with caution

#### Performance Parameters
- **SplitModelToMeshCount** (`SplitModelToMeshCount`): Threshold for per-mesh frustum culling and Occluder selection
  - Models with more meshes than this value processed as individual meshes for both Frustum and Occlusion culling purposes
  - Default: 10 meshes
  - Lower values = better culling accuracy, higher CPU cost.
  - Rarely useful, except importing big chunks of game world as is. Not recommended.

- **SoftwareOcclusionRenderWidth/Height**: Occlusion buffer resolution
  - Higher resolution = more accurate occlusion, Increases memory usage slightly, but CPU cost increases fast.
  - Both options has to be multiples of 8, because of SIMD usage
  - Recommended resolution is 1024x256

### Option Loading in Code

These options are automatically loaded in the VisibilityRequest constructor:

```cpp
VisibilityRequest(Camera* camera, 
               std::unordered_map<uint32_t, PhysicalRenderable *>* objects, 
               std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher> * visibility, 
               const glm::vec3& playerPosition, 
               const OptionsUtil::Options* options) :
    camera(camera), 
    playerPosition(playerPosition), 
    options(options),
    
    // Load all configuration options
    lodDistancesOption(options->getOption<std::vector<long>>(HASH("LodDistanceList"))),
    skipRenderDistanceOption(options->getOption<double>(HASH("SkipRenderDistance"))),
    skipRenderSizeOption(options->getOption<double>(HASH("SkipRenderSize"))),
    maxSkipRenderSizeOption(options->getOption<double>(HASH("MaxSkipRenderSize"))),
    SplitModelToMeshCountOption(options->getOption<long>(HASH("SplitModelToMeshCount"))),
    SoftwareOcclusionOccluderSizeOption(options->getOption<double>(HASH("SoftwareOcclusionOccluderSize"))),
    
    objects(objects), 
    visibility(visibility),
    occlusionCuller(options->getOption<long>(HASH("SoftwareOcclusionRenderWidth")),
                  options->getOption<long>(HASH("SoftwareOcclusionRenderHeight"))) {
}
```

## Performance Optimizations

### 1. Multi-threaded Processing

The engine uses a thread pool to process visibility for multiple cameras in parallel:

```cpp
std::map<VisibilityRequest*, SDL_Thread *> World::occlusionThreadManager() {
    std::map<VisibilityRequest*, SDL_Thread*> visibilityProcessing;
    for (auto &cameraVisibility: cullingResults) {
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, 
                                                         &this->objects, 
                                                         cameraVisibility.second, 
                                                         currentPlayer->getPosition(), 
                                                         options);
        SDL_Thread* thread = SDL_CreateThread(staticOcclusionThread, 
                                             request->camera->getName().c_str(), 
                                             request);
        visibilityProcessing[request] = thread;
    }
    return visibilityProcessing;
}
```

### 2. Dirty State Tracking

Objects and cameras track their "dirty" state to avoid unnecessary recalculation:

```cpp
if(!visibilityRequest->camera->isDirty() && 
   !objectIt->second->isDirtyForFrustum() && 
   skipOcclusionCulling) {
    continue; // Skip processing if nothing changed
}
```

### 3. Level of Detail (LOD) System

The engine automatically selects appropriate LOD levels based on distance and screen size:

```cpp
uint32_t lod = World::getLodLevel(lodDistances, skipRenderDistance, 
                                 skipRenderSize, maxSkipRenderSize, 
                                 viewMatrix, visibilityRequest->playerPosition, 
                                 objectIt->second->getAabbMin(), 
                                 objectIt->second->getAabbMax(), 
                                 objectAverageDepth, objectScreenSize);
```

### 4. Material-Based Sorting

RenderLists organize meshes by material to minimize GPU state changes:

```cpp
class RenderList {
    std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation> perMaterialMeshMap;
    std::multimap<float, std::shared_ptr<const Material>> materialRenderPriorityMap;
};
```

## Data Structures

### VisibilityRequest Class

The `VisibilityRequest` class (defined in `src/VisibilityRequest.h`) encapsulates all data needed for visibility processing:

```cpp
class VisibilityRequest {
    const Camera* const camera;
    glm::vec3 playerPosition;
    const OptionsUtil::Options* options;
    const std::unordered_map<uint32_t, PhysicalRenderable *>* const objects;
    std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher>* visibility;
    mutable OcclusionCullerHelper occlusionCuller;
    mutable std::unordered_map<uint32_t, const std::vector<glm::mat4>*> changedBoneTransforms;
    
    // Configuration options
    const OptionsUtil::Options::Option<std::vector<long>> lodDistancesOption;
    const OptionsUtil::Options::Option<double> skipRenderDistanceOption;
    const OptionsUtil::Options::Option<double> skipRenderSizeOption;
    // ... more options
};
```

### RenderList Class

The `RenderList` class (defined in `src/Occlusion/RenderList.h`) organizes render data efficiently:

```cpp
class RenderList {
    struct PerMaterialRenderInformation {
        std::unordered_map<std::shared_ptr<MeshAsset>, PerMeshRenderInformation> meshesToRender;
        std::unordered_map<std::shared_ptr<MeshAsset>, float> maxDepthPerMesh;
        mutable std::multimap<float, std::shared_ptr<MeshAsset>> meshRenderPriorityMap;
    };
    
    std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation> perMaterialMeshMap;
    std::unordered_map<std::shared_ptr<const Material>, float> maxDepthPerMaterial;
    mutable std::multimap<float, std::shared_ptr<const Material>> materialRenderPriorityMap;
};
```

### Tag Matching Implementation

The tag matching system uses efficient hash-based comparison:

```cpp
static bool isAnyTagMatch(const std::vector<HashUtil::HashedString>& renderTags, 
                         const std::list<HashUtil::HashedString> & objectTags) {
    for (const auto& renderTag:renderTags) {
        for (const auto& objectTag:objectTags) {
            if (renderTag.hash == objectTag.hash) {
                return true;
            }
        }
    }
    return false;
}
```

## Integration with Node Editor System

### Pipeline Extensions

The `src/NodeEditorExtensions` directory contains extensions that integrate the rendering system with the `libs/nodeGraph` GUI:

- **PipelineExtension**: Handles overall pipeline configuration
- **PipelineStageExtension**: Manages individual render stages
- **IterationExtension**: Controls iteration over render targets

### Configuration Loading

The pipeline configuration is loaded from XML and converted to runtime structures:

```cpp
// In World.cpp
renderPipeline = GraphicsPipeline::deserialize("./Data/renderPipeline.xml", 
                                               graphicsWrapper, assetManager, 
                                               options, buildRenderMethods());
```

### Render Method Binding

The system binds render methods to pipeline stages:

```cpp
RenderMethods World::buildRenderMethods() {
    RenderMethods renderMethods;
    renderMethods.renderCameraByTag = std::bind(&World::renderCameraByTag, 
                                               this, std::placeholders::_1, 
                                               std::placeholders::_2, 
                                               std::placeholders::_3);
    // ... other method bindings
    return renderMethods;
}
```