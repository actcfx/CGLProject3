#pragma once

#include <glm/glm.hpp>
#include <string>

class TrainView;
class Shader;
class Model;

class ModelActor {
public:
    virtual ~ModelActor() = default;

protected:
    ModelActor(TrainView* owner, std::string modelPath, float uniformScale);

    void drawInternal(const glm::vec3& position);

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

    void draw(const glm::vec3& position) { drawInternal(position); }
};

class Backpack : public ModelActor {
public:
    explicit Backpack(TrainView* owner);

    void draw(const glm::vec3& position) { drawInternal(position); }
};
