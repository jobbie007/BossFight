#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <random>

// --- TextureManager ---
class TextureManager {
    std::map<std::string, sf::Texture> textures;
public:
    static TextureManager& instance();
    bool load(const std::string& id, const std::string& path);
    sf::Texture* get(const std::string& id);
};

// --- SoundManager ---
class SoundManager {
public:
    static SoundManager& instance();
    bool load(const std::string& id, const std::string& filename);
    sf::SoundBuffer* get(const std::string& id);
    void play(const std::string& id, float volume = 15.f);
    void setLoop(const std::string& id, bool loop);

private:
    SoundManager() = default;
    ~SoundManager() = default;
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::list<sf::Sound> sounds;
};

// --- AnimationState ---
enum class AnimationState {
    //player states
    Idle, Run, Jump, Attack1, Attack2, Attack3, Parry, Dash, Dead, Hurt, Shoot,
    //boss state
    BossIdle, BossAttack1, BossAttack2, BossAttack3, BossUltimate, BossHurt, BossDead,
    None
};

// --- AnimationComponent ---
class AnimationComponent {
public:
    struct Animation {
        const sf::Texture* texture = nullptr;
        int frameCount;
        float frameDuration;
        sf::Vector2i frameSize;
        bool loops;
    };

    AnimationComponent();
    void addAnimation(AnimationState state, const std::string& textureId, int frames, float duration, sf::Vector2i size, bool loop);
    void update(float dt);
    void play(AnimationState state);
    bool isDone() const;
    AnimationState getCurrentState() const;
    sf::Sprite& getSprite();
    const sf::Sprite& getSprite() const;
    int getCurrentFrameIndex() const;
    float getElapsedTimeInState() const;

private:
    sf::Sprite sprite;
    std::map<AnimationState, Animation> animations;
    AnimationState currentState = AnimationState::None;
    int currentFrame = 0;
    float elapsedTime = 0.f;
    bool _isDone = false;
};

// --- Projectile ---
class Projectile {
public:
    enum class Owner { PLAYER, BOSS };

    Projectile(sf::Texture* tex, sf::Vector2f startPos, sf::Vector2f vel, Owner own, int dmg);
    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    sf::FloatRect getGlobalBounds() const;
    bool isActive() const;
    void setActive(bool status);
    Owner getOwner() const;
    int getDamage() const;
    sf::Vector2f getPosition() const;

private:
    sf::Sprite sprite;
    sf::Texture* texture = nullptr;
    sf::Vector2f position;
    sf::Vector2f velocity;
    Owner owner;
    int damage;
    bool active;
};

// --- Player ---
class Player {
public:
    Player(int maxHealth = 100, int currentHealth = 100,
        bool enableDash = true, float parrySuccessTime = 0.8f,
        int baseAttackDamage = 15, float defensePercent = 0.0f,
        bool enableShoot = true);

    void update(float dt);
    sf::Vector2f getPosition() const;
    void move(sf::Vector2f direction);
    void jump();
    void death();
    void dash();
    void parry();
    void attack();
    void shoot();
    bool wantsToShoot();
    void takeDamage(int amount);
    int getAttackDamage() const;
    int getHealth() const;
    int getMaxHealth() const;
    bool isAlive() const;
    sf::FloatRect getGlobalBounds() const;
    void draw(sf::RenderTarget& target) const;
    bool getFacingRight() const;
    bool isAttacking() const;
    bool isParryProtected() const;
    bool getEnableShoot();
    bool getEnableDash();
    //copy constructor
    Player(const Player& other);
    //operator
    Player& operator=(const Player& other);

private:
    void loadResources();
    void initAnimations();
    void handleMovement(float dt);
    void handleAnimations();
    void updateTimers(float dt);

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

};

// --- Boss ---
class Boss {
public:
    enum class BossState { Idle, Attacking1, Attacking2, Attacking3, Ultimate, Dead };

    Boss(sf::Vector2f startPos, Player* playerTarget, float leftBoundary, float rightBoundary, int maxHealth = 500);

    void update(float dt);
    void takeDamage(int amount);
    void death();
    void draw(sf::RenderTarget& target) const;

    sf::FloatRect getAttackHitbox() const;
    sf::FloatRect getGlobalBounds() const;
    sf::Vector2f getPosition() const;
    bool isAlive() const;
    bool isAttackActive() const;
    bool isMeleeAttackActive() const;
    int getHealth() const;
    int getMaxHealth() const;
    bool wantsToShootGround() const;
    bool wantsToShootMid() const;
    bool wantsToShootRain() const;
    void resetGroundProjectileRequest();
    void resetMidProjectileRequest();
    void resetRainProjectileRequest();

private:
    void loadResources();
    void initAnimations();
    void updateTimers(float dt);
    void handleFlashing(float dt);
    void chooseNextAction();
    void performAttack1();
    void performAttack2();
    void performAttack3();
    void performUltimate();
    void startActionDelay();
    void setState(BossState newState);
    void checkAttackTiming();
    bool isInvulnerable() const;

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
};

class BossGame {
public:
    BossGame(Player player);
    void run();
    bool playerWin() const;
    sf::Time getFinalTime();

private:
    void initBackground();
    void setupUI();
    void processInput();
    void handleCollisions();
    void updateUI();
    void spawnPlayerProjectile();
    void spawnBossProjectile(const std::string& textureId, sf::Vector2f startPos, sf::Vector2f velocity, int damage);
    void update(float dt);
    void render();
    sf::FloatRect getPlayerHitbox() const;
    sf::FloatRect getBossHitbox() const;

    sf::Time finalTime;
    // Core  
    sf::RenderWindow window;
    sf::View gameView = window.getDefaultView();;
    sf::Sprite background, dashIcon, shootIcon;
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
    sf::Time endScreen = sf::seconds(5);
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
};

