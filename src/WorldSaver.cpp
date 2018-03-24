//
// Created by engin on 24.03.2018.
//

#include <tinyxml2.h>
#include <string>

#include "WorldSaver.h"
#include "World.h"

/************************************************************************************
 * Map file spec
 * world
 *      Name
 *      Objects
 *          Object (for each)
 *              File
                Mass
                Scale
                    X,Y,Z
                Translate
                    X,Y,Z
                Rotate
                    X,Y,Z
 *      Sky
 *                  ImagesPath
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
    rootNode->InsertEndChild(currentElement);//add objects


    tinyxml2::XMLError eResult = mapDocument.SaveFile(mapName.c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "ERROR " << eResult << std::endl;
    }

    return true;
}

bool WorldSaver::fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode, const World* world ) {
    for(auto it=world->objects.begin(); it != world->objects.end(); it++) {//object ids are not constant, so they can be removed.

        tinyxml2::XMLElement *objectElement = document.NewElement("Object");
        objectsNode->InsertEndChild(objectElement);

        tinyxml2::XMLElement *currentElement = document.NewElement("File");
        currentElement->SetText(dynamic_cast<GameObject*>(it->second)->getName().c_str());
        objectElement->InsertEndChild(currentElement);

        currentElement = document.NewElement("Mass");
        currentElement->SetText((it->second)->getMass());
        objectElement->InsertEndChild(currentElement);

        tinyxml2::XMLElement *parent = document.NewElement("Scale");
        glm::vec3 scale = (it->second)->getScale();
        currentElement = document.NewElement("X");
        currentElement->SetText(scale.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(scale.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(scale.z);
        parent->InsertEndChild(currentElement);
        objectElement->InsertEndChild(parent);

        parent = document.NewElement("Translate");
        glm::vec3 translate = (it->second)->getTranslate();
        currentElement = document.NewElement("X");
        currentElement->SetText(translate.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(translate.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(translate.z);
        parent->InsertEndChild(currentElement);
        objectElement->InsertEndChild(parent);

        parent = document.NewElement("Rotate");
        glm::quat orientation = (it->second)->getOrientation();
        currentElement = document.NewElement("X");
        currentElement->SetText(orientation.x);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Y");
        currentElement->SetText(orientation.y);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("Z");
        currentElement->SetText(orientation.z);
        parent->InsertEndChild(currentElement);
        currentElement = document.NewElement("W");
        currentElement->SetText(orientation.w);
        parent->InsertEndChild(currentElement);
        objectElement->InsertEndChild(parent);
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
