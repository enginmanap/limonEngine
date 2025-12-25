//
// Created by engin on 25/12/2025.
//

#ifndef LIMONENGINE_PYTHONSTDOUT_H
#define LIMONENGINE_PYTHONSTDOUT_H


#include <iostream>

class PythonStdOut {
public:
    // Python calls this when you do print()
    void write(const std::string& text) {
        // Python often sends the newline "\n" as a separate write call.
        // We print whatever we get to std::cout.
        std::cout << text;

        // CRITICAL: Force the output to appear immediately.
        // Without this, Python holds the text in a buffer and you see nothing.
        std::cout.flush();
    }

    // Python calls this explicitly sometimes
    void flush() {
        std::cout.flush();
    }
};

#endif //LIMONENGINE_PYTHONSTDOUT_H