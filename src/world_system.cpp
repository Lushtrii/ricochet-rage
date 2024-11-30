// Header
#include "world_system.hpp"
#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"

// stlib
#include <GLFW/glfw3.h>
#include <cassert>
#include <csignal>
#include <sstream>

#include "physics_system.hpp"

#include "distort.hpp"

DistortToggle toggle;

// const size_t TOTAL_NUM_ENEMIES = 20;          // total number of enemies in the game level
// const size_t MAX_NUM_ENEMIES = 10;            // maximum number of enemies on the screen
// const size_t ENEMY_SPAWN_DELAY_MS = 3000;     // time between enemy spawns
const size_t POWER_UP_SPAWN_DELAY_MS = 12500; // time between power up spawns

void WorldSystem::on_window_minimize(int minimized)
{
    int activeScreen = renderer->getActiveScreen();

    if (minimized && activeScreen == (int)SCREEN_ID::GAME_SCREEN)
    {
        m_isPaused = true;
        renderer->setActiveScreen((int)SCREEN_ID::PAUSE_SCREEN);
        renderer->flipActiveButtions((int)SCREEN_ID::PAUSE_SCREEN);
    }
}

void WorldSystem::on_window_focus(int focused)
{
    int activeScreen = renderer->getActiveScreen();

    if (!focused && activeScreen == (int)SCREEN_ID::GAME_SCREEN)
    {
        m_isPaused = true;
        renderer->setActiveScreen((int)SCREEN_ID::PAUSE_SCREEN);
        renderer->flipActiveButtions((int)SCREEN_ID::PAUSE_SCREEN);
    }
}

// create the underwater world
WorldSystem::WorldSystem()
    : points(0), next_enemy_spawn(0.f), next_power_up_spawn(5.f)
{
    // Seeding rng with random device
    rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem()
{

    // destroy music components
    if (background_music != nullptr)
        Mix_FreeMusic(background_music);
    if (player_death_sound != nullptr)
        Mix_FreeChunk(player_death_sound);
    if (enemy_death_sound != nullptr)
        Mix_FreeChunk(enemy_death_sound);
    if (laser_shot_sound != nullptr)
        Mix_FreeChunk(laser_shot_sound);
    if (invincibility_sound != nullptr)
        Mix_FreeChunk(invincibility_sound);
    if (super_bullets_sound != nullptr)
        Mix_FreeChunk(super_bullets_sound);
    if (health_stealer_sound != nullptr)
        Mix_FreeChunk(health_stealer_sound);

    Mix_CloseAudio();

    // Destroy all created components
    registry.clear_all_components();

    // Close the window
    glfwDestroyWindow(window);
}

// Debugging
namespace
{
    void glfw_err_cb(int error, const char *desc)
    {
        fprintf(stderr, "%d: %s", error, desc);
    }
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow *WorldSystem::create_window()
{
    ///////////////////////////////////////
    // Initialize GLFW
    glfwSetErrorCallback(glfw_err_cb);
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW");
        return nullptr;
    }

    //-------------------------------------------------------------------------
    // If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
    // enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
    // GLFW / OGL Initialization
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);

    // Create the main window (for rendering, keyboard, and mouse input)
    window = glfwCreateWindow(window_width_px, window_height_px, "Ricochet Rage", nullptr, nullptr);
    glfwSetWindowSize(window, window_width_px, window_height_px);

    if (window == nullptr)
    {
        fprintf(stderr, "Failed to glfwCreateWindow");
        return nullptr;
    }

    // Setting callbacks to member functions (that's why the redirect is needed)
    // Input is handled using GLFW, for more info see
    // http://www.glfw.org/docs/latest/input_guide.html
    glfwSetWindowUserPointer(window, this);
    auto key_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2, int _3)
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
    auto cursor_pos_redirect = [](GLFWwindow *wnd, double _0, double _1)
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_move({_0, _1}); };
    auto click_callback = [](GLFWwindow *wnd, int button, int action, int mods)
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_click(button, action, mods); };
    glfwSetKeyCallback(window, key_redirect);
    glfwSetCursorPosCallback(window, cursor_pos_redirect);
    glfwSetMouseButtonCallback(window, click_callback);

    auto minimize_callback = [](GLFWwindow *wnd, int minimized)
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_window_minimize(minimized); };
    glfwSetWindowIconifyCallback(window, minimize_callback);

    auto focus_callback = [](GLFWwindow *wnd, int focused)
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_window_focus(focused); };
    glfwSetWindowFocusCallback(window, focus_callback);

    //////////////////////////////////////
    // Loading music and sounds with SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Failed to initialize SDL Audio");
        return nullptr;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
    {
        fprintf(stderr, "Failed to open audio device");
        return nullptr;
    }

    background_music = Mix_LoadMUS(audio_path("background-music.wav").c_str());
    player_death_sound = Mix_LoadWAV(audio_path("player-death-sound.wav").c_str());
    enemy_death_sound = Mix_LoadWAV(audio_path("enemy-death-sound.wav").c_str());
    laser_shot_sound = Mix_LoadWAV(audio_path("laser-shot-sound.wav").c_str());
    invincibility_sound = Mix_LoadWAV(audio_path("invincibility.wav").c_str());
    super_bullets_sound = Mix_LoadWAV(audio_path("super-bullets.wav").c_str());
    health_stealer_sound = Mix_LoadWAV(audio_path("health-stealer.wav").c_str());

    if (background_music == nullptr || player_death_sound == nullptr || enemy_death_sound == nullptr || laser_shot_sound == nullptr)
    {
        fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n %s\n make sure the data directory is present",
                audio_path("background-music.wav").c_str(),
                audio_path("player-death-sound.wav").c_str(),
                audio_path("enemy-death-sound.wav").c_str(),
                audio_path("laser-shot-sound.wav").c_str(),
                audio_path("invincibility.wav").c_str(),
                audio_path("super-bullets.wav").c_str(),
                audio_path("health-stealer.wav").c_str());
        return nullptr;
    }

    return window;
}

void WorldSystem::init(RenderSystem *renderer_arg)
{
    this->renderer = renderer_arg;
    // Playing background music indefinitely
    Mix_PlayMusic(background_music, -1);
    fprintf(stderr, "Loaded music\n");

    // Set all states to default
    saveFileExists = renderer->doesSaveFileExist();
    if (saveFileExists)
    {
        LoadGameFromFile(renderer_arg);
        // Make the code better later
        currLevels.currStruct = levels[currLevels.current_level];
        init_values();
    }
    else
    {
        currLevels.currStruct = levels[currLevels.current_level];
        restart_game();
    }
}

bool WorldSystem::mouseOverBox(vec2 mousePos, Entity entity)
{
    Motion m = registry.motions.get(entity);
    float minX = m.position.x - abs(m.scale.x) / 2;
    float maxX = m.position.x + abs(m.scale.x) / 2;
    float minY = m.position.y - abs(m.scale.y) / 2;
    float maxY = m.position.y + abs(m.scale.y) / 2;
    return (minX <= mousePos.x) && (mousePos.x <= maxX) && (minY <= mousePos.y) && (mousePos.y <= maxY);
}

vec2 WorldSystem::create_spawn_position()
{
    bool is_valid_spawn = false;
    vec2 spawn_pos;

    while (!is_valid_spawn)
    {
        spawn_pos = {
            150.f + uniform_dist(rng) * (window_width_px - 300.f),
            150.f + uniform_dist(rng) * (window_height_px - 300.f)};
        is_valid_spawn = true;

        // Check if it collides with the player
        Motion &playerMotion = registry.motions.get(player);
        if (length(playerMotion.position - spawn_pos) < 150.0f)
        {
            is_valid_spawn = false;
        }

        for (Entity entity : registry.walls.entities)
        {
            Motion &wall_motion = registry.motions.get(entity);
            if (length(wall_motion.position - spawn_pos) < 100.f)
            {
                is_valid_spawn = false;
                break;
            }
        }
    }

    return spawn_pos;
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
    // Show Current Level
    int w, h;
    glfwGetWindowSize(renderer->getWindow(), &w, &h);
    if (!registry.texts.has(showLevel))
    {
        showLevel = createText(renderer, "Level " + std::to_string(currLevels.current_level + 1), vec2(w / 2 + w * 0.35f, h * 0.11f), 2.0f, {1.0, 0.0, 0.0}, false);
    }
    else
    {
        Text &showLevelText = registry.texts.get(showLevel);
        showLevelText.text = "Level " + std::to_string(currLevels.current_level + 1);
    }

    static int frames = 0;
    static double prevTime = glfwGetTime();
    double currTime = glfwGetTime();

    double delta = currTime - prevTime;

    frames++;

    if (delta >= 1.0)
    {

        int FPS = frames / delta;

        prevTime = currTime;
        frames = 0;

        std::stringstream title_ss;
        title_ss << "Ricochet Rage | FPS: " << FPS;
        glfwSetWindowTitle(window, title_ss.str().c_str());
    }

    // Remove debug info from the last step
    while (registry.debugComponents.entities.size() > 0)
        registry.remove_all_components_of(registry.debugComponents.entities.back());

    // Remove all health bars from last step
    while (registry.healthBars.entities.size() > 0)
        registry.remove_all_components_of(registry.healthBars.entities.back());

    for (Entity entity : registry.texts.entities)
    {
        Text &text = registry.texts.get(entity);
        if (text.timed)
        {
            text.timer -= elapsed_ms_since_last_update / 1000.f;
            if (text.timer < 0)
            {
                registry.remove_all_components_of(entity);
            }
        }
    }

    int activePowerUpCount = 0;

    for (Entity entity : registry.powerUps.entities)
    {
        PowerUp &powerUp = registry.powerUps.get(entity);
        if (powerUp.active)
        {
            powerUp.active_timer -= elapsed_ms_since_last_update / 1000.f;
            if (powerUp.active_timer < 0)
            {
                registry.remove_all_components_of(entity);
            }
            else
            {
                if (powerUp.type == PowerUpType::INVINCIBILITY)
                {
                    registry.colors.get(player) = {0.2f, 0.6f, 0.2f};
                }
                else if (powerUp.type == PowerUpType::SUPER_BULLETS)
                {
                    registry.colors.get(player) = {0.2f, 0.2f, 0.6f};
                }
                else
                {
                    registry.colors.get(player) = {0.3f, 0.2f, 0.3f};
                }
            }
            activePowerUpCount++;
        }
        else
        {
            powerUp.available_timer -= elapsed_ms_since_last_update / 1000.f;
            if (powerUp.available_timer < 0)
            {
                registry.remove_all_components_of(entity);
            }
        }
    }

    if (activePowerUpCount == 0)
    {
        registry.colors.get(player) = {1.f, 0.8f, 0.8f};
    }

    // Removing out of screen entities
    auto &motions_registry = registry.motions;

    // Remove entities that leave the screen on the left side
    // Iterate backwards to be able to remove without unterfering with the next object to visit
    // (the containers exchange the last element with the current)
    for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i)
    {
        Motion &motion = motions_registry.components[i];
        if (motion.position.x + abs(motion.scale.x) < 0.f)
        {
            if (!registry.players.has(motions_registry.entities[i])) // don't remove the player
                registry.remove_all_components_of(motions_registry.entities[i]);
        }
    }

    // Check if the health of enemies or player is 0
    for (Entity &entity : registry.healths.entities)
    {
        Health &health = registry.healths.get(entity);
        health_check(health, entity);
    }

    // Add health bars
    for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i)
    {
        Motion &motion = motions_registry.components[i];
        Entity entity = motions_registry.entities[i];

        if (registry.healths.has(entity))
        {
            Health &health = registry.healths.get(entity);

            float healthNormalized;
            if (registry.bosses.has(entity))
            {
                healthNormalized = health.value / 300.f;
            }
            else
            {
                healthNormalized = health.value / 100.f;
            }
            createHealthBar(
                renderer,
                {motion.position.x, motion.position.y - abs(motion.scale.y) / 2 - 15.f},
                {abs(motion.scale.x) * healthNormalized, 8.f});
        }
    }

    auto &damageEffects = registry.damageEffect;
    // Check for damage effect
    if (damageEffects.has(player))
    {
        DamageEffect &damageEffect = damageEffects.get(player);
        if (damageEffect.is_attacked)
        {
            damageEffect.damage_show_time -= elapsed_ms_since_last_update;
            if (registry.colors.has(player))
            {
                vec3 &playerColor = registry.colors.get(player);
                // make red
                playerColor = {1.0, 0.0, 0.0};
                if (damageEffect.damage_show_time < 0)
                {
                    damageEffect.damage_show_time = damageEffect.max_show_time;
                    playerColor = {1, 0.8f, 0.8f};
                    damageEffect.is_attacked = false;
                }
            }
        }
    }

    // spawn new enemies
    next_enemy_spawn -= elapsed_ms_since_last_update * current_speed;
    LevelStruct &curr_level_struct = *currLevels.currStruct;
    bool enemiesLeft = (curr_level_struct.num_melee + curr_level_struct.num_ranged + curr_level_struct.num_boss) > 0;
    if (enemiesLeft && next_enemy_spawn < 0.f)
    {
        next_enemy_spawn = (curr_level_struct.enemy_spawn_time * 0.5) + uniform_dist(rng) * curr_level_struct.enemy_spawn_time;
        vec2 spawn_pos = create_spawn_position();
        std::cout << "NUM: " << curr_level_struct.num_melee << " "
                  << curr_level_struct.num_ranged << " "
                  << curr_level_struct.num_boss << " "
                  << std::endl;

        float rand;
        if (curr_level_struct.num_melee >= 1 && curr_level_struct.num_ranged >= 1)
        {
            rand = uniform_dist(rng) * 2.0f;
            // std::cout << rand << " RAND" << std::endl;
        }
        else if (curr_level_struct.num_melee >= 1)
        {
            rand = 1;
        }
        else if (curr_level_struct.num_ranged >= 1)
        {
            rand = 2;
        }

        if (curr_level_struct.num_boss >= 1)
        {
            if (curr_level_struct.level_num == 5)
            {
                createCowboyBossEnemy(renderer, spawn_pos);
            }
            else
            {
                createNecromancerEnemy(renderer, spawn_pos);
            }

            curr_level_struct.num_boss--;
        }
        else if (rand <= 1)
        {
            createMeleeEnemy(renderer, spawn_pos);
            curr_level_struct.num_melee--;
        }
        else if (rand <= 2)
        {
            createRangedEnemy(renderer, spawn_pos);
            curr_level_struct.num_ranged--;
        }
    }

    // Level cleared, going to next level
    if (!enemiesLeft && (int)registry.enemies.entities.size() == 0)
    {
        printf("NEXT LEVEL");
        currLevels.current_level++;
        if (currLevels.current_level == currLevels.total_level_index)
        {
            renderer->setActiveScreen((int)SCREEN_ID::WIN_SCREEN);
            renderer->flipActiveButtions(renderer->getActiveScreen());
            reset_level();
            m_isPaused = true;
            std::remove("../Save1.data");
            restart_game();
            return true;
        }
        currLevels.currStruct = levels[currLevels.current_level];
        restart_game();
        return true;
    }

    // spawn power ups
    next_power_up_spawn -= elapsed_ms_since_last_update * current_speed;
    if (next_power_up_spawn < 0.f)
    {
        next_power_up_spawn = POWER_UP_SPAWN_DELAY_MS;
        vec2 spawn_pos = create_spawn_position();
        float spawn_power_up = uniform_dist(rng);

        if (spawn_power_up < 0.33)
            createInvincibilityPowerUp(renderer, spawn_pos);
        else if (spawn_power_up < 0.66)
            createSuperBulletsPowerUp(renderer, spawn_pos);
        else
            createHealthStealerPowerUp(renderer, spawn_pos);
    }

    // Processing the player state
    assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    for (Entity entity : registry.deathTimers.entities)
    {
        registry.deathTimers.remove(entity);
        screen.darken_screen_factor = 0;
        renderer->setActiveScreen((int)SCREEN_ID::DEATH_SCREEN);
        renderer->flipActiveButtions(renderer->getActiveScreen());
        std::remove("../Save1.data");
        reset_level();
        m_isPaused = true;
        restart_game();
        return true;
    }

    return true;
}

void WorldSystem::reset_level()
{
    currLevels.current_level = 0;
    currLevels.currStruct = levels[0];
}

void WorldSystem::init_values()
{
    // Reset the game speed
    current_speed = 1.f;

    // Reset the number of enemies seen
    num_enemies_seen = 0;

    player = registry.players.entities.back();

    printf("Current Level: [%d]\n", currLevels.current_level + 1);
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
    // Debugging for memory/component leaks
    registry.list_all_components();
    printf("Restarting\n");

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, eels, ... but that would be more cumbersome
    //
    assert(registry.motions.size() > 0 && "Motions registry does not contain items");
    for (int i = (int)registry.motions.size() - 1; i >= 0; i--)
    {
        Entity e = registry.motions.entities[i];
        if (!registry.clickables.has(e) && e != renderer->getHoverEntity())
        {
            registry.remove_all_components_of(e);
        }
    }

    // Debugging for memory/component leaks
    registry.list_all_components();

    // create a new player
    createPlayer(renderer, {30, window_height_px});
    init_values();
    registry.colors.insert(player, {1, 0.8f, 0.8f});
    update_player_move_dir();

    // createNecromancerEnemy(renderer, {window_width_px / 2 + 210, window_height_px / 2});
    GenerateMap(renderer, uniform_dist(rng) * INT32_MAX);

    // // create the game walls
    // createWall(renderer, vec2(window_width_px / 2.f, window_height_px / 2.f), vec2(100, 100), 0);
    // createWall(renderer, vec2(window_width_px / 2.f + 100, window_height_px / 2.f), vec2(100, 100), 0);

    // // updated wall
    // // top right corner
    // createWall(renderer, vec2(window_width_px - 50, window_height_px - 675), vec2(50, 50), 0);

    // // top left corner
    // createWall(renderer, vec2(window_width_px - 1200, window_height_px - 675), vec2(50, 50), 0);

    // // bot right corner
    // createWall(renderer, vec2(window_width_px - 50, window_height_px - 75), vec2(50, 50), 0);

    // // bot left corner
    // createWall(renderer, vec2(window_width_px - 1200, window_height_px - 75), vec2(50, 50), 0);

    // for (int i = 100; i <= 1150; i += 50)
    // {
    //     // Top wall
    //     createWall(renderer, vec2(window_width_px - i, window_height_px - 675), vec2(50, 50), 0);

    //     // Bottom wall
    //     createWall(renderer, vec2(window_width_px - i, window_height_px - 75), vec2(50, 50), 0);
    // }

    // for (int i = 125; i <= 625; i += 50)
    // {
    //     // Left wall
    //     createWall(renderer, vec2(window_width_px - 1200, window_height_px - i), vec2(50, 50), 0);

    //     // Right wall
    //     createWall(renderer, vec2(window_width_px - 50, window_height_px - i), vec2(50, 50), 0);
    // }

    // create power ups
    // createInvincibilityPowerUp(renderer, {window_width_px / 2.f - 100, window_height_px / 2.f - 100});
}

void WorldSystem::projectile_hit_character(Entity laser, Entity character)
{
    Health &health = registry.healths.get(character);
    Projectile &projectile = registry.projectiles.get(laser);
    bool is_player_projectile = projectile.is_player_projectile;
    bool is_character_player = registry.players.has(character);
    bool causeDamage = true;
    bool steal_health = false;
    int damageMultiplier = 1;

    for (Entity entity : registry.powerUps.entities)
    {
        PowerUp &powerUp = registry.powerUps.get(entity);

        if (!powerUp.active)
            continue;

        if (powerUp.type == PowerUpType::INVINCIBILITY)
            causeDamage = false;
        else if (powerUp.type == PowerUpType::SUPER_BULLETS && !is_character_player)
            damageMultiplier = 3;
        else if (powerUp.type == PowerUpType::HEALTH_STEALER)
            steal_health = true;
    }

    // Deal damage to enemy always, to player only if invincibility is off
    if ((causeDamage && is_character_player) || !is_character_player)
    {
        int damage = health.applyDamage(projectile.bounces_remaining, is_player_projectile, damageMultiplier);

        // Show damage effect
        vec3 color;
        float scale;

        if (is_character_player)
        {
            color = {1.f, 0.f, 0.133f};
            scale = 1.25f;
        }
        else
        {
            color = {0.f, 1.f, 1.f};
            scale = 0.87f;
        }

        vec2 characterPos = registry.motions.get(character).position;
        vec2 updatedPosition = renderer->calculatePosInCamera(characterPos);
        createText(renderer, "-" + std::to_string(damage), updatedPosition, scale, color);
        health_check(health, character);

        if (steal_health && !is_character_player)
        {
            Health &playerHealth = registry.healths.get(player);
            playerHealth.addHealth(damage);
        }
    }

    // Remove the projectile
    registry.remove_all_components_of(laser);
}

void WorldSystem::health_check(Health &health, const Entity &character)
{
    if (health.value == 0)
    {
        if (registry.players.has(character))
        {
            if (registry.deathTimers.size() < 1)
            {
                registry.deathTimers.emplace(character);
                registry.motions.get(character).velocity = vec2(0, 0);

                Animation &player_anim = registry.animations.get(character);
                player_anim.current_frame = 0;
                player_anim.is_playing = false;
                registry.colors.get(character) = {1.0f, 0.0f, 0.0f}; // red

                Mix_PlayChannel(-1, player_death_sound, 0);
            }
            Mix_PlayChannel(-1, player_death_sound, 0);
        }
        else
        {
            // If enemy dies, remove all components of the enemy
            Mix_PlayChannel(-1, enemy_death_sound, 0);
            registry.remove_all_components_of(character);
        }
    }
}
int run = 0;
// Compute collisions between entities
void WorldSystem::handle_collisions(float elapsed_ms)
{
    elapsed_ms += 0.f; // to hide errors
    // Loop over all collisions detected by the physics system
    std::unordered_map<unsigned int, std::pair<Entity, Entity>> handled_bullets;
    auto &collisionsRegistry = registry.collisions;
    run++;
    for (uint i = 0; i < collisionsRegistry.components.size(); i++)
    {
        // The entity and its collider
        Entity entity = collisionsRegistry.entities[i];
        Entity entity_other = collisionsRegistry.components[i].other;

        // for now, we are only interested in collisions that involve the player
        if (registry.players.has(entity))
        {
            if (registry.enemies.has(entity_other) || registry.walls.has(entity_other))
            {
                Motion &playerMotion = registry.motions.get(entity);
                Motion &wallMotion = registry.motions.get(entity_other);

                vec2 diff = playerMotion.position - wallMotion.position;
                vec2 wallNorm;

                if (abs(diff.y / wallMotion.scale.y) < abs(diff.x / wallMotion.scale.x))
                {
                    wallNorm = {1.0f, 0.0f};
                }
                else
                {
                    wallNorm = {0.0f, 1.0f};
                }

                vec2 slideVelocity = playerMotion.velocity - dot(playerMotion.velocity, wallNorm) * wallNorm;

                float bufferGap = 1.0f;
                float xOverlap = (playerMotion.scale.x / 2 + wallMotion.scale.x / 2 - playerMotion.scale.x + bufferGap) - abs(diff.x);
                float yOverlap = (playerMotion.scale.y / 2 + wallMotion.scale.y / 2 + bufferGap) - abs(diff.y);

                if (wallNorm.x == 1.0f && xOverlap > 0)
                {

                    if (diff.x < 0)
                    {
                        playerMotion.position.x -= xOverlap;
                    }
                    else
                    {
                        playerMotion.position.x += xOverlap;
                    }
                    playerMotion.velocity.x = slideVelocity.x;
                }
                else if (wallNorm.y == 1.0f && yOverlap > 0)
                {

                    if (diff.y < 0)
                    {
                        playerMotion.position.y -= yOverlap;
                    }
                    else
                    {
                        playerMotion.position.y += yOverlap;
                    }
                    playerMotion.velocity.y = slideVelocity.y;
                }
            }

            if (
                registry.projectiles.has(entity_other) &&
                !registry.projectiles.get(entity_other).is_player_projectile &&
                !registry.deathTimers.has(entity))
            {
                projectile_hit_character(entity_other, entity);
            }

            if (registry.powerUps.has(entity_other) && !registry.powerUps.get(entity_other).active)
            {
                PowerUp &powerUp = registry.powerUps.get(entity_other);
                powerUp.active = true;
                registry.renderRequests.remove(entity_other);

                if (powerUp.type == PowerUpType::INVINCIBILITY)
                    Mix_PlayChannel(-1, invincibility_sound, 0);
                else if (powerUp.type == PowerUpType::SUPER_BULLETS)
                    Mix_PlayChannel(-1, super_bullets_sound, 0);
                else if (powerUp.type == PowerUpType::HEALTH_STEALER)
                    Mix_PlayChannel(-1, health_stealer_sound, 0);
            }
        }

        // handle collisions between projectiles and walls
        else if (registry.projectiles.has(entity) && registry.walls.has(entity_other))
        {
            if (handled_bullets.count(entity) == 0)
            {
                std::pair<Entity, Entity> p(entity, entity_other);
                handled_bullets.emplace(entity, p);
            }
            else
            {
                Motion &projMotion = registry.motions.get(entity);
                Motion &wallMotion = registry.motions.get(entity_other);
                Motion &prevWallMotion = registry.motions.get(handled_bullets[entity].second);

                float prevDiff = distance(projMotion.position, prevWallMotion.position);
                float currDiff = distance(projMotion.position, wallMotion.position);

                if (currDiff < prevDiff)
                {
                    std::pair<Entity, Entity> p(entity, entity_other);
                    handled_bullets[entity] = p;
                }
            }
        }

        // collision between enemies and walls
        else if (registry.enemies.has(entity))
        {
            if (registry.walls.has(entity_other))
            {
                if (registry.motions.has(entity))
                {

                    Motion &enemyMotion = registry.motions.get(entity);
                    Motion &wallMotion = registry.motions.get(entity_other);

                    vec2 diff = enemyMotion.position - wallMotion.position;
                    vec2 wallNorm;

                    if (abs(diff.y / wallMotion.scale.y) < abs(diff.x / wallMotion.scale.x))
                    {
                        wallNorm = {1.0f, 0.0f};
                    }
                    else
                    {
                        wallNorm = {0.0f, 1.0f};
                    }

                    vec2 slideVelocity = enemyMotion.velocity - dot(enemyMotion.velocity, wallNorm) * wallNorm;

                    float bufferGap = 1.0f;
                    float xOverlap = (enemyMotion.scale.x / 2 + wallMotion.scale.x / 2 - enemyMotion.scale.x + bufferGap) - abs(diff.x);
                    float yOverlap = (enemyMotion.scale.y / 2 + wallMotion.scale.y / 2 + bufferGap) - abs(diff.y);

                    if (wallNorm.x == 1.0f && xOverlap > 0)
                    {

                        if (diff.x < 0)
                        {
                            enemyMotion.position.x -= xOverlap;
                        }
                        else
                        {
                            enemyMotion.position.x += xOverlap;
                        }
                        enemyMotion.velocity.x = slideVelocity.x;
                    }
                    else if (wallNorm.y == 1.0f && yOverlap > 0)
                    {

                        if (diff.y < 0)
                        {
                            enemyMotion.position.y -= yOverlap;
                        }
                        else
                        {
                            enemyMotion.position.y += yOverlap;
                        }
                        enemyMotion.velocity.y = slideVelocity.y;
                    }
                }
            }

            // Test for projectile hitting enemy

            if (registry.projectiles.has(entity_other))
            {
                Projectile p = registry.projectiles.get(entity_other);
                // Needs to have bounced once before enemy can be hit
                if (p.is_player_projectile)
                {
                    projectile_hit_character(entity_other, entity);
                }
            }
        }
    }

    for (auto &it : handled_bullets)
    {
        Entity entity = it.second.first;
        if (!registry.projectiles.has(entity))
            continue;

        Entity entity_other = it.second.second;
        Projectile &projectile = registry.projectiles.get(entity);
        if (projectile.bounces_remaining-- == 0)
        {
            registry.remove_all_components_of(entity);
            continue;
        }
        else
        {
            TEXTURE_ASSET_ID id = TEXTURE_ASSET_ID::PROJECTILE_CHARGED;
            if (projectile.bounces_remaining == 0)
            {
                id = TEXTURE_ASSET_ID::PROJECTILE_SUPER_CHARGED;
            }
            registry.renderRequests.get(entity).used_texture = id;
        }

        Motion &projMotion = registry.motions.get(entity);
        Motion &wallMotion = registry.motions.get(entity_other);
        vec2 normal;

        // chooses which wall normal to reflect off of based on projectile collision direction
        projMotion.position -= projMotion.last_physic_move; // move projectile outside of wall collision
        vec2 diffVec = projMotion.position - wallMotion.position;
        vec2 size = projMotion.scale + wallMotion.scale;

        if (abs(diffVec.x / size.x) > abs(diffVec.y / size.y))
        {
            normal = vec2(1, 0);
            projMotion.angle = (2 * M_PI) - projMotion.angle;
        }
        else
        {
            normal = vec2(0, 1);
            projMotion.angle = -projMotion.angle - M_PI;
        }

        projMotion.velocity = reflect(projMotion.velocity, normal);
    }

    // Remove all collisions from this simulation step
    registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
    return bool(glfwWindowShouldClose(window));
}

void WorldSystem::update_player_move_dir()
{
    Player p = registry.players.get(player);
    Motion &motion = registry.motions.get(player);
    motion.velocity = length(move_direction) >= 1 ? normalize(move_direction) * p.DEFAULT_SPEED : vec2(0, 0);
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // key is of 'type' GLFW_KEY_
    // action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    Motion &motion = registry.motions.get(player);

    if (!registry.deathTimers.has(player))
    {
        // player dashing
        if (action == GLFW_PRESS && key == GLFW_KEY_SPACE)
        {
            Dash &player_dash = registry.dashes.get(player);
            if (player_dash.charges > 0)
            {
                player_dash.charges--;
                player_dash.recharge_timer = player_dash.recharge_cooldown;
                player_dash.remaining_dash_time = player_dash.max_dash_time;
                player_dash.dash_direction = motion.last_move_direction;
            }
        }
    }

    // player movement
    if (action != GLFW_REPEAT)
    {
        if (key == GLFW_KEY_W)
        {
            move_direction.y += action == GLFW_PRESS ? -1 : 1;
        }
        else if (key == GLFW_KEY_S)
        {
            move_direction.y += action == GLFW_PRESS ? 1 : -1;
        }
        else if (key == GLFW_KEY_A)
        {
            move_direction.x += action == GLFW_PRESS ? -1 : 1;
        }
        else if (key == GLFW_KEY_D)
        {
            move_direction.x += action == GLFW_PRESS ? 1 : -1;
        }
        if (length(move_direction) >= 1)
        {
            motion.last_move_direction = normalize(move_direction);
        }
        if (!registry.deathTimers.has(player))
        {
            update_player_move_dir();
        }
    }

    /* Entity mainMenuEntity; */
    // Exiting game on Esc
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE && !registry.deathTimers.has(player))
    {
        int activeScreen = renderer->getActiveScreen();
        if (activeScreen == (int)SCREEN_ID::MAIN_MENU || activeScreen == (int)SCREEN_ID::DEATH_SCREEN)
        {
            glfwSetWindowShouldClose(window, true);
        }
        else if (activeScreen == (int)SCREEN_ID::TUTORIAL_SCREEN)
        {
            renderer->setActiveScreen((int)SCREEN_ID::MAIN_MENU);
        }
        else if (activeScreen == (int)SCREEN_ID::GAME_SCREEN)
        {
            m_isPaused = !m_isPaused;
            renderer->setActiveScreen((int)SCREEN_ID::PAUSE_SCREEN);
        }
        else if (activeScreen == (int)SCREEN_ID::PAUSE_SCREEN)
        {
            m_isPaused = false;
            renderer->setActiveScreen((int)SCREEN_ID::GAME_SCREEN);

            // button outline no longer stays after unpausing
            for (Entity e : registry.clickables.entities)
            {
                Clickable &c = registry.clickables.get(e);
                c.isCurrentlyHoveredOver = false;
            }
            registry.renderRequests.remove(renderer->getHoverEntity());
        }
        renderer->flipActiveButtions(renderer->getActiveScreen());
    }

    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        restart_game();
    }

    // Debugging
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_RELEASE)
            debugging.in_debug_mode = false;
        else
            debugging.in_debug_mode = true;
    }

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_2)
        {
            toggle = DISTORT_ON;
        }
        else if (key == GLFW_KEY_1)
        {
            toggle = DISTORT_OFF;
        }
    }

    // Control the current speed with `<` `>`
    if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA)
    {
        current_speed -= 0.1f;
        printf("Current speed = %f\n", current_speed);
    }
    if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD)
    {
        current_speed += 0.1f;
        printf("Current speed = %f\n", current_speed);
    }
    current_speed = fmax(0.f, current_speed);
}

void WorldSystem::on_mouse_move(vec2 mouse_position)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // xpos and ypos are relative to the top-left of the window, the player's
    // default facing direction is (1, 0)
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    int activeScreen = renderer->getActiveScreen();
    Entity hoverEntity = renderer->getHoverEntity();
    if (activeScreen == (int)SCREEN_ID::TUTORIAL_SCREEN)
    {
        return;
    }
    else if (activeScreen != (int)SCREEN_ID::GAME_SCREEN)
    {
        for (Entity e : registry.clickables.entities)
        {
            bool mouseOver = mouseOverBox(mouse_position, e);
            Clickable &c = registry.clickables.get(e);
            if (c.screenTiedTo != activeScreen)
            {
                continue;
            }
            if (mouseOver)
            {
                c.isCurrentlyHoveredOver = true;
                Motion &clickableMotion = registry.motions.get(e);
                Motion &hoverMotion = registry.motions.get(hoverEntity);
                hoverMotion.position = clickableMotion.position;
            }
            else
            {
                c.isCurrentlyHoveredOver = false;
            }
        }
    }

    else
    {
        if (registry.deathTimers.has(player))
            return;
        Motion &motion = registry.motions.get(player);

        int w, h;
        glfwGetWindowSize(window, &w, &h);
        vec2 screenCenter = vec2(w / 2, h / 2);
        vec2 direction = screenCenter - mouse_position;
        vec2 direction_normalized = normalize(direction);
        float angle = atan2(direction_normalized.y, direction_normalized.x);
        motion.angle = angle;

        // Deal with mouse gestures
        if (mouseGestures.isToggled)
        {
            if (mouseGestures.isHeld)
            {
                mouseGestures.gesturePath.push_back(mouse_position);
                mouseGestures.renderPath.push_back(vec2(mouse_position.x, window_height_px - mouse_position.y));
                mouseGestures.lastPosition = mouse_position;
            }
            else
            {
                if (!mouseGestures.gesturePath.empty())
                {
                    if (detect_heart_shape())
                    {
                        // Heal the player
                        Health &playerHealth = registry.healths.get(player);
                        playerHealth.value = min(playerHealth.value + 50, 100);
                        int w, h;
                        glfwGetWindowSize(window, &w, &h);
                        // Motion.position assumes top right is (window_width_px, window_height_px) when the y axis is actually flipped, so negative offset
                        vec2 textOffset = vec2(0.01 * w, -0.01 * h);
                        createText(renderer, "+ 50", vec2(w / 2, h / 2) + textOffset, 1.25f, {0.0, 1.0, 0.0});
                    }
                }
                // Just delete everything from the gesturePath
                mouseGestures.gesturePath.clear();
                mouseGestures.gesturePath.shrink_to_fit();
                mouseGestures.renderPath.clear();
                mouseGestures.renderPath.shrink_to_fit();
            }
        }
    }
}

bool WorldSystem::detect_heart_shape()
{
    auto &path = mouseGestures.gesturePath;
    // Find heart structure
    // find peaks of heart, see threshold
    int half_index = path.size() / 2;
    int first_peak_i = 0;
    for (int i = 0; i < half_index; i++)
    {
        if (path[i].y < path[first_peak_i].y)
        {
            first_peak_i = i;
        }
    }
    int second_peak_i = half_index;
    for (int i = half_index; i < path.size(); i++)
    {
        if (path[i].y < path[first_peak_i].y)
        {
            second_peak_i = i;
        }
    }

    bool peakthreshold = abs(path[first_peak_i].y - path[second_peak_i].y) < mouseGestures.peakThreshold;
    // y valley of peaks is smaller than the peaks
    int valley = (second_peak_i + first_peak_i) / 2;
    bool meanSmaller = path[valley].y > path[first_peak_i].y && path[valley].y > path[second_peak_i].y;
    // Is combined
    bool firstEndSimilar = length(path.front() - path.back()) < mouseGestures.threshold;
    bool minSize = path.size() > mouseGestures.minSize;
    return peakthreshold && meanSmaller && firstEndSimilar && minSize;
}

void WorldSystem::on_mouse_click(int button, int action, int mods)
{
    int activeScreen = renderer->getActiveScreen();
    if (action == GLFW_PRESS)
    {
        if (activeScreen == (int)SCREEN_ID::TUTORIAL_SCREEN)
        {
            return;
        }
        else if (activeScreen != (int)SCREEN_ID::GAME_SCREEN)
        {
            for (Entity e : registry.clickables.entities)
            {
                Clickable &c = registry.clickables.get(e);
                if (c.screenTiedTo != activeScreen)
                {
                    continue;
                }
                if (c.isCurrentlyHoveredOver)
                {
                    if (c.screenGoTo == (int)SCREEN_ID::EXIT_SCREEN)
                    {
                        if (c.textureID == (int)TEXTURE_ASSET_ID::SAVE_QUIT_BUTTON)
                        {
                            SaveGameToFile(renderer);
                        }
                        glfwSetWindowShouldClose(window, true);
                    }
                    else
                    {
                        renderer->setActiveScreen(c.screenGoTo);
                        renderer->flipActiveButtions(c.screenGoTo);
                        if (c.screenGoTo == (int)SCREEN_ID::GAME_SCREEN)
                        {
                            m_isPaused = false;
                        }
                    }
                    c.isCurrentlyHoveredOver = false;
                    registry.renderRequests.remove(renderer->getHoverEntity());
                }
            }
        }
        else
        {
            Motion &motion = registry.motions.get(player);
            if (button == GLFW_MOUSE_BUTTON_LEFT && !(mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS && !registry.deathTimers.has(player))
            {
                Mix_PlayChannel(-1, laser_shot_sound, 0);
                createProjectile(renderer, motion.position, motion.angle, true);
            }
        }
    }

    // Mouse Gestures for healing
    bool rightClicked = button == GLFW_MOUSE_BUTTON_RIGHT;
    bool ctrlClicked = button == GLFW_MOUSE_BUTTON_LEFT && (mods & GLFW_MOD_CONTROL);
    if (mouseGestures.isToggled && (rightClicked || ctrlClicked) && !registry.deathTimers.has(player) && activeScreen == (int)SCREEN_ID::GAME_SCREEN)
    {
        mouseGestures.isHeld = action == GLFW_PRESS;
    }
}
