#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <random>

// --- Texture Manager ---
class TextureManager {
public:
    static TextureManager& instance() {
        static TextureManager tm;
        return tm;
    }

    // Destructor to clean up raw pointers
    ~TextureManager() {
        std::cout << "[TextureManager] Cleaning up textures..." << std::endl;

        for (auto const& [id, ptr] : textures) {
            if (ptr) {
                delete ptr;
            }
        }
        textures.clear();
        std::cout << "[TextureManager] Cleanup complete." << std::endl;
    }

    // Load texture, return true on success, false on failure
    bool load(const std::string& id, const std::string& path) {
        sf::Texture* texture = new sf::Texture();
        if (texture->loadFromFile(path)) {
            auto it = textures.find(id);
            if (it != textures.end()) {
                delete it->second;
            }
            textures[id] = texture;
            // std::cout << "[TextureManager] Loaded: " << id << " from " << path << std::endl; // Reduced output
            return true;
        }
        else {
            std::cerr << "[TextureManager] Failed to load texture: " << id << " from " << path << std::endl;
            delete texture;
            return false;
        }
    }

    // Get a pointer to a previously loaded texture, or nullptr if not found
    sf::Texture* get(const std::string& id) const {
        auto it = textures.find(id);
        if (it != textures.end()) {
            return it->second;
        }
        else {
            return nullptr;
        }
    }

private:
    TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    std::map<std::string, sf::Texture*> textures;
};

// --- Animation State Enum ---
enum class AnimationState {
    Idle, Run, Jump, Attack1, Attack2, Attack3, Parry, Dash, Dead, None
};

// --- Animation Component ---
class AnimationComponent {
public:
    struct Animation {
        const sf::Texture* texture = nullptr;
        int frameCount = 0;
        float frameDuration = 0.1f;
        sf::Vector2i frameSize = { 0, 0 };
        bool loops = true;
    };

    AnimationComponent() = default;

    void addAnimation(AnimationState state, const std::string& textureId, int frameCount, float frameDuration, sf::Vector2i frameSize, bool loops = true)
    {
        const sf::Texture* texturePtr = TextureManager::instance().get(textureId);
        if (texturePtr) {
            animations[state] = { texturePtr, frameCount, frameDuration, frameSize, loops };
            if (sprite.getTexture() == nullptr) {
                sprite.setTexture(*texturePtr);
                sprite.setOrigin(static_cast<float>(frameSize.x) / 2.f, static_cast<float>(frameSize.y) / 2.f);
            }
        }
        else {
            std::cerr << "[AnimationComponent] Error adding animation for state "
                << static_cast<int>(state) << ": Texture '" << textureId << "' not found." << std::endl;
        }
    }

    void update(float dt) {
        if (currentState != AnimationState::None && animations.count(currentState)) {
            auto& anim = animations[currentState];


            if (!anim.loops && isDone()) {
                return;
            }

            elapsedTime += dt;

            if (elapsedTime >= anim.frameDuration) {
                elapsedTime -= anim.frameDuration;
                currentFrame++;

                if (currentFrame >= anim.frameCount) {
                    if (anim.loops) {
                        currentFrame = 0;
                    }
                    else {
                        // Clamp to the last frame for non-looping animations
                        currentFrame = anim.frameCount - 1;
                    }
                }

                sprite.setTextureRect(sf::IntRect(
                    anim.frameSize.x * currentFrame, 0,
                    anim.frameSize.x, anim.frameSize.y
                ));
            }
        }
    }

    void play(AnimationState state) {
        // Only switch if the requested state is different OR if we want to restart a looping anim (optional)
        // For now, only switch if different and valid
        if (state != currentState && animations.count(state)) {
            currentState = state;
            const auto& anim = animations[state];

            if (anim.texture == nullptr) {
                std::cerr << "[AnimationComponent] Warning: Attempting to play animation state "
                    << static_cast<int>(state) << " with null texture." << std::endl;
                currentState = AnimationState::None;
                return;
            }

            currentFrame = 0;
            elapsedTime = 0.f;

            sprite.setTexture(*anim.texture);
            sprite.setTextureRect(sf::IntRect(0, 0, anim.frameSize.x, anim.frameSize.y));
            sprite.setOrigin(static_cast<float>(anim.frameSize.x) / 2.f, static_cast<float>(anim.frameSize.y) / 2.f);
        }
    }

    bool isDone() const {
        if (currentState != AnimationState::None && animations.count(currentState)) {
            // Use find to avoid exception if state somehow becomes invalid after count check
            auto it = animations.find(currentState);
            if (it != animations.end()) {
                const auto& anim = it->second;
                return !anim.loops && (currentFrame >= anim.frameCount - 1);
            }
        }
        // If no animation is playing or state is invalid, consider it "done"
        return true;
    }


    AnimationState getCurrentState() const { return currentState; }
    sf::Sprite& getSprite() { return sprite; }
    const sf::Sprite& getSprite() const { return sprite; }

private:
    sf::Sprite sprite;
    std::map<AnimationState, Animation> animations;
    AnimationState currentState = AnimationState::None;
    int currentFrame = 0;
    float elapsedTime = 0.f;
};


// --- Player Class ---
class Player {
public:
    Player(int maxHealth = 100, int currentHealth = 100) : maxHealth(maxHealth), currentHealth(currentHealth) {
        loadResources();
        initAnimations();
        position = { 200.f, 500.f };
        animations.getSprite().setPosition(position);

        rng = std::mt19937(rd());
        attackDistribution = std::uniform_int_distribution<int>(0, 2); // For 3 attack types (0, 1, 2)
        attackStates = { AnimationState::Attack1, AnimationState::Attack2, AnimationState::Attack3 }; // Store attack states
    }

    void loadResources() {
        auto& tm = TextureManager::instance();
        tm.load("player_idle", "../assets/player/Idle.png");
        tm.load("player_run", "../assets/player/Run.png");
        tm.load("player_attack1", "../assets/player/Attack_1.png");
        tm.load("player_attack2", "../assets/player/Attack_2.png");
        tm.load("player_attack3", "../assets/player/Attack_3.png");
        tm.load("player_jump", "../assets/player/Jump.png");
        tm.load("player_dash", "../assets/player/Dash.png");
        tm.load("player_parry", "../assets/player/Parry.png");
        tm.load("player_dead", "../assets/player/Dead.png");
    }

    void initAnimations() {
        // Adjust frame counts, durations, sizes, loops to match your assets
        animations.addAnimation(AnimationState::Idle, "player_idle", 6, 0.2f, { 128, 128 }, true);
        animations.addAnimation(AnimationState::Run, "player_run", 8, 0.1f, { 128, 128 }, true);
        animations.addAnimation(AnimationState::Attack1, "player_attack1", 6, 0.07f, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Attack2, "player_attack2", 4, 0.1f, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Attack3, "player_attack3", 3, 0.101f, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Jump, "player_jump", 12, 0.08f, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Dash, "player_dash", 2, dashDuration, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Parry, "player_parry", 2, 0.4f, { 128, 128 }, false);
        animations.addAnimation(AnimationState::Dead, "player_dead", 3, 1.0f, { 128, 128 }, false);
        animations.play(AnimationState::Idle);
    }

    void update(float dt) {
        if (!isAlive() && animations.getCurrentState() == AnimationState::Dead) {
            animations.update(dt); // Only update animation component if dead
            return;
        }
        // Check for death based on health (natural death)
        if (currentHealth <= 0 && animations.getCurrentState() != AnimationState::Dead) {
            death();
		}

        handleMovement(dt);
        handleAnimations();
        updateTimers(dt);
        animations.update(dt);
    }

    void move(sf::Vector2f direction) {
        if (!isAlive()) return;

        if (dashTimer <= 0 && !isAttacking() && animations.getCurrentState() != AnimationState::Parry) {
            if (direction.x != 0.f) {
                velocity.x = direction.x > 0 ? moveSpeed : -moveSpeed;
                facingRight = direction.x > 0;
            }
            else {
                velocity.x = 0.f;
            }
        }
        else if (dashTimer <= 0) { // If attacking or parrying
            if (isGrounded) {
                velocity.x = 0.f; // Stop horizontal movement while attacking/parrying on ground
            }
        }
    }

    void jump() {
        if (!isAlive()) return;

        if (isGrounded && !isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry) {
            velocity.y = -jumpForce;
            isGrounded = false;
        }
    }

    void death() {
        animations.play(AnimationState::Dead);
        velocity.x = 0; // Stop movement
        velocity.y = 0; // Stop gravity/jump
        currentHealth = 0; // Ensure health is set to 0
        isGrounded = true; 
        std::cout << "[Player] Death triggered!" << std::endl;
    }

    void dash() {
        if (!isAlive()) return;

        // *** Updated to check against multiple attack states ***
        if (canDash && dashTimer <= 0 && !isAttacking() && animations.getCurrentState() != AnimationState::Parry) {
            velocity.x = facingRight ? dashSpeed : -dashSpeed;
            velocity.y = 0; // Dash is horizontal
            dashTimer = dashDuration;
            canDash = false; // Prevent dashing again until cooldown finishes
            dashCooldownTimer = dashCooldown; // Start cooldown timer immediately
            animations.play(AnimationState::Dash);
        }
    }

    void parry() {
        if (!isAlive()) return;

        if (canParry && !isAttacking() && dashTimer <= 0) { 
            animations.play(AnimationState::Parry);
            canParry = false;
            parryTimer = parryCooldown;
            if (isGrounded) {
                velocity.x = 0; // Stop movement while parrying on ground
            }
        }
    }

    void attack() {
        if (!isAlive()) return;

        if (canAttack && !isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry) {
            int attackIndex = attackDistribution(rng); // Generate 0, 1, or 2
            AnimationState selectedAttackState = attackStates[attackIndex];

            animations.play(selectedAttackState); // Play the chosen attack animation
            canAttack = false; // Prevent attacking again until cooldown finishes
            attackTimer = attackCooldown; // Start cooldown timer

            if (isGrounded) {
                velocity.x = 0; // Stop horizontal movement while attacking on ground
            }
            // std::cout << "[Player] Attack " << attackIndex + 1 << "!" << std::endl;
        }
    }

    void takeDamage(int amount) {
        // *** MODIFIED: Check if already dead OR in Dead animation state ***
        if (!isAlive() || animations.getCurrentState() == AnimationState::Dead) return; // Already dead or dying

        currentHealth -= amount;
        currentHealth = std::max(0, currentHealth); // Clamp health at 0
        std::cout << "[Player] Took " << amount << " damage. Health: " << currentHealth << "/" << maxHealth << std::endl;
    }

    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool isAlive() const { return currentHealth > 0; }

    sf::FloatRect getGlobalBounds() const {
        return animations.getSprite().getGlobalBounds();
    }

    void draw(sf::RenderTarget& target) const {
        target.draw(animations.getSprite());
    }

private: 
    AnimationComponent animations;
    sf::Vector2f position = { 0.f, 0.f };
    sf::Vector2f velocity = { 0.f, 0.f };
    bool facingRight = true;
    bool isGrounded = true; 
    const float LEFT_BOUNDARY = 1.0f;

    int maxHealth;
    int currentHealth;

    float moveSpeed = 300.f;
    float jumpForce = 700.f;
    float gravity = 1800.f; 
    const float groundLevel = 455.f;

    bool canDash = true;
    float dashSpeed = 700.f;
    float dashDuration = 0.15f; 
    float dashTimer = 0.f;
    float dashCooldown = 0.8f; 
    float dashCooldownTimer = 0.f;

    bool canAttack = true;
    float attackCooldown = 0.4f;
    float attackTimer = 0.f;

    bool canParry = true;
    float parryCooldown = 1.0f;
    float parryTimer = 0.f; 

    std::random_device rd;
    std::mt19937 rng;
    std::uniform_int_distribution<int> attackDistribution;
    std::vector<AnimationState> attackStates; 

    // check if currently in any attack animation
    bool isAttacking() const {
        AnimationState current = animations.getCurrentState();
        return current == AnimationState::Attack1 ||
            current == AnimationState::Attack2 ||
            current == AnimationState::Attack3;
    }

    void handleMovement(float dt) {
        // Apply gravity ONLY if not dashing
        if (dashTimer <= 0) {
            velocity.y += gravity * dt;
        }

        // Calculate proposed next position
        sf::Vector2f proposedPosition = position + velocity * dt;

        // --- Collision Detection ---

        // 1. Ground Collision
        if (proposedPosition.y >= groundLevel) {
            proposedPosition.y = groundLevel;
            if (velocity.y > 0) { // Only stop downward velocity and set grounded if falling onto ground
                velocity.y = 0;
                isGrounded = true;
                if (dashCooldownTimer <= 0) { // Only reset if not already on cooldown
                    canDash = true;
                }
            }
        }
        else {
            isGrounded = false;
        }

        // 2. Left Boundary Collision
        const float halfWidth = animations.getSprite().getLocalBounds().width / 6.0f; 
        if (proposedPosition.x - halfWidth < LEFT_BOUNDARY) {
            proposedPosition.x = LEFT_BOUNDARY + halfWidth;
            velocity.x = 0; // Stop horizontal movement when hitting left wall
        }

        // --- Update Position ---
        position = proposedPosition;
        animations.getSprite().setPosition(position);

        // Flip sprite based on movement direction, but NOT during attacks/dash/parry
        if (!isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry) {
            float currentScaleX = animations.getSprite().getScale().x;
            if (velocity.x > 0.1f && currentScaleX < 0.f) { // Moving right, facing left
                animations.getSprite().setScale(1.f, 1.f);
                facingRight = true;
            }
            else if (velocity.x < -0.1f && currentScaleX > 0.f) { // Moving left, facing right
                animations.getSprite().setScale(-1.f, 1.f);
                facingRight = false;
            }
        }
        else {
            // Ensure facing direction variable matches sprite scale even during actions
            facingRight = animations.getSprite().getScale().x > 0.f;
        }
    }

    void handleAnimations() {
        AnimationState currentState = animations.getCurrentState();
        AnimationState newState = currentState; // Start assuming no change

        if (currentState == AnimationState::Dead) {
            return; // Do not change animation if dead
        }

        // Allow uninterruptible, non-looping animations (Attack, Dash, Parry, Jump) to finish
        bool isUninterruptibleAction = isAttacking() || currentState == AnimationState::Dash || currentState == AnimationState::Parry;

        if (isUninterruptibleAction) {
            if (animations.isDone()) {
                newState = AnimationState::Idle; // Revert to idle after finishing the action
            }
            else {
                return; // Keep playing the current action animation
            }
        }


        // Determine new state based on physics/actions only if not locked in an action
        if (!isUninterruptibleAction) // Don't check movement states if mid-attack/dash/parry
        {
            if (!isGrounded) {            
                    newState = AnimationState::Jump;            
            }
            else { // Player is grounded
                 if (std::abs(velocity.x) > 10.f) {
                    newState = AnimationState::Run;
                }
                else {
                    newState = AnimationState::Idle;
                }
            }
        }

        animations.play(newState);
    }

    void updateTimers(float dt) {
        // Dash Timer (Duration)
        if (dashTimer > 0) {
            dashTimer -= dt;
            if (dashTimer <= 0) {
                dashTimer = 0;
            }
        }

        // Dash Cooldown Timer
        if (dashCooldownTimer > 0) {
            dashCooldownTimer -= dt;
            if (dashCooldownTimer <= 0) {
                dashCooldownTimer = 0;   
                canDash = true;
            }
        }

        // Attack Cooldown Timer
        if (attackTimer > 0) {
            attackTimer -= dt;
            if (attackTimer <= 0) {
                attackTimer = 0;
                canAttack = true;
            }
        }

        // Parry Cooldown Timer
        if (parryTimer > 0) {
            parryTimer -= dt;
            if (parryTimer <= 0) {
                parryTimer = 0;
                canParry = true;
            }
        }
    }
};


// --- Game Class ---
class BossGame {
public:
    BossGame() : window(sf::VideoMode(1280, 720), "Final Boss"), player()
    {
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(true); 
        loadResources();
        initViews();
        initBackground();
        setupUI();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            // Clamp delta time to prevent large jumps if debugging/stalling
            if (dt > 0.1f) dt = 0.1f;

            processInput();
            update(dt);
            render();
        }
    }

private:
    sf::RenderWindow window;
    sf::View gameView;
    sf::Sprite background;
    Player player;


    sf::RectangleShape playerHealthBarBackground;
    sf::RectangleShape playerHealthBarFill;
    const float HEALTH_BAR_WIDTH = 300.f;
    const float HEALTH_BAR_HEIGHT = 20.f;
    const float HEALTH_BAR_PADDING = 10.f;
    const float HEALTH_BAR_POS_X = 25.f; // Top-left corner X position
    const float HEALTH_BAR_POS_Y = 25.f; // Top-left corner Y position

    void loadResources() {
        TextureManager::instance().load("background", "../assets/background.png");
    }

    void initViews() {
        gameView = window.getDefaultView();
    }

    void initBackground() {
        sf::Texture* bgTexture = TextureManager::instance().get("background");
        if (bgTexture) {
            background.setTexture(*bgTexture);
            // Scale background to fit window size exactly
            auto textureSize = bgTexture->getSize();
            if (textureSize.x > 0 && textureSize.y > 0) {
                background.setScale(
                    static_cast<float>(window.getSize().x) / textureSize.x,
                    static_cast<float>(window.getSize().y) / textureSize.y
                );
            }
            // Set origin to top-left (default)
            background.setPosition(0, 0);
        }
        else {
            std::cerr << "[Game] Failed to set background texture." << std::endl;
        }
    }

    void setupUI() {
        playerHealthBarBackground.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT));
        playerHealthBarBackground.setFillColor(sf::Color(50, 50, 50, 200)); // Dark grey background
        playerHealthBarBackground.setOutlineColor(sf::Color::Black);
        playerHealthBarBackground.setOutlineThickness(2.f);
        playerHealthBarBackground.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y); // Position in screen space

        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT)); // Initial full size
        playerHealthBarFill.setFillColor(sf::Color(0, 200, 0, 220)); // Bright green
        playerHealthBarFill.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y); // Position exactly over background
    }

    void processInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Handle key presses for single actions (jump, attack, dash, parry, debug)
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Space:
                    player.jump();
                    break;
                case sf::Keyboard::E: 
                    player.attack();
                    break;
                case sf::Keyboard::LShift:
                case sf::Keyboard::RShift:
                    player.dash();
                    break;
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                case sf::Keyboard::Q: 
                    player.parry();
                    break;
                case sf::Keyboard::T: // Debug: take damage
                    player.takeDamage(10);
                    break;
                    // *** ADDED: N key press to trigger death DEBUG ***
                case sf::Keyboard::N:
                    std::cout << "[Game] N key pressed - Forcing death state." << std::endl;
                    player.death(); 
                    break;

                default:
                    break;
                }
            }

            // Handle mouse button presses for actions
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    player.attack();
                }
                else if (event.mouseButton.button == sf::Mouse::Right) {
                    player.parry();
                }
            }
        }

        // Handle continuous key holds for movement (outside the event loop)
        if (player.isAlive()) {
            sf::Vector2f movement(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) movement.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) movement.x += 1.f;
            player.move(movement); // Pass direction based on held keys
        }
        else {
            //  Ensure movement stops completely if dead
            player.move({ 0.f, 0.f });
        }
    }

    void updateUI() {
        // Calculate health percentage
        float healthPercent = 0.f;
        if (player.getMaxHealth() > 0) { // Avoid division by zero
            healthPercent = static_cast<float>(player.getHealth()) / player.getMaxHealth();
        }
        healthPercent = std::max(0.f, healthPercent); // Ensure percent doesn't go below 0

        // Update the fill bar width
        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH * healthPercent, HEALTH_BAR_HEIGHT));

        // Change color based on health
        if (healthPercent <= 0.f) { // Explicitly check for 0 or less
            playerHealthBarFill.setFillColor(sf::Color(50, 50, 50, 200)); // Match background when dead? Or hide?
        }
        else if (healthPercent < 0.33f) {
            playerHealthBarFill.setFillColor(sf::Color(220, 0, 0, 220)); // Red when low
        }
        else if (healthPercent < 0.66f) {
            playerHealthBarFill.setFillColor(sf::Color(220, 220, 0, 220)); // Yellow when medium
        }
        else {
            playerHealthBarFill.setFillColor(sf::Color(0, 200, 0, 220)); // Green when high
        }
    }

    void update(float dt) {
        player.update(dt);
        updateUI(); // Update health bar
    }

    void render() {
        window.clear(sf::Color::Cyan); // Fallback background color

        // Draw game world elements using the game view
        window.setView(gameView);
        if (background.getTexture()) {
            window.draw(background);
        }
        player.draw(window);
        // Draw other game elements (enemies, platforms, etc.) here

        // Draw UI elements using the default view (screen coordinates)
        window.setView(window.getDefaultView());
        window.draw(playerHealthBarBackground);
        window.draw(playerHealthBarFill);
        // Draw other UI elements (score, timer, etc.) here

        window.display();
    }
};

int main() {
    BossGame game;
    game.run();
    return 0;
}