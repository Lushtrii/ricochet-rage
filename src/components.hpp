#pragma once
#include "common.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player
{
    float DEFAULT_SPEED = 200.f;
};

enum class EnemyState
{
    ROAMING = 0,
    PURSUING = ROAMING + 1,
    AVOIDWALL = PURSUING + 1,
    ATTACK = AVOIDWALL + 1
};

// anything that is deadly to the player
struct Enemy
{
    EnemyState enemyState = EnemyState::PURSUING;
};

struct Health
{
    int value = 100;

    void applyDamage(int bounces_remaining)
    {
        int damage = 30 - 10 * bounces_remaining;
        if (damage < 0)
            damage = 0;
        value -= damage;
        if (value < 0)
            value = 0;
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
    float take_aim_ms = 1500;
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

struct Wall
{
};

struct PowerUp
{
    float duration = 5;
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

struct Clickable {
    int screenTiedTo;
    int screenGoTo;
    bool isCurrentlyHoveredOver = false;
    bool isActive = false;
    int textureID;
};

// All data relevant to the shape and motion of entities
struct Motion
{
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
    static bool loadFromOBJFile(std::string obj_path, std::vector<TexturedVertex> &out_vertices, std::vector<uint16_t> &out_vertex_indices, std::vector<uint16_t>& out_uv_indices, vec2 &out_size);
    vec2 original_size = {1, 1};
    std::vector<TexturedVertex> vertices;
    std::vector<uint16_t> vertex_indices;
    std::vector<uint16_t> uv_indices;
};

struct Animation {
    float current_time = 0.f;
    float frame_time = 0.2f;
    int current_frame = 0;
    int num_frames;
    int sprite_width;
    int sprite_height; 
    bool is_playing = true;
    bool loop = true;
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
    WALL = PROJECTILE_SUPER_CHARGED + 1,
    INVINCIBILITY = WALL + 1,
    PLAY_BUTTON = INVINCIBILITY + 1,
    TUTORIAL_BUTTON = PLAY_BUTTON + 1,
    EXIT_BUTTON = TUTORIAL_BUTTON + 1,
    BUTTON_BORDER = EXIT_BUTTON + 1,
    RESUME_BUTTON = BUTTON_BORDER + 1,
    TITLE_BUTTON = RESUME_BUTTON + 1,
    SAVE_QUIT_BUTTON = TITLE_BUTTON + 1,
    PLAY_AGAIN_BUTTON = SAVE_QUIT_BUTTON + 1,
    CONTINUE_BUTTON = PLAY_AGAIN_BUTTON + 1,
    HEALTH_BAR = CONTINUE_BUTTON + 1,
    BOSS_ENEMY = HEALTH_BAR + 1,
    TEXTURE_COUNT = BOSS_ENEMY + 1,
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID
{
    TEXTURED = 0,
    WATER = TEXTURED + 1,
    EFFECT_COUNT = WATER + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID
{
    SPRITE = 0,
    SCREEN_TRIANGLE = SPRITE + 1,
    UI_COMPONENT = SCREEN_TRIANGLE + 1,
    PROJECTILE = UI_COMPONENT + 1,
    GEOMETRY_COUNT = PROJECTILE + 1
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
