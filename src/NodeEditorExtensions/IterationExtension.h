//
// Created by engin on 23.04.2019.
//

#ifndef LIMONENGINE_ITERATIONEXTENSION_H
#define LIMONENGINE_ITERATIONEXTENSION_H


#include <nodeGraph/src/NodeExtension.h>

class IterationExtension : public NodeExtension {
public:
    void drawDetailPane(Node *node) override;
private:
    std::string currentIterateOver = "None";
};


#endif //LIMONENGINE_ITERATIONEXTENSION_H
