#include "render_system.hpp"
#include "components.hpp"
// Triangulation header only library https://github.com/mapbox/earcut.hpp
#include "earcut.hpp"

#include "common.hpp"
#include "tiny_ecs_registry.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>

void RenderSystem::initLight() {
    Entity e = Entity();
    Light& l = registry.lights.emplace(e);
    l.position = {window_width_px/2, window_height_px/2};

    gl_has_errors();
    glGenVertexArrays(1, &m_light_VAO);
    gl_has_errors();
}

int RenderSystem::getCurrentFrame(Entity& e) {
   Animation& a = registry.animations.get(e);
   return a.current_frame;
}

std::vector<vec2> RenderSystem::getEntityCurrentVertices(Entity& e) {
    int currentFrame = getCurrentFrame(e);
    std::vector<vec2> verts;
    if (registry.players.has(e)) {
        verts = playerOutlineVertices[currentFrame];
    }
    else if (registry.meleeAttacks.has(e)) {
        verts = meleeEnemyOutlineVertices[currentFrame];
    }
    else if (registry.reloadTimes.has(e)) {
        verts = rangedEnemyOutlineVertices[currentFrame];
    }
    else {
        verts = bossEnemyOutlineVertices[currentFrame];
    }
    std::vector<vec2> updatedVerts;

    Motion& m = registry.motions.get(e);
    Transform t;
    t.translate(m.position);
    if (fabsf(m.angle) < (M_PI/2)) {
        t.rotate(m.angle - M_PI);
        t.scale(vec2(-m.scale.x, m.scale.y));
    }
    else {
        t.rotate(m.angle);
        t.scale(m.scale);
    }

    /* t.scale(m.scale); */

    for (const vec2& v : verts) {
        vec3 updatedV = t.mat * vec3(v, 1);
        updatedVerts.push_back({updatedV.x, updatedV.y});


    }

    return updatedVerts;
}

std::vector<Ray> RenderSystem::generateWallRays() {
    Light& l = registry.lights.components[0];
    std::vector<Ray> rays;

    // Wall rays
    //
    vec2 innerTopLeftCorner = vec2(window_width_px - 1175, window_height_px - 650);
    vec2 innerTopRightCorner = vec2(window_width_px - 75, window_height_px - 650);
    vec2 innerBottomLeftCorner = vec2(window_width_px - 1175, window_height_px - 100);
    vec2 innerBottomRightCorner = vec2(window_width_px - 75, window_height_px - 100);

    Ray rayTopLeft;
    rayTopLeft.pos = l.position;
    rayTopLeft.direction = innerTopLeftCorner - l.position;
    rays.push_back(rayTopLeft);

    Ray rayTopRight;
    rayTopRight.pos = l.position;
    rayTopRight.direction = innerTopRightCorner - l.position;
    rays.push_back(rayTopRight);

    Ray rayBottomLeft;
    rayBottomLeft.pos = l.position;
    rayBottomLeft.direction = innerBottomLeftCorner - l.position;
    rays.push_back(rayBottomLeft);

    Ray rayBottomRight;
    rayBottomRight.pos = l.position;
    rayBottomRight.direction = innerBottomRightCorner - l.position;
    rays.push_back(rayBottomRight);

    return rays;
}

std::vector<Ray> RenderSystem::generateRays(std::vector<vec2>& vertices) {
    // for each ray, generate 2 extra rays to hit walls
    Light& l = registry.lights.components[0];
    std::vector<Ray> rays;

    Transform tLeft;
    Transform tRight;
    tLeft.rotate(-0.001);
    tRight.rotate(0.001);

    for (vec2& v : vertices) {
        Ray r1;
        r1.pos = l.position;
        r1.direction = v - r1.pos; 
        rays.push_back(r1);

        // 2 extra ray for each to hit walls
        Ray r2;
        r2.pos = l.position;
        vec3 r2Dir = tLeft.mat * vec3(r1.direction, 1);
        r2.direction = {r2Dir.x, r2Dir.y};
        rays.push_back(r2);

        Ray r3;
        r3.pos = l.position;
        vec3 r3Dir = tRight.mat * vec3(r1.direction, 1);
        r3.direction = {r3Dir.x, r3Dir.y};
        rays.push_back(r3);
        }

    return rays;
}

std::vector<LineSegment> RenderSystem::generateWallSegments() {
    std::vector<LineSegment> segments;
    // Wall line segments
    vec2 innerTopLeftCorner = vec2(window_width_px - 1175, window_height_px - 650);
    vec2 innerTopRightCorner = vec2(window_width_px - 75, window_height_px - 650);
    vec2 innerBottomLeftCorner = vec2(window_width_px - 1175, window_height_px - 100);
    vec2 innerBottomRightCorner = vec2(window_width_px - 75, window_height_px - 100);
    
    LineSegment top;
    top.pos = innerTopLeftCorner;
    top.direction = innerTopRightCorner - innerTopLeftCorner;
    top.t = 1;
    segments.push_back(top);

    LineSegment bot;
    bot.pos = innerBottomLeftCorner;
    bot.direction = innerBottomRightCorner - innerBottomLeftCorner;
    bot.t = 1;
    segments.push_back(bot);

    LineSegment left;
    left.pos = innerTopLeftCorner;
    left.direction = innerBottomLeftCorner - innerTopLeftCorner;
    left.t = 1;
    segments.push_back(left);

    LineSegment right;
    right.pos = innerTopRightCorner;
    right.direction = innerBottomRightCorner - innerTopRightCorner;
    right.t = 1;
    segments.push_back(right);

    return segments;
}

std::vector<LineSegment> RenderSystem::generateLineSegments(std::vector<vec2>& vertices) {
    std::vector<LineSegment> segments = generateWallSegments();
    for (int i = 0; i < (int) vertices.size(); i++) {
        vec2& currentVert = vertices[i];
        LineSegment l;
        if (i == (int) vertices.size() - 1) {
            l.pos = currentVert;
            l.direction = vertices[0] - currentVert;
            l.t = 1;

        }
        else {
            l.pos = currentVert;
            l.direction = vertices[i+1] - currentVert;
            l.t = 1;
        }

        segments.push_back(l);

    }
    return segments;
}

float RenderSystem::getRayIntersectionDist(Ray& ray, LineSegment& segment) {

    // Confirm ray and segment aren't parallel
    // -1 means does not intersect
    vec2 normalizedRayDir = normalize(ray.direction);
    vec2 normalizedSegmentDir = normalize(segment.direction);
    if (normalizedRayDir == normalizedSegmentDir || 
            normalizedRayDir == -normalizedSegmentDir) {
        return -9999.f;
    }

    float T2 = (ray.direction.x * (segment.pos.y - ray.pos.y) + ray.direction.y * (ray.pos.x - segment.pos.x)) / (segment.direction.x * ray.direction.y - segment.direction.y* ray.direction.x);

    float T1 = (segment.pos.x + segment.direction.x * T2 - ray.pos.x) / ray.direction.x;

    /* if ((T1 > 0) && ((0 < T2) && (T2 < 1))) { */
    if ((T1 > 0) && ((0 <= T2) && (T2 <= 1))) {
        return T1;
    }
    else {
        return -9999.f;
    }
}

std::vector<std::array<float, 2>> RenderSystem::getLitArea(std::vector<Ray>& rays, std::vector<LineSegment>& segments) {
    std::vector<std::pair<vec2, float>> visibilityPolygon;
    for (Ray& r: rays) {
        LineSegment closestSegment;
        float closestDist = 5000.f;
        for (LineSegment& l : segments) {
            float dist = getRayIntersectionDist(r, l);

            if (dist > 0.f && dist < closestDist) {
                closestSegment = l;
                closestDist = dist;
            }
        }
        vec2 intersectionPoint = r.pos + r.direction * vec2(closestDist,closestDist);
        float angle = atan2(r.direction.y, r.direction.x);
        std::pair<vec2, float> p = {intersectionPoint, angle};
        visibilityPolygon.push_back(p);
    }

    // Sort by angle to connect clockwise
    std::sort(visibilityPolygon.begin(), visibilityPolygon.end(),
            [] (std::pair<vec2, float>& a, std::pair<vec2, float>& b) {
                return a.second < b.second;
            });
    
    std::vector<std::array<float, 2>> normalizedVertexOnlyPolygon;
    for (std::pair<vec2, float>& p : visibilityPolygon) {
        std::array<float, 2> point = {p.first.x, p.first.y};
        normalizedVertexOnlyPolygon.push_back(point);
    }
    
    return normalizedVertexOnlyPolygon;
}

void RenderSystem::lightScreen() {
    std::vector<Ray> rays = generateWallRays();
    std::vector<LineSegment> segments = generateWallSegments();

    for (Entity& e : registry.players.entities) {
        std::vector<vec2> vertices = getEntityCurrentVertices(e); 
        std::vector<Ray> entityRays = generateRays(vertices);
        std::vector<LineSegment> entitySegments = generateLineSegments(vertices);
        rays.insert(rays.end(), entityRays.begin(), entityRays.end());
        segments.insert(segments.end(), entitySegments.begin(), entitySegments.end());
    }
    for (Entity& e : registry.meleeAttacks.entities) {
        if (registry.bosses.has(e)) {
            continue;
        }
        std::vector<vec2> vertices = getEntityCurrentVertices(e); 
        std::vector<Ray> entityRays = generateRays(vertices);
        std::vector<LineSegment> entitySegments = generateLineSegments(vertices);

        rays.insert(rays.end(), entityRays.begin(), entityRays.end());
        segments.insert(segments.end(), entitySegments.begin(), entitySegments.end());
    }

    for (Entity& e : registry.reloadTimes.entities) {
        if (registry.bosses.has(e)) {
            continue;
        }
        std::vector<vec2> vertices = getEntityCurrentVertices(e); 
        std::vector<Ray> entityRays = generateRays(vertices);
        std::vector<LineSegment> entitySegments = generateLineSegments(vertices);

        rays.insert(rays.end(), entityRays.begin(), entityRays.end());
        segments.insert(segments.end(), entitySegments.begin(), entitySegments.end());
    }

    for (Entity& e : registry.bosses.entities) {
        // if boss is actively teleporting, don't render
        EnemyState& enemyState = registry.enemies.get(e).enemyState;
        if (enemyState == EnemyState::TELEPORTING) {
            continue;
        }

        std::vector<vec2> vertices = getEntityCurrentVertices(e); 
        std::vector<Ray> entityRays = generateRays(vertices);
        std::vector<LineSegment> entitySegments = generateLineSegments(vertices);

        rays.insert(rays.end(), entityRays.begin(), entityRays.end());
        segments.insert(segments.end(), entitySegments.begin(), entitySegments.end());
    }
    std::vector<std::vector<std::array<float, 2>>> lightPolygonContainer;
    std::vector<std::array<float, 2>> lightPolygon = getLitArea(rays, segments);
    lightPolygonContainer.push_back(lightPolygon);
    
    std::vector<uint16_t> indices = mapbox::earcut<uint16_t>(lightPolygonContainer);
    // Convert to vec2 again
    std::vector<vec3> lightVectorPolygon;
    for (std::array<float,2>& p : lightPolygon) {
        lightVectorPolygon.push_back(vec3(p[0], p[1], 1.0f));
    }

    glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::LIGHT]);
    glBindVertexArray(m_light_VAO);
    bindVBOandIBO(GEOMETRY_BUFFER_ID::VISIBILITY_POLYGON, lightVectorPolygon, indices);

    glStencilMask(0xFF);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    gl_has_errors();

    // Draw everything in visibility polygon normally
	const GLuint light_program = effects[(GLuint)EFFECT_ASSET_ID::LIGHT];
	GLint in_position_loc = glGetAttribLocation(light_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

    mat3 projection = createProjectionMatrix();
	GLuint projection_loc = glGetUniformLocation(light_program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);

	GLuint shadow_on_loc = glGetUniformLocation(light_program, "shadow_on");
    glUniform1f(shadow_on_loc, 0.f);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, nullptr);

    // Now draw shadow
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);
    glUniform1f(shadow_on_loc, 1.0f);

    vec2 innerTopLeftCorner = vec2(window_width_px - 1175, window_height_px - 650);
    vec2 innerTopRightCorner = vec2(window_width_px - 75, window_height_px - 650);
    vec2 innerBottomLeftCorner = vec2(window_width_px - 1175, window_height_px - 100);
    vec2 innerBottomRightCorner = vec2(window_width_px - 75, window_height_px - 100);

	std::vector<vec3> shadow_vertices(4);
    shadow_vertices[0] = { innerTopLeftCorner, 1.0f }; // Top-left
    shadow_vertices[1] = { innerTopRightCorner, 1.0f }; // Top-right
    shadow_vertices[2] = {  innerBottomRightCorner, 1.0f }; // Bottom-right
    shadow_vertices[3] = { innerBottomLeftCorner, 1.0f }; // Bottom-left

	const std::vector<uint16_t> shadow_indices = { 0, 1, 2, 0, 2, 3};

    bindVBOandIBO(GEOMETRY_BUFFER_ID::SHADOW_PLANE, 
            shadow_vertices,
            shadow_indices);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	gl_has_errors();
    glDrawElements(GL_TRIANGLES, (GLsizei) shadow_indices.size(), GL_UNSIGNED_SHORT, nullptr);
    glDisable(GL_STENCIL_TEST);
    /* gl_has_errors(); */

}


