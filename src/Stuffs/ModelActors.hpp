#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class TrainView;
class Shader;
class Model;

class ModelActor {
public:
    virtual ~ModelActor() = default;

protected:
    ModelActor(TrainView* owner, std::string modelPath, float uniformScale);

    void drawInternal(const glm::mat4& modelMatrix, bool doingShadows,
                      float smokeStart = -1.0f, float smokeEnd = -1.0f);

private:
    void ensureResources();

protected:
    TrainView* owner = nullptr;
    Shader* shader = nullptr;
    Model* model = nullptr;
    std::string modelRelativePath;
    float scale = 1.0f;
};

class MinecraftChest : public ModelActor {
public:
    explicit MinecraftChest(TrainView* owner);

    void draw(const glm::mat4& modelMatrix, bool doingShadows,
              float smokeStart, float smokeEnd) {
        drawInternal(modelMatrix, doingShadows, smokeStart, smokeEnd);
    }

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        drawInternal(model, false);
    }
};

class Backpack : public ModelActor {
public:
    explicit Backpack(TrainView* owner);

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        drawInternal(model, false);
    }
};
