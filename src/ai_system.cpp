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
				motion.velocity = vec2((uniform_dist(rng) - 0.5f)* 15.0f, (uniform_dist(rng) - 0.5f) * 15.0f);
				if (length(playerMotion.position - motion.position) < aggroDistance) {
					enemyState = EnemyState::PURSUING;
				}
			}
		// State for pursuing and shooting at player
		} else if (enemyState == EnemyState::PURSUING) {
			if (registry.reloadTimes.has(enemy))
			{
                ranged_enemy_pursue(enemy, elapsed_ms, playerMotion, enemyState);
            } 
			else if (registry.meleeAttacks.has(enemy))
			{
				Motion& enemyMotion = registry.motions.get(enemy);
				if (length(playerMotion.position - enemyMotion.position) > meleeDistance) {
					context_chase(enemy, playerMotion);
				} else {
					enemyState = EnemyState::ATTACK;
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
					if (length(abs(wallEnemyDelta)) > distanceToWalls) {
						printf("Distance to wall: %f %f\n", length(abs(wallEnemyDelta)), distanceToWalls);
						enemyState = EnemyState::PURSUING;
					}
				}
			}
		// State for attacking player
		} else if (enemyState == EnemyState::ATTACK) {
			if (registry.reloadTimes.has(enemy)) {
				ReloadTime &counter = registry.reloadTimes.get(enemy);
				stop_and_shoot(enemy, counter, elapsed_ms, playerMotion);
			} else if (registry.meleeAttacks.has(enemy)) {
				MeleeAttack &meleeAttack = registry.meleeAttacks.get(enemy);
				stop_and_melee(enemy, meleeAttack, elapsed_ms, playerMotion, playerEntity);
				enemyState = EnemyState::PURSUING;
			}
		}
	}
}

// Pursuing logic for a ranged enemy, including shoot
void AISystem::ranged_enemy_pursue(Entity &enemy, float elapsed_ms, Motion &playerMotion, EnemyState &enemyState)
{
    ReloadTime &counter = registry.reloadTimes.get(enemy);
	// to prevent overflow
	if (counter.counter_ms > 0) {
		counter.counter_ms -= elapsed_ms;
	}
    context_chase(enemy, playerMotion);

    if (!line_of_sight_check(enemy, playerMotion) && counter.counter_ms < 0)
    {
		enemyState = EnemyState::ATTACK;
    }
}


// Perform a light of sight check to see if there are any obstacles between the ranged enemy and the player
bool AISystem::line_of_sight_check(Entity &enemy, Motion &playerMotion) {
	Motion& enemyMotion = registry.motions.get(enemy);
	vec2 deltaEnemyPlayer = enemyMotion.position - playerMotion.position;
	for (Entity &obstacle: registry.walls.entities) {
		Motion &wallMotion = registry.motions.get(obstacle);
		if (line_box_collision(enemyMotion, wallMotion, deltaEnemyPlayer)) {
			return true;
		}
	}
	return false;
}

// Whether the line and AABB intersect, not really the smartest method, iterate through points on the line and return true if it intersects
bool AISystem::line_box_collision(Motion &enemyMotion, Motion &obstacleMotion, vec2 &directionDelta) {
	vec2 normalizedDelta = normalize(directionDelta);
	vec2 increment = los_increment * normalizedDelta;
	vec2 curr = vec2(0, 0);
	vec2 bounding_box = vec2(abs(obstacleMotion.scale.x), abs(obstacleMotion.scale.y));
	float left = obstacleMotion.position.x - bounding_box.x/2;
	float right = obstacleMotion.position.x + bounding_box.x/2;
	float bot = obstacleMotion.position.y + bounding_box.y/2;
	float top = obstacleMotion.position.y - bounding_box.y/2;
	while (length(curr) < length(directionDelta)) {
		curr += increment;
		vec2 curr_pos = enemyMotion.position + curr;
		if (curr_pos.x > left && curr_pos.x < right && curr_pos.y > top && curr_pos.y < bot) {
			return true;
		}
	}
	return false;
}


// For smart context chasing, good for avoiding obstacles, although still robotic
void AISystem::context_chase(Entity &enemy,  Motion &playerMotion) {
	std::vector<vec2> directions = {vec2(0, 1), vec2(0, -1), vec2(1, 0), vec2(-1, 0), normalize(vec2(-1, 1)), normalize(vec2(1,1)), normalize(vec2(-1,-1)), normalize(vec2(1,-1))};

	Motion &enemyMotion = registry.motions.get(enemy);
	vec2 enemyPlayerDelta = enemyMotion.position - playerMotion.position;
	// Stop enemy from colliding with player
	if (length(enemyPlayerDelta) < minDistanceToPlayer) {
		enemyMotion.velocity = vec2(0.0f, 0.0f);
		return;
	}
	vec2 angleVector = -normalize(enemyPlayerDelta);
	std::vector<float> interestVector;
	for (auto &direction: directions) {
		interestVector.push_back(direction.x * angleVector.x + direction.y * angleVector.y);
	}
	std::vector<float> dangerVector(interestVector.size());

	float minDistance = std::numeric_limits<float>::max();
	for (Entity &wall: registry.walls.entities) {
		Motion& wallMotion = registry.motions.get(wall);
		vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
		float distanceToWall = length(wallEnemyDelta);
		
		// move away from the wall if it is the closest
		if (distanceToWall < minDistance) {
			minDistance = distanceToWall;
			vec2 danger_delta = -normalize(wallEnemyDelta) * obstacleForce / (distanceToWall);
			for (int i = 0; i < dangerVector.size(); i++) {
				dangerVector[i] = danger_delta.x * directions[i].x + directions[i].y * danger_delta.y;
			}
		}
	}

	// Avoid collisions with other enemies, when too close
	for (Entity &other: registry.enemies.entities) {
		if (other != enemy) {
			Motion &otherMotion = registry.motions.get(other);
			vec2 enemyEnemyDelta = enemyMotion.position - otherMotion.position;
			float distanceToEnemy = length(enemyEnemyDelta);
			if (distanceToEnemy < distanceBetweenEnemies) {
				vec2 danger_delta = -normalize(enemyEnemyDelta) * enemyForce / distanceToEnemy;
				for (int i = 0; i < dangerVector.size(); i++) {
					dangerVector[i] += danger_delta.x * directions[i].x + directions[i].y * danger_delta.y;
				}
			}
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
	enemyMotion.angle = atan2(sumVelocity.y, sumVelocity.x);
}

// Stop, winds up, and performs a melee attack on the player
void AISystem::stop_and_melee(Entity &enemy, MeleeAttack &counter, float elapsed_ms, Motion &playerMotion, Entity &playerEntity) {
	if (registry.motions.has(enemy)) 
	{
		counter.windup -= elapsed_ms;
		Motion& enemyMotion = registry.motions.get(enemy);
		enemyMotion.velocity = vec2(0.0f, 0.0f);
		if (counter.windup < 0) {
			if (registry.healths.has(playerEntity)) {
				Health &playerHealth = registry.healths.get(playerEntity);
				playerHealth.value -= counter.damage;
				printf("10 damage");
				if (registry.damageEffect.has(playerEntity)) {
					DamageEffect &effect = registry.damageEffect.get(playerEntity);
					effect.is_attacked = true;
				}
			}
			counter.windup = counter.windupMax;

		}
	}
}

// Stops and shoots at the enemy at a certain rate
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
            float angle = atan2(angleVector.y, angleVector.x);
            createProjectile(renderer_arg, enemyMotion.position, angle, false);
			counter.shoot_rate = shoot_rate;
		}

        if (counter.take_aim_ms < 0)
        {
			Enemy &enemyState = registry.enemies.get(enemy);
            counter.counter_ms = original_ms;
            counter.take_aim_ms = take_aim_ms;
			enemyState.enemyState = EnemyState::PURSUING;
        }
    }
};

// DEPRECATED: Extremely simple chase that goes to the players direction, does the Roomba thing when approaching a wall
void AISystem::simple_chase(float elapsed_ms, Motion &playersMotion)
{
	elapsed_ms += 0.f; // to hide errors
	for (uint i = 0; i < registry.enemies.size(); i++)
	{
		Entity &curr_entity = registry.enemies.entities[i];
		simple_chase_enemy(curr_entity, playersMotion);
	}
}

// DEPRECATED: Extremely simple chase that goes to the players direction
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