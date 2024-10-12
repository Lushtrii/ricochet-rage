#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

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
    float multiplier = 0.5;
    motion.scale = vec2({multiplier * PLAYER_BB_WIDTH, multiplier * PLAYER_BB_HEIGHT});

    // create an empty player component for our character
    registry.players.emplace(entity);
    registry.dashes.emplace(entity);
    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::PLAYER, // TEXTURE_COUNT indicates that no texture is needed
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

Entity createEnemy(RenderSystem *renderer, vec2 position)
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

    float multiplier = 0.5;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.reloadTimes.emplace(entity);
    
    // Add raycasting to the enemy
    LineOfSight& raycast = registry.lightOfSight.emplace(entity);
    raycast.ray_distance = 1000;
    raycast.ray_width = ENEMY_BB_WIDTH;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::ENEMY,
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
