#include "Game.hpp"
#include "Systems/CrashHandler.hpp"
#include <iostream>
#include <exception>

int main()
{
    // Initialize crash handler for SEH, signals, and uncaught exceptions
    Systems::CrashHandler::init();

    try {
        std::cout << "Starting Game..." << std::endl;
        Game game;
        game.run();
        std::cout << "Game exited normally." << std::endl;
    } catch (const std::exception& e) {
        Systems::CrashHandler::logException(e, "Exception caught in main()");
        return 1;
    } catch (...) {
        Systems::CrashHandler::logCrash("Unknown Fatal Exception caught in main()", "main()");
        return 1;
    }
    return 0;
}