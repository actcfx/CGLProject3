#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2\opencv.hpp>

class Texture2D {
public:
    enum Type {
        TEXTURE_DEFAULT = 0,
        TEXTURE_DIFFUSE,
        TEXTURE_SPECULAR,
        TEXTURE_NORMAL,
        TEXTURE_DISPLACEMENT,
        TEXTURE_HEIGHT,
    };

    Type type;

    Texture2D(const char* path, Type texture_type = Texture2D::TEXTURE_DEFAULT)
        : type(texture_type) {
        cv::Mat img;
        // Load with alpha if present so PNG transparency is preserved
        img = cv::imread(path, cv::IMREAD_UNCHANGED);

        this->size.x = img.cols;
        this->size.y = img.rows;

        //cv::cvtColor(img, img, CV_BGR2RGB);

        glGenTextures(1, &this->id);

        glBindTexture(GL_TEXTURE_2D, this->id);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (img.type() == CV_8UC4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, img.data);
        else if (img.type() == CV_8UC3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0,
                         GL_BGR, GL_UNSIGNED_BYTE, img.data);
        else if (img.type() == CV_8UC1)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, img.cols, img.rows, 0, GL_RED,
                         GL_UNSIGNED_BYTE, img.data);
        glBindTexture(GL_TEXTURE_2D, 0);

        img.release();
    }

    void bind(GLenum bind_unit) {
        glActiveTexture(GL_TEXTURE0 + bind_unit);
        glBindTexture(GL_TEXTURE_2D, this->id);
    }

    static void unbind(GLenum bind_unit) {
        glActiveTexture(GL_TEXTURE0 + bind_unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glm::ivec2 size;

private:
    GLuint id;
};