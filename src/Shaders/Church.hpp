#ifndef CHURCH_HPP
#define CHURCH_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../RenderUtilities/BufferObject.h"
#include "../RenderUtilities/Shader.h"
#include "../RenderUtilities/Texture.h"

class Church {
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

        if (this->plane) {
            glDeleteVertexArrays(1, &this->plane->vao);
            glDeleteBuffers(4, this->plane->vbo);
            glDeleteBuffers(1, &this->plane->ebo);
            delete this->plane;
            this->plane = nullptr;
        }

        if (this->texture) {
            delete this->texture;
            this->texture = nullptr;
        }
    }

public:
    VAO* plane = nullptr;
    Texture2D* texture = nullptr;
    Shader* shader = nullptr;
    UBO* commonMatrices = nullptr;

    Church() {}

    ~Church() { cleanup(); }

    void initSimple() {
        cleanup();

        if (!this->shader) {
            this->shader =
                new Shader("./shaders/simpleChurch.vert", nullptr, nullptr,
                           nullptr, "./shaders/simpleChurch.frag");
        }

        if (!this->commonMatrices) {
            this->commonMatrices = new UBO();
            this->commonMatrices->size = 2 * sizeof(glm::mat4);
            glGenBuffers(1, &this->commonMatrices->ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, this->commonMatrices->ubo);
            glBufferData(GL_UNIFORM_BUFFER, this->commonMatrices->size, NULL,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        if (!this->plane) {
            GLfloat vertices[] = { -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f,
                                   0.5f,  0.0f, 0.5f,  0.5f,  0.0f, -0.5f };
            GLfloat normal[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
            GLfloat textureCoordinate[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                                            1.0f, 1.0f, 0.0f, 1.0f };
            GLuint element[] = { 0, 1, 2, 0, 2, 3 };

            this->plane = new VAO;
            this->plane->element_amount = sizeof(element) / sizeof(GLuint);
            glGenVertexArrays(1, &this->plane->vao);
            glGenBuffers(3, this->plane->vbo);
            glGenBuffers(1, &this->plane->ebo);
            glBindVertexArray(this->plane->vao);

            // Position attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            // Normal attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            // Texture Coordinate attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinate),
                         textureCoordinate, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);
            //Element attribute
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element,
                         GL_STATIC_DRAW);

            // Unbind VAO
            glBindVertexArray(0);
        }

        if (!this->texture) {
            this->texture = new Texture2D("./images/church.png");
        }
    }

    void initColorful() {
        cleanup();

        if (!this->shader) {
            this->shader =
                new Shader("./shaders/colorfulChurch.vert", nullptr, nullptr,
                           nullptr, "./shaders/colorfulChurch.frag");
        }

        if (!this->commonMatrices) {
            this->commonMatrices = new UBO();
            this->commonMatrices->size = 2 * sizeof(glm::mat4);
            glGenBuffers(1, &this->commonMatrices->ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, this->commonMatrices->ubo);
            glBufferData(GL_UNIFORM_BUFFER, this->commonMatrices->size, NULL,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        if (!this->plane) {
            GLfloat vertices[] = { -0.5f, 0.0f, -sqrt(3.0f) / 6.0f,
                                   0.5f,  0.0f, -sqrt(3.0f) / 6.0f,
                                   0.0f,  0.0f, sqrt(3.0f) / 3.0f };

            GLfloat normal[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                 1.0f, 0.0f, 0.0f, 1.0f };

            GLfloat texture_coordinate[] = {
                0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f
            };

            GLuint element[] = { 0, 1, 2 };

            GLfloat colors[] = {
                1.0f, 0.0f, 0.0f,  // Red
                0.0f, 1.0f, 0.0f,  // Green
                0.0f, 0.0f, 1.0f   // Blue
            };

            this->plane = new VAO();
            this->plane->element_amount = sizeof(element) / sizeof(GLuint);
            glGenVertexArrays(1, &this->plane->vao);
            glGenBuffers(4, this->plane->vbo);
            glGenBuffers(1, &this->plane->ebo);

            glBindVertexArray(this->plane->vao);

            // Position attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            // Normal attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            // Texture Coordinate attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate),
                         texture_coordinate, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);

            // Element attribute
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element,
                         GL_STATIC_DRAW);

            // Color attribute
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(3);

            // Unbind VAO
            glBindVertexArray(0);
        }

        if (!this->texture) {
            this->texture = new Texture2D("./images/church.png");
        }
    }

    void initSierpinski() {
        cleanup();

        if (!this->shader) {
            this->shader =
                new Shader("./shaders/snowflake.vert", nullptr, nullptr,
                           nullptr, "./shaders/snowflake.frag");
        }

        if (!this->commonMatrices) {
            this->commonMatrices = new UBO();
            this->commonMatrices->size = 2 * sizeof(glm::mat4);
            glGenBuffers(1, &this->commonMatrices->ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, this->commonMatrices->ubo);
            glBufferData(GL_UNIFORM_BUFFER, this->commonMatrices->size, NULL,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        if (!this->plane) {
            GLfloat vertices[] = { -0.5f, 0.0f, -sqrt(3.0f) / 6.0f,
                                   0.5f,  0.0f, -sqrt(3.0f) / 6.0f,
                                   0.0f,  0.0f, sqrt(3.0f) / 3.0f };

            GLfloat normal[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                 1.0f, 0.0f, 0.0f, 1.0f };

            GLfloat texture_coordinate[] = {
                0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f
            };

            GLuint element[] = { 0, 1, 2 };

            GLfloat barycentrics[] = {
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f
            };

            this->plane = new VAO();
            this->plane->element_amount = sizeof(element) / sizeof(GLuint);
            glGenVertexArrays(1, &this->plane->vao);
            glGenBuffers(4, this->plane->vbo);
            glGenBuffers(1, &this->plane->ebo);

            glBindVertexArray(this->plane->vao);

            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate),
                         texture_coordinate, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(barycentrics), barycentrics,
                         GL_STATIC_DRAW);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(3);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element,
                         GL_STATIC_DRAW);

            glBindVertexArray(0);
        }

        if (!this->texture) {
            this->texture = nullptr;
        }
    }
};

#endif  // CHURCH_HPP
