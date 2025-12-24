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
    float scaleXZ = 2.0f;  // Scale the grid horizontally
    int blockSize = 10;    // size of blocks in pixels (Minecraft style)
    std::vector<float> heightMap;

    VAO* plane = nullptr;
    Shader* shader = nullptr;

    TrainWindow* tw = nullptr;

public:
    int getWidth() const { return width; }

    int getDepth() const { return depth; }

    float getScaleXZ() const { return scaleXZ; }

    glm::mat4 getModelMatrix() const {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(-width / 2.0f * scaleXZ, -10.0f,
                                                -depth / 2.0f * scaleXZ));
        return model;
    }

    // Public method to get terrain height at world coordinates (x, z)
    float getHeightAtWorldPos(float worldX, float worldZ) const {
        // Convert world coordinates to grid coordinates
        float centerOffsetX = (width / 2.0f) * scaleXZ;
        float centerOffsetZ = (depth / 2.0f) * scaleXZ;

        float gridX = (worldX + centerOffsetX) / scaleXZ;
        float gridZ = (worldZ + centerOffsetZ) / scaleXZ;

        // Bilinear interpolation
        int x0 = static_cast<int>(std::floor(gridX));
        int z0 = static_cast<int>(std::floor(gridZ));
        int x1 = x0 + 1;
        int z1 = z0 + 1;

        // Bounds check
        if (x0 < 0 || x1 >= width || z0 < 0 || z1 >= depth)
            return 0.0f;

        float fx = gridX - x0;
        float fz = gridZ - z0;

        float h00 = getHeight(x0, z0);
        float h10 = getHeight(x1, z0);
        float h01 = getHeight(x0, z1);
        float h11 = getHeight(x1, z1);

        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;
        float finalHeight = h0 * (1.0f - fz) + h1 * fz;

        return finalHeight - 10.0f;
    }

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
            heightMap.resize(width * depth, 0.0f);
            generateBasin();
            return;
        }

        width = image.cols;
        depth = image.rows;
        heightMap.resize(width * depth);

        int step = blockSize;
        float quantStep = step * 2;

        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                if (tw && tw->minecraftButton->value()) {
                    int sampleX = (x / step) * step;
                    int sampleZ = (z / step) * step;

                    if (sampleX >= width)
                        sampleX = width - 1;
                    if (sampleZ >= depth)
                        sampleZ = depth - 1;

                    unsigned char color =
                        image.at<unsigned char>(sampleZ, sampleX);
                    float rawH = color - 100.0f;
                    float h = std::floor(rawH / quantStep) * quantStep - 6.0f;
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
        float hL = getHeight(x - 1, z);
        float hR = getHeight(x + 1, z);
        float hD = getHeight(x, z - 1);
        float hU = getHeight(x, z + 1);
        glm::vec3 normal(hL - hR, 2.0f, hD - hU);
        return glm::normalize(normal);
    }

    // [New] Generate color based on height (Ported from Shader)
    glm::vec3 getColor(float height) {
        // Linear interpolation helper
        auto mix = [](glm::vec3 a, glm::vec3 b, float t) {
            return a * (1.0f - t) + b * t;
        };
        auto clamp = [](float v, float min, float max) {
            return std::max(min, std::min(v, max));
        };

        if (height < -40.0f) {
            float t = clamp((height + 100.0f) / 60.0f, 0.0f, 1.0f);
            return mix(glm::vec3(0.4f, 0.4f, 0.5f),
                       glm::vec3(0.76f, 0.7f, 0.5f), t);
        } else if (height < -6.0f) {
            float t = clamp((height + 40.0f) / 34.0f, 0.0f, 1.0f);
            return mix(glm::vec3(0.76f, 0.7f, 0.5f),
                       glm::vec3(0.5f, 0.4f, 0.3f), t);
        } else if (height < 60.0f) {
            float t = clamp((height + 6.0f) / 66.0f, 0.0f, 1.0f);
            return mix(glm::vec3(0.5f, 0.4f, 0.3f), glm::vec3(0.2f, 0.6f, 0.2f),
                       t);
        } else if (height < 120.0f) {
            float t = clamp((height - 60.0f) / 60.0f, 0.0f, 1.0f);
            return mix(glm::vec3(0.2f, 0.6f, 0.2f), glm::vec3(0.5f, 0.5f, 0.5f),
                       t);
        } else {
            float t = clamp((height - 120.0f) / 40.0f, 0.0f, 1.0f);
            return mix(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.9f, 0.9f, 0.9f),
                       t);
        }
    }

public:
    Terrain(int width = 200, int depth = 200) : width(width), depth(depth) {}

    ~Terrain() {
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            // Assuming buffer object handles deletion of array, but standard is:
            glDeleteBuffers(3, plane->vbo);  // Now using 3 buffers
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
        // No shader init needed
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
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            glDeleteBuffers(3, plane->vbo);
            glDeleteBuffers(1, &plane->ebo);
            delete plane;
            plane = nullptr;
        }

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLfloat> colors;  // [New] Vertex Colors
        std::vector<GLuint> indices;

        if (tw && tw->minecraftButton->value()) {
            GLuint curIndex = 0;
            const float cell = blockSize * scaleXZ;

            auto pushQuad = [&](const glm::vec3& a, const glm::vec3& b,
                                const glm::vec3& c, const glm::vec3& d,
                                const glm::vec3& n, const glm::vec3& col) {
                vertices.insert(vertices.end(),
                                { a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z,
                                  d.x, d.y, d.z });
                normals.insert(normals.end(), { n.x, n.y, n.z, n.x, n.y, n.z,
                                                n.x, n.y, n.z, n.x, n.y, n.z });
                colors.insert(colors.end(),
                              { col.r, col.g, col.b, col.r, col.g, col.b, col.r,
                                col.g, col.b, col.r, col.g, col.b });
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

                    glm::vec3 col = getColor(h);  // Get color based on height

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

                    pushQuad(p010, p110, p111, p011, glm::vec3(0, 1, 0),
                             col);  // Top
                    pushQuad(p000, p001, p101, p100, glm::vec3(0, -1, 0),
                             col);  // Bottom
                    pushQuad(p100, p101, p111, p110, glm::vec3(1, 0, 0),
                             col);  // +X
                    pushQuad(p000, p010, p011, p001, glm::vec3(-1, 0, 0),
                             col);  // -X
                    pushQuad(p001, p011, p111, p101, glm::vec3(0, 0, 1),
                             col);  // +Z
                    pushQuad(p000, p100, p110, p010, glm::vec3(0, 0, -1),
                             col);  // -Z
                }
            }
        } else {
            for (int z = 0; z < depth; ++z) {
                for (int x = 0; x < width; ++x) {
                    float h = getHeight(x, z);
                    vertices.push_back(x * scaleXZ);
                    vertices.push_back(h);
                    vertices.push_back(z * scaleXZ);

                    glm::vec3 n = getNormal(x, z);
                    normals.push_back(n.x);
                    normals.push_back(n.y);
                    normals.push_back(n.z);

                    glm::vec3 c = getColor(h);
                    colors.push_back(c.r);
                    colors.push_back(c.g);
                    colors.push_back(c.b);
                }
            }

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
        glGenBuffers(3, plane->vbo);  // 0:Pos, 1:Normal, 2:Color
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

        // Color
        glBindBuffer(GL_ARRAY_BUFFER, plane->vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                     colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (void*)0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                     indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void generateBasin() {
        float centerX = width / 2.0f;
        float centerZ = depth / 2.0f;
        float maxRadius = std::min(centerX, centerZ);

        for (int z = 0; z < depth; ++z) {
            for (int x = 0; x < width; ++x) {
                float dx = x - centerX;
                float dz = z - centerZ;
                float dist = std::sqrt(dx * dx + dz * dz);
                float r = dist / maxRadius;
                float h = 30.0f * (r * r);
                if (h > 40.0f)
                    h = 40.0f;
                setHeight(x, z, h);
            }
        }
        if (plane) {
            glDeleteVertexArrays(1, &plane->vao);
            glDeleteBuffers(3, plane->vbo);
            glDeleteBuffers(1, &plane->ebo);
            delete plane;
            plane = nullptr;
            init(tw);
        }
    }

    void draw(const glm::mat4& view, const glm::mat4& proj,
              const glm::mat4& lightSpace, GLuint shadowMap,
              const glm::vec3& lightDir, const glm::vec3& viewPos,
              bool enableShadow, bool enableLight,
              // Point light inputs
              const glm::vec3& pointLightPos, GLuint pointShadowMap,
              float pointFarPlane, bool enablePointShadow,
              bool enablePointLight,
              // Spot light inputs
              const glm::vec3& spotLightPos, const glm::vec3& spotLightDir,
              const glm::mat4& spotLightMatrix, GLuint spotShadowMap,
              float spotFarPlane, float spotInnerCos, float spotOuterCos,
              bool enableSpotShadow, bool enableSpotLight,
              // Clip plane
              const glm::vec4& clipPlane, bool enableClip) {
        if (!plane)
            return;

        if (!shader) {
            shader = new Shader("./shaders/terrain.vert", nullptr, nullptr,
                                nullptr, "./shaders/terrain.frag");
        }

        shader->Use();

        glm::mat4 model = getModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "u_model"), 1,
                           GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "u_view"), 1,
                           GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "u_proj"), 1,
                           GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(
            glGetUniformLocation(shader->Program, "u_lightSpace"), 1, GL_FALSE,
            glm::value_ptr(lightSpace));
        glUniform4fv(glGetUniformLocation(shader->Program, "u_clipPlane"), 1,
                     glm::value_ptr(clipPlane));
        glUniform1i(glGetUniformLocation(shader->Program, "u_enableClip"),
                    enableClip ? 1 : 0);

        glUniform3fv(glGetUniformLocation(shader->Program, "u_lightDir"), 1,
                     glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(shader->Program, "u_viewPos"), 1,
                     glm::value_ptr(viewPos));
        glUniform1i(glGetUniformLocation(shader->Program, "u_enableShadow"),
                    enableShadow ? 1 : 0);
        glUniform1i(glGetUniformLocation(shader->Program, "u_enableLight"),
                    enableLight ? 1 : 0);

        glUniform3fv(glGetUniformLocation(shader->Program, "u_pointLightPos"),
                     1, glm::value_ptr(pointLightPos));
        glUniform1i(glGetUniformLocation(shader->Program, "u_enablePointLight"),
                    enablePointLight ? 1 : 0);
        glUniform1i(
            glGetUniformLocation(shader->Program, "u_enablePointShadow"),
            enablePointShadow ? 1 : 0);
        glUniform1f(glGetUniformLocation(shader->Program, "u_pointFarPlane"),
                    pointFarPlane);

        glUniform3fv(glGetUniformLocation(shader->Program, "u_spotLightPos"), 1,
                     glm::value_ptr(spotLightPos));
        glUniform3fv(glGetUniformLocation(shader->Program, "u_spotLightDir"), 1,
                     glm::value_ptr(spotLightDir));
        glUniformMatrix4fv(
            glGetUniformLocation(shader->Program, "u_spotLightMatrix"), 1,
            GL_FALSE, glm::value_ptr(spotLightMatrix));
        glUniform1f(glGetUniformLocation(shader->Program, "u_spotFarPlane"),
                    spotFarPlane);
        glUniform1f(glGetUniformLocation(shader->Program, "u_spotInnerCos"),
                    spotInnerCos);
        glUniform1f(glGetUniformLocation(shader->Program, "u_spotOuterCos"),
                    spotOuterCos);
        glUniform1i(glGetUniformLocation(shader->Program, "u_enableSpotShadow"),
                    enableSpotShadow ? 1 : 0);
        glUniform1i(glGetUniformLocation(shader->Program, "u_enableSpotLight"),
                    enableSpotLight ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glUniform1i(glGetUniformLocation(shader->Program, "u_shadowMap"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowMap);
        glUniform1i(glGetUniformLocation(shader->Program, "u_pointShadowMap"),
                    1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, spotShadowMap);
        glUniform1i(glGetUniformLocation(shader->Program, "u_spotShadowMap"),
                    2);

        glBindVertexArray(plane->vao);
        glDrawElements(GL_TRIANGLES, plane->element_amount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glUseProgram(0);
    }

    void drawPointShadow(Shader* depthShader, const glm::mat4& lightMatrix,
                         const glm::vec3& lightPos, float farPlane) {
        if (!plane || !depthShader)
            return;

        depthShader->Use();
        glm::mat4 model = getModelMatrix();
        glUniformMatrix4fv(
            glGetUniformLocation(depthShader->Program, "u_model"), 1, GL_FALSE,
            glm::value_ptr(model));
        glUniformMatrix4fv(
            glGetUniformLocation(depthShader->Program, "u_lightMatrix"), 1,
            GL_FALSE, glm::value_ptr(lightMatrix));
        glUniform3fv(glGetUniformLocation(depthShader->Program, "u_lightPos"),
                     1, glm::value_ptr(lightPos));
        glUniform1f(glGetUniformLocation(depthShader->Program, "u_farPlane"),
                    farPlane);

        glBindVertexArray(plane->vao);
        glDrawElements(GL_TRIANGLES, plane->element_amount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void drawDepth() {
        if (!plane)
            return;

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(-width / 2.0f * scaleXZ, -10.0f, -depth / 2.0f * scaleXZ);

        glBindBuffer(GL_ARRAY_BUFFER, plane->vbo[0]);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane->ebo);
        glDrawElements(GL_TRIANGLES, plane->element_amount, GL_UNSIGNED_INT, 0);

        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glPopMatrix();
    }
};