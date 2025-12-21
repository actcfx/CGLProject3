#pragma once

#include <glm/glm.hpp>
#include <vector>

// Generates and draws a geodesic sphere by subdividing an icosahedron.
class SubdivisionSphere {
public:
    SubdivisionSphere(int recursionLevel = 2, float radius = 10.0f);

    void setCenter(const glm::vec3& newCenter);
    void setRadius(float newRadius);
    void setColor(const glm::vec3& rgb);
    void setRecursionLevel(int level);

    void draw(bool doingShadows) const;

private:
    struct Triangle {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
    };

    void rebuildMesh();
    void buildIcosahedron(std::vector<Triangle>& tris) const;
    static glm::vec3 midpoint(const glm::vec3& lhs, const glm::vec3& rhs);
    static glm::vec3 projectToUnit(const glm::vec3& v);

    int recursionLevel = 0;
    float radius = 1.0f;
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.9f, 0.9f, 1.0f);
    std::vector<glm::vec3> unitDirections;
};
