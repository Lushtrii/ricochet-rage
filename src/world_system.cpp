// Header
#include "world_system.hpp"
#include "common.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"

// stlib
#include <GLFW/glfw3.h>
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// create the underwater world
WorldSystem::WorldSystem() : points(0)
{
    // Seeding rng with random device
    rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	
	// destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);

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
    { ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_click(button, action, mods);};
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


	background_music = Mix_LoadMUS(audio_path("backgroundmusic.wav").c_str());

	if (background_music == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s make sure the data directory is present",
			audio_path("backgroundmusic.wav").c_str());
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
    restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
    // Updating window title with points
    std::stringstream title_ss;
    title_ss << "Points: " << points;
    glfwSetWindowTitle(window, title_ss.str().c_str());

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

    // Processing the player state
    assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_counter_ms = 3000.f;
    for (Entity entity : registry.deathTimers.entities)
    {
        // progress timer
        DeathTimer &counter = registry.deathTimers.get(entity);
        counter.counter_ms -= elapsed_ms_since_last_update;
        if (counter.counter_ms < min_counter_ms)
        {
            min_counter_ms = counter.counter_ms;
        }

        // restart the game once the death timer expired
        if (counter.counter_ms < 0)
        {
            registry.deathTimers.remove(entity);
            screen.darken_screen_factor = 0;
            restart_game();
            return true;
        }
    }
    // reduce window brightness if the player is dying
    screen.darken_screen_factor = 1 - min_counter_ms / 3000;

    return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
    // Debugging for memory/component leaks
    registry.list_all_components();
    printf("Restarting\n");

    // Reset the game speed
    current_speed = 1.f;

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, eels, ... but that would be more cumbersome
    while (registry.motions.entities.size() > 0)
        registry.remove_all_components_of(registry.motions.entities.back());

    // Debugging for memory/component leaks
    registry.list_all_components();

    // create a new player
    player = createPlayer(renderer, {window_width_px / 2 - 150, window_height_px / 2});
    registry.colors.insert(player, {1, 0.8f, 0.8f});

    createEnemy(renderer, {window_width_px / 2 + 300, window_height_px / 2});

    // create the game walls

    float wallThickness = 50.f;

    vec2 top_pos = {window_width_px / 2.f, wallThickness};
    vec2 bot_pos = {window_width_px / 2.f, window_height_px - wallThickness};

    vec2 left_pos = {wallThickness, window_height_px / 2.f};
    vec2 right_pos = {window_width_px - wallThickness, window_height_px / 2.f};

    createWall(renderer, top_pos, vec2(window_width_px, wallThickness), 0);
    createWall(renderer, bot_pos, vec2(window_width_px, wallThickness), 0);

    createWall(renderer, left_pos, vec2(wallThickness, window_height_px), 0);
    createWall(renderer, right_pos, vec2(wallThickness, window_height_px), 0);

    createWall(renderer, vec2(window_width_px / 2.f, window_height_px / 2.f), vec2(100, 100), 0);
}

void WorldSystem::projectile_hit_character(Entity laser, Entity character)
{
    Health &health = registry.healths.get(character);
    Projectile &projectile = registry.projectiles.get(laser);
    health.applyDamage(projectile.bounces_remaining);    

    if (health.value == 0) {
        if (registry.players.has(character)) {
            // If player dies, respawn and reset game state
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            restart_game();
        } else {
            // If enemy dies, remove all components of the enemy
            registry.remove_all_components_of(character);
        }
    }
    
    // Remove the projectile
    registry.remove_all_components_of(laser);
}

// Compute collisions between entities
void WorldSystem::handle_collisions(float elapsed_ms)
{
    elapsed_ms += 0.f; // to hide errors
    // Loop over all collisions detected by the physics system
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

            if (registry.projectiles.has(entity_other) && !registry.projectiles.get(entity_other).is_player_projectile)
            {
                projectile_hit_character(entity_other, entity);
            }
        }

        // handle collisions between projectiles and walls
        else if (registry.projectiles.has(entity))
        {
            if (registry.walls.has(entity_other))
            {
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

                // calculate which side of the wall the bullet hit
                vec2 diff = projMotion.position - wallMotion.position;
                vec2 diff_ratio = diff / wallMotion.scale;
                vec2 normal;
                if (abs(diff_ratio.x) > abs(diff_ratio.y))
                {
                    normal = vec2(1, 0);
                    projMotion.angle = -projMotion.angle;
                }
                else
                {
                    normal = vec2(0, 1);
                    projMotion.angle = M_PI - projMotion.angle;
                }

                vec2 reflectedVelocity = reflect(projMotion.velocity, normal);
                projMotion.position -= projMotion.last_physic_move; // move projectile outside of wall collision
                projMotion.velocity = reflectedVelocity;
            }
        } 
        
        // collision between enemies and walls
        else if (registry.enemies.has(entity)) {
            if (registry.walls.has(entity_other)) {
                if (registry.motions.has(entity)) {
                    Motion& enemyMotion = registry.motions.get(entity);
                    enemyMotion.position -= enemyMotion.last_physic_move;
                }
            }

            if (registry.projectiles.has(entity_other) && registry.projectiles.get(entity_other).is_player_projectile) {
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

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // key is of 'type' GLFW_KEY_
    // action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    const float playerSpeed = 100.f;
    Motion &motion = registry.motions.get(player);

    // player dashing 
    if (action == GLFW_PRESS && key == GLFW_KEY_SPACE) {
        Dash& player_dash = registry.dashes.get(player);
        if (player_dash.charges > 0) {
            player_dash.charges--;
            player_dash.recharge_timer = player_dash.recharge_cooldown;
            player_dash.remaining_dash_time = player_dash.max_dash_time;
            player_dash.dash_direction = motion.last_move_direction;
        }
    }

    // player movement
    if (key == GLFW_KEY_W)
    {
        motion.velocity.y = (action == GLFW_PRESS || action == GLFW_REPEAT) ? -playerSpeed : 0;
    }
    else if (key == GLFW_KEY_S)
    {
        motion.velocity.y = (action == GLFW_PRESS || action == GLFW_REPEAT) ? playerSpeed : 0;
    }
    else if (key == GLFW_KEY_A)
    {
        motion.velocity.x = (action == GLFW_PRESS || action == GLFW_REPEAT) ? -playerSpeed : 0;
    }
    else if (key == GLFW_KEY_D)
    {
        motion.velocity.x = (action == GLFW_PRESS || action == GLFW_REPEAT) ? playerSpeed : 0;
    }
    if (length(motion.velocity) >= 1) {
        motion.last_move_direction = normalize(motion.velocity);
    }

    // Exiting game on Esc
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, true);
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
    Motion &motion = registry.motions.get(player);
    vec2 direction = motion.position - mouse_position;
    vec2 direction_normalized = normalize(direction);
    float angle = atan2(direction_normalized.y, direction_normalized.x);
    motion.angle = angle;
}

void WorldSystem::on_mouse_click(int button, int action, int mods) {
    Motion &motion = registry.motions.get(player);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        createProjectile(renderer, motion.position, motion.angle, true);
    }

}
