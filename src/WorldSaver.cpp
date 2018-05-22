//
// Created by engin on 24.03.2018.
//

#include <tinyxml2.h>
#include <string>

#include "WorldSaver.h"
#include "World.h"
#include "GameObjects/Light.h"
#include "Assets/Animations/AnimationCustom.h"

/************************************************************************************
 * Map file spec
 * world
 *      Name
 *      Objects
 *          Object (for each)
 *              File
                Mass
                ID
                Transformation
                    Scale
                        X,Y,Z
                    Translate
                        X,Y,Z
                    Rotate
                        X,Y,Z
                    Animation
                AI (True, False)
                    AI_ID (If AI true)

 *      Sky
 *                  ImagesPath
 *                  ID
                    Right
                    Left
                    Top
                    Bottom
                    Back
                    Front
 *      Lights
 *              Light (For each)
                    Type  (POINT, DIRECTIONAL)
                    Position
                        X,Y,Z
                    Color
                        R,G,B
 *      LoadedAnimations
 *              LoadedAnimation
 *                  Name
 *                  Index
 *
 */

#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); return a_eResult; }

/**
 *
 * @return
 */
bool WorldSaver::saveWorld(const std::string& mapName, const World* world) {
    tinyxml2::XMLDocument mapDocument;
    tinyxml2::XMLNode * rootNode = mapDocument.NewElement("World");
    mapDocument.InsertFirstChild(rootNode);
    tinyxml2::XMLElement * currentElement = mapDocument.NewElement("Name");
    currentElement->SetText(mapName.c_str());
    rootNode->InsertEndChild(currentElement);
    //after current element is inserted, we can reuse
    currentElement = mapDocument.NewElement("Objects");
    if(!fillObjects(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add objects

    currentElement = mapDocument.NewElement("Lights");
    if(!fillLights(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add lights

    currentElement = mapDocument.NewElement("Sky");
    if(!addSky(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add lights

    currentElement = mapDocument.NewElement("LoadedAnimations");
    if(!fillLoadedAnimations(mapDocument, currentElement, world)) {
        return false;
    };
    rootNode->InsertEndChild(currentElement);//add lights


    tinyxml2::XMLError eResult = mapDocument.SaveFile(mapName.c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "ERROR " << eResult << std::endl;
    }

    return true;
}

bool WorldSaver::fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode, const World* world ) {
    for(auto it=world->objects.begin(); it != world->objects.end(); it++) {//object ids are not constant, so they can be removed.
        (it->second)->fillObjects(document, objectsNode);
    }
    return true;
}

bool WorldSaver::fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world) {
    for(auto it=world->lights.begin(); it != world->lights.end(); it++) {//object ids are not constant, so they can be removed.

        tinyxml2::XMLElement *lightElement = document.NewElement("Light");
        lightsNode->InsertEndChild(lightElement);

        tinyxml2::XMLElement *currentElement = document.NewElement("Type");
        switch((*it)->getLightType()) {
            case Light::DIRECTIONAL:
                currentElement->SetText("DIRECTIONAL");
                break;
            case Light::POINT:
                currentElement->SetText("POINT");
                break;
        }
        lightElement->InsertEndChild(currentElement);

        tinyxml2::XMLElement *parent = document.NewElement("Position");
        glm::vec3 position = (*it)->getPosition();
        currentElement = document.NewElement("X");
        currentElement->SetText(position.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(position.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(position.z);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);

        parent = document.NewElement("Color");
        glm::vec3 color = (*it)->getColor();
        currentElement = document.NewElement("R");
        currentElement->SetText(color.r);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("G");
        currentElement->SetText(color.g);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("B");
        currentElement->SetText(color.b);
        parent->InsertEndChild(currentElement);
        lightElement->InsertEndChild(parent);
    }
    return true;
}

bool WorldSaver::addSky(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *skyNode, const World *world) {
    //ImagesPath, Right, Left, Top, Bottom, Back, Front

    tinyxml2::XMLElement *currentElement = document.NewElement("ImagesPath");
    currentElement->SetText(world->sky->getPath().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(world->sky->getWorldObjectID());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Right");
    currentElement->SetText(world->sky->getRight().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Left");
    currentElement->SetText(world->sky->getLeft().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Top");
    currentElement->SetText(world->sky->getTop().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Bottom");
    currentElement->SetText(world->sky->getDown().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Back");
    currentElement->SetText(world->sky->getBack().c_str());
    skyNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Front");
    currentElement->SetText(world->sky->getFront().c_str());
    skyNode->InsertEndChild(currentElement);
    return true;
}

bool WorldSaver::fillLoadedAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *loadedAnimationsNode, const World *world) {
    for(size_t index = 0; index < world->loadedAnimations.size(); index++) {

        tinyxml2::XMLElement *animationElement = document.NewElement("LoadedAnimation");
        loadedAnimationsNode->InsertEndChild(animationElement);
        tinyxml2::XMLElement *currentElement = document.NewElement("Name");
        currentElement->SetText(world->loadedAnimations[index].getName().c_str());
        animationElement->InsertEndChild(currentElement);
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(index).c_str());
        animationElement->InsertEndChild(currentElement);
    }
    return true;

}
