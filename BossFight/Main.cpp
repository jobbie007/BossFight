
#include "BossFight.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>

// --- Main Function --- 
int main() {
    Player player(
        100,     // maxHealth
        100,     // currentHealth
        true,    // enableDash
        0.8f,    // parrySuccessTime
        13,      // baseAttackDamage
        1.f,     // defensePercent
        true     // enableShoot
    );
    BossGame game(player);
    game.run();
    if (game.playerWin()) {
        std::cout << "You win!" << std::endl;
    }
    else {
        std::cout << "You lose!" << std::endl;
    }
    // Print final time
    std::cout << "Final Time: ";
    // Format the time as mm:ss
    int totalSeconds = static_cast<int>(game.getFinalTime().asSeconds());
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::cout << minutes << ":" << seconds << std::endl;
    std::cout << game.getFinalTime().asSeconds() << " seconds";
    return 0;
}