#ifndef TOTEM_OF_UNDYING_HPP
#define TOTEM_OF_UNDYING_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../RenderUtilities/BufferObject.h"
#include "../RenderUtilities/Shader.h"
#include "../RenderUtilities/Texture.h"

class TotemOfUndying {
private:
    void cleanup() {
        if (this->shader) {
            delete this->shader;
            this->shader = nullptr;
        }

        if (this->commonMatrices) {
            glDeleteBuffers(1, &this->commonMatrices->ubo);
            delete this->commonMatrices;
            this->commonMatrices = nullptr;
        }

        if (this->quad) {
            glDeleteVertexArrays(1, &this->quad->vao);
            glDeleteBuffers(3, this->quad->vbo);
            glDeleteBuffers(1, &this->quad->ebo);
            delete this->quad;
            this->quad = nullptr;
        }

        if (this->texture) {
            delete this->texture;
            this->texture = nullptr;
        }
    }

public:
    VAO* quad = nullptr;
    Texture2D* texture = nullptr;
    Shader* shader = nullptr;
    UBO* commonMatrices = nullptr;

    TotemOfUndying() {}

    ~TotemOfUndying() { cleanup(); }

    void init() {
        cleanup();

        // Initialize shader (use custom totem shader for better alpha handling)
        if (!this->shader) {
            this->shader = new Shader("./shaders/totem.vert", nullptr, nullptr,
                                      nullptr, "./shaders/totem.frag");
        }

        // Initialize UBO for common matrices
        if (!this->commonMatrices) {
            this->commonMatrices = new UBO();
            this->commonMatrices->size = 2 * sizeof(glm::mat4);
            glGenBuffers(1, &this->commonMatrices->ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, this->commonMatrices->ubo);
            glBufferData(GL_UNIFORM_BUFFER, this->commonMatrices->size, NULL,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        // Create billboard quad
        if (!this->quad) {
            // Billboard vertices (centered, facing camera along +Z axis)
            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f,  // Bottom-left
                0.5f,  -0.5f, 0.0f,  // Bottom-right
                0.5f,  0.5f,  0.0f,  // Top-right
                -0.5f, 0.5f,  0.0f   // Top-left
            };

            // Normals (all facing forward)
            GLfloat normal[] = {
                0.0f, 0.0f, 1.0f,  // Bottom-left
                0.0f, 0.0f, 1.0f,  // Bottom-right
                0.0f, 0.0f, 1.0f,  // Top-right
                0.0f, 0.0f, 1.0f   // Top-left
            };

            // Texture coordinates
            GLfloat textureCoordinate[] = {
                0.0f, 1.0f,  // Bottom-left
                1.0f, 1.0f,  // Bottom-right
                1.0f, 0.0f,  // Top-right
                0.0f, 0.0f   // Top-left
            };

            // Element indices
            GLuint element[] = { 0, 1, 2, 0, 2, 3 };

            this->quad = new VAO;
            this->quad->element_amount = sizeof(element) / sizeof(GLuint);
            glGenVertexArrays(1, &this->quad->vao);
            glGenBuffers(3, this->quad->vbo);
            glGenBuffers(1, &this->quad->ebo);
            glBindVertexArray(this->quad->vao);

            // Position attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->quad->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            // Normal attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->quad->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            // Texture Coordinate attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->quad->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinate),
                         textureCoordinate, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);

            // Element attribute
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->quad->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element,
                         GL_STATIC_DRAW);

            // Unbind VAO
            glBindVertexArray(0);
        }

        // Load texture
        if (!this->texture) {
            this->texture = new Texture2D("./images/totemOfUndying.png");
        }
    }

    void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
              const glm::vec3& cameraPos, float smokeStart,
              float smokeEnd) {
        if (!this->shader || !this->quad || !this->texture) {
            return;
        }

        // Save current GL state
        GLint prevProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

        GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
        GLint blendSrcRgb, blendDstRgb;
        glGetIntegerv(GL_BLEND_SRC, &blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST, &blendDstRgb);

        GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);

        GLint prevActiveTexture;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
        GLint prevTexture2D;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture2D);

        // Billboard position (center, y + 20) - keep fixed
        glm::vec3 billboardPos(0.0f, 20.0f, 0.0f);

        // Upright cylindrical billboard: only rotate to face camera, keep vertical
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 toCam = glm::normalize(cameraPos - billboardPos);
        if (glm::dot(toCam, toCam) < 1e-6f) {
            toCam = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        glm::vec3 right = glm::normalize(glm::cross(toCam, worldUp));
        if (glm::dot(right, right) < 1e-6f) {
            right = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        glm::vec3 up = glm::normalize(glm::cross(right, toCam));

        // Create model matrix for billboard (always faces camera)
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // Translate to billboard position
        modelMatrix = glm::translate(modelMatrix, billboardPos);

        // Scale axes explicitly so size is preserved after setting orientation
        const float billboardScale = 20.0f;  // match church scale
        glm::vec3 scaledRight = right * billboardScale;
        glm::vec3 scaledUp = up * billboardScale;

        // Build billboard matrix by setting the scaled axes; keep forward for completeness
        modelMatrix[0] = glm::vec4(scaledRight, 0.0f);
        modelMatrix[1] = glm::vec4(scaledUp, 0.0f);
        modelMatrix[2] = glm::vec4(toCam, 0.0f);
        modelMatrix[3] = glm::vec4(billboardPos, 1.0f);

        // Use shader
        this->shader->Use();

        // Set uniforms
        glUniformMatrix4fv(
            glGetUniformLocation(this->shader->Program, "u_model"), 1, GL_FALSE,
            &modelMatrix[0][0]);
        glUniformMatrix4fv(
            glGetUniformLocation(this->shader->Program, "u_view"), 1, GL_FALSE,
            &viewMatrix[0][0]);
        glUniformMatrix4fv(
            glGetUniformLocation(this->shader->Program, "u_projection"), 1,
            GL_FALSE, &projectionMatrix[0][0]);

        const GLint camLoc =
            glGetUniformLocation(this->shader->Program, "u_cameraPos");
        if (camLoc >= 0) {
            glUniform3fv(camLoc, 1, &cameraPos[0]);
        }
        const GLint smokeLoc =
            glGetUniformLocation(this->shader->Program, "u_smokeParams");
        if (smokeLoc >= 0) {
            glUniform2f(smokeLoc, smokeStart, smokeEnd);
        }

        // Bind texture
        this->texture->bind(0);
        glUniform1i(glGetUniformLocation(this->shader->Program, "u_texture"),
                    0);

        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);

        // Bind VAO and draw
        glBindVertexArray(this->quad->vao);
        glDrawElements(GL_TRIANGLES, this->quad->element_amount,
                       GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Restore state
        if (!wasBlendEnabled) {
            glDisable(GL_BLEND);
        } else {
            glBlendFunc(blendSrcRgb, blendDstRgb);
        }

        if (wasDepthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if (wasCullEnabled)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        glActiveTexture(prevActiveTexture);
        glBindTexture(GL_TEXTURE_2D, prevTexture2D);

        // Restore previous shader program
        glUseProgram(prevProgram);
    }
};

#endif  // TOTEM_OF_UNDYING_HPP
