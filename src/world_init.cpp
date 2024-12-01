#include "world_init.hpp"
#include "common.hpp"
#include "components.hpp"
#include "render_system.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_system.hpp"
#include "wfc/tiling_wfc.hpp"
#include "wfc/array2D.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

void SaveGameToFile(RenderSystem *renderer)
{
    std::ofstream f("../Save1.data");

    // Remove all health bars so they do not re-appear when reloaded
    while (registry.healthBars.entities.size() > 0)
        registry.remove_all_components_of(registry.healthBars.entities.back());

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
        if (registry.renderRequests.has(e) && !registry.clickables.has(e) && e != renderer->getHoverEntity())
        {
            RenderRequest &r = registry.renderRequests.get(e);
            f << "render_request" << "\n";
            f << (int)r.used_effect << "\n";
            f << (int)r.used_geometry << "\n";
            f << (int)r.used_texture << "\n";
        }
        if (registry.meshPtrs.has(e))
        {
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
            f << p.available_timer << "\n";
            f << p.active_timer << "\n";
            f << p.active << "\n";
            f << (int)p.type << "\n";
        }
        if (registry.screenStates.has(e))
        {
            ScreenState &s = registry.screenStates.get(e);
            f << "screen_state" << "\n";
            f << s.darken_screen_factor << "\n";
        }
        if (registry.animations.has(e))
        {
            Animation &a = registry.animations.get(e);
            f << "animation" << "\n";
            f << a.current_time << "\n";
            f << a.frame_time << "\n";
            f << a.current_frame << "\n";
            f << a.num_frames << "\n";
            f << a.sprite_width << "\n";
            f << a.sprite_height << "\n";
            f << a.is_playing << "\n";
            f << a.loop << "\n";
        }
        if (registry.bosses.has(e))
        {
            f << "boss" << '\n';
        }
        if (registry.teleporters.has(e))
        {
            Teleporter &t = registry.teleporters.get(e);
            f << "teleporter" << "\n";
            f << t.animation_time << "\n";
            f << t.max_teleport_time << "\n";
            f << t.prevScale.x << "\n"
              << t.prevScale.y << "\n";
        }
        if (registry.teleporting.has(e))
        {
            Teleporting &t = registry.teleporting.get(e);
            f << "teleporting" << "\n";
            f << t.starting_time << "\n";
            f << t.max_time << "\n";
        }
        if (registry.necromancers.has(e))
        {
            Necromancer &necro = registry.necromancers.get(e);
            f << "necromancer" << "\n";
            f << necro.spawningMinions << "\n";
            f << necro.centerPosition.x << "\n";
            f << necro.centerPosition.y << "\n";
        }
        if (registry.lightUps.has(e))
        {
            LightUp &l = registry.lightUps.get(e);
            f << "light_up" << "\n";
            f << l.timer << "\n";
        }
        // Save current level
        f << "currentlevel" << "\n";
        f << currLevels.current_level << "\n";
        f << currLevels.total_level_index << "\n";
    }
    f.close();
}

int LoadInt(std::ifstream &f)
{
    std::string line;
    getline(f, line);
    return std::stoi(line);
}

unsigned int LoadUnsignedInt(std::ifstream &f)
{
    std::string line;
    getline(f, line);
    return (unsigned int)std::stoi(line);
}

float LoadFloat(std::ifstream &f)
{
    std::string line;
    getline(f, line);
    return std::stof(line);
}

bool LoadBool(std::ifstream &f)
{
    std::string line;
    getline(f, line);
    return std::stoi(line);
}

PowerUpType LoadPowerUpType(std::ifstream &f)
{
    std::string line;
    getline(f, line);
    return (PowerUpType)std::stoi(line);
}

bool LoadGameFromFile(RenderSystem *renderer)
{
    bool saveFileExists = renderer->doesSaveFileExist();
    if (!saveFileExists)
    {
        return false;
    }
    std::ifstream f("../Save1.data");

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
            motion.angle = LoadFloat(f);
            motion.last_move_direction.x = LoadFloat(f);
            motion.last_move_direction.y = LoadFloat(f);
            motion.last_physic_move.x = LoadFloat(f);
            motion.last_physic_move.y = LoadFloat(f);
            motion.position.x = LoadFloat(f);
            motion.position.y = LoadFloat(f);
            motion.scale.x = LoadFloat(f);
            motion.scale.y = LoadFloat(f);
            motion.velocity.x = LoadFloat(f);
            motion.velocity.y = LoadFloat(f);
        }
        else if (line == "render_request")
        {
            RenderRequest &renderRequest = registry.renderRequests.emplace(e);
            renderRequest.used_effect = static_cast<EFFECT_ASSET_ID>(LoadInt(f));
            renderRequest.used_geometry = static_cast<GEOMETRY_BUFFER_ID>(LoadInt(f));
            renderRequest.used_texture = static_cast<TEXTURE_ASSET_ID>(LoadInt(f));
        }
        else if (line == "mesh")
        {
            Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
            registry.meshPtrs.emplace(e, &mesh);
        }
        else if (line == "player")
            registry.players.emplace(e);
        else if (line == "collision")
        {
            Entity other;
            other = LoadUnsignedInt(f);
            registry.collisions.emplace(e, other);
        }
        else if (line == "enemy")
        {
            Enemy &enemy = registry.enemies.emplace(e);
            enemy.enemyState = static_cast<EnemyState>(LoadInt(f));
        }
        else if (line == "health")
        {
            Health &h = registry.healths.emplace(e);
            h.value = LoadInt(f);
        }
        else if (line == "dash")
        {
            Dash &d = registry.dashes.emplace(e);
            d.charges = LoadFloat(f);
            d.dash_direction.x = LoadFloat(f);
            d.dash_direction.y = LoadFloat(f);
            d.intial_velocity = LoadFloat(f);
            d.max_dash_charges = LoadFloat(f);
            d.max_dash_time = LoadFloat(f);
            d.recharge_cooldown = LoadFloat(f);
            d.recharge_timer = LoadFloat(f);
            d.remaining_dash_time = LoadFloat(f);
        }
        else if (line == "wall")
        {
            registry.walls.emplace(e);
        }
        else if (line == "color")
        {
            vec3 &c = registry.colors.emplace(e);
            c.x = LoadFloat(f);
            c.y = LoadFloat(f);
            c.z = LoadFloat(f);
        }
        else if (line == "death_timer")
        {
            DeathTimer &d = registry.deathTimers.emplace(e);
            d.counter_ms = LoadFloat(f);
        }
        else if (line == "line_of_sight")
        {
            LineOfSight &l = registry.lightOfSight.emplace(e);
            l.ray_distance = LoadFloat(f);
            l.ray_width = LoadFloat(f);
        }
        else if (line == "projectile")
        {
            Projectile &p = registry.projectiles.emplace(e);
            p.bounces_remaining = LoadInt(f);
            p.is_player_projectile = LoadInt(f);
        }
        else if (line == "reload_time")
        {
            ReloadTime &r = registry.reloadTimes.emplace(e);
            r.counter_ms = LoadFloat(f);
            r.shoot_rate = LoadFloat(f);
            r.take_aim_ms = LoadFloat(f);
        }
        else if (line == "power_up")
        {
            PowerUp &p = registry.powerUps.emplace(e);
            p.available_timer = LoadFloat(f);
            p.active_timer = LoadFloat(f);
            p.active = LoadBool(f);
            p.type = LoadPowerUpType(f);
        }
        else if (line == "screen_state")
        {
            ScreenState &s = registry.screenStates.emplace(e);
            s.darken_screen_factor = LoadFloat(f);
        }
        else if (line == "animation")
        {
            Animation &a = registry.animations.emplace(e);
            a.current_time = LoadFloat(f);
            a.frame_time = LoadFloat(f);
            a.current_frame = LoadInt(f);
            a.num_frames = LoadInt(f);
            a.sprite_width = LoadInt(f);
            a.sprite_height = LoadInt(f);
            a.is_playing = LoadBool(f);
            a.loop = LoadBool(f);
        }
        else if (line == "boss")
        {
            Boss &b = registry.bosses.emplace(e);
        }
        else if (line == "teleporter")
        {
            Teleporter &t = registry.teleporters.emplace(e);
            t.animation_time = LoadFloat(f);
            t.max_teleport_time = LoadFloat(f);
            t.prevScale = vec2(LoadFloat(f), LoadFloat(f));
        }
        else if (line == "teleporting")
        {
            Teleporting &t = registry.teleporting.emplace(e);
            t.starting_time = LoadFloat(f);
            t.max_time = LoadFloat(f);
        }
        else if (line == "currentlevel")
        {
            currLevels.current_level = LoadInt(f);
            currLevels.total_level_index = LoadInt(f);
        }
        else if (line == "necromancer")
        {
            Necromancer &n = registry.necromancers.emplace(e);
            n.spawningMinions = LoadBool(f);
            n.centerPosition.x = LoadFloat(f);
            n.centerPosition.y = LoadFloat(f);
        }
        else if (line == "light_up")
        {
            LightUp &l = registry.lightUps.emplace(e);
            l.timer = LoadFloat(f);
        }
    }

    return true;
}

void NextRoom(RenderSystem *renderer, int seed)
{
    for (int i = (int)registry.motions.size() - 1; i >= 0; i--)
    {
        Entity e = registry.motions.entities[i];
        if (registry.players.has(e))
        {
            Motion &m = registry.motions.get(e);
            m.position = vec2(30, window_height_px / 2);
        }
        else if (!registry.clickables.has(e) && e != renderer->getHoverEntity())
        {
            registry.remove_all_components_of(e);
        }
    }

    GenerateMap(renderer, seed);
}

void GenerateMap(RenderSystem *renderer, int seed)
{

    std::vector<Tile<int>> tiles;
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> neighbors_ids;
    std::vector<int> bend({0, 0, 0, 1, 1,
                           0, 0, 0, 1, 1,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0});
    std::vector<int> corner({1, 1, 0, 0, 0,
                             1, 1, 0, 0, 0,
                             1, 1, 0, 0, 0,
                             1, 1, 1, 1, 1,
                             1, 1, 1, 1, 1});
    std::vector<int> corridor({1, 0, 0, 0, 1,
                               1, 0, 0, 0, 1,
                               1, 0, 0, 0, 1,
                               1, 0, 0, 0, 1,
                               1, 0, 0, 0, 1});
    std::vector<int> door({0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0,
                           1, 0, 0, 0, 1,
                           1, 0, 0, 0, 1});
    std::vector<int> empty({0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0});
    std::vector<int> side({1, 1, 1, 1, 1,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0});
    std::vector<int> t({1, 1, 1, 1, 1,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        1, 0, 0, 0, 1});
    std::vector<int> turn({1, 0, 0, 0, 1,
                           1, 0, 0, 0, 0,
                           1, 0, 0, 0, 0,
                           1, 0, 0, 0, 0,
                           1, 1, 1, 1, 1});
    std::vector<int> wall({1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1});
    // std::vector<int> way4({1, 0, 0, 0, 1,
    //                        0, 0, 0, 0, 0,
    //                        0, 0, 0, 0, 0,
    //                        0, 0, 0, 0, 0,
    //                        1, 0, 0, 0, 1});

    tiles.emplace_back(Array2D<int>(5, 5, bend), Symmetry::L, 0.5);
    tiles.emplace_back(Array2D<int>(5, 5, corner), Symmetry::L, 0.25);
    tiles.emplace_back(Array2D<int>(5, 5, corridor), Symmetry::I, 1.0);
    tiles.emplace_back(Array2D<int>(5, 5, door), Symmetry::T, 3.5);
    tiles.emplace_back(Array2D<int>(5, 5, empty), Symmetry::X, 0.10);
    tiles.emplace_back(Array2D<int>(5, 5, side), Symmetry::T, 1.0);
    tiles.emplace_back(Array2D<int>(5, 5, t), Symmetry::T, 3.5);
    tiles.emplace_back(Array2D<int>(5, 5, turn), Symmetry::L, 0.5);
    tiles.emplace_back(Array2D<int>(5, 5, wall), Symmetry::X, 0.05);
    // tiles.emplace_back(Array2D<int>(5, 5, way4), Symmetry::X, 1.0);

    // neighbors_ids.emplace_back(, 0, , 0);
    neighbors_ids.emplace_back(CORNER, 1, CORNER, 0);
    neighbors_ids.emplace_back(CORNER, 2, CORNER, 0);
    neighbors_ids.emplace_back(CORNER, 0, DOOR, 0);
    neighbors_ids.emplace_back(CORNER, 0, SIDE, 2);
    neighbors_ids.emplace_back(CORNER, 1, SIDE, 1);
    neighbors_ids.emplace_back(CORNER, 1, T, 1);
    neighbors_ids.emplace_back(CORNER, 1, TURN, 0);
    neighbors_ids.emplace_back(CORNER, 2, TURN, 0);
    neighbors_ids.emplace_back(WALL, 0, CORNER, 0);
    neighbors_ids.emplace_back(CORRIDOR, 1, CORRIDOR, 1);
    neighbors_ids.emplace_back(CORRIDOR, 1, DOOR, 3);
    neighbors_ids.emplace_back(CORRIDOR, 0, SIDE, 1);
    neighbors_ids.emplace_back(CORRIDOR, 1, T, 0);
    neighbors_ids.emplace_back(CORRIDOR, 1, T, 3);
    neighbors_ids.emplace_back(CORRIDOR, 1, TURN, 1);
    neighbors_ids.emplace_back(CORRIDOR, 0, WALL, 0);
    neighbors_ids.emplace_back(DOOR, 1, DOOR, 3);
    neighbors_ids.emplace_back(DOOR, 3, EMPTY, 0);
    neighbors_ids.emplace_back(DOOR, 0, SIDE, 2);
    neighbors_ids.emplace_back(DOOR, 1, T, 0);
    neighbors_ids.emplace_back(DOOR, 1, T, 3);
    neighbors_ids.emplace_back(DOOR, 1, TURN, 1);
    neighbors_ids.emplace_back(EMPTY, 0, EMPTY, 0);
    neighbors_ids.emplace_back(EMPTY, 0, SIDE, 3);
    neighbors_ids.emplace_back(SIDE, 0, SIDE, 0);
    neighbors_ids.emplace_back(SIDE, 3, SIDE, 1);
    neighbors_ids.emplace_back(SIDE, 3, T, 1);
    neighbors_ids.emplace_back(SIDE, 3, TURN, 0);
    neighbors_ids.emplace_back(SIDE, 3, WALL, 0);
    neighbors_ids.emplace_back(T, 0, T, 2);
    neighbors_ids.emplace_back(T, 0, TURN, 1);
    neighbors_ids.emplace_back(T, 3, WALL, 0);
    neighbors_ids.emplace_back(TURN, 0, TURN, 2);
    neighbors_ids.emplace_back(TURN, 1, WALL, 0);
    neighbors_ids.emplace_back(WALL, 0, WALL, 0);
    neighbors_ids.emplace_back(BEND, 0, BEND, 1);
    neighbors_ids.emplace_back(CORNER, 0, BEND, 2);
    neighbors_ids.emplace_back(DOOR, 0, BEND, 2);
    neighbors_ids.emplace_back(EMPTY, 0, BEND, 0);
    neighbors_ids.emplace_back(SIDE, 0, BEND, 1);

    // neighbors_ids.emplace_back(WAY4, 0, WAY4, 0);
    // neighbors_ids.emplace_back(WAY4, 0, CORRIDOR, 1);
    // neighbors_ids.emplace_back(WAY4, 0, DOOR, 3);
    // neighbors_ids.emplace_back(WAY4, 0, T, 0);
    // neighbors_ids.emplace_back(WAY4, 0, T, 3);
    // neighbors_ids.emplace_back(WAY4, 0, SIDE, 3);
    // neighbors_ids.emplace_back(WAY4, 0, TURN, 1);

    int height = 6;
    int width = 10;
    int tileLength = 5;
    bool periodic_output = true;

    Array2D<int> result(1, 1);
    bool success = false;
    while (!success)
    {
        success = false;
        try
        {
            TilingWFC<int> wfc = TilingWFC<int>(tiles, neighbors_ids, height, width, {periodic_output}, seed);

            wfc.set_tile(EMPTY, 0, height / 2, 0);
            wfc.set_tile(EMPTY, 0, height / 2, width - 1);

            result = wfc.run();

            // check if a valid path from start to end exists (can the level be completed?)
            std::pair<int, int> start = {result.height / 2, 0};
            std::pair<int, int> end = {result.height / 2, result.width - 1};
            std::unordered_set<std::pair<int, int>, pair_hash> visited;
            std::vector<std::pair<int, int>> Q;
            Q.push_back(start);
            while (Q.size() > 0)
            {
                std::pair<int, int> current = Q.back();
                Q.pop_back();

                visited.insert(current);

                int y = current.first;
                int x = current.second;

                if (x > 0 && result.get(y, x - 1) == 0 && visited.count({y, x - 1}) == 0)
                {
                    Q.push_back({y, x - 1});
                }
                if (x < result.width - 1 && result.get(y, x + 1) == 0 && visited.count({y, x + 1}) == 0)
                {
                    Q.push_back({y, x + 1});
                }
                if (y > 0 && result.get(y - 1, x) == 0 && visited.count({y - 1, x}) == 0)
                {
                    Q.push_back({y - 1, x});
                }
                if (y < result.height - 1 && result.get(y + 1, x) == 0 && visited.count({y + 1, x}) == 0)
                {
                    Q.push_back({y + 1, x});
                }
            }

            int path_count = 0;
            for (int x = 0; x < result.width; x++)
            {
                for (int y = 0; y < result.height; y++)
                {
                    path_count += result.get(y, x) == 0 ? 1 : 0;
                }
            }

            // std::cout << "COUNT: " << visited.size() << " " << path_count << std::endl;

            success = (path_count == visited.size());
        }
        catch (...)
        {
        }
        seed++;
    }

    vec2 tileSize = vec2(50, 50);
    for (int x = 0; x < result.width; x++)
    {
        for (int y = 0; y < result.height; y++)
        {
            int value = result.get(y, x);

            // Outer edge of room or if wfc randomly generates selected tile as wall, create tile)
            if (value == 1 || x == 0 || y == 0 || x == result.width-1 || y == result.height-1)
                createTile(renderer, vec2(x, y), tileSize, (TT)value);
        }
    }
}

void createTile(RenderSystem *renderer, vec2 pos, vec2 size, TT type)
{
    createWall(renderer, (pos * size.x) + (size * 0.5f), size);
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
    float multiplier = 0.5f;
    motion.scale = vec2({-multiplier * PLAYER_BB_WIDTH, multiplier * PLAYER_BB_HEIGHT});

    // create an empty player component for our character
    registry.players.emplace(entity);
    registry.dashes.emplace(entity);
    registry.damageEffect.emplace(entity);

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

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
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.meleeAttacks.emplace(entity);
    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

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
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.reloadTimes.emplace(entity);
    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

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

// Create Boss Enemy
Entity createCowboyBossEnemy(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    Health &bossHealth = registry.healths.emplace(entity);
    bossHealth.value = 300;

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.75f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);

    // Make more rapid attacks but more time in between
    ReloadTime &bossReload = registry.reloadTimes.emplace(entity);

    // Also a melee enemy
    registry.meleeAttacks.emplace(entity);
    registry.teleporters.emplace(entity);

    registry.bosses.emplace(entity);
    registry.necromancers.emplace(entity);

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

    // Add raycasting to the enemy
    LineOfSight &raycast = registry.lightOfSight.emplace(entity);
    raycast.ray_distance = 1000;
    raycast.ray_width = ENEMY_BB_WIDTH;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::NECROMANCER_ENEMY,
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

// Create a melee minion that deals less damage
Entity createMeleeMinion(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    registry.healths.insert(entity, {25});

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.3f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.meleeAttacks.emplace(entity);
    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

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

Entity createRangedMinion(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    registry.healths.insert(entity, {25});

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.3f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);
    registry.reloadTimes.emplace(entity);
    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

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

// Create Necromancer Enemy
Entity createNecromancerEnemy(RenderSystem *renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting enemy health
    Health &bossHealth = registry.healths.emplace(entity);
    bossHealth.value = 300;

    // Initialize the motion
    auto &motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = {0.f, 0.f};
    motion.position = position;

    float multiplier = 0.75f;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({multiplier * ENEMY_BB_WIDTH, multiplier * ENEMY_BB_HEIGHT});

    // create an empty enemies component
    registry.enemies.emplace(entity);

    // Make more rapid attacks but more time in between
    ReloadTime &bossReload = registry.reloadTimes.emplace(entity);

    // Also a melee enemy
    registry.meleeAttacks.emplace(entity);
    registry.teleporters.emplace(entity);

    registry.bosses.emplace(entity);
    registry.necromancers.emplace(entity);

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 5;

    // Add raycasting to the enemy
    LineOfSight &raycast = registry.lightOfSight.emplace(entity);
    raycast.ray_distance = 1000;
    raycast.ray_width = ENEMY_BB_WIDTH;

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::NECROMANCER_ENEMY,
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::SPRITE});

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
    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 50;
    animation.sprite_width = 50;
    animation.num_frames = 1;
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
    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::PROJECTILE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion &motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = angle + M_PI / 2;

    vec2 direction = vec2(-cos(angle), -sin(angle));
    motion.velocity = direction * speed;
    motion.scale = vec2(PROJECTILE_BB_WIDTH, PROJECTILE_BB_HEIGHT) * scaleMultiplier;

    // create an empty player component for our character
    Projectile &projectile = registry.projectiles.emplace(entity);
    projectile.is_player_projectile = is_player_projectile;

    /* Animation &animation = registry.animations.emplace(entity); */
    /* animation.sprite_height = 32; */
    /* animation.sprite_width = 13; */
    /* animation.num_frames = 1; */

    registry.renderRequests.insert(
        entity,
        {TEXTURE_ASSET_ID::PROJECTILE, // TEXTURE_COUNT indicates that no texture is needed
         EFFECT_ASSET_ID::TEXTURED,
         GEOMETRY_BUFFER_ID::PROJECTILE});

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

    PowerUp &powerUp = registry.powerUps.emplace(entity);
    powerUp.type = PowerUpType::INVINCIBILITY;

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 50;
    animation.sprite_width = 50;
    animation.num_frames = 1;
    registry.renderRequests.insert(entity, {TEXTURE_ASSET_ID::INVINCIBILITY,
                                            EFFECT_ASSET_ID::TEXTURED,
                                            GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

// create super bullets power up
Entity createSuperBulletsPowerUp(RenderSystem *renderer, vec2 position)
{
    const float scaleMultiplier = 0.5;
    auto entity = Entity();

    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.scale = vec2(POWERUP_BB_WIDTH, POWERUP_BB_HEIGHT) * scaleMultiplier;

    PowerUp &powerUp = registry.powerUps.emplace(entity);
    powerUp.type = PowerUpType::SUPER_BULLETS;

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 1;
    registry.renderRequests.insert(entity, {TEXTURE_ASSET_ID::SUPER_BULLETS,
                                            EFFECT_ASSET_ID::TEXTURED,
                                            GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

// create health stealer power up
Entity createHealthStealerPowerUp(RenderSystem *renderer, vec2 position)
{
    const float scaleMultiplier = 0.5;
    auto entity = Entity();

    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.scale = vec2(POWERUP_BB_WIDTH, POWERUP_BB_HEIGHT) * scaleMultiplier;

    PowerUp &powerUp = registry.powerUps.emplace(entity);
    powerUp.type = PowerUpType::HEALTH_STEALER;

    Animation &animation = registry.animations.emplace(entity);
    animation.sprite_height = 32;
    animation.sprite_width = 32;
    animation.num_frames = 1;
    registry.renderRequests.insert(entity, {TEXTURE_ASSET_ID::HEALTH_STEALER,
                                            EFFECT_ASSET_ID::TEXTURED,
                                            GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

Entity createHealthBar(RenderSystem *renderer, vec2 position, vec2 scale)
{
    Entity entity = Entity();

    Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    registry.renderRequests.insert(
        entity, {TEXTURE_ASSET_ID::HEALTH_BAR,
                 EFFECT_ASSET_ID::TEXTURED,
                 GEOMETRY_BUFFER_ID::SPRITE});

    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.scale = scale;

    registry.healthBars.emplace(entity);
    return entity;
}

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color)
{
    Entity entity = Entity();

    Text &screenText = registry.texts.emplace(entity);
    screenText.text = text;
    screenText.position = position;
    screenText.scale = scale;
    screenText.color = color;

    return entity;
}

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color, bool timed)
{
    Entity entity = Entity();

    Text &screenText = registry.texts.emplace(entity);
    screenText.text = text;
    screenText.position = position;
    screenText.scale = scale;
    screenText.color = color;
    screenText.timed = timed;

    return entity;
}
