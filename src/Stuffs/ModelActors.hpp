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

class McChest : public ModelActor {
public:
    explicit McChest(TrainView* owner);

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        drawInternal(model, false);
    }
};

class McMinecart : public ModelActor {
public:
    explicit McMinecart(TrainView* owner);

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        drawInternal(model, false);
    }

    void draw(const glm::mat4& modelMatrix, bool doingShadows,
              float smokeStart, float smokeEnd) {
        drawInternal(modelMatrix, doingShadows, smokeStart, smokeEnd);
    }
};

class McFox : public ModelActor {
public:
    explicit McFox(TrainView* owner);

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
        drawInternal(model, false);
    }
};

class McVillager : public ModelActor {
public:
    explicit McVillager(TrainView* owner);

    void draw(const glm::vec3& position) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        drawInternal(model, false);
    }

    void draw(const glm::mat4& modelMatrix, bool doingShadows,
              float smokeStart, float smokeEnd) {
        drawInternal(modelMatrix, doingShadows, smokeStart, smokeEnd);
    }
};