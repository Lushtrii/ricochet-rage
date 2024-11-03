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

const size_t TOTAL_NUM_ENEMIES = 40;      // total number of enemies in the game level
const size_t MAX_NUM_ENEMIES = 10;        // maximum number of enemies on the screen
const size_t ENEMY_SPAWN_DELAY_MS = 4000; // time between enemy spawns

// create the underwater world
WorldSystem::WorldSystem()
    : points(0), next_enemy_spawn(0.f)
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

    if (background_music == nullptr || player_death_sound == nullptr || enemy_death_sound == nullptr || laser_shot_sound == nullptr)
    {
        fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n %s\n make sure the data directory is present",
                audio_path("background-music.wav").c_str(),
                audio_path("player-death-sound.wav").c_str(),
                audio_path("enemy-death-sound.wav").c_str(),
                audio_path("laser-shot-sound.wav").c_str());
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
    if (saveFileExists) {
        LoadGameFromFile(renderer_arg);
        init_values();
    }
    else 
    {
        restart_game();
    }
}

bool WorldSystem::mouseOverBox(vec2 mousePos, Entity entity) {
    Motion m = registry.motions.get(entity);
    float minX = m.position.x - abs(m.scale.x)/2;
    float maxX = m.position.x + abs(m.scale.x)/2;
    float minY = m.position.y - abs(m.scale.y)/2;
    float maxY = m.position.y + abs(m.scale.y)/2;
    return (minX <= mousePos.x) && (mousePos.x <= maxX)
        && (minY <= mousePos.y) && (mousePos.y <= maxY);
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{

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
    for (Entity &entity: registry.healths.entities) {
        Health &health = registry.healths.get(entity);
        health_check(health, entity);
    }

    auto &damageEffects = registry.damageEffect;
    // Check for damage effect
    if (damageEffects.has(player)) {
        DamageEffect &damageEffect = damageEffects.get(player);
        if (damageEffect.is_attacked) {
            damageEffect.damage_show_time -= elapsed_ms_since_last_update;
            if (registry.colors.has(player)) {
                vec3 &playerColor = registry.colors.get(player);
                // make red
                playerColor = {1.0, 0.0, 0.0};
                if (damageEffect.damage_show_time < 0) {
                    damageEffect.damage_show_time = damageEffect.max_show_time;
                    playerColor = {1, 0.8f, 0.8f};
                    damageEffect.is_attacked = false;
                }
            }

        }
    }


    // spawn new enemies
    next_enemy_spawn -= elapsed_ms_since_last_update * current_speed;
    if (num_enemies_seen < (int) TOTAL_NUM_ENEMIES && registry.enemies.components.size() <= MAX_NUM_ENEMIES && next_enemy_spawn < 0.f)
    {
        num_enemies_seen++;
        next_enemy_spawn = (ENEMY_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (ENEMY_SPAWN_DELAY_MS / 2);

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
            if (length(playerMotion.position - spawn_pos) < 150.0f) {
                is_valid_spawn = false;
            }

            for (Entity entity : registry.walls.entities) {
                Motion &wall_motion = registry.motions.get(entity);
                if (length(wall_motion.position - spawn_pos) < 100.f)
                {
                    is_valid_spawn = false;
                    break;
                }
            }
        }

        bool spawn_melee = uniform_dist(rng) < 0.5;
        if (spawn_melee)
        {
            createMeleeEnemy(renderer, spawn_pos);
        }
        else
        {
            createRangedEnemy(renderer, spawn_pos);
        }
    }

    // Win condition
    if (num_enemies_seen == TOTAL_NUM_ENEMIES && (int) registry.enemies.entities.size() == 0) {
        renderer->setActiveScreen((int)SCREEN_ID::WIN_SCREEN);
        renderer->flipActiveButtions(renderer->getActiveScreen());
        m_isPaused = true;
        std::remove("../Save1.data");
        restart_game();
        return true;
    }


    // Processing the player state
    assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    for (Entity entity : registry.deathTimers.entities)
    {
        registry.deathTimers.remove(entity);
        screen.darken_screen_factor = 0;
        renderer->setActiveScreen((int) SCREEN_ID::DEATH_SCREEN);
        renderer->flipActiveButtions(renderer->getActiveScreen());
        std::remove("../Save1.data");
        m_isPaused = true;
        restart_game();
        return true;
    }

    return true;
}

void WorldSystem::init_values()
{
    // Reset the game speed
    current_speed = 1.f;

    // Reset the number of enemies seen
    num_enemies_seen = 0;

    player = registry.players.entities.back();
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
    for (int i = (int)registry.motions.size() - 1; i >= 0; i--) {
        Entity e = registry.motions.entities[i];
        if (!registry.clickables.has(e) && e != renderer->getHoverEntity()) {
            registry.remove_all_components_of(e);
        }
    }

    // Debugging for memory/component leaks
    registry.list_all_components();

    // create a new player
    createPlayer(renderer, {window_width_px / 2 - 150, window_height_px / 2});
    init_values();
    registry.colors.insert(player, {1, 0.8f, 0.8f});
    update_player_move_dir();

    // create the game walls
    createWall(renderer, vec2(window_width_px / 2.f, window_height_px / 2.f), vec2(100, 100), 0);
    createWall(renderer, vec2(window_width_px / 2.f + 100, window_height_px / 2.f), vec2(100, 100), 0);

    // updated wall
    // top right corner
    createWall(renderer, vec2(window_width_px - 50, window_height_px - 675), vec2(50, 50), 0);

    // top left corner
    createWall(renderer, vec2(window_width_px - 1200, window_height_px - 675), vec2(50, 50), 0);

    // bot right corner
    createWall(renderer, vec2(window_width_px - 50, window_height_px - 75), vec2(50, 50), 0);

    // bot left corner
    createWall(renderer, vec2(window_width_px - 1200, window_height_px - 75), vec2(50, 50), 0);

    for (int i = 100; i <= 1150; i += 50)
    {
        // Top wall
        createWall(renderer, vec2(window_width_px - i, window_height_px - 675), vec2(50, 50), 0);

        // Bottom wall
        createWall(renderer, vec2(window_width_px - i, window_height_px - 75), vec2(50, 50), 0);
    }

    for (int i = 125; i <= 625; i += 50)
    {
        // Left wall
        createWall(renderer, vec2(window_width_px - 1200, window_height_px - i), vec2(50, 50), 0);

        // Right wall
        createWall(renderer, vec2(window_width_px - 50, window_height_px - i), vec2(50, 50), 0);
    }

    // create power ups
    // createInvincibilityPowerUp(renderer, {window_width_px / 2.f - 100, window_height_px / 2.f - 100});
}

void WorldSystem::projectile_hit_character(Entity laser, Entity character)
{
    Health &health = registry.healths.get(character);
    Projectile &projectile = registry.projectiles.get(laser);
    health.applyDamage(projectile.bounces_remaining);

    health_check(health, character);

    // Remove the projectile
    registry.remove_all_components_of(laser);
}

void WorldSystem::health_check(Health &health, const Entity &character)
{
    if (health.value == 0)
    {
        if (registry.players.has(character))
        {
            if (registry.deathTimers.size() < 1) {
                registry.deathTimers.emplace(character);
                registry.motions.get(character).velocity = vec2(0,0);
            
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

// Compute collisions between entities
void WorldSystem::handle_collisions(float elapsed_ms)
{
    elapsed_ms += 0.f; // to hide errors
    // Loop over all collisions detected by the physics system
    std::unordered_map<unsigned int, bool> handled_bullets; 
    auto &collisionsRegistry = registry.collisions;
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
                playerMotion.position -= playerMotion.last_physic_move;
            }

            if (
                registry.projectiles.has(entity_other) &&
                !registry.projectiles.get(entity_other).is_player_projectile &&
                !registry.deathTimers.has(entity))
            {
                projectile_hit_character(entity_other, entity);
            }
        }

        // handle collisions between projectiles and walls
        else if (registry.projectiles.has(entity) && registry.walls.has(entity_other))
        {
            if (handled_bullets.count(entity) == 0) {
                handled_bullets.emplace(entity, true);
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
                    vec2 normal = vec2(1,0);
                    vec2 normal2 = vec2(0,1);
                    
                    // chooses which wall normal to reflect off of based on projectile collision direction
                    projMotion.position -= projMotion.last_physic_move; // move projectile outside of wall collision
                    float dist = length(projMotion.last_physic_move);
                    vec2 reflectedVelocity = reflect(projMotion.velocity, normal);
                    projMotion.position += normalize(reflectedVelocity) * dist;
        
                    if (PhysicsSystem::collides(projMotion, wallMotion)) {
                        projMotion.position -= normalize(reflectedVelocity) * dist;
                        reflectedVelocity = reflect(projMotion.velocity, normal2);
                        projMotion.angle = M_PI - projMotion.angle;
                    }
                    else {
                        projMotion.angle = -projMotion.angle;
                    }
                    
                    projMotion.velocity = reflectedVelocity;
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
                    enemyMotion.position -= enemyMotion.last_physic_move;
                }
            }

            if (registry.projectiles.has(entity_other) && registry.projectiles.get(entity_other).is_player_projectile)
            {
                projectile_hit_character(entity_other, entity);
            }
        }
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
    const float player_speed = 100.f;
    Motion &motion = registry.motions.get(player);
    motion.velocity = length(move_direction) >= 1 ? normalize(move_direction) * player_speed : vec2(0, 0);
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
        if (!registry.deathTimers.has(player)) {
            update_player_move_dir();
        }
        
    }

    /* Entity mainMenuEntity; */
    // Exiting game on Esc
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE && !registry.deathTimers.has(player))
    {
        int activeScreen = renderer->getActiveScreen();
        if (activeScreen == (int) SCREEN_ID::MAIN_MENU || activeScreen == (int) SCREEN_ID::DEATH_SCREEN) {
            glfwSetWindowShouldClose(window, true);
        }
        else if (activeScreen == (int)SCREEN_ID::TUTORIAL_SCREEN) {
            renderer->setActiveScreen((int)SCREEN_ID::MAIN_MENU);
        }
        else if (activeScreen == (int)SCREEN_ID::GAME_SCREEN){
            m_isPaused = !m_isPaused;
            renderer->setActiveScreen((int)SCREEN_ID::PAUSE_SCREEN);
        }
        else if (activeScreen == (int) SCREEN_ID::PAUSE_SCREEN) {
            m_isPaused = false;
            renderer->setActiveScreen((int)SCREEN_ID::GAME_SCREEN);
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
    if (activeScreen == (int)SCREEN_ID::TUTORIAL_SCREEN) {
        return;
    }
    else if (activeScreen != (int)SCREEN_ID::GAME_SCREEN) {
        for (Entity e : registry.clickables.entities) {
            bool mouseOver = mouseOverBox(mouse_position, e);
            Clickable& c = registry.clickables.get(e);
            if (c.screenTiedTo != activeScreen) {
                continue;
            }
            if (mouseOver) {
                c.isCurrentlyHoveredOver = true;
                Motion& clickableMotion = registry.motions.get(e);
                Motion& hoverMotion = registry.motions.get(hoverEntity);
                hoverMotion.position = clickableMotion.position;
                }
            else {
                c.isCurrentlyHoveredOver = false;
                }
            }
    }

    else {
        if (registry.deathTimers.has(player))
            return;
        Motion &motion = registry.motions.get(player);
        vec2 direction = motion.position - mouse_position;
        vec2 direction_normalized = normalize(direction);
        float angle = atan2(direction_normalized.y, direction_normalized.x);
        motion.angle = angle;
    }
}

void WorldSystem::on_mouse_click(int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        int activeScreen = renderer->getActiveScreen();
        if (activeScreen == (int) SCREEN_ID::TUTORIAL_SCREEN) {
            return;
        }
        else if (activeScreen != (int)SCREEN_ID::GAME_SCREEN) {
            for (Entity e : registry.clickables.entities) {
                Clickable& c = registry.clickables.get(e);
                if (c.screenTiedTo != activeScreen) {
                    continue;
                }
                if (c.isCurrentlyHoveredOver) {
                    if (c.screenGoTo == (int)SCREEN_ID::EXIT_SCREEN) {
                        if (c.textureID == (int)TEXTURE_ASSET_ID::SAVE_QUIT_BUTTON) {
                        SaveGameToFile(renderer);
                        }
                        glfwSetWindowShouldClose(window, true);
                    }
                    else {
                        renderer->setActiveScreen(c.screenGoTo);
                        renderer->flipActiveButtions(c.screenGoTo);
                        if (c.screenGoTo == (int)SCREEN_ID::GAME_SCREEN) {
                            m_isPaused = false;
                        }
                    }
                    c.isCurrentlyHoveredOver = false;
                    registry.renderRequests.remove(renderer->getHoverEntity());
                }
            }

        }
        else {
            Motion &motion = registry.motions.get(player);
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !registry.deathTimers.has(player))
            {
                Mix_PlayChannel(-1, laser_shot_sound, 0);
                createProjectile(renderer, motion.position, motion.angle, true);
            }

        }
    }
}
