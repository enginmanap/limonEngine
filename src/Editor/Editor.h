//
// Created by engin on 28/09/2021.
//

#ifndef LIMONENGINE_EDITOR_H
#define LIMONENGINE_EDITOR_H

class World;

class Editor {
public:
    static void renderEditor(World& world);

private:
    static void addAnimationDefinitionToEditor(World& world);

};


#endif //LIMONENGINE_EDITOR_H
