#pragma once
#include "common.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player
{
    float DEFAULT_SPEED = 250.f;
};

enum class EnemyState
{
    ROAMING = 0,
    PURSUING = ROAMING + 1,
    AVOIDWALL = PURSUING + 1,
    ATTACK = AVOIDWALL + 1,
    TELEPORTING = ATTACK + 1,
    SPAWN_MINIONS = TELEPORTING + 1
};

// anything that is deadly to the player
struct Enemy
{
    EnemyState enemyState = EnemyState::PURSUING;
};

struct Health
{
    int value = 100;

    void addHealth(int health)
    {
        value = min(100, value + health);
    }

    int applyDamage(int bounces_remaining, bool is_player_projectile, int damageMultiplier)
    {
        int damage;

        if (bounces_remaining == 0)
            damage = 50;
        else if (bounces_remaining == 1)
            damage = 25;
        else
            damage = 1;

        if (!is_player_projectile)
            damage = 10;

        damage *= damageMultiplier;
        value = max(0, value - damage);
        return damage;
    }
};

struct HealthBar
{
};

struct LineOfSight
{
    float ray_distance = 300;
    float ray_width = 300;
};

struct ReloadTime
{
    float counter_ms = 3000;
    float take_aim_ms = 500;
    float shoot_rate = 500;
};

struct Projectile
{
    static const int MAX_BOUNCES = 2;
    int bounces_remaining = MAX_BOUNCES;
    int is_player_projectile = true;
};

struct MeleeAttack
{
    int damage = 10;
    float windup = 500;
    float windupMax = 500;
};

struct DamageEffect
{
    bool is_attacked = false;
    float damage_show_time = 200;
    float max_show_time = 200;
};

struct Boss
{
};

struct Necromancer
{
    bool spawningMinions = false;
    vec2 centerPosition = vec2(0.0f, 0.0f);
};

struct Teleporter
{
    float animation_time = 1000.0f;
    float max_teleport_time = 1000.0f;
    vec2 prevScale = vec2(0.0f, 0.0f);
};
struct Teleporting
{
    float starting_time = 0.0f;
    float max_time = 1000.0f;
};

struct Wall
{
};

enum class PowerUpType
{
    INVINCIBILITY = 0,
    SUPER_BULLETS = INVINCIBILITY + 1,
    HEALTH_STEALER = SUPER_BULLETS + 1,
    POWER_UP_COUNT = HEALTH_STEALER + 1
};

struct PowerUp
{
    float available_timer = 10.f;
    float active_timer = 10.f;
    bool active = false;
    PowerUpType type;
};

struct Dash
{
    // const
    float intial_velocity = 2500;
    float max_dash_time = 0.3f;
    float recharge_cooldown = 5;
    float max_dash_charges = 2;

    // varying
    float recharge_timer = 0;
    float charges = max_dash_charges;
    float remaining_dash_time = 0;
    vec2 dash_direction = vec2(0, 1);
};

struct Clickable
{
    int screenTiedTo;
    int screenGoTo;
    bool isCurrentlyHoveredOver = false;
    bool isActive = false;
    int textureID;
};

// All data relevant to the shape and motion of entities
struct Motion
{
    Entity entity;
    vec2 position = {0, 0};
    float angle = 0;
    vec2 velocity = {0, 0};
    vec2 scale = {10, 10};
    vec2 last_physic_move = vec2(0, 0);
    vec2 last_move_direction = vec2(0, 1);
};

// Stucture to store collision information
struct Collision
{
    // Note, the first object is stored in the ECS container.entities
    Entity other; // the second object involved in the collision
    Collision(Entity &other) { this->other = other; };
};

// Data structure for toggling debug mode
struct Debug
{
    bool in_debug_mode = 0;
    bool in_freeze_mode = 0;
};
extern Debug debugging;

// Determines active screen
struct ScreenState
{
    float darken_screen_factor = -1;
    int activeScreen = 0;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
    // Note, an empty struct has size 1
};

// A timer that will be associated to dying player
struct DeathTimer
{
    float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & player.vs.glsl)
struct ColoredVertex
{
    vec3 position;
    vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
    vec3 position;
    vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
    static bool loadFromOBJFile(std::string obj_path, std::vector<TexturedVertex> &out_vertices, std::vector<uint16_t> &out_vertex_indices, std::vector<uint16_t> &out_uv_indices, vec2 &out_size);
    vec2 original_size = {1, 1};
    std::vector<TexturedVertex> vertices;
    std::vector<uint16_t> vertex_indices;
    std::vector<uint16_t> uv_indices;
};

struct LightUp
{
    float timer = 2.5f;
};

struct Animation
{
    float current_time = 0.f;
    float frame_time = 0.2f;
    int current_frame = 0;
    int num_frames;
    int sprite_width;
    int sprite_height;
    bool is_playing = true;
    bool loop = true;
};

struct Ray
{
    vec2 pos;
    vec2 direction;
};

// In parametric form: p + t * direction
struct LineSegment
{
    vec2 pos;
    vec2 direction;
    int t;
};

// font character structure
struct Character
{
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Offset to advance to next glyph
    char character;
};

struct Text
{
    std::string text;
    vec2 position;
    glm::vec3 color;
    float scale;
    float timer = 0.2f;
    bool timed = true;
};

struct TextRenderRequest
{
    std::string text;
    float x;
    float y;
    float scale;
    glm::vec3 color;
    glm::mat4 transform;

    TextRenderRequest(std::string t, float px, float py, float s, const glm::vec3 c, const glm::mat4 trans)
        : text(t), x(px), y(py), scale(s), color(c), transform(trans) {}
};

struct Light
{
    glm::vec2 position;
};
// Mouse Gestures
struct MouseGestures
{
    bool isHeld = false;
    vec2 position;
    vec2 lastPosition;
    std::vector<vec2> gesturePath;
    std::vector<vec2> renderPath;
    float threshold = 100.0f;
    float peakThreshold = 200.0f;
    int minSize = 20;
    bool isToggled = false;
};
extern MouseGestures mouseGestures;

// Levels
struct LevelStruct
{
    int level_num = 0;
    int num_melee = 0;
    int num_ranged = 0;
    int num_boss = 0;
    int max_active_melee = 0;
    int max_active_ranged = 0;
    int wave_size = 0;
    int enemy_spawn_time = 3000;
};

struct CurrLevels
{
    int current_level = 0;
    int total_level_index = 10;
    LevelStruct *currStruct = NULL;
};
extern CurrLevels currLevels;

struct GridNode
{
    vec2 position;
    ivec2 coord;
    vec2 size;
    bool notWalkable;
    float gCost;
    float hCost;
    float fCost() const { return gCost + hCost; }
    GridNode *parentNode;
};

struct GridMap
{
    std::vector<std::vector<GridNode>> gridMap;
    std::vector<Entity> exposed_walls;
    int mapWidth = window_width_px;
    int mapHeight = window_height_px;
    int matrixWidth = 0;
    int matrixHeight = 0;
};

struct Pathfinder
{
    std::vector<GridNode *> path;
    float refresh_rate = 1000.0f;
    float max_refresh_rate = 1000.0f;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID
{
    PLAYER = 0,
    MELEE_ENEMY = PLAYER + 1,
    RANGED_ENEMY = MELEE_ENEMY + 1,
    PROJECTILE = RANGED_ENEMY + 1,
    PROJECTILE_CHARGED = PROJECTILE + 1,
    PROJECTILE_SUPER_CHARGED = PROJECTILE_CHARGED + 1,
    PROJECTILE_ENEMY = PROJECTILE_SUPER_CHARGED + 1,
    WALL = PROJECTILE_ENEMY + 1,
    INVINCIBILITY = WALL + 1,
    SUPER_BULLETS = INVINCIBILITY + 1,
    HEALTH_STEALER = SUPER_BULLETS + 1,
    PLAY_BUTTON = HEALTH_STEALER + 1,
    TUTORIAL_BUTTON = PLAY_BUTTON + 1,
    EXIT_BUTTON = TUTORIAL_BUTTON + 1,
    BUTTON_BORDER = EXIT_BUTTON + 1,
    RESUME_BUTTON = BUTTON_BORDER + 1,
    TITLE_BUTTON = RESUME_BUTTON + 1,
    SAVE_QUIT_BUTTON = TITLE_BUTTON + 1,
    PLAY_AGAIN_BUTTON = SAVE_QUIT_BUTTON + 1,
    CONTINUE_BUTTON = PLAY_AGAIN_BUTTON + 1,
    HEALTH_BAR = CONTINUE_BUTTON + 1,
    PLAYER_HEALTH_BAR = HEALTH_BAR + 1,
    BOSS_ENEMY = PLAYER_HEALTH_BAR + 1,
    NECROMANCER_ENEMY = BOSS_ENEMY + 1,
    TEXTURE_COUNT = NECROMANCER_ENEMY + 1,
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID
{
    TEXTURED = 0,
    WATER = TEXTURED + 1,
    LIGHT = WATER + 1,
    EFFECT_COUNT = LIGHT + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID
{
    SPRITE = 0,
    SCREEN_TRIANGLE = SPRITE + 1,
    UI_COMPONENT = SCREEN_TRIANGLE + 1,
    PROJECTILE = UI_COMPONENT + 1,
    VISIBILITY_POLYGON = PROJECTILE + 1,
    SHADOW_PLANE = VISIBILITY_POLYGON + 1,
    FLOOR = SHADOW_PLANE + 1,
    GEOMETRY_COUNT = FLOOR + 1
};

enum class SCREEN_ID
{
    MAIN_MENU = 0,
    GAME_SCREEN = MAIN_MENU + 1,
    TUTORIAL_SCREEN = GAME_SCREEN + 1,
    EXIT_SCREEN = TUTORIAL_SCREEN + 1,
    PAUSE_SCREEN = EXIT_SCREEN + 1,
    DEATH_SCREEN = PAUSE_SCREEN + 1,
    WIN_SCREEN = DEATH_SCREEN + 1,
};

const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest
{
    TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
    EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
    GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};
