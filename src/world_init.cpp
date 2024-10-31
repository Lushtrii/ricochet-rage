#include "world_init.hpp"
#include "common.hpp"
#include "tiny_ecs_registry.hpp"

#include <iostream>
#include <fstream>
#include <string>

void SaveGameToFile()
{
    std::ofstream f("../Save1.data");

    for (Entity e : registry.motions.entities)
    {
        f << "entity" << "\n";
        if (registry.motions.has(e))
        {
            Motion &motion = registry.motions.get(e);
            f << "motion" << "\n";
            f << motion.angle << "\n";
            f << motion.last_move_direction.x << "\n"
              << motion.last_move_direction.y << "\n";
            f << motion.last_physic_move.x << "\n"
              << motion.last_physic_move.y << "\n";
            f << motion.position.x << "\n"
              << motion.position.y << "\n";
            f << motion.scale.x << "\n"
              << motion.scale.y << "\n";
            f << motion.velocity.x << "\n"
              << motion.velocity.y << "\n";
        }
        if (registry.renderRequests.has(e))
        {
            RenderRequest &r = registry.renderRequests.get(e);
            f << "render_request" << "\n";
            f << (int)r.used_effect << "\n";
            f << (int)r.used_geometry << "\n";
            f << (int)r.used_texture << "\n";
        }
        if (registry.meshPtrs.has(e))
        {
            Mesh *&m = registry.meshPtrs.get(e);
            f << "mesh" << "\n";
        }
        if (registry.players.has(e))
        {
            f << "player" << "\n";
        }
        if (registry.collisions.has(e))
        {
            Collision &c = registry.collisions.get(e);
            f << "collision" << "\n";
            f << c.other << "\n";
        }
        if (registry.enemies.has(e))
        {
            Enemy &enemy = registry.enemies.get(e);
            f << "enemy" << "\n";
            f << (int)enemy.enemyState << "\n";
        }
        if (registry.healths.has(e))
        {
            Health &h = registry.healths.get(e);
            f << "health" << "\n";
            f << h.value << "\n";
        }
        if (registry.dashes.has(e))
        {
            Dash &d = registry.dashes.get(e);
            f << "dash" << "\n";
            f << d.charges << "\n";
            f << d.dash_direction.x << "\n"
              << d.dash_direction.y << "\n";
            f << d.intial_velocity << "\n";
            f << d.max_dash_charges << "\n";
            f << d.max_dash_time << "\n";
            f << d.recharge_cooldown << "\n";
            f << d.recharge_timer << "\n";
            f << d.remaining_dash_time << "\n";
        }
        if (registry.walls.has(e))
        {
            f << "wall" << "\n";
        }
        if (registry.colors.has(e))
        {
            vec3 &c = registry.colors.get(e);
            f << "color" << "\n";
            f << c.x << "\n"
              << c.y << "\n"
              << c.z << "\n";
        }
        if (registry.deathTimers.has(e))
        {
            DeathTimer &d = registry.deathTimers.get(e);
            f << "death_timer" << "\n";
            f << d.counter_ms << "\n";
        }
        if (registry.lightOfSight.has(e))
        {
            LineOfSight &l = registry.lightOfSight.get(e);
            f << "line_of_sight" << "\n";
            f << l.ray_distance << "\n";
            f << l.ray_width << "\n";
        }
        if (registry.projectiles.has(e))
        {
            Projectile &p = registry.projectiles.get(e);
            f << "projectile" << "\n";
            f << p.bounces_remaining << "\n";
            f << p.is_player_projectile << "\n";
        }
        if (registry.reloadTimes.has(e))
        {
            ReloadTime &r = registry.reloadTimes.get(e);
            f << "reload_time" << "\n";
            f << r.counter_ms << "\n";
            f << r.shoot_rate << "\n";
            f << r.take_aim_ms << "\n";
        }
        if (registry.powerUps.has(e))
        {
            PowerUp &p = registry.powerUps.get(e);
            f << "power_up" << "\n";
            f << p.duration << "\n";
        }
        if (registry.screenStates.has(e))
        {
            ScreenState &s = registry.screenStates.get(e);
            f << "screen_state" << "\n";
            f << s.darken_screen_factor << "\n";
        }
    }
    f.close();
}

void LoadInt(std::ifstream &f, int &out)
{
    std::string line;
    getline(f, line);
    out = std::stoi(line);
}

void LoadFloat(std::ifstream &f, float &out)
{
    std::string line;
    getline(f, line);
    out = std::stof(line);
}

bool LoadGameFromFile(RenderSystem *renderer)
{
    std::ifstream f("../Save1.data");
    if (!f.is_open())
    {
        return false;
    }

    std::string line;
    Entity e;
    while (getline(f, line))
    {
        if (line == "entity")
        {
            e = Entity();
        }
        else if (line == "motion")
        {
            Motion &motion = registry.motions.emplace(e);
            LoadFloat(f, motion.angle);
            LoadFloat(f, motion.last_move_direction.x);
            LoadFloat(f, motion.last_move_direction.y);
            LoadFloat(f, motion.last_physic_move.x);
            LoadFloat(f, motion.last_physic_move.y);
            LoadFloat(f, motion.position.x);
            LoadFloat(f, motion.position.y);
            LoadFloat(f, motion.scale.x);
            LoadFloat(f, motion.scale.y);
            LoadFloat(f, motion.velocity.x);
            LoadFloat(f, motion.velocity.y);
        }
        else if (line == "render_request")
        {
            RenderRequest &renderRequest = registry.renderRequests.emplace(e);
            LoadInt(f, (int &)renderRequest.used_effect);
            LoadInt(f, (int &)renderRequest.used_geometry);
            LoadInt(f, (int &)renderRequest.used_texture);
        }
        else if (line == "mesh")
        {
            Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
            registry.meshPtrs.emplace(e, &mesh);
        }
        else if (line == "player")
            Player &player = registry.players.emplace(e);
        else if (line == "collision")
        {
            Entity other;
            LoadInt(f, (int &)other);
            Collision &c = registry.collisions.emplace(e, other);
        }
        else if (line == "enemy")
        {
            Enemy &enemy = registry.enemies.emplace(e);
            LoadInt(f, (int &)enemy.enemyState);
        }
        else if (line == "health")
        {
            Health &h = registry.healths.emplace(e);
            LoadInt(f, h.value);
        }
        else if (line == "dash")
        {
            Dash &d = registry.dashes.emplace(e);
            LoadFloat(f, d.charges);
            LoadFloat(f, d.dash_direction.x);
            LoadFloat(f, d.dash_direction.y);
            LoadFloat(f, d.intial_velocity);
            LoadFloat(f, d.max_dash_charges);
            LoadFloat(f, d.max_dash_time);
            LoadFloat(f, d.recharge_cooldown);
            LoadFloat(f, d.recharge_timer);
            LoadFloat(f, d.remaining_dash_time);
        }
        else if (line == "wall")
        {
            Wall &w = registry.walls.emplace(e);
        }
        else if (line == "color")
        {
            vec3 &c = registry.colors.emplace(e);
            LoadFloat(f, c.x);
            LoadFloat(f, c.y);
            LoadFloat(f, c.z);
        }
        else if (line == "death_timer")
        {
            DeathTimer &d = registry.deathTimers.emplace(e);
            LoadFloat(f, d.counter_ms);
        }
        else if (line == "line_of_sight")
        {
            LineOfSight &l = registry.lightOfSight.emplace(e);
            LoadFloat(f, l.ray_distance);
            LoadFloat(f, l.ray_width);
        }
        else if (line == "projectile")
        {
            Projectile &p = registry.projectiles.emplace(e);
            LoadInt(f, p.bounces_remaining);
            LoadInt(f, p.is_player_projectile);
        }
        else if (line == "reload_time")
        {
            ReloadTime &r = registry.reloadTimes.emplace(e);
            LoadFloat(f, r.counter_ms);
            LoadFloat(f, r.shoot_rate);
            LoadFloat(f, r.take_aim_ms);
        }
        else if (line == "power_up")
        {
            PowerUp &p = registry.powerUps.emplace(e);
            LoadFloat(f, p.duration);
        }
        else if (line == "screen_state")
        {
            ScreenState &s = registry.screenStates.emplace(e);
            LoadFloat(f, s.darken_screen_factor);
        }
    }

    return true;
}

Entity createPlayer(RenderSystem *renderer, vec2 pos)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting player health
    registry.healths.emplace(entity);

    // Setting initial motion values
    Motion &motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    /* motion.scale = mesh.original_size * 300.f; */
    float multiplier = -0.5f;
    motion.scale = vec2({multiplier * PLAYER_BB_WIDTH, multiplier * PLAYER_BB_HEIGHT});

    // create an empty player component for our character
    registry.players.emplace(entity);
    registry.dashes.emplace(entity);
    registry.damageEffect.emplace(entity);
    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::PLAYER, // TEXTURE_COUNT indicates that no texture is needed
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}
Entity createMeleeEnemy(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    registry.healths.emplace(entity);

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.5f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, -multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.meleeAttacks.emplace(entity);

    // Add raycasting to the enemy
    LineOfSight &raycast = registry.lightOfSight.emplace(entity);
    raycast.ray_distance = 1000;
    raycast.ray_width = ENEMY_BB_WIDTH;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::MELEE_ENEMY,
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

Entity createRangedEnemy(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    registry.healths.emplace(entity);

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.5f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, -multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.reloadTimes.emplace(entity);

    // Add raycasting to the enemy
    LineOfSight &raycast = registry.lightOfSight.emplace(entity);
    raycast.ray_distance = 1000;
    raycast.ray_width = ENEMY_BB_WIDTH;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::RANGED_ENEMY,
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

Entity createLine(vec2 position, vec2 scale)
{
    Entity entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    registry.renderRequests.insert(
        entity, {TEXTURE_ASSET_ID::TEXTURE_COUNT,
                 EFFECT_ASSET_ID::TEXTURED,
                 GEOMETRY_BUFFER_ID::DEBUG_LINE});

    // Create motion
    Motion &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0, 0};
    motion.position = position;
    motion.scale = scale;

    registry.debugComponents.emplace(entity);
    return entity;
}

// create our wall entity
Entity createWall(RenderSystem *renderer, vec2 position, vec2 size, float angle)
{
    auto entity = Entity();

    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Initialize the wall
    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.angle = angle * (M_PI / 180.0f);
    motion.scale = size;

    registry.walls.emplace(entity);
    registry.renderRequests.insert(entity, {TEXTURE_ASSET_ID::WALL, // wall.png in render_system.hpp/components.hpp
                                            EFFECT_ASSET_ID::TEXTURED,
                                            GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

// create a projectile
Entity createProjectile(RenderSystem *renderer, vec2 pos, float angle, bool is_player_projectile, float speed)
{
    const float scaleMultiplier = 0.5;

    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion &motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = angle - M_PI / 2;

    vec2 direction = vec2(-cos(angle), -sin(angle));
    motion.velocity = direction * speed;
    motion.scale = vec2(PROJECTILE_BB_WIDTH, PROJECTILE_BB_HEIGHT) * scaleMultiplier;

    // create an empty player component for our character
    Projectile &projectile = registry.projectiles.emplace(entity);
    projectile.is_player_projectile = is_player_projectile;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::PROJECTILE, // TEXTURE_COUNT indicates that no texture is needed
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

// create invincibility power up
Entity createInvincibilityPowerUp(RenderSystem *renderer, vec2 position)
{

    const float scaleMultiplier = 0.5;

    auto entity = Entity();

    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.scale = vec2(POWERUP_BB_WIDTH, POWERUP_BB_HEIGHT) * scaleMultiplier;

    registry.powerUps.emplace(entity);

    registry.renderRequests.insert(entity, {TEXTURE_ASSET_ID::INVINCIBILITY,
                                            EFFECT_ASSET_ID::TEXTURED,
                                            GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}