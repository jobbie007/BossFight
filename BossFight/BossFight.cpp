#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <random>
#include <list>

// --- Texture Manager ---
class TextureManager {
    std::map<std::string, sf::Texture> textures;
public://allows one local TextureManager globally
    static TextureManager& instance() {
        static TextureManager tm;
        return tm;
    }

    bool load(const std::string& id, const std::string& path) {
        sf::Texture texture;
        if (texture.loadFromFile(path)) {
            textures.emplace(id, std::move(texture));
            return true;
        }
        return false;
    }

    sf::Texture* get(const std::string& id) {
        auto it = textures.find(id);
        if (it != textures.end())
            return &it->second;
        return nullptr;
    }
};

// --- Sound Manager ---
class SoundManager {
public:
    // Static instance method for Singleton pattern
    static SoundManager& instance() {
        static SoundManager manager; // Instantiated on first call
        return manager;
    }

    // Load a sound buffer from file and store it directly in the map
    bool load(const std::string& id, const std::string& filename) {
        sf::SoundBuffer buffer; // Create buffer on the stack first
        if (!buffer.loadFromFile(filename)) {
            std::cerr << "[SoundManager] Error loading sound: " << filename << std::endl;
            return false;
        }
        // Move the loaded buffer into the map
        soundBuffers.emplace(id, std::move(buffer));
        std::cout << "[SoundManager] Loaded sound: " << filename << " as '" << id << "'" << std::endl;
        return true;
    }

    // Get a pointer to a sound buffer by ID
    sf::SoundBuffer* get(const std::string& id) {
        auto it = soundBuffers.find(id);
        if (it != soundBuffers.end()) {
            // Return the address of the SoundBuffer stored in the map
            return &it->second;
        }
        std::cerr << "[SoundManager] Error: Sound buffer not found: " << id << std::endl;
        return nullptr;
    }

    // Play a sound by ID using a managed sf::Sound instance
    void play(const std::string& id, float volume = 15.f) {
        sf::SoundBuffer* buffer = get(id);
        if (!buffer) {
            std::cerr << "[SoundManager] Cannot play sound: Buffer '" << id << "' not found." << std::endl;
            return;
        }

        // --- Sound Instance Management ---
        // 1. Clean up sounds that have finished playing to reuse resources.
        //    Iterate and remove sounds whose status is Stopped.
        sounds.remove_if([](const sf::Sound& s) {
            return s.getStatus() == sf::Sound::Stopped;
            });

        // 2. Create a new sound instance in the list.
        //    emplace_back constructs the sf::Sound directly in the list.
        sounds.emplace_back(*buffer); // Pass the found buffer to the constructor

        // 3. Configure and play the newly added sound (it's always at the back).
        sounds.back().setVolume(std::clamp(volume, 0.f, 100.f)); // Ensure volume is valid
        sounds.back().play();
    }

    void setLoop(const std::string& id, bool loop) {
        auto it = soundBuffers.find(id);
        if (it != soundBuffers.end()) {
            // Create a temporary sf::Sound instance to configure looping  
            sf::Sound tempSound(it->second);
            tempSound.setLoop(loop);
        }
        else {
            std::cerr << "[SoundManager] Error: Sound buffer not found: " << id << std::endl;
        }
    }

private:
    SoundManager() = default;
    // Private destructor (ensures deletion only through static instance lifecycle)
    ~SoundManager() = default;

    // Map to store the loaded sound buffers directly
    std::map<std::string, sf::SoundBuffer> soundBuffers;

    // List to manage currently active/playing sound instances (sf::Sound)
    // This is necessary because sf::SoundBuffer only holds data, sf::Sound plays it.
    std::list<sf::Sound> sounds;
};

// --- Animation State Enum ---
enum class AnimationState {
    // Player States
    Idle, Run, Jump, Attack1, Attack2, Attack3, Parry, Dash, Dead, Hurt, Shoot,
    // Boss States
    BossIdle, BossAttack1, BossAttack2, BossAttack3, BossUltimate, BossHurt, BossDead,
    None
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

    void addAnimation(AnimationState state, const std::string& textureId, int frames, float duration, sf::Vector2i size, bool loop) {
        if (auto tex = TextureManager::instance().get(textureId)) {
            animations.emplace(state, Animation{ tex, frames, duration, size, loop });
        }
        else {
            std::cerr << "[AnimationComponent] Error: Texture " << textureId << " not found." << std::endl;
        }
    }

    void update(float dt) {
        auto it = animations.find(currentState);
        if (it == animations.end()) return;
        auto& anim = it->second;
        if (!anim.texture || (!anim.loops && _isDone)) return;

        elapsedTime += dt;
        if (elapsedTime < anim.frameDuration) return;

        int advance = int(elapsedTime / anim.frameDuration);
        elapsedTime = std::fmod(elapsedTime, anim.frameDuration);
        currentFrame += advance;

        if (currentFrame >= anim.frameCount) {
            if (anim.loops) {
                currentFrame %= anim.frameCount;
            }
            else {
                currentFrame = anim.frameCount - 1; // Stay on last frame
                _isDone = true;
            }
        }

        sprite.setTextureRect({ anim.frameSize.x * currentFrame, 0,anim.frameSize.x, anim.frameSize.y });
    }

    void play(AnimationState state) {
        auto it = animations.find(state);
        if (it == animations.end()) return;
        auto& anim = it->second;
        if (state == currentState && anim.loops) return;

        currentState = state;
        currentFrame = 0;
        elapsedTime = 0.f;
        _isDone = false;

        sprite.setTexture(*anim.texture);
        sprite.setTextureRect({ 0,0, anim.frameSize.x, anim.frameSize.y });
        sprite.setOrigin(anim.frameSize.x / 2.f, anim.frameSize.y / 2.f);
    }

    bool isDone() const {
        if (auto it = animations.find(currentState); it != animations.end()) {
            return !it->second.loops && _isDone;
        }
        return true;
    }

    AnimationState getCurrentState() const { return currentState; }
    sf::Sprite& getSprite() { return sprite; }
    const sf::Sprite& getSprite() const { return sprite; }

    //get functions
    int getCurrentFrameIndex() const { return currentFrame; }
    float getElapsedTimeInState() const { return elapsedTime + currentFrame * (animations.count(currentState) ? animations.at(currentState).frameDuration : 0.f); }


private:
    sf::Sprite sprite;
    std::map<AnimationState, Animation> animations;
    AnimationState currentState = AnimationState::None;
    int currentFrame = 0;
    float elapsedTime = 0.f;
    bool _isDone = false; // Internal flag for non-looping animations
};

// --- Projectile Class ---
class Projectile {
public:
    enum class Owner { PLAYER, BOSS };

    Projectile(sf::Texture* tex, sf::Vector2f startPos, sf::Vector2f vel, Owner own, int dmg) :
        texture(tex), position(startPos), velocity(vel), owner(own), damage(dmg), active(true)
    {
        if (texture) {
            sprite.setTexture(*texture);
            sprite.setOrigin(texture->getSize().x / 2.f, texture->getSize().y / 2.f);
            sprite.setPosition(position);
        }
        else {
            std::cerr << "[Projectile] Error: Null texture provided." << std::endl;
            active = false; // Deactivate if no texture
        }
    }

    //  Update projectile position
    void update(float dt) {
        if (!active) return;
        position += velocity * dt;
        sprite.setPosition(position);
    }

    // Draw the projectile
    void draw(sf::RenderTarget& target) const {
        if (active && texture) {
            target.draw(sprite);
        }
    }

    //  Get collision bounds
    sf::FloatRect getGlobalBounds() const {
        return sprite.getGlobalBounds();
    }

    //  Check if active
    bool isActive() const {
        return active;
    }

    // Deactivate the projectile ( on hit or off screen)
    void setActive(bool status) {
        active = status;
    }

    //  Get the owner
    Owner getOwner() const {
        return owner;
    }

    // Get the damage value
    int getDamage() const {
        return damage;
    }

    //  Get current position (for boundary checks)
    sf::Vector2f getPosition() const {
        return position;
    }


private:
    sf::Sprite sprite;
    sf::Texture* texture = nullptr;
    sf::Vector2f position;
    sf::Vector2f velocity;
    Owner owner;
    int damage;
    bool active;
};

// --- Player Class ---
class Player {
public:
    Player(int maxHealth = 100, int currentHealth = 100,
        bool enableDash = true, float parrySuccessTime = 0.8f,
        int baseAttackDamage = 20, float defensePercent = 0.0f,
        bool enableShoot = true) :
        maxHealth(maxHealth),
        currentHealth(currentHealth),
        enableDash(enableDash),
        parrySuccessDuration(parrySuccessTime),
        attackDamage(baseAttackDamage),
        defensePercent(std::clamp(defensePercent, 0.0f, 1.0f)),
        enableShoot(enableShoot),
        rng(std::random_device{}())
    {
        loadResources();
        initAnimations();
        position = { 200.f, 500.f };
        animations.getSprite().setPosition(position);

        // Initialize RNG distribution using member rng
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
        tm.load("player_hurt", "../assets/player/Hurt.png");
        tm.load("player_projectile", "../assets/player/Projectile.png");
        tm.load("player_shoot", "../assets/player/Shoot.png");
    }

    void initAnimations() {
        // Adjust frame counts, durations, sizes, loops assets
        animations.addAnimation(AnimationState::Idle, "player_idle", 8, 0.2f, { 160, 128 }, true);
        animations.addAnimation(AnimationState::Run, "player_run", 8, 0.1f, { 160, 128 }, true);
        animations.addAnimation(AnimationState::Attack1, "player_attack1", 6, 0.05f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Attack2, "player_attack2", 10, 0.035f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Attack3, "player_attack3", 16, 0.019f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Jump, "player_jump", 11, 0.08f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Dash, "player_dash", 5, 0.036f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Parry, "player_parry", 6, 0.08f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Dead, "player_dead", 7, 0.2f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Hurt, "player_hurt", 2, hurtDuration, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Shoot, "player_shoot", 13, 0.05f, { 160, 128 }, false);
        animations.play(AnimationState::Idle);
    }

    void update(float dt) {

        if (isHurt) {
            hurtTimer -= dt;
            hurtFlashIntervalTimer -= dt;

            // Handle flashing
            if (hurtFlashIntervalTimer <= 0.f) {
                hurtFlashIntervalTimer = hurtFlashInterval;
                animations.getSprite().setColor(
                    (animations.getSprite().getColor() == defaultColor) ? damageColor : defaultColor
                );
            }

            // Check if hurt duration is over
            if (hurtTimer <= 0.f) {
                isHurt = false;
                hurtTimer = 0.f;
                animations.getSprite().setColor(defaultColor); //Reset color
                // std::cout << "[Player] Hurt state ended." << std::endl;
                velocity.x = 0; // set 0 so player doesn't turn around
            }
            else {
                // While hurt, apply physics (gravity, friction from knockback) but block input
                handleMovement(dt); // Apply gravity and update position based on knockback velocity
                animations.update(dt); // Update the Hurt animation frame
                return; // Skip rest of normal update logic while hurt
            }
        }

        // --- Normal Update Logic (only if not hurt) ---
        handleMovement(dt);
        handleAnimations();
        updateTimers(dt);
        animations.update(dt);

        if (animations.getCurrentState() == AnimationState::Shoot) {
            if (animations.getCurrentFrameIndex() >= SHOOT_FRAME_TRIGGER &&
                !hasShotDuringAnimation) {
                shootTriggered = true;
                hasShotDuringAnimation = true;
            }
        }
    }

    sf::Vector2f getPosition() const {
        return animations.getSprite().getPosition();
    }

    void move(sf::Vector2f direction) {
        if (isHurt || !isAlive()) return; // Block input if hurt or dead

        // Allow movement unless dashing or attacking/parrying on the ground
        if (dashTimer <= 0 && !(isGrounded && (isAttacking() || animations.getCurrentState() == AnimationState::Parry))) {
            if (direction.x != 0.f) {
                velocity.x = direction.x > 0 ? moveSpeed : -moveSpeed;
            }
            else {
                velocity.x = 0.f; // Explicitly stop if no horizontal input
            }
        }
        else if (isGrounded && (isAttacking() || animations.getCurrentState() == AnimationState::Parry)) {
            velocity.x = 0.f; // Stop horizontal movement when attacking/parrying on ground
        }
    }

    void jump() {
        if (isHurt || !isAlive()) return;
        SoundManager::instance().play("player_jump");
        if (isGrounded && !isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry) {
            velocity.y = -jumpForce;
            isGrounded = false;
        }
    }

    void death() {
        if (animations.getCurrentState() == AnimationState::Dead) return;

        animations.play(AnimationState::Dead);
        velocity.x = 0; // Stop movement
        velocity.y = 0; // Stop gravity/jump
        currentHealth = 0; // Ensure health is set to 0
        isGrounded = true; // Keep death anim on ground
        isHurt = false; // Not hurt anymore
        animations.getSprite().setColor(defaultColor); // Reset color
        // std::cout << "[Player] Death triggered!" << std::endl;
    }

    void dash() {
        if (isHurt || !isAlive() || !enableDash) return;

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
        if (isHurt || !isAlive()) return;
        SoundManager::instance().play("player_parry");
        if (canParry && !isAttacking() && dashTimer <= 0) {
            animations.play(AnimationState::Parry);
            canParry = false;
            parryTimer = parryCooldown;
            parrySuccessWindow = parrySuccessDuration;
            if (isGrounded) {
                velocity.x = 0; // Stop movement while parrying on ground
            }
        }
    }

    void attack() {
        if (isHurt || !isAlive()) return;

        if (canAttack && !isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry) {
            int attackIndex = attackDistribution(rng); // Generate 0, 1, or 2
            AnimationState selectedAttackState = attackStates[attackIndex];
            SoundManager::instance().play("player_attack");
            animations.play(selectedAttackState); // Play the chosen attack animation
            canAttack = false; // Prevent attacking again until cooldown finishes
            attackTimer = attackCooldown; // Start cooldown timer

            if (isGrounded) {
                velocity.x = 0; // Stop horizontal movement while attacking on ground
            }
            // std::cout << "[Player] Attack " << attackIndex + 1 << "!" << std::endl;
        }
    }

    void shoot() {
        if (isHurt || !isAlive() || !canShoot || isAttacking() || animations.getCurrentState() == AnimationState::Parry || dashTimer > 0 || !enableShoot) return;
        SoundManager::instance().play("player_shoot");
        canShoot = false;
        shootTimer = shootCooldown;
        animations.play(AnimationState::Shoot);
        hasShotDuringAnimation = false;
        // std::cout << "[Player] Shoot initiated!" << std::endl;
    }

    bool wantsToShoot() {
        if (shootTriggered) {
            shootTriggered = false; // Reset after checking  
            return true; // Indicate that the player wants to shoot  
        }
        return false;
    }

    void takeDamage(int amount) {
        // Prevent taking damage if already dead, or already in the hurt state
        if (!isAlive() || animations.getCurrentState() == AnimationState::Dead || isHurt) {
            return;
        }
        SoundManager::instance().play("player_damaged");
        int actualDamage = static_cast<int>(amount * (1 - defensePercent));
        currentHealth -= actualDamage;
        currentHealth = std::max(0, currentHealth);
        std::cout << "[Player] Took " << actualDamage << " damage. Health: " << currentHealth << "/" << maxHealth << std::endl;
        if (isAlive()) {
            isHurt = true;
            hurtTimer = hurtDuration;
            hurtFlashIntervalTimer = 0.f; // Start flash immediately
            animations.getSprite().setColor(damageColor); // Start flash with red color
            animations.play(AnimationState::Hurt);

            velocity.x = -knockbackForceX; //knock back always to the left
            velocity.y = knockbackForceY;
            isGrounded = false; // Become airborne

            // std::cout << "[Player] Hurt state applied with knockback!" << std::endl;
        }
        else {
            death(); // Call the death function if health is 0 or less
        }
    }

    int getAttackDamage() const { return attackDamage; }
    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool isAlive() const { return currentHealth > 0; }

    // Use visual bounds for collision checks
    sf::FloatRect getGlobalBounds() const {
        return animations.getSprite().getGlobalBounds();
    }

    void draw(sf::RenderTarget& target) const {
        target.draw(animations.getSprite());
    }

    bool getFaceingRight() { return facingRight; }

    // Added helper for collision detection logic
    bool isAttacking() const {
        AnimationState current = animations.getCurrentState();
        /* int currentFrame = animations.getCurrentFrameIndex();
         if (current == AnimationState::Attack1 && currentFrame >= 0 && currentFrame <= 6 ||
             current == AnimationState::Attack2 && currentFrame >= 0 && currentFrame <= 6 ||
             current == AnimationState::Attack3 && currentFrame >= 0 && currentFrame <= 6) {
             return true;
         }
         else return false;*/

        return current == AnimationState::Attack1 ||
            current == AnimationState::Attack2 ||
            current == AnimationState::Attack3;

    }

    bool isParryProtected() const {
        return parrySuccessWindow > 0.0f || animations.getCurrentState() == AnimationState::Parry
            || animations.getCurrentState() == AnimationState::Dash;
    }

private:
    AnimationComponent animations;
    sf::Vector2f position = { 0.f, 0.f };
    sf::Vector2f velocity = { 0.f, 0.f };
    bool facingRight = true;
    bool isGrounded = true;
    const float LEFT_BOUNDARY = 1.0f;
    //input health from constructor
    int maxHealth;
    int currentHealth;

    const float moveSpeed = 300.f;
    const float jumpForce = 700.f;
    const float gravity = 1800.f;
    const float groundLevel = 485.f;

    bool canDash = true;
    float dashSpeed = 800.f;
    float dashDuration = 0.15f;
    float dashTimer = 0.f;
    float dashCooldown = 0.4f;
    float dashCooldownTimer = 0.f;

    bool canAttack = true;
    float attackCooldown = 0.4f;
    float attackTimer = 0.f;

    bool canShoot = true;
    float shootCooldown = 1.f;
    float shootTimer = 0.f;
    bool shootTriggered = false;
    bool hasShotDuringAnimation = false;
    const int SHOOT_FRAME_TRIGGER = 10; // Frame to trigger shooting

    bool canParry = true;
    float parryCooldown = 0.8f;
    float parryTimer = 0.f;
    float parrySuccessWindow = 0.0f;

    //constructor changeable variables
    float parrySuccessDuration;
    int attackDamage;
    float defensePercent;
    bool enableShoot;
    bool enableDash;

    float knockbackForceX = 60.f; // Horizontal knockback speed
    float knockbackForceY = -300.f; // Vertical knockback speed (up)

    bool isHurt = false;
    float hurtDuration = 0.4f;
    float hurtTimer = 0.f;
    float hurtFlashInterval = 0.08f;
    float hurtFlashIntervalTimer = 0.f;
    sf::Color defaultColor = sf::Color::White;
    sf::Color damageColor = sf::Color(255, 80, 80, 230); // Red tint

    std::random_device rd;
    std::mt19937 rng;
    std::uniform_int_distribution<int> attackDistribution;
    std::vector<AnimationState> attackStates;


    void handleMovement(float dt) {
        // Apply gravity ONLY if not dashing
        if (dashTimer <= 0) {
            velocity.y += gravity * dt;
        }

        // Calculate proposed next position
        sf::Vector2f proposedPosition = position + velocity * dt;

        // Ground Collision
        if (proposedPosition.y >= groundLevel) {
            proposedPosition.y = groundLevel;
            if (velocity.y > 0) { // Only stop downward velocity and set grounded if falling onto ground
                velocity.y = 0;
                isGrounded = true;
                // Reset dash on landing if cooldown is over
                if (dashCooldownTimer <= 0) {
                    canDash = true;
                }
            }
        }
        else {
            // If not on ground level, potentially airborne
            // Set isGrounded false only if moving upwards or has significant downward velocity
            if (velocity.y < -0.1f || velocity.y > 0.1f) {
                isGrounded = false;
            }
        }


        const float LEFT_BOUNDARY = 25.0f;  // Hardcoded left limit
        const float RIGHT_BOUNDARY = 1060.0f; // Hardcoded right limit
        // Left boundary check
        if (proposedPosition.x < LEFT_BOUNDARY) {
            proposedPosition.x = LEFT_BOUNDARY;  // Snap to boundary
            if (velocity.x < 0) velocity.x = 0;  // Stop leftward movement
        }
        if (proposedPosition.x > RIGHT_BOUNDARY) { // Right boundary check (by boss)
            proposedPosition.x = RIGHT_BOUNDARY;
            if (velocity.x > 0) velocity.x = 0;
        }

        // --- Update Position ---
        position = proposedPosition;
        animations.getSprite().setPosition(position);

        // Flip sprite based on movement direction, but NOT during attacks/dash/parry/hurt
        if (!isAttacking() && dashTimer <= 0 && animations.getCurrentState() != AnimationState::Parry && !isHurt) {
            float currentScaleX = animations.getSprite().getScale().x;
            // Flip based on velocity
            if (velocity.x > 1.0f && currentScaleX < 0.f) { // Moving right, facing left
                animations.getSprite().setScale(1.f, 1.f);
                facingRight = true;
            }
            else if (velocity.x < -1.0f && currentScaleX > 0.f) { // Moving left, facing right
                animations.getSprite().setScale(-1.f, 1.f);
                facingRight = false;
            }
        }
        else {
            // Ensure facingRight bool matches scale even during actions
            facingRight = animations.getSprite().getScale().x > 0.f;
        }
    }

    void handleAnimations() {
        AnimationState currentState = animations.getCurrentState();
        AnimationState newState = currentState; // Start assuming no change

        if (currentState == AnimationState::Dead) {
            return; // Do not change animation if dead
        }

        if (isHurt) {
            // Ensure hurt animation is playing (or let it finish if non-looping)
            // This state is primarily handled in update()
            if (currentState != AnimationState::Hurt && !animations.isDone()) {
                // If somehow not in Hurt anim but isHurt is true, force it (unless anim just finished)
                // animations.play(AnimationState::Hurt);
            }
            return; // Don't change animation while hurt state is active via hurtTimer
        }


        // Allow uninterruptible, non-looping animations (Attack, Dash, Parry, Jump) to finish
        bool isUninterruptibleAction = isAttacking() || currentState == AnimationState::Dash || currentState == AnimationState::Parry || currentState == AnimationState::Shoot;

        if (isUninterruptibleAction) {
            if (animations.isDone()) {
                newState = isGrounded ? AnimationState::Idle : AnimationState::Jump; // Revert after action
            }
            else {
                return; // Keep playing the current action animation
            }
        }
        else { // If not in an uninterruptible action
            if (!isGrounded) {
                // If airborne and not attacking/dashing/parrying, should be Jumping/Falling
                 // Only switch to Jump if not already in Jump (or falling variant if you add one)
                if (currentState != AnimationState::Jump) {
                    newState = AnimationState::Jump;
                }
            }
            else { // Player is grounded
                // Determine Run or Idle based on horizontal velocity
                if (std::abs(velocity.x) > 10.f) { // Use a threshold
                    newState = AnimationState::Run;
                }
                else {
                    newState = AnimationState::Idle;
                }
            }
        }


        // Only play if the new state is different
        if (newState != currentState) {
            animations.play(newState);
        }
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

                if (isGrounded) {
                    canDash = true && enableDash;
                }
            }
        }
        else {
            // Ensure canDash is true if cooldown is 0 and player is grounded
            if (isGrounded && !canDash) {
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
        //parry no damage window
        if (parrySuccessWindow > 0.0f) {
            parrySuccessWindow -= dt;
        }

        // Shoot Cooldown Timer
        if (shootTimer > 0) {
            shootTimer -= dt;
            if (shootTimer <= 0) {
                shootTimer = 0;
                canShoot = true && enableShoot;
            }
        }
    }
};

// --- Boss Class ---
class Boss {
public:
    enum class BossState {
        Idle, Attacking1, Attacking2, Attacking3, Ultimate, Dead
    };

    Boss(sf::Vector2f startPos, Player* playerTarget, float bLeft, float bRight, int maxHealth = 500) :position(startPos), targetPlayer(playerTarget),
        leftBoundary(bLeft), rightBoundary(bRight), maxHealth(maxHealth), currentHealth(maxHealth), currentState(BossState::Idle), rng(rd())
    {
        loadResources();
        initAnimations();
        animations.getSprite().setPosition(position);
        animations.getSprite().setScale(-1.f, 1.f);

        // Initialize RNG distributions
        actionChoiceDistribution = std::uniform_int_distribution<int>(0, 2); // For 3 attack types (0, 1, 2)
        groundAttackIntervalDistribution = std::uniform_real_distribution<float>(groundAttackIntervalMin, groundAttackIntervalMax);
        groundAttackTimer = groundAttackIntervalDistribution(rng);

        startActionDelay();
    }

    void loadResources() {
        auto& tm = TextureManager::instance();
        tm.load("boss_idle", "../assets/boss/Idle.png");
        tm.load("boss_attack1", "../assets/boss/Attack1.png");
        tm.load("boss_attack2", "../assets/boss/Attack2.png");
        tm.load("boss_attack3", "../assets/boss/Attack3.png");
        tm.load("boss_ultimate", "../assets/boss/Ultimate.png");
        tm.load("boss_dead", "../assets/boss/Dead.png");
        tm.load("boss_projectile_ground", "../assets/boss/Projectile_Ground.png");
        tm.load("boss_projectile_mid", "../assets/boss/Projectile_Mid.png");
        tm.load("boss_projectile_rain", "../assets/boss/Projectile_Rain.png");
    }

    void initAnimations() {
        animations.addAnimation(AnimationState::BossIdle, "boss_idle", 8, 0.15f, { 800, 800 }, true);
        animations.addAnimation(AnimationState::BossAttack1, "boss_attack1", 8, 0.12f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossAttack2, "boss_attack2", 8, 0.12f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossAttack3, "boss_attack3", 2, 0.4f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossUltimate, "boss_ultimate", 2, 0.3f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossDead, "boss_dead", 9, 0.18f, { 800, 800 }, false);
        animations.play(AnimationState::BossIdle);
    }

    void update(float dt) {
        if (currentState == BossState::Dead) {
            animations.update(dt);
            return;
        }

        updateTimers(dt);
        handleFlashing(dt);

        if (currentState == BossState::Idle) { // Only fire ground attack when not busy with other attacks
            groundAttackTimer -= dt;
            if (groundAttackTimer <= 0.f) {
                triggerGroundProjectile = true; // Signal BossGame to spawn
                SoundManager::instance().play("ground_attack", 25);
                groundAttackTimer = groundAttackIntervalDistribution(rng); // Reset timer
            }
        }

        timeSinceLastAction += dt;
        if (currentState == BossState::Idle && timeSinceLastAction >= currentActionDelay) {
            chooseNextAction();
        }

        bool isAttackingState = (currentState == BossState::Attacking1 ||
            currentState == BossState::Attacking2 ||
            currentState == BossState::Attacking3 ||
            currentState == BossState::Ultimate);

        if (isAttackingState && animations.isDone()) {
            setState(BossState::Idle);
            startActionDelay();
            attackActive = false; // Ensure melee attack flag is off
            // Reset projectile flags if needed (though BossGame handles spawn requests)
            triggerMidProjectile = false;
            triggerRainProjectile = false;
        }
        // If an attack is ongoing, check timing for melee hits or projectile spawns
        else if (isAttackingState) {
            checkAttackTiming(); // Modified to handle projectiles
        }

        if ((currentState == BossState::Attacking1 || currentState == BossState::Ultimate) && animations.isDone()) {
            setState(BossState::Idle);
            startActionDelay();
            attackActive = false;
        }
        else if (currentState == BossState::Attacking2 || currentState == BossState::Ultimate) {
            checkAttackTiming();
        }

        animations.update(dt);
        animations.getSprite().setPosition(position);
    }

    void takeDamage(int amount) {
        if (isInvulnerable() || currentState == BossState::Dead) return;

        currentHealth -= amount;
        currentHealth = std::max(0, currentHealth);
        std::cout << "[Boss] Took " << amount << " damage. Health: " << currentHealth << "/" << maxHealth << std::endl;

        // Start damage flash effect
        flashTimer = flashDuration;
        animations.getSprite().setColor(damageColor);

        if (currentHealth <= 0) {
            death();
        }
    }

    void death() {
        if (currentState != BossState::Dead) {
            setState(BossState::Dead);
            animations.play(AnimationState::BossDead);
            currentHealth = 0;
            animations.getSprite().setColor(defaultColor);
            velocity = { 0.f, 0.f };
            attackActive = false;
        }
    }

    void draw(sf::RenderTarget& target) const {
        target.draw(animations.getSprite());
    }

    sf::FloatRect getAttackHitbox() const {

        float width = ATTACK_HITBOX.x;
        return sf::FloatRect(
            position.x + -90 - width / 2,
            position.y - NORMAL_HITBOX.y / 2 + 30,
            width,
            NORMAL_HITBOX.y
        );
    }

    sf::FloatRect getGlobalBounds() const {
        bool isAttacking = currentState == BossState::Attacking1 ||
            currentState == BossState::Ultimate;

        float width = isAttacking ? ATTACK_HITBOX.x : NORMAL_HITBOX.x;
        float height = NORMAL_HITBOX.y;

        // Calculate horizontal position based on direction and attack state
        float xPosition = position.x;
        if (isAttacking) {
            // Center of attack hitbox extends 
            xPosition += (-NORMAL_HITBOX.x / 2);
        }

        return sf::FloatRect(
            xPosition - width / 2,
            position.y - height / 2 + 30, // Adjusted since boss height is higher
            width,
            height
        );
    }

    sf::Vector2f getPosition() const { return animations.getSprite().getPosition(); }
    bool isAlive() const { return currentState != BossState::Dead; }
    bool isAttackActive() const { return attackActive; }
    bool isMeleeAttackActive() const { return attackActive; }
    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool wantsToShootGround() const { return triggerGroundProjectile; }
    bool wantsToShootMid() const { return triggerMidProjectile; }
    bool wantsToShootRain() const { return triggerRainProjectile; }

    void resetGroundProjectileRequest() { triggerGroundProjectile = false; }
    void resetMidProjectileRequest() { triggerMidProjectile = false; }
    void resetRainProjectileRequest() { triggerRainProjectile = false; }

private:
    AnimationComponent animations;
    sf::Vector2f position;
    sf::Vector2f velocity = { 0.f, 0.f };
    Player* targetPlayer = nullptr;
    float leftBoundary;
    float rightBoundary;
    int maxHealth;
    int currentHealth;
    BossState currentState = BossState::Idle;

    // Combat parameters
    float currentActionDelay = 2.0f;
    float attackCooldown1 = 1.5f;
    float attackCooldown2 = 2.5f;
    float attackCooldown3 = 3.0f;
    float ultimateCooldown = 8.0f;
    const sf::Vector2f NORMAL_HITBOX = { 150.f, 200.f };
    const sf::Vector2f ATTACK_HITBOX = { 220.f, 200.f }; // Wider hitbox for attacks

    //Tracker variables
    float timeSinceLastAction = 0.f;
    float currentAttackCooldown1 = 0.f;
    float currentAttackCooldown2 = 0.f;
    float currentAttackCooldown3 = 0.f;
    float currentUltimateCooldown = 0.f;
    bool attackActive = false;

    // Ground Attack variables
    float groundAttackTimer = 0.f;
    const float groundAttackIntervalMin = 4.0f;
    const float groundAttackIntervalMax = 8.0f;
    std::uniform_real_distribution<float> groundAttackIntervalDistribution;
    bool triggerGroundProjectile = false;
    bool triggerMidProjectile = false;
    bool triggerRainProjectile = false; // Flag to signal spawn
    float rainSpawnTimer = 0.f; // Timer within ultimate to spawn projectiles
    const float rainSpawnInterval = 0.15f; //how much delay after animation

    // Flash effect
    float flashTimer = 0.f;
    const float flashDuration = 0.3f;
    const float flashInterval = 0.08f;
    float flashIntervalTimer = 0.f;
    sf::Color defaultColor = sf::Color::White;
    sf::Color damageColor = sf::Color(200, 80, 80, 200);

    // Random number generation
    std::random_device rd; //Creates a non-deterministic random bit generator
    std::mt19937 rng;
    std::uniform_int_distribution<int> actionChoiceDistribution;
    std::uniform_int_distribution<int> attackChoiceDistribution;

    bool isInvulnerable() const {
        return currentState == BossState::Attacking1 ||
            currentState == BossState::Attacking2 ||
            currentState == BossState::Attacking3 ||
            currentState == BossState::Ultimate ||
            flashTimer > 0.f;
    }

    void setState(BossState newState) {
        if (currentState != newState) {
            BossState oldState = currentState;
            currentState = newState;

            switch (newState) {
            case BossState::Idle:
                animations.play(AnimationState::BossIdle);
                break;
            case BossState::Attacking1:
                animations.play(AnimationState::BossAttack1);
                SoundManager::instance().play("boss_attack", 70);
                break;
            case BossState::Attacking2:
                animations.play(AnimationState::BossAttack2);
                SoundManager::instance().play("boss_attack", 70);
                break;
            case BossState::Attacking3:
                animations.play(AnimationState::BossAttack3);
                SoundManager::instance().play("fire_spray");
                break;
            case BossState::Ultimate:
                animations.play(AnimationState::BossUltimate);
                SoundManager::instance().play("fire_spray");
                rainSpawnTimer = 0.f; // Reset rain timer on entering ultimate
                break;
            case BossState::Dead:
                // Handled in death() function
                break;
            }
        }
    }

    void startActionDelay() {
        std::uniform_real_distribution<float> delayVar(0.8f, 1.3f);
        currentActionDelay = 1.8f * delayVar(rng);// Randomize delay between 0.8 and 1.3 seconds
        timeSinceLastAction = 0.f;
    }

    void chooseNextAction() {
        if (currentUltimateCooldown <= 0.f && targetPlayer && isAlive()) {
            performUltimate();
            return;
        }

        // Choose randomly between Attack1, Attack2, Attack3
        int action = actionChoiceDistribution(rng); // 0, 1, 2

        // Try the chosen action if cooldown allows, otherwise default to delaying again
        if (action == 0 && currentAttackCooldown1 <= 0.f) performAttack1();
        else if (action == 1 && currentAttackCooldown2 <= 0.f) performAttack2();
        else if (action == 2 && currentAttackCooldown3 <= 0.f) performAttack3();
        else {
            // If chosen action is on cooldown, or invalid, just wait again
            setState(BossState::Idle); // Ensure idle state
            startActionDelay();
        }
    }

    void performAttack1() {
        setState(BossState::Attacking1);
        currentAttackCooldown1 = attackCooldown1;
        timeSinceLastAction = 0.f;
    }

    void performAttack2() {
        setState(BossState::Attacking2);
        currentAttackCooldown2 = attackCooldown2;
        timeSinceLastAction = 0.f;
    }

    void performAttack3() {
        setState(BossState::Attacking3);
        currentAttackCooldown3 = attackCooldown3;
        timeSinceLastAction = 0.f;
    }

    void performUltimate() {
        setState(BossState::Ultimate);
        currentUltimateCooldown = ultimateCooldown;
        timeSinceLastAction = 0.f;
    }

    void updateTimers(float dt) {
        if (currentAttackCooldown1 > 0.f) currentAttackCooldown1 -= dt;
        if (currentAttackCooldown2 > 0.f) currentAttackCooldown2 -= dt;
        if (currentAttackCooldown3 > 0.f) currentAttackCooldown3 -= dt;
        if (currentUltimateCooldown > 0.f) currentUltimateCooldown -= dt;
    }

    void handleFlashing(float dt) {
        if (flashTimer > 0.f) {
            flashTimer -= dt;
            flashIntervalTimer -= dt;

            if (flashIntervalTimer <= 0.f) {
                flashIntervalTimer = flashInterval;
                animations.getSprite().setColor(
                    (animations.getSprite().getColor() == defaultColor) ?
                    damageColor : defaultColor
                );
            }

            if (flashTimer <= 0.f) {
                animations.getSprite().setColor(defaultColor);
            }
        }
    }

    void checkAttackTiming() {
        AnimationState animState = animations.getCurrentState();
        int currentFrame = animations.getCurrentFrameIndex();
        float elapsedInState = animations.getElapsedTimeInState();

        attackActive = false;
        triggerMidProjectile = false;
        triggerRainProjectile = false;

        if (animState == AnimationState::BossAttack1 && currentFrame >= 4 && currentFrame <= 6) {
            attackActive = true;
        }
        else if (animState == AnimationState::BossAttack2 && currentFrame >= 5 && currentFrame <= 7) {
            attackActive = true;
        }
        if (animState == AnimationState::BossAttack3 && currentFrame == 1) {
            triggerMidProjectile = true; // Signal BossGame
            // std::cout << "[Boss] Triggering Mid Projectile (Frame " << currentFrame << ")" << std::endl;
        }

        if (animState == AnimationState::BossUltimate && currentFrame >= 1 && currentFrame <= 2) {
            // Use a timer to spawn projectiles at intervals during the rain phase
            rainSpawnTimer += 1; //  dt passed within this frame check
            if (rainSpawnTimer >= rainSpawnInterval) {
                triggerRainProjectile = true; // Signal BossGame to spawn one projectile
                rainSpawnTimer = 0.f; // Reset interval timer
                // std::cout << "[Boss] Triggering Rain Projectile (Frame " << currentFrame << ")" << std::endl;
            }
        }
    }
};

// --- Game Class ---
class BossGame {
public:

    BossGame()
        : window(sf::VideoMode(1280, 720), "Final Boss"),
        player(
            100,     // maxHealth
            100,     // currentHealth
            true,    // enableDash
            0.75f,    // parrySuccessTime
            15,      // baseAttackDamage
            0.f,     // defensePercent
            true     // enableShoot
        ),
        boss({ 1100.f, 385.f }, & player, 600.f, 1250.f, 600),
        gameRng(rd()), gameClock(), timerText()
    {
        auto& sm = SoundManager::instance();
        sm.load("background", "../assets/sounds/Bossbackground.ogg");
        sm.load("player_shoot", "../assets/sounds/player_shoot.ogg");
        sm.load("player_damaged", "../assets/sounds/player_damaged.ogg");
        sm.load("player_jump", "../assets/sounds/player_jump.ogg");
        sm.load("player_parry", "../assets/sounds/player_parry.ogg");
        sm.load("player_attack", "../assets/sounds/player_attack.ogg");
        sm.load("ground_attack", "../assets/sounds/ground_attack.ogg");
        sm.load("fire_spray", "../assets/sounds/fire_spray.ogg");
        sm.load("boss_attack", "../assets/sounds/boss_attack.ogg");

        SoundManager::instance().play("background", 10);
        SoundManager::instance().setLoop("background", true);

        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(true);
        initBackground();
        setupUI();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            //helps prevent time skips/lag (should update at 60fps 0.01667s)
            if (dt > 0.1f) dt = 0.1f;
            processInput();
            update(dt);
            render();
        }
    }

    bool playerWin() { return player.isAlive() && !boss.isAlive(); }
    //public final time 
    sf::Time finalTime;
private:
    sf::RenderWindow window;
    sf::View gameView = window.getDefaultView();;
    sf::Sprite background;
    Player player;
    Boss boss;
    std::list<Projectile> projectiles;
    std::random_device rd; // Obtain a random seed
    std::mt19937 gameRng;
    bool showDebugBoxes = false;

    // UI elements
    sf::RectangleShape playerHealthBarBackground;
    sf::RectangleShape playerHealthBarFill;
    const float HEALTH_BAR_WIDTH = 300.f;
    const float HEALTH_BAR_HEIGHT = 20.f;
    const float HEALTH_BAR_PADDING = 10.f;
    const float HEALTH_BAR_POS_X = 25.f;
    const float HEALTH_BAR_POS_Y = 25.f;
    sf::Clock gameClock; // play time
    sf::Text timerText;
    sf::Time endScreen = sf::seconds(3);
    sf::Text endGameText;

    sf::RectangleShape bossHealthBarBackground;
    sf::RectangleShape bossHealthBarFill;
    const float BOSS_HEALTH_BAR_WIDTH = 400.f;
    const float BOSS_HEALTH_BAR_HEIGHT = 25.f;
    const float BOSS_HEALTH_BAR_POS_X = 1280.f - BOSS_HEALTH_BAR_WIDTH - 25.f;
    const float BOSS_HEALTH_BAR_POS_Y = 25.f;

    //hitboxes
    const sf::Vector2f PLAYER_HITBOX_SIZE = { 60.f, 80.f };
    const sf::Vector2f BOSS_HITBOX_SIZE = { 150.f, 200.f };

    struct TutorialMessage {
        sf::Text text;
        float duration;
        float timer;
        bool active;
    };

    std::vector<TutorialMessage> tutorialMessages;
    sf::Font font;
    sf::Text playerHealthText;
    sf::Text bossHealthText;

    void initBackground() {
        // Load background using TextureManager
        if (!TextureManager::instance().load("background", "../assets/background.png")) {
            std::cerr << "[Game] Failed to load background" << std::endl;
        }
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
            background.setPosition(0, 0);
        }
        else {
            std::cerr << "[Game] Failed to set background texture." << std::endl;
        }
    }

    void setupUI() {

        if (!font.loadFromFile("../assets/arial.ttf")) {
            std::cerr << "Failed to load font!" << std::endl;
        }
        // Tutorial messages
        std::vector<std::pair<std::string, float>> messageConfig = {
        {"Move with A/D Space", 4.5},
        {"Attack E,left click", 5.0},
        {"Shoot with F", 5.5},
        {"Dash with SHIFT", 6.},
        {"Parry with Q or right click", 6.5}
        };

        float verticalPosition = 150.f;
        for (auto& [msg, duration] : messageConfig) {
            TutorialMessage tutorial;
            tutorial.text.setFont(font);
            tutorial.text.setString(msg);
            tutorial.text.setCharacterSize(30);
            tutorial.text.setFillColor(sf::Color::White);
            tutorial.text.setOutlineColor(sf::Color::Black);
            tutorial.text.setOutlineThickness(1.8);

            sf::FloatRect bounds = tutorial.text.getLocalBounds();
            tutorial.text.setOrigin(bounds.width / 2, bounds.height / 2);
            tutorial.text.setPosition(window.getSize().x / 2, verticalPosition);

            tutorial.duration = duration;
            tutorial.timer = duration;
            tutorial.active = true;

            tutorialMessages.push_back(tutorial);
            verticalPosition += 50.f;
        }

        //Timer text
        timerText.setFont(font);
        timerText.setCharacterSize(35);
        timerText.setFillColor(sf::Color::White);
        timerText.setOutlineColor(sf::Color::Black);
        timerText.setOutlineThickness(1.8);
        // initial placeholder:
        timerText.setString("00:00");

        // center it at the top:
        auto bounds = timerText.getLocalBounds();
        timerText.setOrigin(bounds.width / 2, bounds.height / 2);
        timerText.setPosition(window.getSize().x / 2, 100);

        // Endgame text (hidden until game over)
        endGameText.setFont(font);
        endGameText.setCharacterSize(150);
        endGameText.setFillColor(sf::Color::Black);
        endGameText.setOutlineColor(sf::Color::White);
        endGameText.setOutlineThickness(2.f);
        endGameText.setString(""); // start empty
        //offset since ground is high
        endGameText.setPosition(window.getSize().x / 2, window.getSize().y / 2 - 110);

        // Player health text
        playerHealthText.setFont(font);
        playerHealthText.setCharacterSize(20);
        playerHealthText.setFillColor(sf::Color::White);
        playerHealthText.setPosition(HEALTH_BAR_POS_X + HEALTH_BAR_WIDTH + 10, HEALTH_BAR_POS_Y);

        // Boss health text
        bossHealthText.setFont(font);
        bossHealthText.setCharacterSize(20);
        bossHealthText.setFillColor(sf::Color::White);
        bossHealthText.setPosition(BOSS_HEALTH_BAR_POS_X - 150, BOSS_HEALTH_BAR_POS_Y);

        playerHealthBarBackground.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT));
        playerHealthBarBackground.setFillColor(sf::Color(50, 50, 50, 200)); // Dark grey background
        playerHealthBarBackground.setOutlineColor(sf::Color::Black);
        playerHealthBarBackground.setOutlineThickness(2.f);
        playerHealthBarBackground.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y); // Position in screen space

        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT)); // Initial full size
        playerHealthBarFill.setFillColor(sf::Color(0, 200, 0, 220)); // Bright green
        playerHealthBarFill.setPosition(HEALTH_BAR_POS_X, HEALTH_BAR_POS_Y); // Position exactly over background

        // Boss Health Bar
        bossHealthBarBackground.setSize(sf::Vector2f(BOSS_HEALTH_BAR_WIDTH, BOSS_HEALTH_BAR_HEIGHT));
        bossHealthBarBackground.setFillColor(sf::Color(50, 50, 50, 200));
        bossHealthBarBackground.setOutlineColor(sf::Color::Black);
        bossHealthBarBackground.setOutlineThickness(2.f);
        bossHealthBarBackground.setPosition(BOSS_HEALTH_BAR_POS_X, BOSS_HEALTH_BAR_POS_Y);

        bossHealthBarFill.setSize(sf::Vector2f(BOSS_HEALTH_BAR_WIDTH, BOSS_HEALTH_BAR_HEIGHT));
        bossHealthBarFill.setFillColor(sf::Color(200, 0, 0, 220)); // Red for boss
        bossHealthBarFill.setPosition(BOSS_HEALTH_BAR_POS_X, BOSS_HEALTH_BAR_POS_Y);
    }

    sf::FloatRect getPlayerHitbox() const {
        sf::Vector2f pos = player.getPosition();
        return {
            pos.x - PLAYER_HITBOX_SIZE.x / 2, //shrink hitbox by half as starting point
            pos.y - PLAYER_HITBOX_SIZE.y / 2,
            PLAYER_HITBOX_SIZE.x,             //set custom hitbox size
            PLAYER_HITBOX_SIZE.y
        };
    }

    sf::FloatRect getBossHitbox() const {
        sf::Vector2f pos = boss.getPosition();
        return {
            pos.x - BOSS_HITBOX_SIZE.x / 2,
            pos.y - BOSS_HITBOX_SIZE.y / 2 + 30, //boss hitbox starting position taller than half
            BOSS_HITBOX_SIZE.x,
            BOSS_HITBOX_SIZE.y
        };
    }

    void processInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Handle key presses for single actions (jump, attack, dash, parry, debug)
            if (event.type == sf::Event::KeyPressed) {
                // Only process game actions if player is alive
                if (player.isAlive()) {
                    switch (event.key.code) {
                    case sf::Keyboard::Space: player.jump(); break;
                    case sf::Keyboard::E:     player.attack(); break;
                    case sf::Keyboard::LShift:
                    case sf::Keyboard::RShift:player.dash(); break;
                    case sf::Keyboard::Q:     player.parry(); break;
                    case sf::Keyboard::F:     player.shoot(); break;
                        // Debug keys 
                    case sf::Keyboard::T: player.takeDamage(10); break;  // player damage
                    case sf::Keyboard::N: player.death(); break;         // Force player death
                        // Boss Debug Keys
                    case sf::Keyboard::Y: boss.takeDamage(100); break;   // Boss take damage
                    case sf::Keyboard::M: boss.death(); break;           // Force boss death
                    case sf::Keyboard::F1: showDebugBoxes = !showDebugBoxes; break; //debug hitboxes

                    default: break; // Ignore other keys for single press actions
                    }
                }
                // Allow Escape key anytime
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }

            // Handle mouse button presses for actions (only if player alive)
            if (player.isAlive() && event.type == sf::Event::MouseButtonPressed) {
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
    }

    // Added for collision logic
    void handleCollisions() {
        // Player attack Boss
        sf::FloatRect playerHitbox = getPlayerHitbox();

        if (player.isAttacking() && boss.isAlive() && player.getFaceingRight()) {
            sf::FloatRect playerAttackBounds = player.getGlobalBounds();

            if (playerAttackBounds.intersects(boss.getGlobalBounds())) {
                boss.takeDamage(player.getAttackDamage());
            }
        }

        // Boss attack Player
        if (boss.isMeleeAttackActive() && player.isAlive() && !player.isParryProtected()) {
            sf::FloatRect bossAttackBounds = boss.getAttackHitbox(); // Use boss defined attack hitbox

            if (bossAttackBounds.intersects(playerHitbox)) {
                player.takeDamage(10); // Damage amount for Boss melee
                // std::cout << "Boss melee hit Player!" << std::endl;
            }
        }
        // --- Projectile Collisions ---
        auto it = projectiles.begin();
        while (it != projectiles.end()) {
            if (!it->isActive()) {
                ++it; // Skip inactive projectiles
                continue;
            }

            sf::FloatRect projBounds = it->getGlobalBounds();

            // Player Projectile -> Boss
            if (it->getOwner() == Projectile::Owner::PLAYER && boss.isAlive()) {
                if (projBounds.intersects(boss.getGlobalBounds())) {
                    boss.takeDamage(it->getDamage());
                    it->setActive(false); // Deactivate projectile on hit
                    // std::cout << "Player projectile hit Boss!" << std::endl;
                }
            }
            // Boss Projectile -> Player
            else if (it->getOwner() == Projectile::Owner::BOSS && player.isAlive()) {
                if (projBounds.intersects(playerHitbox)) {
                    if (!player.isParryProtected()) { // Check for parry/dash invulnerability
                        player.takeDamage(it->getDamage());
                        // std::cout << "Boss projectile hit Player!" << std::endl;
                    }
                    it->setActive(false); // Deactivate projectile on hit (or parry)
                }
            }
            ++it;
        }

    }

    void updateUI() {
        // Calculate player health percentage
        float healthPercent = 0.f;
        if (player.getMaxHealth() > 0) { // Avoid division by zero
            healthPercent = static_cast<float>(player.getHealth()) / player.getMaxHealth();
        }
        healthPercent = std::max(0.f, healthPercent); // Ensure percent doesn't go below 0

        // Update the player fill bar width
        playerHealthBarFill.setSize(sf::Vector2f(HEALTH_BAR_WIDTH * healthPercent, HEALTH_BAR_HEIGHT));

        // Change player health bar color based on health
        if (healthPercent <= 0.f) {
            playerHealthBarFill.setFillColor(sf::Color(50, 50, 50, 200)); // Match background when dead?
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

        // Update player health text
        playerHealthText.setString(std::to_string(player.getHealth()));
        // Center in player health bar
        sf::FloatRect textBounds = playerHealthText.getLocalBounds();
        playerHealthText.setPosition(
            HEALTH_BAR_POS_X + (HEALTH_BAR_WIDTH - textBounds.width) / 2,
            HEALTH_BAR_POS_Y + (HEALTH_BAR_HEIGHT - 25) / 2
        );

        // Update boss health text
        bossHealthText.setString(std::to_string(boss.getHealth()));
        // Center in boss health bar
        textBounds = bossHealthText.getLocalBounds();
        bossHealthText.setPosition(
            BOSS_HEALTH_BAR_POS_X + (BOSS_HEALTH_BAR_WIDTH - textBounds.width) / 2,
            BOSS_HEALTH_BAR_POS_Y + (BOSS_HEALTH_BAR_HEIGHT - 25) / 2
        );
        //boss health bar update
        float bossHealthPercent = 0.f;
        if (boss.getMaxHealth() > 0) {
            bossHealthPercent = static_cast<float>(boss.getHealth()) / boss.getMaxHealth();
        }
        bossHealthPercent = std::max(0.f, bossHealthPercent);
        bossHealthBarFill.setSize(sf::Vector2f(BOSS_HEALTH_BAR_WIDTH * bossHealthPercent, BOSS_HEALTH_BAR_HEIGHT));
        bossHealthBarFill.setFillColor(sf::Color(200, 0, 0, 220));

    }

    void spawnPlayerProjectile() {
        if (!player.isAlive()) return;

        sf::Texture* tex = TextureManager::instance().get("player_projectile");
        if (!tex) {
            std::cerr << "[Game] Player projectile texture not found!" << std::endl;
            return;
        }

        sf::Vector2f startPos = player.getPosition();
        startPos.x += 50; // Offset to the right of the player

        float projectileSpeed = 800.f;
        sf::Vector2f velocity;
        velocity.y = 0; // Horizontal shot
        velocity.x = player.getFaceingRight() ? projectileSpeed : -projectileSpeed;

        int damage = 10;

        projectiles.emplace_back(tex, startPos, velocity, Projectile::Owner::PLAYER, damage);
        // std::cout << "[Game] Spawned Player Projectile" << std::endl;

    }

    void spawnBossProjectile(const std::string& textureId, sf::Vector2f startPos, sf::Vector2f velocity, int damage) {
        if (!boss.isAlive()) return;

        sf::Texture* tex = TextureManager::instance().get(textureId);
        if (!tex) {
            std::cerr << "[Game] Boss projectile texture '" << textureId << "' not found!" << std::endl;
            return;
        }

        projectiles.emplace_back(tex, startPos, velocity, Projectile::Owner::BOSS, damage);
        // std::cout << "[Game] Spawned Boss Projectile: " << textureId << std::endl;
    }


    void update(float dt) {
        player.update(dt);
        boss.update(dt);

        //display tutorial message
        for (auto& msg : tutorialMessages) {
            if (msg.active) {
                msg.timer -= dt;

                // Calculate alpha based on remaining time
                float alpha = std::clamp(msg.timer * 255.f / msg.duration, 0.f, 255.f);
                msg.text.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
                msg.text.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));

                if (msg.timer <= 0) {
                    msg.active = false;
                }
            }
        }

        if (player.wantsToShoot()) { // Check if player initiated shoot via 'F' key
            spawnPlayerProjectile();
        }

        // Boss Ground Projectile
        if (boss.wantsToShootGround()) {
            sf::Vector2f startPos = boss.getPosition();
            startPos.x += 160.f; // start position to the right of the boss for player reaction
            startPos.y += 130; // Y position for ground level
            sf::Vector2f velocity = { -400.f, 0.f }; // Always shoots left

            //boss damage
            spawnBossProjectile("boss_projectile_ground", startPos, velocity, 10);
            boss.resetGroundProjectileRequest();
        }

        // Boss Mid Projectile (Attack 3)
        if (boss.wantsToShootMid()) {
            sf::Vector2f startPos = { boss.getPosition().x, boss.getPosition().y + std::uniform_real_distribution<float>(-20.f, 20.f)(gameRng) };
            startPos.y -= 10; // Adjust Y
            sf::Vector2f velocity = { -500.f, 0.f };
            spawnBossProjectile("boss_projectile_mid", startPos, velocity, 10);
            boss.resetMidProjectileRequest();
        }

        // Boss Rain Projectile (Ultimate)
        if (boss.wantsToShootRain()) {
            // Spawn at random X position above the screen
            std::uniform_real_distribution<float> rainXDist(0.f, (float)window.getSize().x - 200);
            sf::Vector2f startPos = { rainXDist(gameRng), -100.f }; // Start above screen
            sf::Vector2f velocity = { 0.f, 600.f }; // Straight down
            spawnBossProjectile("boss_projectile_rain", startPos, velocity, 12);
            boss.resetRainProjectileRequest();
        }

        sf::Vector2u windowSize = window.getSize();
        auto it = projectiles.begin();
        while (it != projectiles.end()) {
            it->update(dt);

            // Check bounds / Deactivate if off-screen
            sf::Vector2f pos = it->getPosition();
            if (pos.x < -50 || pos.x > windowSize.x + 50 || pos.y < -100 || pos.y > windowSize.y + 50) {
                it->setActive(false);
            }

            // Remove inactive projectiles
            if (!it->isActive()) {
                it = projectiles.erase(it); // Erase returns iterator to the next element
            }
            else {
                ++it;
            }
        }

        // Update timers
        if (player.isAlive() && boss.isAlive()) {
            sf::Time elapsed = gameClock.getElapsedTime();
            int totalSeconds = static_cast<int>(elapsed.asSeconds());
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;

            // format mm:ss
            char buf[6];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
            timerText.setString(buf);
            timerText.setPosition(window.getSize().x / 2, 30);
            finalTime = gameClock.getElapsedTime();
        }
        else {
            if (endScreen.asSeconds() > 0) {
                endScreen -= sf::seconds(dt);
                if (playerWin()) {
                    auto bb = endGameText.getLocalBounds();
                    endGameText.setOrigin(bb.width / 2, bb.height / 2);
                    endGameText.setString("You Win!");
                }
                else {
                    auto bb = endGameText.getLocalBounds();
                    endGameText.setOrigin(bb.width / 2, bb.height / 2);
                    endGameText.setString("You Lose!");
                }
                return;
            }
            else { window.close(); }
        }
        //no collisions checks if either boss or player is dead (prevents draws)
        handleCollisions(); // collision checks
        updateUI(); // Update health bars for both
    }

    void render() {
        window.clear(sf::Color::Cyan); // Fallback background color from original

        // Draw game world elements 
        window.setView(gameView);
        if (background.getTexture()) {
            window.draw(background);
        }
        player.draw(window);
        boss.draw(window);

        window.draw(timerText); // Draw timer text

        // Draw end game text
        if (!endGameText.getString().isEmpty()) {
            window.draw(endGameText);
        }

        for (const auto& proj : projectiles) {
            proj.draw(window);
        }

        // Draw debug boxes if enabled
        if (showDebugBoxes) {
            sf::RectangleShape playerBox(sf::Vector2f(getPlayerHitbox().width, getPlayerHitbox().height));
            playerBox.setPosition(getPlayerHitbox().left, getPlayerHitbox().top);
            playerBox.setFillColor(sf::Color::Transparent);
            playerBox.setOutlineColor(sf::Color::Green);
            playerBox.setOutlineThickness(2.f);
            window.draw(playerBox);

            sf::FloatRect bossBounds = boss.getGlobalBounds();
            sf::RectangleShape bossBox(sf::Vector2f(bossBounds.width, bossBounds.height));
            bossBox.setPosition(bossBounds.left, bossBounds.top);
            bossBox.setFillColor(sf::Color::Transparent);
            bossBox.setOutlineColor(sf::Color::Red);
            bossBox.setOutlineThickness(2.f);
            window.draw(bossBox);

            sf::FloatRect attackBox = boss.getAttackHitbox();
            sf::RectangleShape attackRect(sf::Vector2f(attackBox.width, attackBox.height));
            attackRect.setPosition(attackBox.left + 15, attackBox.top);
            attackRect.setFillColor(sf::Color::Transparent);
            attackRect.setOutlineColor(sf::Color::Magenta);
            attackRect.setOutlineThickness(2.f);
            window.draw(attackRect);

            for (const auto& proj : projectiles) {
                if (proj.isActive()) {
                    sf::FloatRect bounds = proj.getGlobalBounds();
                    sf::RectangleShape projBox(sf::Vector2f(bounds.width, bounds.height));
                    projBox.setPosition(bounds.left, bounds.top);
                    projBox.setFillColor(sf::Color::Transparent);
                    projBox.setOutlineColor(proj.getOwner() == Projectile::Owner::PLAYER ? sf::Color::Cyan : sf::Color::Yellow);
                    projBox.setOutlineThickness(1.f);
                    window.draw(projBox);
                }
            }
        }

        // Draw UI elements using the default view (screen coordinates)
        window.setView(window.getDefaultView()); // Switch to default view for UI
        for (auto& msg : tutorialMessages) {
            if (msg.active) {
                window.draw(msg.text);
            }
        }
        window.draw(playerHealthBarBackground);
        window.draw(playerHealthBarFill);
        window.draw(bossHealthBarBackground);
        window.draw(bossHealthBarFill);

        // Draw health text
        window.draw(playerHealthText);
        window.draw(bossHealthText);
        window.display();
    }
};

// --- Main Function --- 
int main() {
    BossGame game;
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
    int totalSeconds = static_cast<int>(game.finalTime.asSeconds());
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::cout << (minutes < 10 ? "0" : "") << minutes << ":" << (seconds < 10 ? "0" : "") << seconds << std::endl;
    std::cout << game.finalTime.asSeconds();
    return 0;
}
