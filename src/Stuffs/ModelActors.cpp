#include "ModelActors.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>

#include "../RenderUtilities/Model.h"
#include "../RenderUtilities/Shader.h"
#include "../TrainView.H"

namespace {
bool fileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f) {
        std::fclose(f);
        return true;
    }
    return false;
}

std::string resolveAssetPath(const std::string& relative) {
    if (fileExists(relative))
        return relative;

    const char* prefixes[] = {
        "./", "../", "../../", "../../../", "../../../../",
        "../../../../../" };
    for (const char* prefix : prefixes) {
        std::string candidate = std::string(prefix) + relative;
        if (fileExists(candidate))
            return candidate;
    }
    return relative;
}
}

ModelActor::ModelActor(TrainView* view, std::string path, float uniformScale)
    : owner(view), modelRelativePath(std::move(path)), scale(uniformScale) {}

void ModelActor::ensureResources() {
    if (!shader) {
        shader = new Shader("./shaders/model.vert", nullptr, nullptr, nullptr,
                            "./shaders/model.frag");
    }
    if (!model) {
        const std::string assetPath = resolveAssetPath(modelRelativePath);
        model = new Model(assetPath);
    }
}

void ModelActor::drawInternal(const glm::vec3& position) {
    if (!owner)
        return;

    ensureResources();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, owner->w(), owner->h());
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    owner->setProjection();

    glm::mat4 projectionMatrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &projectionMatrix[0][0]);
    glm::mat4 viewMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &viewMatrix[0][0]);

    glm::mat4 modelMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;

    shader->Use();
    const GLint mvpLoc = glGetUniformLocation(shader->Program, "uMVP");
    if (mvpLoc >= 0) {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
    }

    model->Draw(*shader);

    glUseProgram(0);
}

MinecraftChest::MinecraftChest(TrainView* owner)
    : ModelActor(owner, "./assets/models/Minecraft Chest/model/Obj/chest.obj",
                 5.0f) {}

Backpack::Backpack(TrainView* owner)
    : ModelActor(owner, "./assets/models/backpack/backpack.obj", 5.0f) {}
