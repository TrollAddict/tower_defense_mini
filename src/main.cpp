#include <cstdlib>
#include <iostream>

#include "game/Game.hpp"

int main() {
    try {
        td::Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
