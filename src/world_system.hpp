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

    // Check for collisions
    void handle_collisions(float elapsed_ms);

    // Should the game be over ?
    bool is_over() const;

private:
    void update_player_move_dir();
    // Input callback functions
    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 pos);
    void on_mouse_click(int button, int action, int mods);

    // Helper functions
    void projectile_hit_character(Entity laser, Entity character);

    void health_check(Health &health, const Entity &character);

    // restart level
    void init_values();
    void restart_game();

    // OpenGL window handle
    GLFWwindow *window;

    // Number of points attained by player, displayed in the window title
    unsigned int points;
    float next_enemy_spawn;

    // Game state
    RenderSystem *renderer;
    float current_speed;
    Entity player;

    int num_enemies_seen = 0;

    vec2 move_direction = vec2(0, 0);

    // music references
    Mix_Music *background_music;
    Mix_Chunk *player_death_sound;
    Mix_Chunk *enemy_death_sound;
    Mix_Chunk *laser_shot_sound;

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
