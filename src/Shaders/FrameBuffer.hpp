#pragma once
#include <glad/glad.h>
#include <iostream>

class FrameBuffer {
public:
    unsigned int ID = 0;       // FBO ID
    unsigned int texture = 0;  // Texture ID (Color Buffer)
    unsigned int rbo = 0;      // Render Buffer ID (Depth/Stencil)
    int width = 0;
    int height = 0;

    // Quad Resources
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    FrameBuffer(const int w, const int h) {
        initQuad();
        resize(w, h);
    }

    ~FrameBuffer() {
        if (ID)
            glDeleteFramebuffers(1, &ID);
        if (texture)
            glDeleteTextures(1, &texture);
        if (rbo)
            glDeleteRenderbuffers(1, &rbo);
        if (quadVAO)
            glDeleteVertexArrays(1, &quadVAO);
        if (quadVBO)
            glDeleteBuffers(1, &quadVBO);
    }

    // Call this when window size changes
    void resize(const int w, const int h) {
        if (width == w && height == h && ID != 0)
            return;  // No change needed

        width = w;
        height = h;

        // Clean up old resources if they exist
        if (ID) {
            glDeleteFramebuffers(1, &ID);
            glDeleteTextures(1, &texture);
            glDeleteRenderbuffers(1, &rbo);
        }

        // 1. Create Framebuffer
        glGenFramebuffers(1, &ID);
        glBindFramebuffer(GL_FRAMEBUFFER, ID);

        // 2. Create Texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, NULL);

        // Important for Pixel Art style: GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture, 0);

        // 3. Create Renderbuffer (Depth/Stencil)
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                              height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, rbo);

        // 4. Check status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
                      << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Start writing to this FBO
    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, ID);
        glViewport(0, 0, width, height);
    }

    // Stop writing (return to default screen)
    static void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    // Bind the result texture for reading in Shader
    void bindTexture(int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    // Draw the full screen quad
    void drawQuad() const {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

private:
    void initQuad() {
        if (quadVAO != 0)
            return;

        constexpr float quadVertices[] = {
            // positions   // texCoords
            -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }
};