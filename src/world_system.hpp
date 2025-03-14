#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
    WorldSystem();

    // Creates a window
    GLFWwindow *create_window();

    // starts the game
    void init(RenderSystem *renderer);

    // Releases all associated resources
    ~WorldSystem();

    // Steps the game ahead by ms milliseconds
    bool step(float elapsed_ms);

    void reset_level();

    // Check for collisions
    void handle_collisions(float elapsed_ms);

    // Should the game be over ?
    bool is_over() const;

    bool isPaused() const { return m_isPaused; };

    void on_window_minimize(int minimized);

    void on_window_focus(int focused);

private:
    void update_player_move_dir();
    // Input callback functions
    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 pos);
    bool detect_heart_shape();
    void on_mouse_click(int button, int action, int mods);

    // Helper functions
    void projectile_hit_character(Entity laser, Entity character);
    bool mouseOverBox(vec2 mousePos, Entity entity);

    void health_check(Health &health, const Entity &character);

    // restart level
    void init_values();
    void restart_game();

    vec2 create_spawn_position();

    // OpenGL window handle
    GLFWwindow *window;

    // Number of points attained by player, displayed in the window title
    unsigned int points;
    float next_enemy_spawn;
    float next_power_up_spawn;

    // Game state
    RenderSystem *renderer;
    bool m_isPaused = true;
    float current_speed;
    Entity player;
    bool saveFileExists = false;

    int num_enemies_seen = 0;

    vec2 move_direction = vec2(0, 0);

    // music references
    Mix_Music *background_music;
    Mix_Chunk *player_death_sound;
    Mix_Chunk *enemy_death_sound;
    Mix_Chunk *laser_shot_sound;
    Mix_Chunk *invincibility_sound;
    Mix_Chunk *super_bullets_sound;
    Mix_Chunk *health_stealer_sound;
    Mix_Chunk *level_cleared_sound;


    int currNumEnemies = 0;
    int currNumMelees = 0;
    int currNumRanged = 0;

    // Show Level
    Entity showLevel;

    Entity showProgress;
    int startingNumEnemies;


    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
