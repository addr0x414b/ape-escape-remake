#include "debugger.h"

// Print a message to the console. Throw an exception if it's an error
void Debugger::consoleMessage(const char* message, bool error) {
    if (error) {
        throw std::runtime_error(message);
    } else {
        std::cout << message << std::endl;
    }
}