//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_TRIGGERINTERFACE_H
#define LIMONENGINE_TRIGGERINTERFACE_H


class TriggerInterface {
public:
    virtual bool addEditorElements() = 0;
    virtual bool run() = 0;

    virtual ~TriggerInterface() = default;
};


#endif //LIMONENGINE_TRIGGERINTERFACE_H
