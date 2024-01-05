#include "debugger.h"

void Debugger::consoleMessage(const char* message, bool error) {
    if (error) {
        throw std::runtime_error(message);
    } else {
        std::cout << message << std::endl;
    }
}