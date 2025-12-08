#pragma once

#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "../RenderUtilities/BufferObject.h"
#include "../RenderUtilities/Shader.h"

class Terrain {
private:
    int width;
    int depth;
    float scaleXZ = 2.0f;  // Scale the grid horizontally
    std::vector<float> heightMap;

    VAO* plane = nullptr;
    Shader* shader = nullptr;

public:
    int getWidth() const { return width; }
    int getDepth() const { return depth; }
    float getScaleXZ() const { return scaleXZ; }

private:
    void setHeight(int x, int z, float h) {
        if (x >= 0 && x < width && z >= 0 && z < depth) {
            heightMap[z * width + x] = h;
        }
    }

    void loadHeightMap(const char* fileName) {
        cv::Mat image = cv::imread(fileName, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            std::cout << "Failed to load height map: " << fileName << std::endl;
            // Fallback to basin if load fails
            heightMap.resize(width * depth, 0.0f);
            generateBasin();
            return;
        }

        // Apply Gaussian Blur to smooth the terrain
        // Kernel size (15, 15) for significant smoothing
        cv::GaussianBlur(image, image, cv::Size(15, 15), 0);

        width = image.cols;
        depth = image.rows;
        heightMap.resize(width * depth);

        // Normalize and populate heightMap
        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                unsigned char color = image.at<unsigned char>(z, x);
                float h = color / 255.0f * 150.0f - 50.0f;
                setHeight(x, z, h);
            }
        }
    }

    float getHeight(int x, int z) const {
        if (x >= 0 && x < width && z >= 0 && z < depth) {
            return heightMap[z * width + x];
        }
        return 0.0f;
    }

    glm::vec3 getNormal(int x, int z) {
        // Calculate normal using neighbors
        float hL = getHeight(x - 1, z);
        float hR = getHeight(x + 1, z);
        float hD = getHeight(x, z - 1);
        float hU = getHeight(x, z + 1);

        // Normal vector = (hL - hR, 2.0, hD - hU) normalized
        // Assuming grid spacing of 1.0 (before scaleXZ)
        glm::vec3 normal(hL - hR, 2.0f, hD - hU);
        return glm::normalize(normal);
    }

public:
    void init() {
        if (!this->shader) {
            this->shader =
                new Shader("./shaders/terrain.vert", nullptr, nullptr, nullptr,
                           "./shaders/terrain.frag");
        }

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLuint> indices;

        // Generate vertices
        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                // Position
                vertices.push_back(x * scaleXZ);
                vertices.push_back(getHeight(x, z));
                vertices.push_back(z * scaleXZ);

                // Normal
                glm::vec3 n = getNormal(x, z);
                normals.push_back(n.x);
                normals.push_back(n.y);
                normals.push_back(n.z);
            }
        }

        // Generate indices
        for (int z = 0; z < depth - 1; ++z) {
            for (int x = 0; x < width - 1; ++x) {
                int topLeft = z * width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * width + x;
                int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        plane = new VAO();
        plane->element_amount = indices.size();

        glGenVertexArrays(1, &plane->vao);
        glGenBuffers(2, plane->vbo);  // 0: pos, 1: normal
        glGenBuffers(1, &plane->ebo);

        glBindVertexArray(plane->vao);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, plane->vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (void*)0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, plane->vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                     normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (void*)0);
        glEnableVertexAttribArray(1);

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                     indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    Terrain(int width = 200, int depth = 200) : width(width), depth(depth) {
        loadHeightMap("./images/terrainHeightMap.jpg");
    }

    ~Terrain() {
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            glDeleteBuffers(2, plane->vbo);
            glDeleteBuffers(1, &plane->ebo);
            delete plane;
            plane = nullptr;
        }
        if (shader) {
            delete shader;
            shader = nullptr;
        }
    }

    void generateBasin() {
        float centerX = width / 2.0f;
        float centerZ = depth / 2.0f;
        // Scale factor to make the basin fit nicely
        float maxRadius = std::min(centerX, centerZ);

        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                float dx = x - centerX;
                float dz = z - centerZ;
                float dist = std::sqrt(dx * dx + dz * dz);

                // Normalized distance from center (0 at center, 1 at edge)
                float r = dist / maxRadius;

                // Basin function: h = A * r^2
                // Let's make the rim height 30.0f
                float h = 30.0f * (r * r);

                // Clamp or modify as needed
                if (h > 40.0f)
                    h = 40.0f;

                setHeight(x, z, h);
            }
        }
        // Re-init buffers if they exist (simple way)
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            glDeleteBuffers(2, plane->vbo);
            glDeleteBuffers(1, &plane->ebo);
            delete plane;
            plane = nullptr;
            init();
        }
    }

    void draw(const glm::mat4& view, const glm::mat4& projection,
              const glm::vec3& cameraPos) {
        if (!shader || !plane)
            return;

        shader->Use();

        glm::mat4 model = glm::mat4(1.0f);
        // Center the terrain at (0,0,0) or adjust as needed.
        // The grid goes from 0 to width, 0 to depth.
        // Let's center it.
        model = glm::translate(model, glm::vec3(-width / 2.0f * scaleXZ, -10.0f,
                                                -depth / 2.0f * scaleXZ));

        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "u_model"), 1,
                           GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "u_view"), 1,
                           GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(
            glGetUniformLocation(shader->Program, "u_projection"), 1, GL_FALSE,
            &projection[0][0]);
        glUniform3fv(glGetUniformLocation(shader->Program, "u_viewPos"), 1,
                     &cameraPos[0]);

        // Light properties (simple directional)
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
        glUniform3fv(glGetUniformLocation(shader->Program, "u_lightDir"), 1,
                     &lightDir[0]);

        glBindVertexArray(plane->vao);
        glDrawElements(GL_TRIANGLES, plane->element_amount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glUseProgram(0);
    }
};
