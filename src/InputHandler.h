//
// Created by Engin Manap on 14.02.2016.
//

#ifndef UBERGAME_INPUTHANDLER_H
#define UBERGAME_INPUTHANDLER_H

#include <map>
#include <string>
#include <SDL2/SDL.h>


class InputHandler {

    SDL_Event event;
    std::map<std::string,bool> inputStates;

public:
    void mapInput();

    bool getInputState(std::string input){
        if(inputStates.find(input) == inputStates.end())
            return false;
        return inputStates[input];
    }

};

#endif //UBERGAME_INPUTHANDLER_H
