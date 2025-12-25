/************************************************************************
     File:        TrainView.H

     Author:
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu

     Comment:
						The TrainView is the window that actually shows the
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within
						a TrainWindow
						that is the outer window with all the widgets.
						The TrainView needs
						to be aware of the window - since it might need to
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#pragma once

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;
class ControlPoint;
class Shader;

#include "Utilities/Pnt3f.H"

#include <chrono>
#include <glm/gtc/constants.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################

// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"

#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"

#include "RenderUtilities/Model.h"
#include "Shaders/Church.hpp"
#include "Shaders/FrameBuffer.hpp"
#include "Shaders/Skybox.hpp"
#include "Shaders/Water.hpp"
#include "Stuffs/ModelActors.hpp"
#include "Stuffs/Terrain.hpp"

class TotemOfUndying;
class SubdivisionSphere;

class TrainView : public Fl_Gl_Window {
    friend class Water;

public:
    // note that we keep the "standard widget" constructor arguments
    TrainView(int x, int y, int w, int h, const char* l = 0);

    // overrides of important window things
    virtual int handle(int);
    virtual void draw();

    // all of the actual drawing happens in this routine
    // it has to be encapsulated, since we draw differently if
    // we're drawing shadows (no colors, for example)
    void drawStuff(bool doingShadows = false);

    void drawTrack(bool doingShadows);
    void drawTrain(bool doingShadows);
    void drawOden(bool doingShadows);

    // setup the projection - assuming that the projection stack has been
    // cleared for you
    void setProjection();

    // Reset the Arc ball control
    void resetArcball();

    // pick a point (for when the mouse goes down)
    void doPick();

public:
    int displace = 0;
    ArcBallCam arcball;  // keep an ArcBall for the UI
    int selectedCube{};  // simple - just remember which cube is selected

    TrainWindow* tw{};   // The parent of this display window
    CTrack* m_pTrack{};  // The track of the entire scene

    // New members
    Church* church = nullptr;
    Water* water = nullptr;
    Skybox* skybox = nullptr;
    TotemOfUndying* totem = nullptr;
    Terrain* terrain = nullptr;
    SubdivisionSphere* subdivisionSphere = nullptr;
    int currentSphereRecursion = -1;

    // 3D Models
    McChest* mcChest = nullptr;
    McMinecart* mcMinecart = nullptr;
    McFox* mcFox = nullptr;
    McVillager* mcVillager = nullptr;
    Tunnel* tunnel = nullptr;
    Ghast* ghast = nullptr;
    TNT* tnt = nullptr;
    Jet* jet = nullptr;
    Soldier* soldier = nullptr;

    float getSmokeStart() const { return smokeStartDistance; }

    float getSmokeEnd() const { return smokeEndDistance; }

    GLuint getShadowMap() const { return shadowDepthMap; }

    glm::mat4 getLightSpaceMatrix() const { return lightSpaceMatrix; }

    glm::vec3 getDirLightDir() const { return dirLightDir; }

    GLuint getPointShadowMap() const { return pointShadowDepthMap; }

    float getPointFarPlane() const { return pointShadowFarPlane; }

    glm::vec3 getPointLightPos() const;

    GLuint getSpotShadowMap() const { return spotShadowDepthMap; }

    float getSpotFarPlane() const { return spotShadowFarPlane; }

    glm::mat4 getSpotLightMatrix() const { return spotLightMatrix; }

    glm::vec3 getSpotLightPos() const;

    glm::vec3 getSpotLightDir() const;

    // Accessors for the train's current frame
    Pnt3f getTrainForward() const { return trainForward; }

private:
    // ---------- Lighting ----------
    void setLighting();

    // ---------- Shadow Mapping ----------
    void initShadowMap();
    void renderShadowMap();
    void initPointShadowMap();
    void renderPointShadowMap();
    void initSpotShadowMap();
    void renderSpotShadowMap();
    void updateLightMatrices();
    glm::vec3 computePointLightPos() const;
    glm::vec3 computeSpotLightPos() const;
    glm::vec3 computeSpotLightDir() const;

    // Train position and orientation for spotlight
    Pnt3f trainPosition;
    Pnt3f trainForward;
    Pnt3f trainUp;

    GLuint shadowFBO = 0;
    GLuint shadowDepthMap = 0;
    int shadowMapResolution = 2048;
    GLuint pointShadowFBO = 0;
    GLuint pointShadowDepthMap = 0;
    int pointShadowMapResolution = 1024;
    float pointShadowNearPlane = 1.0f;
    float pointShadowFarPlane = 800.0f;
    Shader* pointShadowShader = nullptr;
    GLuint spotShadowFBO = 0;
    GLuint spotShadowDepthMap = 0;
    int spotShadowMapResolution = 2048;
    float spotShadowNearPlane = 1.0f;
    float spotShadowFarPlane = 400.0f;
    Shader* spotShadowShader = nullptr;
    glm::mat4 spotLightMatrix{ 1.0f };
    glm::mat4 lightView{ 1.0f };
    glm::mat4 lightProj{ 1.0f };
    glm::mat4 lightSpaceMatrix{ 1.0f };
    glm::vec3 dirLightDir{ -0.3f, -1.0f, -0.4f };

    float wheelAngle = 0.0f;
    bool wheelParamInitialized = false;

    // ---------- Arc Length ----------
    struct ArcLengthTable {
        size_t segmentCount = 0;
        std::vector<float> segOffset;
        std::vector<std::vector<float>> cumLen;
        std::vector<std::vector<float>> tSamples;
        float totalLen = 0.0f;
    };

    static constexpr int ARCLEN_SAMPLES = 128;

    float currentTension(float fallback) const;
    void buildBasisMatrix(int mode, float out[4][4]) const;

    // ---------- Shaders and Textures ----------
    Shader* shader = nullptr;
    Texture2D* texture = nullptr;
    VAO* plane = nullptr;
    UBO* commonMatrices = nullptr;

    bool glInited = false;

    void setUBO();
    void clearGlad();
    void drawPlane();

    // --------- Frame buffer objects ----------
    FrameBuffer* mainFrameBuffer = nullptr;
    FrameBuffer* tempFrameBuffer = nullptr;

    Shader* pixelShader = nullptr;
    Shader* toonShader = nullptr;
    Shader* paintShader = nullptr;
    Shader* crosshatchShader = nullptr;
    Shader* stippleShader = nullptr;
    Shader* grayscaleShader = nullptr;
    Shader* edgeShader = nullptr;
    Shader* fxaaShader = nullptr;

    Shader* odenBumpShader = nullptr;
    Texture2D* odenBumpTexture = nullptr;

    void initFrameBufferShader();

    float smokeStartDistance = 100.0f;
    float smokeEndDistance = 800.0f;

    // ---------- Ghast motion ----------
    glm::vec3 ghastPosition{ -80.0f, 100.0f, 80.0f };
    glm::vec3 ghastDirection{ 1.0f, 0.0f, 0.0f };
    glm::vec3 ghastMinBounds{ -110.0f, 70.0f, 50.0f };
    glm::vec3 ghastMaxBounds{ -50.0f, 130.0f, 110.0f };
    float ghastSpeed = 35.0f;
    float ghastYaw = glm::half_pi<float>();
    float ghastBaseYawOffset = glm::half_pi<float>();
    float ghastModeDuration = 5.0f;
    float ghastTimeLeft = 0.0f;
    std::chrono::steady_clock::time_point ghastLastUpdate{};
    glm::vec3 villagerWorldPos{ 0.0f };
    bool villagerPosValid = false;

    struct GhastFireball {
        glm::vec3 pos;
        glm::vec3 vel;
        float ttl;
    };

    std::vector<GhastFireball> ghastFireballs;
    float ghastFireInterval = 3.5f;
    float ghastFireCooldown = 0.0f;
    float ghastFireSpeed = 180.0f;
    float ghastFireLifetime = 8.0f;
    glm::vec3 ghastMouthOffset{ 0.0f, 10.0f, 0.0f };
    float ghastProjectileRadius = 6.0f;
    float villagerHitRadius = 6.0f;
    float trainHitRadius = 12.0f;

    void updateGhastMotion();
    glm::vec3 sampleGhastDirection();
    void spawnGhastFireball(const glm::vec3& origin, const glm::vec3& dir);
    void updateGhastFireballs(float dt);
    void drawGhastFireballs(bool doingShadows);
    bool tryPickTnt();
    bool buildMouseRay(glm::vec3& outP0, glm::vec3& outP1,
                       double* outModel = nullptr, double* outProj = nullptr,
                       int* outViewport = nullptr);
    bool fetchViewMatrices(double* outModel, double* outProj, int* outViewport);
    void resetTrainAndVillager();
};