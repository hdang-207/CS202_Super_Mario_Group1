#include "Game.hpp"
#include <iostream>
#include <exception>

int main()
{
    try {
        std::cout << "Starting Game..." << std::endl;
        Game game;
        game.run();
        std::cout << "Game exited normally." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Exception occurred!" << std::endl;
        return 1;
    }
    return 0;
}