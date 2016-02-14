//
// Created by Engin Manap on 10.02.2016.
//

#ifndef UBERGAME_SDL2HELPER_CPP_H
#define UBERGAME_SDL2HELPER_CPP_H

#include <iostream>

#include <SDL2/SDL.h>
#include <map>
#include <string>

class SDL2Helper {
    SDL_Window *window;
    SDL_GLContext context;
    SDL_Event event;
    std::map<std::string,bool> inputStates;
public:

    SDL2Helper(const char*, int, int);
    ~SDL2Helper();

    void swap(){
        SDL_GL_SwapWindow(window);
    };

    void mapInput();

    bool getInputState(std::string input){
        if(inputStates.find(input) == inputStates.end())
            return false;
        return inputStates[input];
    }

};
#endif //UBERGAME_SDL2HELPER_CPP_H
