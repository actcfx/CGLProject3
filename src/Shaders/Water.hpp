#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../RenderUtilities/BufferObject.h"
#include "../RenderUtilities/Shader.h"
#include "../RenderUtilities/Texture.h"

class TrainView;  // Forward declaration

class Water {
public:
    Water();
    ~Water();

    void initSineWave();
    void initHeightMapWave();
    void initReflectionWater();
    void initWaterFBOs(int width, int height);

    // Render methods need access to TrainView to draw the scene
    void renderReflection(TrainView* tw);
    void renderRefraction(TrainView* tw);

    VAO* plane = nullptr;
    Texture2D* texture = nullptr;
    Shader* shader = nullptr;
    UBO* commonMatrices = nullptr;

    // FBOs
    unsigned int reflectionFBO = 0;
    unsigned int reflectionTexture = 0;
    unsigned int reflectionDepthRBO = 0;
    unsigned int refractionFBO = 0;
    unsigned int refractionTexture = 0;
    unsigned int refractionDepthTexture = 0;

    int waterFBOWidth = 1024;
    int waterFBOHeight = 1024;
    float waterHeight = 0.0f;

    // Wave parameters
    std::vector<glm::vec2> waveDirections;
    std::vector<float> waveWavelengths;
    std::vector<float> waveAmplitudes;
    std::vector<float> waveSpeeds;
    glm::vec2 heightMapScroll = glm::vec2(0.01f, 0.01f);
    float heightMapScale = 0.05f;

private:
    void cleanup();
};
