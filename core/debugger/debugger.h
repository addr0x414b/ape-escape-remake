#ifndef DEBUGGER_H
#define DEBUGGER_H
#include <iostream>

class Debugger {
   public:
    // Print a message to the console. Throw an exception if it's an error
    void consoleMessage(const char* message, bool error);
};
#endif