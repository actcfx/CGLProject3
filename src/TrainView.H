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

#include <vector>
#include "Utilities/Pnt3f.H"

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

class TrainView : public Fl_Gl_Window {
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
    ArcBallCam arcball;  // keep an ArcBall for the UI
    int selectedCube;    // simple - just remember which cube is selected

    TrainWindow* tw;   // The parent of this display window
    CTrack* m_pTrack;  // The track of the entire scene

private:
    // ---------- Lighting ----------
    void setLighting();

    // Train position and orientation for spotlight
    Pnt3f trainPosition;
    Pnt3f trainForward;
    Pnt3f trainUp;

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
    Shader* skyboxShader = nullptr;
    Texture2D* texture = nullptr;
    std::vector<Texture2D*> skyboxTextures = {};
    VAO* plane = nullptr;
    UBO* commonMatrices = nullptr;
    // Performance-related cached GL objects for skybox
    unsigned int skyboxVAO = 0;
    unsigned int skyboxVBO = 0;
    unsigned int skyboxTexture = 0;
    bool glInited = false;

    // ---------- Water Reflection & Refraction ----------
    unsigned int reflectionFBO = 0;
    unsigned int reflectionTexture = 0;
    unsigned int reflectionDepthRBO = 0;
    unsigned int refractionFBO = 0;
    unsigned int refractionTexture = 0;
    unsigned int refractionDepthTexture = 0;
    int waterFBOWidth = 1024;
    int waterFBOHeight = 1024;
    float waterHeight = 10.0f;

    void setUBO();
    void initSimpleChurch();
    void initColorfulChurch();
    void initHeightMapWave();
    void initSineWave();
    void initReflectionWater();
    unsigned int loadCubeMap(std::vector<std::string> faces);
    void clearGlad();
    void initWaterFBOs();
    void renderReflection();
    void renderRefraction();

    void drawPlane();

    // --------- Water Simulation Parameters ---------
    std::vector<glm::vec2> waveDirections;
    std::vector<float> waveWavelengths;
    std::vector<float> waveAmplitudes;
    std::vector<float> waveSpeeds;
    glm::vec2 heightMapScroll = glm::vec2(0.05f, 0.02f);
    float heightMapScale = 0.2f;

    // --------- Frame buffer object for pixelization ----------
    void initPixelizationFBO();
    void freePixelizationFBO();
    void drawScreenQuad();

    GLuint pixelizationFBO = 0;
    GLuint pixelizationTexture = 0;
    GLuint pixelizationRBO = 0;

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    Shader* pixelizationShader = nullptr;
};