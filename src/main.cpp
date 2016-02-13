//
// Created by Engin Manap on 10.02.2016.
//

#include <iostream>

#include "GLHelper.h"
#include "SDL2Helper.h"
#include "Model.h"

#define PROGRAM_NAME "UberGame"

int main(int argc, char *argv[]){
    int height=1024, width=768;

    bool quit=false;

    SDL2Helper sdlHelper(PROGRAM_NAME, height,width);

    GLHelper glHelper;
    glHelper.reshape(height,width);

    Model star(&glHelper);

    while(!quit){
        glHelper.clearFrame();
        //glHelper.render();
        star.render();
        sdlHelper.swap();
        quit = sdlHelper.isQuit();
    }
    return 0;
}

