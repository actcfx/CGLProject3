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
#include "../TrainWindow.H"

class TrainWindow;  // Forward declaration

class Terrain {
private:
    int width;
    int depth;
    float scaleXZ = 2.0f;      // Scale the grid horizontally
    int blockSize = 10;        // size of blocks in pixels (Minecraft style)
    std::vector<float> heightMap;

    VAO* plane = nullptr;
    Shader* shader = nullptr;

    TrainWindow* tw = nullptr;

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

        // Remove Gaussian Blur to keep edges sharp for Minecraft look
        // cv::GaussianBlur(image, image, cv::Size(15, 15), 0);

        width = image.cols;
        depth = image.rows;
        heightMap.resize(width * depth);

        int step = blockSize;  // Step size for X and Z (block width/depth)
        float quantStep = step * 2;

        // Normalize and populate heightMap
        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                if (tw && tw->minecraftButton->value()) {
                    // Quantize coordinates to create blocks
                    int sampleX = (x / step) * step;
                    int sampleZ = (z / step) * step;

                    // Boundary check
                    if (sampleX >= width)
                        sampleX = width - 1;
                    if (sampleZ >= depth)
                        sampleZ = depth - 1;

                    unsigned char color =
                        image.at<unsigned char>(sampleZ, sampleX);

                    float rawH = color - 100.0f;
                    float h = std::floor(rawH / quantStep) * quantStep + 14.0f;

                    setHeight(x, z, h);
                } else {
                    unsigned char color = image.at<unsigned char>(z, x);
                    float h = color - 100.0f;
                    setHeight(x, z, h);
                }
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
    Terrain(int width = 200, int depth = 200) : width(width), depth(depth) {}

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

    void init(TrainWindow* tw) {
        if (!this->tw) {
            this->tw = tw;
        }

        if (!this->shader) {
            this->shader =
                new Shader("./shaders/terrain.vert", nullptr, nullptr, nullptr,
                           "./shaders/terrain.frag");
        }

        loadHeightMap("./images/terrainHeightMap.jpg");
        buildMesh();
    }

    void rebuild() {
        if (tw) {
            loadHeightMap("./images/terrainHeightMap.jpg");
            buildMesh();
        }
    }

    void buildMesh() {
        // Clean up existing buffers
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            glDeleteBuffers(2, plane->vbo);
            glDeleteBuffers(1, &plane->ebo);
            delete plane;
            plane = nullptr;
        }

        // Generate coarse block-based geometry to produce a Minecraft-like
        // stepped look. We duplicate vertices per-triangle so that each
        // triangle has a constant (flat) normal.
        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLuint> indices;
        if (tw && tw->minecraftButton->value()) {
            // Voxel-style blocks: build cubes per block cell with vertical sides.
            GLuint curIndex = 0;
            const float cell = blockSize * scaleXZ;

            auto pushQuad = [&](const glm::vec3& a, const glm::vec3& b,
                                const glm::vec3& c, const glm::vec3& d,
                                const glm::vec3& n) {
                // two triangles: a,b,c and a,c,d
                vertices.insert(vertices.end(),
                                { a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z,
                                  d.x, d.y, d.z });
                normals.insert(normals.end(), { n.x, n.y, n.z, n.x, n.y, n.z,
                                                n.x, n.y, n.z, n.x, n.y, n.z });
                indices.insert(indices.end(),
                               { curIndex, curIndex + 1, curIndex + 2, curIndex,
                                 curIndex + 2, curIndex + 3 });
                curIndex += 4;
            };

            for (int z = 0; z < depth; z += blockSize) {
                for (int x = 0; x < width; x += blockSize) {
                    float h = getHeight(x, z);
                    float y0 = std::min(h, -100.0f);
                    float y1 = h;

                    float x0 = x * scaleXZ;
                    float z0 = z * scaleXZ;
                    float x1 = x0 + cell;
                    float z1 = z0 + cell;

                    glm::vec3 p000(x0, y0, z0);
                    glm::vec3 p100(x1, y0, z0);
                    glm::vec3 p110(x1, y1, z0);
                    glm::vec3 p010(x0, y1, z0);
                    glm::vec3 p001(x0, y0, z1);
                    glm::vec3 p101(x1, y0, z1);
                    glm::vec3 p111(x1, y1, z1);
                    glm::vec3 p011(x0, y1, z1);

                    // Top (+Y)
                    pushQuad(p010, p110, p111, p011, glm::vec3(0, 1, 0));
                    // Bottom (-Y)
                    pushQuad(p000, p001, p101, p100, glm::vec3(0, -1, 0));
                    // +X
                    pushQuad(p100, p101, p111, p110, glm::vec3(1, 0, 0));
                    // -X
                    pushQuad(p000, p010, p011, p001, glm::vec3(-1, 0, 0));
                    // +Z
                    pushQuad(p001, p011, p111, p101, glm::vec3(0, 0, 1));
                    // -Z
                    pushQuad(p000, p100, p110, p010, glm::vec3(0, 0, -1));
                }
            }
        } else {
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
            init(tw);
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
