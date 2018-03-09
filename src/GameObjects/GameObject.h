//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject {
public:
    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX };
    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ~GameObject() {};
};


#endif //LIMONENGINE_GAMEOBJECT_H
