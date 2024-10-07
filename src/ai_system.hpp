#pragma once

#include <vector>
#include <random>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"
#include "world_init.hpp"

class AISystem
{
	RenderSystem *renderer_arg;
public:
	void init(RenderSystem *renderer_arg);
    void step(float elapsed_ms);
private:
    void simple_chase(float elapsed_ms, Motion &playersMotion);
    void simple_chase_enemy(Entity &curr_entity, Motion &playersMotion);
    void stop_and_shoot(Entity &enemy, ReloadTime &counter, float elapsed_ms, Motion &playerMotion);
    void context_chase(Entity &enemy,  Motion &playerMotion);

    float const enemySpeed = 50.f;
	float const followingConstant = 0.4f;
	float const distanceToWalls = 150.0f;
    float const aggroDistance = 400.0f;
    const float original_ms = 3000;
	const float take_aim_ms = 1500;
	const float shoot_rate = 500;
    const float obstacleForce = 120.0f;

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};