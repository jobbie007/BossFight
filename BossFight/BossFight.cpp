#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <random>

// --- Texture Manager ---
class TextureManager {
    std::map<std::string, std::unique_ptr<sf::Texture>> textures;
public:
    static TextureManager& instance() {
        static TextureManager tm;
        return tm;
    }

    bool load(const std::string& id, const std::string& path) {
        auto texture = std::make_unique<sf::Texture>();
        if (texture->loadFromFile(path)) {
            textures[id] = std::move(texture);
            return true;
        }
        return false;
    }

    sf::Texture* get(const std::string& id) const {
        auto it = textures.find(id);
        return it != textures.end() ? it->second.get() : nullptr;
    }
};

// --- Animation State Enum ---
enum class AnimationState {
    // Player States
    Idle, Run, Jump, Attack1, Attack2, Attack3, Parry, Dash, Dead, Hurt,
    // Boss States
    BossIdle, BossAttack1, BossAttack2, BossUltimate, BossHurt, BossDead, BossMove,
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
            animations[state] = { tex, frames, duration, size, loop };
		}
        else {
            std::cerr << "[AnimationComponent] Error: Texture " << textureId << " not found." << std::endl;
        }
    }

    void update(float dt) {
        if (currentState != AnimationState::None && animations.count(currentState)) {
            auto& anim = animations[currentState];

 
            if (!anim.texture) {
                 std::cerr << "[AnimationComponent] Error: Update called on state " << static_cast<int>(currentState) << " with null texture." << std::endl;
                return;
            }

            if (!anim.loops && isDone()) {
                return; // Don't update non-looping finished animations
            }
            elapsedTime += dt;

            if (elapsedTime >= anim.frameDuration) {
                // Use integer division to handle potential frame skips if dt is large
                int frameAdvancement = static_cast<int>(elapsedTime / anim.frameDuration);
                elapsedTime = std::fmod(elapsedTime, anim.frameDuration); // Keep remainder

                currentFrame += frameAdvancement;

                if (currentFrame >= anim.frameCount) {
                    if (anim.loops) {
                        currentFrame %= anim.frameCount; // Loop back
                    }
                    else {
                        currentFrame = anim.frameCount - 1; // Clamp to last frame
                        _isDone = true;
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
        if (state != currentState || !animations[state].loops) { // Allow restarting non-looping
            const auto& anim = animations[state];

            currentState = state;
            currentFrame = 0;
            elapsedTime = 0.f;
            _isDone = false;

            sprite.setTexture(*anim.texture);
            sprite.setTextureRect(sf::IntRect(0, 0, anim.frameSize.x, anim.frameSize.y));
            sprite.setOrigin(static_cast<float>(anim.frameSize.x) / 2.f, static_cast<float>(anim.frameSize.y) / 2.f);
        }
    }

    bool isDone() const {
        if (currentState != AnimationState::None && animations.count(currentState)) {
            const auto& anim = animations.at(currentState);
            return !anim.loops && _isDone;
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

// --- Player Class ---
class Player {
public:
    Player(int maxHealth = 100, int currentHealth = 100) :
        maxHealth(maxHealth),
        currentHealth(currentHealth),
        rng(rd()) // Initialize member RNG
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
    }

    void initAnimations() {
        // Adjust frame counts, durations, sizes, loops assets
        animations.addAnimation(AnimationState::Idle, "player_idle", 8, 0.2f, { 160, 128 }, true);
        animations.addAnimation(AnimationState::Run, "player_run", 8, 0.1f, { 160, 128 }, true);
        animations.addAnimation(AnimationState::Attack1, "player_attack1", 6, 0.06f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Attack2, "player_attack2", 5, 0.09f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Attack3, "player_attack3", 16, 0.026f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Jump, "player_jump", 11, 0.08f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Dash, "player_dash", 5, 0.036f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Parry, "player_parry", 6, 0.08f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Dead, "player_dead", 7, 0.2f, { 160, 128 }, false);
        animations.addAnimation(AnimationState::Hurt, "player_hurt", 2, hurtDuration, { 160, 128 }, false); 
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
        if (isHurt || !isAlive()) return;

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

        if (canParry && !isAttacking() && dashTimer <= 0) {
            animations.play(AnimationState::Parry);
            canParry = false;
            parryTimer = parryCooldown;
            parrySuccessWindow = PARRY_SUCCESS_DURATION;
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
        // Prevent taking damage if already dead, or already in the hurt state (prevents chain-stuns)
        if (!isAlive() || animations.getCurrentState() == AnimationState::Dead || isHurt) {
            return;
        }

        currentHealth -= amount;
        currentHealth = std::max(0, currentHealth);
        std::cout << "[Player] Took " << amount << " damage. Health: " << currentHealth << "/" << maxHealth << std::endl;
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

    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool isAlive() const { return currentHealth > 0; }

    // Use visual bounds for collision checks as per original code
    sf::FloatRect getGlobalBounds() const {
        return animations.getSprite().getGlobalBounds();
    }

    void draw(sf::RenderTarget& target) const {
        target.draw(animations.getSprite());
    }

    void setRightBoundary(float boundary) {
        rightBoundary = boundary;
    }
    bool getFaceingRight(){ return facingRight;}

    // Added helper for collision detection logic
    bool isAttacking() const {
        AnimationState current = animations.getCurrentState();
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
    float rightBoundary = 1280.0f;

    int maxHealth;
    int currentHealth;

    float moveSpeed = 300.f;
    float jumpForce = 700.f;
    float gravity = 1800.f;
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

    bool canParry = true;
    float parryCooldown = 0.8f;
    float parryTimer = 0.f;
    float parrySuccessWindow = 0.0f;
    const float PARRY_SUCCESS_DURATION = 0.8f;

    float knockbackForceX = 60.f; // Horizontal knockback speed
    float knockbackForceY = -300.f; // Vertical knockback speed

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
        // Left boundary check
        if (proposedPosition.x < LEFT_BOUNDARY) {
            proposedPosition.x = LEFT_BOUNDARY;  // Snap to boundary
            if (velocity.x < 0) velocity.x = 0;  // Stop leftward movement
        }
        if (proposedPosition.x > rightBoundary) {
            proposedPosition.x = rightBoundary;
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
        bool isUninterruptibleAction = isAttacking() || currentState == AnimationState::Dash || currentState == AnimationState::Parry;

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
                    canDash = true;
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
    }
};

// --- Boss Class ---
class Boss {
public:
    enum class BossState {
        Idle, Attacking, Ultimate, Moving, Dead
    };

    Boss(sf::Vector2f startPos, Player* playerTarget, float bLeft, float bRight, int maxHealth = 500) :
        position(startPos),
        targetPlayer(playerTarget),
        leftBoundary(bLeft),
        rightBoundary(bRight),
        maxHealth(maxHealth),
        currentHealth(maxHealth),
        currentState(BossState::Idle),
        rng(rd())
    {
        minMoveDuration = 0.4f;
        maxMoveDuration = 2.0f;
        moveDurationDistribution = std::uniform_real_distribution<float>(minMoveDuration, maxMoveDuration);
        loadResources();
        initAnimations();
        animations.getSprite().setPosition(position);
        animations.getSprite().setScale(-1.f, 1.f);

        // Initialize RNG distributions
        actionChoiceDistribution = std::uniform_int_distribution<int>(0, 2);
        moveDirectionDistribution = std::uniform_int_distribution<int>(0, 1);
        moveDurationDistribution = std::uniform_real_distribution<float>(minMoveDuration, maxMoveDuration);
        attackChoiceDistribution = std::uniform_int_distribution<int>(0, 1);
        startActionDelay();
    }

    void loadResources() {
        auto& tm = TextureManager::instance();
        if (!tm.load("boss_idle", "../assets/boss/Idle.png")) std::cout << "boss idle not found";
        if (!tm.load("boss_attack1", "../assets/boss/Attack1.png")) std::cout << "boss attack1 not found";
        if (!tm.load("boss_attack2", "../assets/boss/Attack2.png")) std::cout << "boss attack2 not found";
        if (!tm.load("boss_ultimate", "../assets/boss/Ultimate.png")) std::cout << "boss ultimate not found";
        if (!tm.load("boss_dead", "../assets/boss/Dead.png")) std::cout << "boss dead not found";
        if (!tm.load("boss_run", "../assets/boss/Run.png")) std::cout << "boss run not found";
    }

    void initAnimations() {
        animations.addAnimation(AnimationState::BossIdle, "boss_idle", 8, 0.15f, { 800, 800 }, true);
        animations.addAnimation(AnimationState::BossAttack1, "boss_attack1", 8, 0.12f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossAttack2, "boss_attack2", 8, 0.12f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossUltimate, "boss_ultimate", 2, 0.5f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossDead, "boss_dead", 9, 0.18f, { 800, 800 }, false);
        animations.addAnimation(AnimationState::BossMove, "boss_run", 1, 0.6f, { 800, 800 }, true);
        animations.play(AnimationState::BossIdle);
    }

    void update(float dt) {
        if (currentState == BossState::Dead) {
            animations.update(dt);
            return;
        }

        updateTimers(dt);
        handleFlashing(dt);

        timeSinceLastAction += dt;
        if (currentState == BossState::Idle && timeSinceLastAction >= currentActionDelay) {
            chooseNextAction();
        }

        if (currentState == BossState::Moving) {
            handleMovement(dt);
        }

        if ((currentState == BossState::Attacking || currentState == BossState::Ultimate) && animations.isDone()) {
            setState(BossState::Idle);
            startActionDelay();
            attackActive = false;
        }
        else if (currentState == BossState::Attacking || currentState == BossState::Ultimate) {
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
        sf::FloatRect normal = getGlobalBounds();
        return sf::FloatRect(normal.left ,normal.top,normal.width,normal.height);
    }

    sf::FloatRect getGlobalBounds() const {
        bool isAttacking = currentState == BossState::Attacking ||
            currentState == BossState::Ultimate;

        float width = isAttacking ? ATTACK_HITBOX.x : NORMAL_HITBOX.x;
        float height = NORMAL_HITBOX.y;

        // Get facing direction from sprite scale
        float direction = animations.getSprite().getScale().x > 0 ? 1.f : -1.f;

        // Calculate horizontal position based on direction and attack state
        float xPosition = position.x;
        if (isAttacking) {
            // Center of attack hitbox extends in facing direction
            xPosition += (NORMAL_HITBOX.x / 2 * direction);
        }

        return sf::FloatRect(
            xPosition - width / 2,
            position.y - height / 2 + HITBOX_Y_OFFSET,
            width,
            height
        );
    }

    sf::Vector2f getPosition() const {
        return animations.getSprite().getPosition();
    }

    bool isAlive() const {
        return currentState != BossState::Dead;
    }

    bool isAttackActive() const {
        return attackActive;
    }

    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }

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
    float moveSpeed = 120.f;
    float moveTimer = 0.f;
    float minMoveDuration = 0.5f;
    float maxMoveDuration = 1.5f;
    float timeSinceLastAction = 0.f;
    float currentActionDelay = 2.0f;
    float attackCooldown1 = 1.5f;
    float attackCooldown2 = 2.5f;
    float ultimateCooldown = 15.0f;
    float currentAttackCooldown1 = 0.f;
    float currentAttackCooldown2 = 0.f;
    float currentUltimateCooldown = 0.f;
    bool attackActive = false;
    const sf::Vector2f NORMAL_HITBOX = { 150.f, 200.f };
    const sf::Vector2f ATTACK_HITBOX = { 220.f, 200.f }; // Wider hitbox for attacks
    const float HITBOX_Y_OFFSET = 30.f;
    // Flash effect
    float flashTimer = 0.f;
    const float flashDuration = 0.3f;
    const float flashInterval = 0.08f;
    float flashIntervalTimer = 0.f;
    sf::Color defaultColor = sf::Color::White;
    sf::Color damageColor = sf::Color(200, 80, 80, 200);

    // Random number generation
    std::random_device rd;
    std::mt19937 rng;
    std::uniform_int_distribution<int> actionChoiceDistribution;
    std::uniform_int_distribution<int> moveDirectionDistribution;
    std::uniform_real_distribution<float> moveDurationDistribution;
    std::uniform_int_distribution<int> attackChoiceDistribution;

    bool isInvulnerable() const {
        return currentState == BossState::Attacking ||
            currentState == BossState::Ultimate ||
			currentState == BossState::Moving ||
            flashTimer > 0.f;
    }

    void setState(BossState newState) {
        if (currentState != newState) {
            currentState = newState;
            if (newState == BossState::Idle) {
                velocity.x = 0;
                animations.play(AnimationState::BossIdle);
            }
            else if (newState == BossState::Moving) {
                animations.play(AnimationState::BossMove);
            }
        }
    }

    void startActionDelay() {
        std::uniform_real_distribution<float> delayVar(0.8f, 1.3f);
        currentActionDelay = 1.8f * delayVar(rng);
        timeSinceLastAction = 0.f;
    }

    void chooseNextAction() {
        if (currentUltimateCooldown <= 0.f && targetPlayer && isAlive()) {
            performUltimate();
            return;
        }
        int action = actionChoiceDistribution(rng);
        if (action == 0 && currentAttackCooldown1 <= 0.f) performAttack1();
        else if (action == 1 && currentAttackCooldown2 <= 0.f) performAttack2();
        else if (action == 2) startMoving();
        else startActionDelay();
    }

    void startMoving() {
        int direction = moveDirectionDistribution(rng);
        float targetSpeed = (direction == 0) ? -moveSpeed : moveSpeed;

        if ((direction == 0 && position.x > leftBoundary + 75.f) ||
            (direction == 1 && position.x < rightBoundary - 75.f)) {
            setState(BossState::Moving);
            velocity.x = targetSpeed;
            moveTimer = moveDurationDistribution(rng);
        }
        else {
            startActionDelay();
        }
    }

    void performAttack1() {
        setState(BossState::Attacking);
        animations.play(AnimationState::BossAttack1);
        currentAttackCooldown1 = attackCooldown1;
        timeSinceLastAction = 0.f;
        velocity.x = 0;
    }

    void performAttack2() {
        setState(BossState::Attacking);
        animations.play(AnimationState::BossAttack2);
        currentAttackCooldown2 = attackCooldown2;
        timeSinceLastAction = 0.f;
        velocity.x = 0;
    }

    void performUltimate() {
        setState(BossState::Ultimate);
        animations.play(AnimationState::BossUltimate);
        currentUltimateCooldown = ultimateCooldown;
        timeSinceLastAction = 0.f;
        velocity.x = 0;
    }

    void handleMovement(float dt) {
        position += velocity * dt;
        moveTimer -= dt;

        position.x = std::clamp(position.x, leftBoundary + 75.f, rightBoundary - 75.f);

        if (moveTimer <= 0.f) {
            setState(BossState::Idle);
            startActionDelay();
        }
    }

    void updateTimers(float dt) {
        if (currentAttackCooldown1 > 0.f) currentAttackCooldown1 -= dt;
        if (currentAttackCooldown2 > 0.f) currentAttackCooldown2 -= dt;
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

        attackActive = false;

        if (animState == AnimationState::BossAttack1 && currentFrame >= 3 && currentFrame <= 6) {
            attackActive = true;
        }
        else if (animState == AnimationState::BossAttack2 && currentFrame >= 4 && currentFrame <= 8) {
            attackActive = true;
        }
        else if (animState == AnimationState::BossUltimate && currentFrame >= 6 && currentFrame <= 12) {
            attackActive = true;
        }
    }
};

// --- Game Class ---
class BossGame {
public:

    BossGame() :
        window(sf::VideoMode(1280, 720), "Final Boss"),
        player(), // Default player constructor
  // Initialize Boss position, passing target, boundaries, health
        boss({ 950.f, 385.f }, & player, 600.f, 1250.f, 1000)
    {
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(true);
        initBackground(); // Load background separately
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
    sf::View gameView=window.getDefaultView();;
    sf::Sprite background;
    Player player;
    Boss boss; 
    bool showDebugBoxes = false;
    // UI Elements  + Boss Health Bar
    sf::RectangleShape playerHealthBarBackground;
    sf::RectangleShape playerHealthBarFill;
    const float HEALTH_BAR_WIDTH = 300.f;
    const float HEALTH_BAR_HEIGHT = 20.f;
    const float HEALTH_BAR_PADDING = 10.f; // Unused in original positioning logic
    const float HEALTH_BAR_POS_X = 25.f;
    const float HEALTH_BAR_POS_Y = 25.f;

    sf::RectangleShape bossHealthBarBackground; 
    sf::RectangleShape bossHealthBarFill;       
    const float BOSS_HEALTH_BAR_WIDTH = 400.f;
    const float BOSS_HEALTH_BAR_HEIGHT = 25.f;
    const float BOSS_HEALTH_BAR_POS_X = 1280.f - BOSS_HEALTH_BAR_WIDTH - 25.f;
    const float BOSS_HEALTH_BAR_POS_Y = 25.f;

    //hitboxes
    const sf::Vector2f PLAYER_HITBOX_SIZE = { 60.f, 100.f };
    const sf::Vector2f BOSS_HITBOX_SIZE = { 150.f, 200.f };
    const float BOSS_HITBOX_Y_OFFSET = 30.f;

    void initBackground() {
        // Load background using TextureManager
        if (!TextureManager::instance().load("background", "../assets/background.png")) {
            std::cerr << "[Game] Failed to load background texture in Texturemanager." << std::endl;
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
            pos.x - PLAYER_HITBOX_SIZE.x / 2,
            pos.y - PLAYER_HITBOX_SIZE.y / 2,
            PLAYER_HITBOX_SIZE.x,
            PLAYER_HITBOX_SIZE.y
        };
    }

    sf::FloatRect getBossHitbox() const {
        sf::Vector2f pos = boss.getPosition();
        return {
            pos.x - BOSS_HITBOX_SIZE.x / 2,
            pos.y - BOSS_HITBOX_SIZE.y / 2 + BOSS_HITBOX_Y_OFFSET,
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
                        // Debug keys from original
                    case sf::Keyboard::T: player.takeDamage(10); break;
                    case sf::Keyboard::N: player.death(); break;
                        // Add Boss Debug Keys
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
        else {
            // Ensure movement stops completely if dead
            player.move({ 0.f, 0.f });
        }
    }

    // Added for collision logic
    void handleCollisions() {
        // Player attack Boss
        if (player.isAttacking() && boss.isAlive() && player.getFaceingRight()) {
            sf::FloatRect playerHitbox = getPlayerHitbox();
            sf::FloatRect bossHitbox = boss.getGlobalBounds();

            if (playerHitbox.intersects(bossHitbox)) {
                boss.takeDamage(15);
            }
        }

        // Boss attack Player
        if (boss.isAttackActive() && player.isAlive() && !player.isParryProtected()) {
            sf::FloatRect bossHitbox = boss.getGlobalBounds();
            sf::FloatRect playerHitbox = getPlayerHitbox();

            if (bossHitbox.intersects(playerHitbox)) {
                player.takeDamage(5);
            }
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

        //boss health bar update
        float bossHealthPercent = 0.f;
        if (boss.getMaxHealth() > 0) {
            bossHealthPercent = static_cast<float>(boss.getHealth()) / boss.getMaxHealth();
        }
        bossHealthPercent = std::max(0.f, bossHealthPercent);
        bossHealthBarFill.setSize(sf::Vector2f(BOSS_HEALTH_BAR_WIDTH * bossHealthPercent, BOSS_HEALTH_BAR_HEIGHT));
        bossHealthBarFill.setFillColor(sf::Color(200, 0, 0, 220));
    }


    void update(float dt) {
        player.update(dt);
        boss.update(dt); 

        const float bossHitboxThird = BOSS_HITBOX_SIZE.x / 3;
        const float bossLeftEdge = boss.getPosition().x - (BOSS_HITBOX_SIZE.x / 2);
        player.setRightBoundary(bossLeftEdge + bossHitboxThird - (PLAYER_HITBOX_SIZE.x / 2));

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
		// Draw debug boxes if enabled
        if (showDebugBoxes) {
            sf::RectangleShape playerBox(sf::Vector2f(getPlayerHitbox().width, getPlayerHitbox().height));
            playerBox.setPosition(getPlayerHitbox().left, getPlayerHitbox().top);
            playerBox.setFillColor(sf::Color::Transparent);
            playerBox.setOutlineColor(sf::Color::Green);
            playerBox.setOutlineThickness(2.f);
            window.draw(playerBox);

            sf::RectangleShape bossBox(sf::Vector2f(getBossHitbox().width, getBossHitbox().height));
            bossBox.setPosition(getBossHitbox().left, getBossHitbox().top);
            bossBox.setFillColor(sf::Color::Transparent);
            bossBox.setOutlineColor(sf::Color::Red);
            bossBox.setOutlineThickness(2.f);
            window.draw(bossBox);
           

           
                sf::FloatRect attackBox = boss.getAttackHitbox();
                sf::RectangleShape attackRect(sf::Vector2f(attackBox.width, attackBox.height));
                attackRect.setPosition(attackBox.left, attackBox.top);
                attackRect.setFillColor(sf::Color::Transparent);
                attackRect.setOutlineColor(sf::Color::Magenta);
                attackRect.setOutlineThickness(2.f);
                window.draw(attackRect);
            

        }
        // Draw UI elements using the default view (screen coordinates)
        window.setView(window.getDefaultView()); // Switch to default view for UI
        window.draw(playerHealthBarBackground);
        window.draw(playerHealthBarFill);
        window.draw(bossHealthBarBackground);
        window.draw(bossHealthBarFill);     
        // Draw other UI elements (score, timer, etc.) here

        window.display();
    }
};

// --- Main Function --- 
int main() {
    BossGame game;
    game.run();
    return 0;
}