#include "Water.hpp"
#include <GL/glu.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "../TrainView.H"
#include "../TrainWindow.H"
#include "../Utilities/3DUtils.H"

Water::Water() {}

Water::~Water() {
    cleanup();
}

void Water::cleanup() {
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

    if (reflectionFBO) {
        glDeleteFramebuffers(1, &reflectionFBO);
        reflectionFBO = 0;
    }
    if (reflectionTexture) {
        glDeleteTextures(1, &reflectionTexture);
        reflectionTexture = 0;
    }
    if (reflectionDepthRBO) {
        glDeleteRenderbuffers(1, &reflectionDepthRBO);
        reflectionDepthRBO = 0;
    }
    if (refractionFBO) {
        glDeleteFramebuffers(1, &refractionFBO);
        refractionFBO = 0;
    }
    if (refractionTexture) {
        glDeleteTextures(1, &refractionTexture);
        refractionTexture = 0;
    }
    if (refractionDepthTexture) {
        glDeleteTextures(1, &refractionDepthTexture);
        refractionDepthTexture = 0;
    }
}

void Water::initSineWave() {
    cleanup();

    if (!this->shader) {
        this->shader = new Shader("./shaders/sineWave.vert", nullptr, nullptr,
                                  nullptr, "./shaders/sineWave.frag");
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

    // Initialize wave parameters
    waveDirections.clear();
    waveWavelengths.clear();
    waveAmplitudes.clear();
    waveSpeeds.clear();

    // Add some waves
    // Adjusted for size 1.0 (Church size)
    waveDirections.push_back(glm::vec2(1.0f, 0.5f));
    waveWavelengths.push_back(0.5f);
    waveAmplitudes.push_back(0.04f);
    waveSpeeds.push_back(0.1f);

    if (!this->plane) {
        const int N = 200;        // High resolution
        const float size = 1.0f;  // Same as Church
        const float step = size / N;

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLfloat> texcoords;
        std::vector<GLfloat> colors;
        std::vector<GLuint> elements;

        // Generate grid
        for (int j = 0; j <= N; ++j) {
            for (int i = 0; i <= N; ++i) {
                float x = -size / 2.0f + i * step;
                float z = -size / 2.0f + j * step;

                vertices.push_back(x);
                vertices.push_back(0.0f);
                vertices.push_back(z);

                normals.push_back(0.0f);
                normals.push_back(1.0f);
                normals.push_back(0.0f);

                texcoords.push_back((float)i / N);
                texcoords.push_back((float)j / N);

                colors.push_back(1.0f);
                colors.push_back(1.0f);
                colors.push_back(1.0f);
            }
        }

        for (int j = 0; j < N; ++j) {
            for (int i = 0; i < N; ++i) {
                int row1 = j * (N + 1);
                int row2 = (j + 1) * (N + 1);

                elements.push_back(row1 + i);
                elements.push_back(row2 + i);
                elements.push_back(row1 + i + 1);

                elements.push_back(row1 + i + 1);
                elements.push_back(row2 + i);
                elements.push_back(row2 + i + 1);
            }
        }

        this->plane = new VAO();
        this->plane->element_amount = elements.size();

        glGenVertexArrays(1, &this->plane->vao);
        glGenBuffers(4, this->plane->vbo);
        glGenBuffers(1, &this->plane->ebo);

        glBindVertexArray(this->plane->vao);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                     normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Texture Coords
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat),
                     texcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Colors
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                     colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Elements
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint),
                     elements.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
}

void Water::initHeightMapWave() {
    cleanup();

    if (!this->shader) {
        this->shader =
            new Shader("./shaders/heightMapWave.vert", nullptr, nullptr,
                       nullptr, "./shaders/heightMapWave.frag");
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

    if (!this->texture) {
        this->texture = new Texture2D("./images/waterHeightMap.jpg");
    }

    if (!this->plane) {
        const int N = 200;        // High resolution
        const float size = 1.0f;  // Same as Church
        const float step = size / N;

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLfloat> texcoords;
        std::vector<GLfloat> colors;
        std::vector<GLuint> elements;

        // Generate grid
        for (int j = 0; j <= N; ++j) {
            for (int i = 0; i <= N; ++i) {
                float x = -size / 2.0f + i * step;
                float z = -size / 2.0f + j * step;

                vertices.push_back(x);
                vertices.push_back(0.0f);
                vertices.push_back(z);

                normals.push_back(0.0f);
                normals.push_back(1.0f);
                normals.push_back(0.0f);

                texcoords.push_back((float)i / N);
                texcoords.push_back((float)j / N);

                colors.push_back(1.0f);
                colors.push_back(1.0f);
                colors.push_back(1.0f);
            }
        }

        for (int j = 0; j < N; ++j) {
            for (int i = 0; i < N; ++i) {
                int row1 = j * (N + 1);
                int row2 = (j + 1) * (N + 1);

                elements.push_back(row1 + i);
                elements.push_back(row2 + i);
                elements.push_back(row1 + i + 1);

                elements.push_back(row1 + i + 1);
                elements.push_back(row2 + i);
                elements.push_back(row2 + i + 1);
            }
        }

        this->plane = new VAO();
        this->plane->element_amount = elements.size();

        glGenVertexArrays(1, &this->plane->vao);
        glGenBuffers(4, this->plane->vbo);
        glGenBuffers(1, &this->plane->ebo);

        glBindVertexArray(this->plane->vao);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                     normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Texture Coords
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat),
                     texcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Colors
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                     colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Elements
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint),
                     elements.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
}

void Water::initReflectionWater(TrainView* tw) {
    cleanup();

    if (!this->shader) {
        this->shader = new Shader("./shaders/reflection.vert", nullptr, nullptr,
                                  nullptr, "./shaders/reflection.frag");
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
        const int N =
            200;  // match resolution of terrain / other water types so top view looks seamless
        // base mesh size (unit plane). Actual size is handled by model matrix scale in drawPlane
        float size = 1.0f;
        const float step = size / N;  // side length of each square

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLfloat> texcoords;
        std::vector<GLfloat> colors;
        std::vector<GLuint> elements;

        // Generate grid vertices (N+1) x (N+1)
        for (int j = 0; j <= N; ++j) {
            for (int i = 0; i <= N; ++i) {
                float x = -size / 2.0f + i * step;
                float z = -size / 2.0f + j * step;

                vertices.push_back(x);
                vertices.push_back(0.0f);
                vertices.push_back(z);

                normals.push_back(0.0f);
                normals.push_back(1.0f);
                normals.push_back(0.0f);

                texcoords.push_back((float)i / N);
                texcoords.push_back((float)j / N);

                // color gradient (for visualization)
                colors.push_back((float)i / N);
                colors.push_back((float)j / N);
                colors.push_back(1.0f - (float)i / N);
            }
        }

        // Generate indices (2 triangles per square)
        for (int j = 0; j < N; ++j) {
            for (int i = 0; i < N; ++i) {
                int row1 = j * (N + 1);
                int row2 = (j + 1) * (N + 1);

                GLuint a = row1 + i;
                GLuint b = row1 + i + 1;
                GLuint c = row2 + i;
                GLuint d = row2 + i + 1;

                // two right triangles forming a square
                // triangle 1: a-b-c (lower left)
                elements.push_back(a);
                elements.push_back(b);
                elements.push_back(c);

                // triangle 2: b-d-c (upper right)
                elements.push_back(b);
                elements.push_back(d);
                elements.push_back(c);
            }
        }

        this->plane = new VAO();
        this->plane->element_amount = static_cast<GLuint>(elements.size());

        glGenVertexArrays(1, &this->plane->vao);
        glGenBuffers(4, this->plane->vbo);
        glGenBuffers(1, &this->plane->ebo);
        glBindVertexArray(this->plane->vao);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                     normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (GLvoid*)0);
        glEnableVertexAttribArray(1);

        // Texcoord
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat),
                     texcoords.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                              (GLvoid*)0);
        glEnableVertexAttribArray(2);

        // Color
        glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                     colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                              (GLvoid*)0);
        glEnableVertexAttribArray(3);

        // Element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint),
                     elements.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
}

void Water::initWaterFBOs(int width, int height) {
    bool resize = (width != waterFBOWidth || height != waterFBOHeight);
    waterFBOWidth = width;
    waterFBOHeight = height;

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    // Reflection FBO
    if (reflectionFBO == 0) {
        glGenFramebuffers(1, &reflectionFBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);

    if (reflectionTexture == 0 || resize) {
        if (reflectionTexture == 0) {
            glGenTextures(1, &reflectionTexture);
        }
        glBindTexture(GL_TEXTURE_2D, reflectionTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, waterFBOWidth, waterFBOHeight, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, reflectionTexture, 0);
    }

    if (reflectionDepthRBO == 0 || resize) {
        if (reflectionDepthRBO == 0) {
            glGenRenderbuffers(1, &reflectionDepthRBO);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                              waterFBOWidth, waterFBOHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, reflectionDepthRBO);
    }

    GLenum reflectionStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (reflectionStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Reflection FBO incomplete: " << reflectionStatus
                  << std::endl;
    }

    // Refraction FBO
    if (refractionFBO == 0) {
        glGenFramebuffers(1, &refractionFBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);

    if (refractionTexture == 0 || resize) {
        if (refractionTexture == 0) {
            glGenTextures(1, &refractionTexture);
        }
        glBindTexture(GL_TEXTURE_2D, refractionTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, waterFBOWidth, waterFBOHeight, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, refractionTexture, 0);
    }

    if (refractionDepthTexture == 0 || resize) {
        if (refractionDepthTexture == 0) {
            glGenTextures(1, &refractionDepthTexture);
        }
        glBindTexture(GL_TEXTURE_2D, refractionDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, waterFBOWidth,
                     waterFBOHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, refractionDepthTexture, 0);
    }

    GLenum refractionStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (refractionStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Refraction FBO incomplete: " << refractionStatus
                  << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
}

void Water::renderReflection(TrainView* tw) {
    if (tw->tw->shaderBrowser->value() != 5)
        return;  // Only for reflection water

    initWaterFBOs(tw->w(), tw->h());

    // Save previous FBO, viewport, and matrices to restore later
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Save current camera modelview matrix
    glm::mat4 originalModelView;
    glGetFloatv(GL_MODELVIEW_MATRIX, &originalModelView[0][0]);

    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    glViewport(0, 0, waterFBOWidth, waterFBOHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.12f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the same projection as main scene
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    // Projection is already set by setProjection(), don't change it

    // Apply mirror transformation on top of existing camera view
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    // Start with current camera view
    glLoadMatrixf(&originalModelView[0][0]);
    // Then apply mirror transformation
    glTranslatef(0.0f, waterHeight, 0.0f);
    glScalef(1.0f, -1.0f, 1.0f);
    glTranslatef(0.0f, -waterHeight, 0.0f);

    // Clip everything below the water plane for reflection
    glEnable(GL_CLIP_PLANE0);
    double clipPlane[4] = { 0.0, 1.0, 0.0, -waterHeight };
    glClipPlane(GL_CLIP_PLANE0, clipPlane);

    // Set up lighting for reflection view
    tw->setLighting();

    // Draw skybox in reflected view
    if (tw->skybox) {
        glm::mat4 view_matrix;
        glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
        glm::mat4 projection_matrix;
        glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
        tw->skybox->draw(view_matrix, projection_matrix);
    }

    // Draw all scene objects in reflection view
    glUseProgram(0);

    // Draw terrain in reflection (respect clip plane — keep GL_CLIP_PLANE0 enabled)
    if (tw->terrain) {
        tw->terrain->draw();
    }

    glEnable(GL_LIGHTING);
    setupObjects();

    // Draw all scene elements: track, train, oden, control points
    tw->drawStuff();

    // Disable clip plane before restoring matrices/state
    glDisable(GL_CLIP_PLANE0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Restore previous framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
               prevViewport[3]);
}

void Water::renderRefraction(TrainView* tw) {
    if (tw->tw->shaderBrowser->value() != 5)
        return;  // Only for reflection water

    initWaterFBOs(tw->w(), tw->h());

    // Save previous FBO, viewport, and matrices to restore later
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Save current camera modelview matrix
    glm::mat4 originalModelView;
    glGetFloatv(GL_MODELVIEW_MATRIX, &originalModelView[0][0]);

    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);
    glViewport(0, 0, waterFBOWidth, waterFBOHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.12f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the same projection as main scene
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    // Projection is already set, don't change it

    // Regular view for refraction (use current camera view)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(&originalModelView[0][0]);

    // Clip everything above the water plane for refraction
    // This shows only objects underwater
    glEnable(GL_CLIP_PLANE0);
    double clipPlane[4] = { 0.0, -1.0, 0.0, waterHeight };
    glClipPlane(GL_CLIP_PLANE0, clipPlane);

    // Set up lighting for refraction view
    tw->setLighting();

    // Draw all scene objects that intersect with water (for underwater appearance)
    glEnable(GL_LIGHTING);
    setupObjects();

    // Draw terrain in refraction
    // Draw terrain in refraction (respect clip plane — keep GL_CLIP_PLANE0 enabled)
    if (tw->terrain) {
        tw->terrain->draw();
    }

    // Draw all scene elements clipped below water: track, train, oden, control points
    tw->drawStuff();

    // Disable clip plane before restoring matrices/state
    glDisable(GL_CLIP_PLANE0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Restore previous framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
               prevViewport[3]);
}
