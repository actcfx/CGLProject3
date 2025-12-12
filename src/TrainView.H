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

#include "Shaders/Church.hpp"
#include "Shaders/FrameBuffer.hpp"
#include "Shaders/Skybox.hpp"
#include "Shaders/Water.hpp"
#include "Stuffs/totemOfUndying.hpp"
#include "RenderUtilities/Model.h"
#include "Stuffs/ModelActors.hpp"

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
    Backpack* backpack = nullptr;
    McChest* mcChest = nullptr;
    McMinecart* mcMinecart = nullptr;
    McFox* mcFox = nullptr;

    float getSmokeStart() const { return smokeStartDistance; }
    float getSmokeEnd() const { return smokeEndDistance; }

    // Accessors for the train's current frame
    Pnt3f getTrainForward() const { return trainForward; }
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

    void initFrameBufferShader();

    float smokeStartDistance = 120.0f;
    float smokeEndDistance = 500.0f;
};