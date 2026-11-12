//
// Created by Engin Manap on 11/11/2026.
//

#include "LegacyWorldLoaderV1.h"

#include "V1UnitScale.h"
#include "../World.h"
#include "../WorldAPIAccessor.h"
#include "../GameObjects/ModelGroup.h"
#include "../GamePlay/APISerializer.h"

void LegacyWorldLoaderV1::prepareDocument(tinyxml2::XMLNode *worldNode) const {
    V1UnitScale::convertWorld(worldNode);//before anything reads a position
}

// Groups save children nested in <Children>. loadModelGroup loads them and returns inner models here so they can be added to the world.
bool LegacyWorldLoaderV1::loadObjectGroups(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI,
        std::vector<Model*> &notStaticObjects, bool &isAIGridStartPointSet, glm::vec3 &aiGridStartPoint) const {
    tinyxml2::XMLElement* objectGroupsListNode =  worldNode->FirstChildElement("ObjectGroups");
    if (objectGroupsListNode == nullptr) {
        std::cout << "World doesn't have Object Groups clause." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectGroupNode =  objectGroupsListNode->FirstChildElement("ObjectGroup");
    if (objectGroupNode == nullptr) {
        std::cout << "World doesn't have any object Groups." << std::endl;
        return true;
    }

    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;
    std::map<uint32_t , ModelGroup*> modelGroups;
    std::vector<std::unique_ptr<ObjectInformation>> innerModels;

    while(objectGroupNode != nullptr) {
        ModelGroup* modelGroup = loadModelGroup(world, graphicsWrapper, assetManager, objectGroupNode, requiredSounds,
                                                modelGroups, innerModels, limonAPI, nullptr);
        world->modelGroups[modelGroup->getWorldObjectID()] = modelGroup;
        objectGroupNode = objectGroupNode->NextSiblingElement("ObjectGroup");
    } // end of while (objects)

    for (auto iterator = modelGroups.begin(); iterator != modelGroups.end(); ++iterator) {
        world->modelGroups[iterator->first] = iterator->second;
    }

    for (auto modelIterator = innerModels.begin(); modelIterator != innerModels.end(); ++modelIterator) {
        ObjectInformation* objectInfo = modelIterator->get();
        if(objectInfo) { //if not null
            if(objectInfo->modelActor != nullptr) {
                world->addActor(objectInfo->modelActor);
            }
            if(!isAIGridStartPointSet && objectInfo->isAIGridStartPointSet ) { //if this is the first actor to set AI grid start point
                aiGridStartPoint = objectInfo->aiGridStartPoint;
                isAIGridStartPointSet = true;
            }
        }
        // We will add static objects first, build AI grid, then add other objects
        if(objectInfo->model->getMass() == 0 && !objectInfo->model->isAnimated()) {
            world->addModelToWorld(objectInfo->model);
        } else {
            notStaticObjects.push_back(objectInfo->model);
        }
    }

    return true;
}

void LegacyWorldLoaderV1::loadPlayerAttachment(tinyxml2::XMLElement *playerNode, World *world, LimonAPI *limonAPI) const {
    tinyxml2::XMLElement* attachmentNode = playerNode->FirstChildElement("Attachment");
    if(attachmentNode == nullptr) {
        attachmentNode = playerNode->FirstChildElement("Attachement");//maps saved before 5806295d
    }
    if(attachmentNode == nullptr) {
        return;
    }
    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;
    tinyxml2::XMLElement* objectNode = attachmentNode->FirstChildElement("Object");
    if(objectNode == nullptr) {
        return;
    }
    std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(world, assetManager, objectNode, requiredSounds, limonAPI, nullptr);
    if(objectInfos.empty()) {
        return;
    }
    for(auto& info : objectInfos) {
        if(info->modelActor != nullptr) {
            std::cerr << "There was an AI attached to player model, this shouldn't happen. Ignoring" << std::endl;
            delete info->modelActor;
        }
        world->addModelToWorld(info->model);
    }
    Model* attachmentRoot = objectInfos.back()->model;//we know the root of the list is the last element
    //V1 saved the player's look rotation as the orientation, only the translate was the offset
    attachmentRoot->getTransformation()->setOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    world->apiAccessor->attach(attachmentRoot, world->getStartingPlayer(), -1, false, true);
}

bool LegacyWorldLoaderV1::loadObjectsFromXML(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI) const {
    std::vector<std::vector<std::string>> preloadAssetFiles; //used to load the assets in parallel instead of serial
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);

    //first load the groups
    loadObjectGroups(worldNode, world, limonAPI, notStaticObjects, isAIGridStartPointSet, aiGridStartPoint);

    tinyxml2::XMLElement* objectsListNode =  worldNode->FirstChildElement("Objects");
    if (objectsListNode == nullptr) {
        std::cerr << "World doesn't have Objects clause, this might be a mistake." << std::endl;
        return true;
    }

    tinyxml2::XMLElement* objectNode =  objectsListNode->FirstChildElement("Object");
    if (objectNode == nullptr) {
        std::cout << "World doesn't have any objects, this might be a mistake." << std::endl;
        return true;
    }


    std::unordered_map<std::string, std::shared_ptr<Sound>> requiredSounds;


    tinyxml2::XMLElement* objectNodeForPreload =  objectNode;
    while(objectNodeForPreload != nullptr) {
        tinyxml2::XMLElement *objectAttribute =  objectNodeForPreload->FirstChildElement("File");
        if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
            objectNodeForPreload = objectNodeForPreload->NextSiblingElement("Object");
            continue;
        }
        std::string modelFile = objectAttribute->GetText();
        tinyxml2::XMLElement *flipAttributeForPreload = objectNodeForPreload->FirstChildElement("Flip");
        std::string flipAxesForPreload;
        if (flipAttributeForPreload != nullptr && flipAttributeForPreload->GetText() != nullptr) {
            flipAxesForPreload = flipAttributeForPreload->GetText();
        }
        std::string assetKeyForPreload = flipAxesForPreload.empty() ? modelFile : modelFile + "?flip" + flipAxesForPreload;
        std::vector<std::string> temp;
        temp.emplace_back(assetKeyForPreload);
        preloadAssetFiles.emplace_back(temp);
        objectNodeForPreload = objectNodeForPreload->NextSiblingElement("Object");
    }

    std::vector<std::shared_ptr<ModelAsset>> preloadAssets = assetManager->parallelLoadAssetList<ModelAsset>(preloadAssetFiles); //loads the assets in parallel()

    while(objectNode != nullptr) {

        std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(world, assetManager, objectNode,
                                                                                 requiredSounds, limonAPI, nullptr);//this map is used to load all the sounds, while sharing same objects.

        for (auto objectIterator = objectInfos.begin(); objectIterator != objectInfos.end(); ++objectIterator) {
            if((*objectIterator)->modelActor != nullptr) {
                world->addActor((*objectIterator)->modelActor);
            }
            if(!isAIGridStartPointSet && (*objectIterator)->isAIGridStartPointSet ) { //if this is the first actor to set AI grid start point
                aiGridStartPoint = (*objectIterator)->aiGridStartPoint;
                isAIGridStartPointSet = true;
            }

            // We will add static objects first, build AI grid, then add other objects
            if((*objectIterator)->model->getMass() == 0 && !(*objectIterator)->model->isAnimated()) {
                world->addModelToWorld((*objectIterator)->model);
            } else {
                notStaticObjects.push_back((*objectIterator)->model);
            }
        }

        //DON'T ADD NEW ATTRIBUTES HERE STATIC AND OTHER OBJECTS ARE HANDLED DIFFERENTLY, ADD ATTRIBUTES BEFORE THAT

        objectNode = objectNode->NextSiblingElement("Object");
    } // end of while (objects)

    world->createGridFrom(aiGridStartPoint, isAIGridStartPointSet);

    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        world->addModelToWorld(notStaticObjects[i]);
    }

    //V1 links children while constructing them, not through the accessor attach, so settle bodies and pairs here
    for (auto objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt) {
        if (objectIt->second->getParentObject() == nullptr && objectIt->second->hasChildren()) {
            world->apiAccessor->applyBodyType(objectIt->second);
            world->apiAccessor->setHierarchyRootIndex(objectIt->second, static_cast<int>(objectIt->second->getWorldObjectID()));
            world->apiAccessor->refreshNearestModelGroup(objectIt->second);
        }
    }
    for (auto groupIt = world->modelGroups.begin(); groupIt != world->modelGroups.end(); ++groupIt) {
        if (groupIt->second->getParentObject() == nullptr) {
            world->apiAccessor->applyBodyType(groupIt->second);
            world->apiAccessor->setHierarchyRootIndex(groupIt->second, static_cast<int>(groupIt->first));
            world->apiAccessor->refreshNearestModelGroup(groupIt->second);
        }
    }

    //clear up the preloaded asset counts.
    for(const auto& assetFile:preloadAssetFiles) {
        assetManager->freeAsset({assetFile});
    }
    return true;
}

std::vector<std::unique_ptr<WorldLoader::ObjectInformation>>
LegacyWorldLoaderV1::loadObject(World *world, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                                std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI,
                                PhysicalRenderable *parentObject) {
    std::vector<std::unique_ptr<ObjectInformation>> loadedObjects;

    tinyxml2::XMLElement *objectAttribute =  objectNode->FirstChildElement("File");
    if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
        return loadedObjects;
        }
    std::string modelFile = objectAttribute->GetText();
    objectAttribute =  objectNode->FirstChildElement("Mass");
    float modelMass;
    if (objectAttribute == nullptr) {
            //std::cout << "Object does not have mass, assume 0." << std::endl;
            modelMass = 0;
        } else {
            modelMass = std::stof(objectAttribute->GetText());
        }
    int id;
    objectAttribute =  objectNode->FirstChildElement("ID");
    if (objectAttribute == nullptr) {
            std::cerr << "Object does not have ID. Can't be loaded" << std::endl;
            return loadedObjects;
        } else {
            id = std::stoi(objectAttribute->GetText());
        }

    bool disconnected = false;
    objectAttribute =  objectNode->FirstChildElement("Disconnected");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
            //std::cout << "Object disconnect status is not set. defaulting to False" << std::endl;
#endif
        } else {
            std::string disConnectedText = objectAttribute->GetText();
            if(disConnectedText == "True") {
                disconnected = true;
            } else if(disConnectedText == "False") {
                disconnected = false;
            } else {
                //std::cout << "Object disconnect status is unknown. defaulting to False" << std::endl;
            }
        }

    std::string flipAxes;
    tinyxml2::XMLElement *flipAttribute = objectNode->FirstChildElement("Flip");
    if (flipAttribute != nullptr && flipAttribute->GetText() != nullptr) {
        flipAxes = flipAttribute->GetText();
    }

    std::unique_ptr<ObjectInformation> loadedObjectInformation = std::make_unique<ObjectInformation>();
    loadedObjectInformation->model = new Model(id, assetManager, modelMass, modelFile, disconnected, world->allocateCullingEntry(), flipAxes);

    int32_t parentBoneID = -1;
    objectAttribute =  objectNode->FirstChildElement("ParentBoneID");

    if (objectAttribute != nullptr) {
        parentBoneID = std::stoi(objectAttribute->GetText());
    }

    loadedObjectInformation->model->setParentObject(parentObject, parentBoneID);

    objectAttribute =  objectNode->FirstChildElement("StepOnSound");

    if (objectAttribute == nullptr) {
        //std::cerr << "Object does not have step on sound." << std::endl;
    } else {
        std::string stepOnSound = objectAttribute->GetText();
        if(requiredSounds.find(stepOnSound) == requiredSounds.end()) {
            requiredSounds[stepOnSound] = std::make_shared<Sound>(0, assetManager, stepOnSound);//since the step on is not managed by world, not feed world object ID
            requiredSounds[stepOnSound]->changeGain(0.125f);
        }
        loadedObjectInformation->model->setPlayerStepOnSound(requiredSounds[stepOnSound]);
    }

    objectAttribute =  objectNode->FirstChildElement("Transformation");
    if(objectAttribute == nullptr) {
            std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
            world->deleteModel(loadedObjectInformation->model);
        return loadedObjects;
    }
    loadedObjectInformation->model->getTransformation()->deserialize(objectAttribute);
    if(parentObject != nullptr) {
        Model* parentModel = dynamic_cast<Model*>(parentObject);
        if(parentModel != nullptr) {
            if(parentBoneID != -1) {
                //version 1 offsets are against the skinning matrix, 96a5237c moved bone attachments to the bone's own frame
                Transformation* childTransformation = loadedObjectInformation->model->getTransformation();
                glm::mat4 legacyLocal = glm::translate(glm::mat4(1.0f), childTransformation->getTranslate()) *
                                        glm::mat4_cast(childTransformation->getOrientation()) *
                                        glm::scale(glm::mat4(1.0f), childTransformation->getScale());
                glm::mat4 boneLocal = parentModel->getModelAsset()->getBoneBindOffset(parentBoneID) * legacyLocal;
                glm::vec3 translate, scale, skew;
                glm::quat orientation;
                glm::vec4 perspective;
                glm::decompose(boneLocal, scale, orientation, translate, skew, perspective);
                childTransformation->setTransformations(translate, scale, orientation);
            }
            loadedObjectInformation->model->getTransformation()->setParentTransform(
                    parentModel->getAttachmentTransformForKnownBone(parentBoneID));
        } else {
            loadedObjectInformation->model->getTransformation()->setParentTransform(
                    parentObject->getTransformation());
        }
    }
    //Since we are not loading objects recursively, these can be set here safely
    objectAttribute =  objectNode->FirstChildElement("Actor");
    if (objectAttribute == nullptr) {
#ifndef NDEBUG
        //std::cout << "Object does not have AI." << std::endl;
#endif
    } else {
        ActorInterface* actor = APISerializer::deserializeActorInterface(objectAttribute, limonAPI, loadedObjectInformation->model->getWorldObjectID());

        loadedObjectInformation->aiGridStartPoint = GLMConverter::BltToGLM(loadedObjectInformation->model->getRigidBody()->getCenterOfMassPosition());
        loadedObjectInformation->isAIGridStartPointSet = true;
        if(actor != nullptr) {//most likely shared library not found, but in general possible.
            loadedObjectInformation->modelActor = actor;
            loadedObjectInformation->model->attachAI(loadedObjectInformation->modelActor);
        }
    }

    objectAttribute =  objectNode->FirstChildElement("Animation");
    if (objectAttribute == nullptr || objectAttribute->GetText() == nullptr) {
#ifndef NDEBUG
        //std::cout << "Object does not have default animation." << std::endl;
#endif
    } else {
        loadedObjectInformation->model->setAnimation(objectAttribute->GetText());
    }

    //now load children

    tinyxml2::XMLElement* childrenNode =  objectNode->FirstChildElement("Children");

    if(childrenNode != nullptr) {
        //means there are children

        tinyxml2::XMLElement* childrenCountNode =  childrenNode->FirstChildElement("Count");
        if(childrenCountNode == nullptr || childrenCountNode->GetText() == nullptr) {
            std::cerr << "Object has children node, but count it unknown. Children can't be loaded! " << std::endl;
                    } else {
            tinyxml2::XMLElement *childNode = childrenNode->FirstChildElement("Child");
            while (childNode != nullptr) {
                tinyxml2::XMLElement *childObjectNode = childNode->FirstChildElement("Object");
                std::vector<std::unique_ptr<ObjectInformation>> objectInfos = loadObject(world, assetManager,
                                                                                          childObjectNode,
                                                                                          requiredSounds,
                                                                                          limonAPI,
                                                                                          loadedObjectInformation->model);

                loadedObjectInformation->model->addChild(objectInfos[objectInfos.size()-1]->model);//we know the root of the list is the last element

                std::move(std::begin(objectInfos), std::end(objectInfos), std::back_inserter(loadedObjects));
                childNode = childNode->NextSiblingElement("Child");
            }
        }
    }
    //version 1 mesh materials have no MaterialKey, nothing can resolve them
    loadObjectTags(objectNode, loadedObjectInformation->model);


    loadedObjects.push_back(std::move(loadedObjectInformation));


    return loadedObjects;
}

ModelGroup *LegacyWorldLoaderV1::loadModelGroup(World *world, GraphicsInterface* graphicsWrapper, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *modelGroupNode,
                                                std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                                std::map<uint32_t, ModelGroup *> &childGroups,
                                                std::vector<std::unique_ptr<ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                                ModelGroup *parentGroup) {
    // Group children are saved inside <Children>. V2 saves them flat (see ModelGroup::deserialize) and reattaches them via addChild.
    ModelGroup* modelGroup = nullptr;

    tinyxml2::XMLElement* groupAttribute;
    std::string readName;
    uint32_t readID;
    groupAttribute = modelGroupNode->FirstChildElement("Name");
    if (groupAttribute == nullptr) {
        std::cerr << "Trigger must have a Name." << std::endl;
        return nullptr;
    }
    readName = groupAttribute->GetText();

    groupAttribute = modelGroupNode->FirstChildElement("ID");
    if (groupAttribute == nullptr) {
        std::cerr << "Trigger must have a ID." << std::endl;
        return nullptr;
    }
    readID = std::stoul(groupAttribute->GetText());

    modelGroup = new ModelGroup(graphicsWrapper, readID, readName);

    modelGroup->setParentObject(parentGroup);

    groupAttribute =  modelGroupNode->FirstChildElement("Transformation");
    if(groupAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;

        return nullptr;
    }
    modelGroup->transformation.deserialize(groupAttribute);
    if(parentGroup != nullptr) {
        modelGroup->getTransformation()->setParentTransform(parentGroup->getTransformation());
    }

    //now fill the children

    groupAttribute =  modelGroupNode->FirstChildElement("ChildCount");

    if(groupAttribute == nullptr) {
        std::cerr << "Model group " << modelGroup->getName() << " has no child count saved, no child will be loaded." << std::endl;
        return modelGroup;
    }

    uint32_t childCount = std::stoul(groupAttribute->GetText());

    modelGroup->children.resize(childCount);

    tinyxml2::XMLElement* childrenNode =  modelGroupNode->FirstChildElement("Children");

    if(childrenNode == nullptr) {
        std::cerr << "Children node not found for Model group " << modelGroup->getName() << " no child will be loaded." << std::endl;
        return modelGroup;
    }

    tinyxml2::XMLElement* childNode =  childrenNode->FirstChildElement("Child");

    while (childNode != nullptr) {
        if(childNode->Attribute("Index") == nullptr) {
            std::cerr << "child without ID found, this child of " << modelGroup->getName() << " will not be loaded. " << std::endl;
        } else {
            size_t childIndex = std::stoul(std::string(childNode->Attribute("Index")));

            if(childNode->FirstChildElement("Object")) {
                std::vector<std::unique_ptr<ObjectInformation>> childVector =
                loadObject(world, assetManager, childNode->FirstChildElement("Object"), requiredSounds,
                           limonAPI, modelGroup);

                childObjects.push_back(std::move(childVector[childVector.size() -1]));
                Model* model = childObjects.at(childObjects.size()-1)->model;
                modelGroup->children[childIndex] = model;

                modelGroup->children[childIndex]->getTransformation()->setParentTransform(modelGroup->getTransformation());
            } else if(childNode->FirstChildElement("ObjectGroup")) {
                ModelGroup* newModelGroup = loadModelGroup(world, graphicsWrapper, assetManager,
                                                           childNode->FirstChildElement("ObjectGroup"),
                                                           requiredSounds, childGroups, childObjects, limonAPI,
                                                           modelGroup);
                childGroups[newModelGroup->getWorldObjectID()] = newModelGroup;
                modelGroup->children[childIndex] = newModelGroup;
            } else {
                std::cerr << "The child is not Object, or Object group. Can't load unknown type, skipping." << std::endl;
            }
        }
        childNode =  childNode->NextSiblingElement("Child");
    }


    return modelGroup;
}
