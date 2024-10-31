#pragma once
#include <vector>

#include "tiny_ecs.hpp"
#include "components.hpp"

#include <string>

class ECSRegistry
{
    // Callbacks to remove a particular or all entities in the system
    std::vector<ContainerInterface *> registry_list;

public:
    // Manually created list of all components this game has
    // TODO: A1 add a LightUp component
    ComponentContainer<DeathTimer> deathTimers;
    ComponentContainer<Motion> motions;
    ComponentContainer<Collision> collisions;
    ComponentContainer<Player> players;
    ComponentContainer<Projectile> projectiles;
    ComponentContainer<Mesh *> meshPtrs;
    ComponentContainer<RenderRequest> renderRequests;
    ComponentContainer<ScreenState> screenStates;
    ComponentContainer<Enemy> enemies;
    ComponentContainer<DebugComponent> debugComponents;
    ComponentContainer<vec3> colors;
    ComponentContainer<Wall> walls;
    ComponentContainer<ReloadTime> reloadTimes;
    ComponentContainer<LineOfSight> lightOfSight;
    ComponentContainer<Dash> dashes;
    ComponentContainer<Health> healths;
    ComponentContainer<PowerUp> powerUps;
    ComponentContainer<MeleeAttack> meleeAttacks;
    ComponentContainer<DamageEffect> damageEffect;

    // constructor that adds all containers for looping over them
    // IMPORTANT: Don't forget to add any newly added containers!
    ECSRegistry()
    {
        // TODO: A1 add a LightUp component
        registry_list.push_back(&deathTimers);
        registry_list.push_back(&motions);
        registry_list.push_back(&collisions);
        registry_list.push_back(&players);
        registry_list.push_back(&meshPtrs);
        registry_list.push_back(&renderRequests);
        registry_list.push_back(&screenStates);
        registry_list.push_back(&enemies);
        registry_list.push_back(&debugComponents);
        registry_list.push_back(&colors);
        registry_list.push_back(&projectiles);
        registry_list.push_back(&walls);
        registry_list.push_back(&reloadTimes);
        registry_list.push_back(&lightOfSight);
        registry_list.push_back(&dashes);
        registry_list.push_back(&healths);
        registry_list.push_back(&powerUps);
        registry_list.push_back(&meleeAttacks);
        registry_list.push_back(&damageEffect);
    }

    void clear_all_components()
    {
        for (ContainerInterface *reg : registry_list)
            reg->clear();
    }

    void list_all_components()
    {
        printf("Debug info on all registry entries:\n");
        for (ContainerInterface *reg : registry_list)
            if (reg->size() > 0)
                printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
    }

    void list_all_components_of(Entity e)
    {
        printf("Debug info on components of entity %u:\n", (unsigned int)e);
        for (ContainerInterface *reg : registry_list)
            if (reg->has(e))
                printf("type %s\n", typeid(*reg).name());
    }

    void remove_all_components_of(Entity e)
    {
        for (ContainerInterface *reg : registry_list)
            reg->remove(e);
    }
};

extern ECSRegistry registry;
