// internal
#include "ai_system.hpp"

void AISystem::init(RenderSystem *renderSystem) {
	this->renderer_arg = renderSystem;
}

void AISystem::step(float elapsed_ms)
{
	// Find a player is it exists, else, do nothing
	if (registry.players.size() < 1)
	{
		return;
	}
	Entity &playerEntity = registry.players.entities[0];
	if (!registry.motions.has(playerEntity))
	{
		return;
	}
	Motion &playerMotion = registry.motions.get(playerEntity);

	for (Entity &enemy : registry.enemies.entities)
	{
		// State for roaming
		EnemyState& enemyState = registry.enemies.get(enemy).enemyState;
		if (enemyState == EnemyState::ROAMING) {
			if (registry.motions.has(enemy)) {
				Motion& motion = registry.motions.get(enemy);
				motion.velocity = vec2((uniform_dist(rng) - 0.5f)* 10.0f, (uniform_dist(rng) - 0.5f) * 10.0f);
				if (length(playerMotion.position - motion.position) < aggroDistance) {
					enemyState = EnemyState::PURSUING;
				}
			}
		// State for pursuing and shooting at player
		} else if (enemyState == EnemyState::PURSUING) {
			if (registry.reloadTimes.has(enemy))
			{
				ReloadTime &counter = registry.reloadTimes.get(enemy);
				counter.counter_ms -= elapsed_ms;

				if (counter.counter_ms > 0)
				{
					context_chase(enemy, playerMotion);
				}
				else
				{
					stop_and_shoot(enemy, counter, elapsed_ms, playerMotion);
				}
			}
		// State for avoiding obstacles MAY NOT NEED TO USE
		} else if (enemyState == EnemyState::AVOIDWALL) {
			if (registry.motions.has(enemy)) {
				Motion& enemyMotion = registry.motions.get(enemy);
				for (Entity &wall: registry.walls.entities) {
					Motion& wallMotion = registry.motions.get(wall);
					// If in collision course with the wall, go around it
					vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
					vec2 vectorToPlayer = -normalize(wallEnemyDelta);
					if (length(abs(wallEnemyDelta)) > distanceToWalls) {
						printf("Distance to wall: %f %f\n", length(abs(wallEnemyDelta)), distanceToWalls);
						enemyState = EnemyState::PURSUING;
					}
				}
			}
	
		}		
	}
}

// For smart context chasing, good for avoiding obstacles, although still robotic
void AISystem::context_chase(Entity &enemy,  Motion &playerMotion) {
	std::vector<vec2> directions = {vec2(0, 1), vec2(0, -1), vec2(1, 0), vec2(-1, 0), normalize(vec2(-1, 1)), normalize(vec2(1,1)), normalize(vec2(-1,-1)), normalize(vec2(1,-1))};

	Motion &enemyMotion = registry.motions.get(enemy);
	vec2 angleVector = -normalize(enemyMotion.position - playerMotion.position);
	std::vector<float> interestVector;
	for (auto &direction: directions) {
		interestVector.push_back(direction.x * angleVector.x + direction.y * angleVector.y);
	}
	std::vector<float> dangerVector(interestVector.size());
	for (Entity &wall: registry.walls.entities) {
		Motion& wallMotion = registry.motions.get(wall);
		vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
		vec2 danger_delta = -normalize(wallEnemyDelta) * obstacleForce / length(wallEnemyDelta);
		for (int i = 0; i < dangerVector.size(); i++) {
			dangerVector[i] = danger_delta.x * directions[i].x + directions[i].y * danger_delta.y;
		}
	}
	// Context Vector
	vec2 sumVelocity = vec2(0,0);
	std::vector<float> contextVector(interestVector.size());
	for (int i = 0; i < interestVector.size(); i++) {
		contextVector[i] = interestVector[i] - dangerVector[i];
		sumVelocity += contextVector[i] * directions[i];
	}
	enemyMotion.velocity = normalize(sumVelocity) * enemySpeed;
}

void AISystem::stop_and_shoot(Entity &enemy, ReloadTime &counter, float elapsed_ms, Motion &playerMotion)
{
    if (registry.motions.has(enemy))
    {
        counter.take_aim_ms -= elapsed_ms;
		counter.shoot_rate -= elapsed_ms;
        Motion &enemyMotion = registry.motions.get(enemy);
        enemyMotion.velocity = vec2(0.0f, 0.0f);

		if (counter.shoot_rate < 0) {
            vec2 angleVector = normalize(enemyMotion.position - playerMotion.position);
            enemyMotion.angle = atan2(angleVector.y, angleVector.x);
            createProjectile(renderer_arg, enemyMotion.position, enemyMotion.angle, false);
			counter.shoot_rate = shoot_rate;
		}

        if (counter.take_aim_ms < 0)
        {
            counter.counter_ms = original_ms;
            counter.take_aim_ms = take_aim_ms;
        }
    }
};

// Extremely simple chase that goes to the players direction, does the Roomba thing when approaching a wall
void AISystem::simple_chase(float elapsed_ms, Motion &playersMotion)
{
	(float)elapsed_ms;
	for (uint i = 0; i < registry.enemies.size(); i++)
	{
		Entity &curr_entity = registry.enemies.entities[i];
		simple_chase_enemy(curr_entity, playersMotion);
	}
}

void AISystem::simple_chase_enemy(Entity &enemy, Motion &playersMotion)
{
	if (registry.motions.has(enemy))
	{
		Motion &enemyMotion = registry.motions.get(enemy);
		for (Entity &wall: registry.walls.entities) {
			Motion& wallMotion = registry.motions.get(wall);
			vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
			// Go directly at the player
			vec2 angleVector = -normalize(enemyMotion.position - playersMotion.position + followingConstant * playersMotion.velocity);
			float angle = atan2(angleVector.y, angleVector.x);
			enemyMotion.velocity = angleVector * enemySpeed;
			enemyMotion.angle = angle;

			if (length(abs(wallEnemyDelta)) < distanceToWalls) {
				Enemy &enemyComp = registry.enemies.get(enemy);
				enemyComp.enemyState = EnemyState::AVOIDWALL;
				printf("State %d", enemyComp.enemyState);
				if (angleVector.x < angleVector.y) {
					enemyMotion.velocity = vec2(angleVector.x * enemySpeed, 0.0f);
				} else {
					enemyMotion.velocity = vec2(0.0f, angleVector.y * enemySpeed);
				}
			}
		}
	}
};