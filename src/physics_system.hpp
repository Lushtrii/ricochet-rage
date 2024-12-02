#pragma once

#include "common.hpp"
#include "components.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
	static bool collides(const Motion& motion1, const Motion& motion2);
	void step(float elapsed_ms);

    vec2 calculateVertexPos(const Motion& motion, const TexturedVertex& tv);

    bool doesMeshCollide(const Motion& meshMotion, const std::vector<TexturedVertex>& meshVertices, const Motion& otherMotion);

    bool isPointInBox(const vec2 point, const Motion& motion);

	PhysicsSystem()
	{
	}

};
