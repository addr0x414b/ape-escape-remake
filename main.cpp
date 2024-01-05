#include "core/debugger/debugger.h"
#include "servers/display_server.h"

#ifdef NDEBUG
const bool DEBUG = false;
#else
const bool DEBUG = true;
#endif

int main() {
    Debugger debugger;
    DisplayServer displayServer;

    if (DEBUG) {
        debugger.consoleMessage("\n[DEBUG MODE]", false);
    } else {
        debugger.consoleMessage("\n[RELEASE MODE]", false);
    }

    displayServer.init();
    displayServer.run();

    debugger.consoleMessage("\nShutdown initiated...", false);
    displayServer.cleanup();
    debugger.consoleMessage("\nProgram shutdown successful", false);
    return 0;
}