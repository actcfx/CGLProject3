#include "SubdivisionSphere.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include <glad/glad.h>

SubdivisionSphere::SubdivisionSphere(int level, float initialRadius) {
    recursionLevel = -1;
    setRadius(initialRadius);
    setRecursionLevel(level);
}

void SubdivisionSphere::setCenter(const glm::vec3& newCenter) {
    center = newCenter;
}

void SubdivisionSphere::setRadius(float newRadius) {
    radius = (newRadius <= 0.0f) ? 1.0f : newRadius;
}

void SubdivisionSphere::setColor(const glm::vec3& rgb) {
    color = rgb;
}

void SubdivisionSphere::setRecursionLevel(int level) {
    if (level < 0)
        level = 0;
    if (level > 6)
        level = 6;
    if (level == recursionLevel)
        return;
    recursionLevel = level;
    rebuildMesh();
}

void SubdivisionSphere::draw(bool doingShadows) const {
    if (unitDirections.empty())
        return;

    if (!doingShadows) {
        glColor3f(color.r, color.g, color.b);
    }

    glBegin(GL_TRIANGLES);
    for (const glm::vec3& dir : unitDirections) {
        glm::vec3 normal = dir;
        float lenSq = glm::dot(normal, normal);
        if (lenSq > 1e-6f)
            normal /= std::sqrt(lenSq);
        else
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 position = center + normal * radius;
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(position.x, position.y, position.z);
    }
    glEnd();
}

void SubdivisionSphere::rebuildMesh() {
    std::vector<Triangle> tris;
    buildIcosahedron(tris);

    for (int i = 0; i < recursionLevel; ++i) {
        std::vector<Triangle> next;
        next.reserve(tris.size() * 4);
        for (const Triangle& tri : tris) {
            glm::vec3 ab = projectToUnit(midpoint(tri.a, tri.b));
            glm::vec3 bc = projectToUnit(midpoint(tri.b, tri.c));
            glm::vec3 ca = projectToUnit(midpoint(tri.c, tri.a));

            next.push_back({tri.a, ab, ca});
            next.push_back({tri.b, bc, ab});
            next.push_back({tri.c, ca, bc});
            next.push_back({ab, bc, ca});
        }
        tris.swap(next);
    }

    unitDirections.clear();
    unitDirections.reserve(tris.size() * 3);
    for (const Triangle& tri : tris) {
        unitDirections.push_back(projectToUnit(tri.a));
        unitDirections.push_back(projectToUnit(tri.b));
        unitDirections.push_back(projectToUnit(tri.c));
    }
}

void SubdivisionSphere::buildIcosahedron(std::vector<Triangle>& tris) const {
    static const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;

    std::array<glm::vec3, 12> vertices = {
        glm::vec3(-1.0f,  phi,  0.0f), glm::vec3( 1.0f,  phi,  0.0f),
        glm::vec3(-1.0f, -phi,  0.0f), glm::vec3( 1.0f, -phi,  0.0f),
        glm::vec3( 0.0f, -1.0f,  phi), glm::vec3( 0.0f,  1.0f,  phi),
        glm::vec3( 0.0f, -1.0f, -phi), glm::vec3( 0.0f,  1.0f, -phi),
        glm::vec3( phi,  0.0f, -1.0f), glm::vec3( phi,  0.0f,  1.0f),
        glm::vec3(-phi,  0.0f, -1.0f), glm::vec3(-phi,  0.0f,  1.0f)
    };

    for (glm::vec3& v : vertices) {
        v = projectToUnit(v);
    }

    static const std::array<std::array<int, 3>, 20> faces = {
        std::array<int, 3>{0, 11, 5},  {0, 5, 1},   {0, 1, 7},   {0, 7, 10},
        {0, 10, 11},                   {1, 5, 9},   {5, 11, 4},  {11, 10, 2},
        {10, 7, 6},                    {7, 1, 8},   {3, 9, 4},   {3, 4, 2},
        {3, 2, 6},                     {3, 6, 8},   {3, 8, 9},   {4, 9, 5},
        {2, 4, 11},                    {6, 2, 10},  {8, 6, 7},   {9, 8, 1}
    };

    tris.clear();
    tris.reserve(faces.size());
    for (const auto& face : faces) {
        tris.push_back({vertices[(size_t)face[0]], vertices[(size_t)face[1]],
                        vertices[(size_t)face[2]]});
    }
}

glm::vec3 SubdivisionSphere::midpoint(const glm::vec3& lhs,
                                       const glm::vec3& rhs) {
    return (lhs + rhs) * 0.5f;
}

glm::vec3 SubdivisionSphere::projectToUnit(const glm::vec3& v) {
    float lenSq = glm::dot(v, v);
    if (lenSq <= 1e-8f)
        return glm::vec3(0.0f, 1.0f, 0.0f);
    return v / std::sqrt(lenSq);
}
