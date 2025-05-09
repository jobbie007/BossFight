## 🎮 Overview

This is a 2D side-scrolling boss fight game developed using C++ and the SFML (Simple and Fast Multimedia Library). The player controls a hero character who must defeat a formidable boss in a demonic arena. The game features dynamic animations, multiple attack types for both player and boss, projectile systems, and a clear UI.

## ✨ Features

*   **Player Character:**
    *   Movement: Walk, Jump.
    *   Combat:
        *   Melee Attack Combo (3 randomized animations).
        *   Ranged Attack (Shooting projectiles).
        *   Parry: Deflect incoming damage if timed correctly.
        *   Dash: Quick evasive maneuver with invincibility frames.
    *   Health system and damage feedback (hurt animation, flashing).
    *   Death animation.
*   **Boss Character:**
    *   Complex AI with multiple attack patterns:
        *   Melee swipes.
        *   Ground-based projectile attack.
        *   Mid-air projectile spray.
        *   "Ultimate" attack: Projectile rain.
    *   Distinct animations for idle, attacks, and death.
    *   Health system and damage feedback.
*   **Gameplay Mechanics:**
    *   Projectile system for both player and boss.
    *   Collision detection for attacks and projectiles.
    *   Parry mechanic for skill-based defense.
    *   Boundary checks to keep characters within the arena.
    *   Dynamic background.
*   **User Interface (UI):**
    *   Player and Boss health bars.
    *   Numerical health display.
    *   In-game timer.
    *   Tutorial messages at the start of the game.
    *   "You Win!" / "You Lose!" end game screens.
    *   Ability icons (Dash, Shoot).
*   **Resource Management:**
    *   `TextureManager`: Singleton for efficient loading and access to textures.
    *   `SoundManager`: Singleton for loading, playing, and managing sound effects and background music.
*   **Animation System:**
    *   `AnimationComponent`: Handles sprite sheet animations, supporting different states, frame rates, looping, and origins.


---

### Requirements

* **Language/Framework**: C++, SFML
* **Platforms**: Windows, macOS, Linux (as supported by the engine)

---

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/jobbie007/BossFight.git
   ```
2. Or Navigate to the project release:

   ```bash
   download the latest release zip file and run the .exe file 
   ```

---

### Game Controls

| Action     | Key / Button    |
| ---------- | --------------- |
| Move Left  | A               |
| Move Right | D               |
| Jump       | space           |
| Attack     | E / Left Mouse  |
| Parry      | Q / Right Mouse |
| Shoot      | F               |

---

### License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

### Acknowledgments

* Inspiration from classic boss-rush games.
* Thanks to the open-source community for libraries and tools.

### 🚀 Future Enhancements (Potential Ideas)
* Add more levels or different bosses.

* Implement power-ups or new abilities for the player.

* Introduce different enemy types besides the main boss.

* Enhance visual effects (particles for impacts, spells).

* Add a main menu and options screen.

* More sophisticated AI patterns for the boss.

Happy fighting! Feel free to contribute or report issues.
