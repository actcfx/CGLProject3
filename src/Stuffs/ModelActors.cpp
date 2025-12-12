#include "ModelActors.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_inverse.hpp>
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

void ModelActor::drawInternal(const glm::mat4& modelMatrix, bool doingShadows,
                              float smokeStart, float smokeEnd) {
    if (!owner)
        return;

    ensureResources();

    glm::mat4 viewMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &viewMatrix[0][0]);
    glm::mat4 projectionMatrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &projectionMatrix[0][0]);

    glm::mat4 scaledModel =
        modelMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    glm::mat3 normalMatrix =
        glm::mat3(glm::transpose(glm::inverse(scaledModel)));

    glm::vec3 cameraPos(0.0f);
    if (!doingShadows) {
        glm::mat4 invView = glm::inverse(viewMatrix);
        cameraPos = glm::vec3(invView[3]);
    }

    float resolvedStart = smokeStart;
    float resolvedEnd = smokeEnd;
    if (owner && (resolvedStart < 0.0f || resolvedEnd <= resolvedStart)) {
        resolvedStart = owner->getSmokeStart();
        resolvedEnd = owner->getSmokeEnd();
    }

    glm::vec2 smokeParams(-1.0f, -1.0f);
    if (resolvedEnd > resolvedStart && resolvedStart >= 0.0f) {
        smokeParams = glm::vec2(resolvedStart, resolvedEnd);
    }

    shader->Use();

    auto setMat4 = [&](const char* name, const glm::mat4& mat) {
        const GLint loc = glGetUniformLocation(shader->Program, name);
        if (loc >= 0) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
        }
    };

    auto setMat3 = [&](const char* name, const glm::mat3& mat) {
        const GLint loc = glGetUniformLocation(shader->Program, name);
        if (loc >= 0) {
            glUniformMatrix3fv(loc, 1, GL_FALSE, &mat[0][0]);
        }
    };

    setMat4("uModel", scaledModel);
    setMat4("uView", viewMatrix);
    setMat4("uProjection", projectionMatrix);
    setMat3("uNormalMatrix", normalMatrix);

    const GLint shadowLoc = glGetUniformLocation(shader->Program, "uShadowPass");
    if (shadowLoc >= 0) {
        glUniform1i(shadowLoc, doingShadows ? 1 : 0);
    }

    const GLint smokeLoc = glGetUniformLocation(shader->Program, "uSmokeParams");
    if (smokeLoc >= 0) {
        glUniform2fv(smokeLoc, 1, &smokeParams[0]);
    }

    const GLint smokeEnabledLoc =
        glGetUniformLocation(shader->Program, "smokeEnabled");
    if (smokeEnabledLoc >= 0 && owner && owner->tw && owner->tw->smokeButton) {
        glUniform1i(smokeEnabledLoc, owner->tw->smokeButton->value() ? 1 : 0);
    }

    const GLint camLoc = glGetUniformLocation(shader->Program, "uCameraPos");
    if (camLoc >= 0) {
        glUniform3fv(camLoc, 1, &cameraPos[0]);
    }

    GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    model->Draw(*shader);
    if (wasCullEnabled) glEnable(GL_CULL_FACE);
    model->Draw(*shader);

    glUseProgram(0);
}

McChest::McChest(TrainView* owner)
    : ModelActor(owner, "./assets/models/minecraftChest/model/Obj/chest.obj", 5.0f) {}

McMinecart::McMinecart(TrainView* owner)
    : ModelActor(owner, "./assets/models/minecraftMinecart/scene.gltf", 10.0f) {}

McFox::McFox(TrainView* owner)
    : ModelActor(owner, "./assets/models/minecraftFox/Fox.fbx", 0.03f) {}

McVillager::McVillager(TrainView* owner)
    : ModelActor(owner, "./assets/models/minecraftVillager/scene.gltf", 1.0f) {}