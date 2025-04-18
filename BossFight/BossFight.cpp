#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>     
#include <random>    
#include <vector>    

// --- Texture Manager  ---
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
    Idle, Run, Jump, Fall, Attack1, Attack2, Attack3, Parry, Dash, Dead, None
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
    Player(int maxHealth = 100,int currentHealth=100) : maxHealth(maxHealth), currentHealth(currentHealth) {
        loadResources();
        initAnimations();
        position = { 200.f, 500.f };
        animations.getSprite().setPosition(position);

        rng = std::mt19937(rd()); // Seed the random number generator
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
        animations.addAnimation(AnimationState::Dead, "player_dead", 3, 0.4f, { 128, 128 }, false);
        animations.play(AnimationState::Idle);
    }

    void update(float dt) {
        handleMovement(dt);
        handleAnimations(); 
        updateTimers(dt);
        animations.update(dt);
    }

    void move(sf::Vector2f direction) {
        // *** Updated to check against multiple attack states ***
        if (dashTimer <= 0 && !isAttacking()) {
            if (direction.x != 0.f) {
                velocity.x = direction.x > 0 ? moveSpeed : -moveSpeed;
                facingRight = direction.x > 0;
            }
            else {
                velocity.x = 0.f;
            }
        }
        else if (dashTimer <= 0) { // If attacking
            if (isGrounded) {
                velocity.x = 0.f;
            }
        }
    }

    void jump() {
        // *** Updated to check against multiple attack states ***
        if (isGrounded && !isAttacking() && dashTimer <= 0) {
            velocity.y = -jumpForce;
            isGrounded = false;
            // Play jump animation immediately upon initiating jump
            animations.play(AnimationState::Jump);
        }
    }

    void dash() {
        // *** Updated to check against multiple attack states ***
        if (canDash && dashTimer <= 0 && !isAttacking()) {
            velocity.x = facingRight ? dashSpeed : -dashSpeed;
            velocity.y = 0;
            dashTimer = dashDuration;
            canDash = false;
            animations.play(AnimationState::Dash);
        }
    }

    void parry() {
        if (canParry && !isAttacking() && dashTimer <= 0) {
            animations.play(AnimationState::Parry);
            canParry = false;
            parryTimer = parryCooldown;
            // Add parry effect logic here
        }
    }

    void attack() {
        // *** Updated to check against multiple attack states ***
        if (canAttack && !isAttacking() && dashTimer <= 0) {
            int attackIndex = attackDistribution(rng); // Generate 0, 1, or 2 for 3 different attack states
            AnimationState selectedAttackState = attackStates[attackIndex];

            animations.play(selectedAttackState); // Play the chosen attack animation
            canAttack = false;
            attackTimer = attackCooldown;
            if (isGrounded) {
                velocity.x = 0;
            }
            // std::cout << "[Player] Attack " << attackIndex + 1 << "!" << std::endl;
        }
    }

    void takeDamage(int amount) {
        if (!isAlive()) return; // Already dead
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
    bool isGrounded = false;
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
    std::vector<AnimationState> attackStates; // To easily pick state by index

    // Helper to check if currently in any attack animation
    bool isAttacking() const {
        AnimationState current = animations.getCurrentState();
        return current == AnimationState::Attack1 ||
            current == AnimationState::Attack2 ||
            current == AnimationState::Attack3;
    }

    void handleMovement(float dt) {
        if (dashTimer <= 0) {
            velocity.y += gravity * dt;
        }

        sf::Vector2f proposedPosition = position + velocity * dt;

        
        const float halfWidth = 20.0;  //left boundary
        if (proposedPosition.x - halfWidth < LEFT_BOUNDARY) {
            proposedPosition.x = LEFT_BOUNDARY + halfWidth;
            velocity.x = 0;  // Stop horizontal movement when hitting wall
        }

        
        position = proposedPosition;


        if (position.y >= groundLevel) {
            position.y = groundLevel;
            if (velocity.y > 0) {
                velocity.y = 0;
                isGrounded = true;
            }
        }
        else {
            isGrounded = false;
        }

        animations.getSprite().setPosition(position);

        float currentScaleX = animations.getSprite().getScale().x;
        if (facingRight && currentScaleX < 0.f) {
            animations.getSprite().setScale(1.f, 1.f);
        }
        else if (!facingRight && currentScaleX > 0.f) {
            animations.getSprite().setScale(-1.f, 1.f);
        }
    }

    void handleAnimations() {
        AnimationState currentState = animations.getCurrentState();
        AnimationState newState = currentState; // Start assuming no change

        // Allow non-looping animations (Attack, Dash, Jump, Fall) to finish
        if (isAttacking() || currentState == AnimationState::Dash || currentState == AnimationState::Jump || currentState == AnimationState::Fall) {
            if (animations.isDone()) {
                newState = AnimationState::Idle; // Revert to idle after finishing
            }
            else {
                return; // Keep playing the current non-looping animation
            }
        }

        // Determine new state based on physics/actions only if not in an uninterruptible animation
        if (!isAttacking() && currentState != AnimationState::Dash) {
            if (!isGrounded) {
                // Use Jump state for upward movement, Fall state for downward
                if (velocity.y < 0) {
                    newState = AnimationState::Jump;
                }
                else {
                    newState = AnimationState::Fall;
                }
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

		if (currentState == AnimationState::Parry && !animations.isDone()) {
			return; // Don't change state if parrying
		}

        // Play the new state animation (play() handles not restarting the same animation)
        animations.play(newState);
    }

    void updateTimers(float dt) {
        if (dashTimer > 0) {
            dashTimer -= dt;
            if (dashTimer <= 0) {
                dashTimer = 0;
                if (std::abs(velocity.x) > moveSpeed) {
                    velocity.x = 0;
                }
                dashCooldownTimer = dashCooldown;
            }
        }
        else if (dashCooldownTimer > 0) {
            dashCooldownTimer -= dt;
            if (dashCooldownTimer <= 0) {
                dashCooldownTimer = 0;
                canDash = true;
            }
        }

        if (attackTimer > 0) {
            attackTimer -= dt;
            if (attackTimer <= 0) {
                attackTimer = 0;
                canAttack = true;
            }
        }

        if (parryTimer > 0) {
            parryTimer -= dt;
            if (parryTimer <= 0) {
                canParry = true;
            }
        }
    }
};


// --- Game Class ---
class BossGame {
public:
    BossGame() : window(sf::VideoMode(1280, 720), "SFML Platformer"), player()
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
    const float HEALTH_BAR_POS_X = 25.f; //  X position
    const float HEALTH_BAR_POS_Y = 25.f; //  Y position
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
            auto textureSize = bgTexture->getSize();
            if (textureSize.x > 0 && textureSize.y > 0) {
                background.setScale(
                    static_cast<float>(window.getSize().x) / textureSize.x,
                    static_cast<float>(window.getSize().y) / textureSize.y
                );
            }
        }
        else {
            std::cerr << "[Game] Failed to set background texture." << std::endl;
        }
    }

    void setupUI() {
        playerHealthBarBackground.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT));
        playerHealthBarBackground.setFillColor(sf::Color(50, 50, 50, 200));
        playerHealthBarBackground.setOutlineColor(sf::Color::Black);
        playerHealthBarBackground.setOutlineThickness(2.f);
        playerHealthBarBackground.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y); //position of health bar

        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT));
        playerHealthBarFill.setFillColor(sf::Color(0, 200, 0, 220));
        playerHealthBarFill.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y);
        
    }

    void processInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Space:
                    player.jump();
                    break;

                case sf::Keyboard::E:
                    player.attack();
                    break;

                case sf::Keyboard::LShift:
                    player.dash();
                    break;

                case sf::Keyboard::Escape:
                    window.close();
                    break;

                case sf::Keyboard::Q:
                    player.parry();
                    break;

                case sf::Keyboard::T:
                    player.takeDamage(10); // Debugging: take 10 damage
                    break;

                default:
                    break;
                }
            }


            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    player.attack();
                }
                else if (event.mouseButton.button == sf::Mouse::Right) {
                    player.parry();
                }
            }
        }


        sf::Vector2f movement(0.f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) movement.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) movement.x += 1.f;
        player.move(movement);
    }
    void updateUI() {
        // Calculate health percentage
        float healthPercent = static_cast<float>(player.getHealth()) / player.getMaxHealth();
        healthPercent = std::max(0.f, healthPercent); // Ensure percent doesn't go below 0

        // Update the fill bar width
        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH * healthPercent, HEALTH_BAR_HEIGHT));

        // Optional: Change color based on health
        if (healthPercent < 0.33f) {
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
        updateUI();
		if (player.getHealth() <= 0) {
			std::cout << "[Game] Player is dead!" << std::endl;
            
		}
    }

    void render() {
        window.clear(sf::Color::Cyan);
        window.setView(gameView);

        if (background.getTexture()) {
            window.draw(background);
        }

        player.draw(window);
        window.setView(window.getDefaultView());
        window.draw(playerHealthBarBackground);
        window.draw(playerHealthBarFill);
        window.display();
    }
};


// --- Main Function ---
int main() {
    BossGame game;
    game.run();
    return 0;
}
