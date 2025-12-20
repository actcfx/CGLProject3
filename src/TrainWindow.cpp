/************************************************************************
     File:        TrainWindow.H

     Author:
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu

     Comment:
						this class defines the window in which the project
						runs - its the outer windows that contain all of
						the widgets, including the "TrainView" which has the
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing
						itself into the FlTk "idle" loop so that we get periodic
						updates (if we're running the train).


     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/
#define STB_IMAGE_IMPLEMENTATION
#include <FL/Fl_Box.h>
#include <FL/fl.h>

// for the 3D models
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// for using the real time clock
#include <time.h>

#include "CallBacks.H"
#include "RenderUtilities/Shader.h"
#include "TrainView.H"
#include "TrainWindow.H"

//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::TrainWindow(const int x, const int y)
    : Fl_Double_Window(x, y, 800, 600, "Train and Roller Coaster")
//========================================================================
{
    // make all of the widgets
    begin();  // add to this widget
    {
        int pty = 5;  // where the last widgets were drawn

        trainView = new TrainView(5, 5, 590, 590);
        trainView->tw = this;
        trainView->m_pTrack = &m_Track;
        this->resizable(trainView);

        // to make resizing work better, put all the widgets in a group
        widgets = new Fl_Group(600, 5, 190, 590);
        widgets->begin();

        runButton = new Fl_Button(605, pty, 60, 20, "Run");
        togglify(runButton);

        Fl_Button* fb = new Fl_Button(700, pty, 25, 20, "@>>");
        fb->callback((Fl_Callback*)forwCB, this);
        Fl_Button* rb = new Fl_Button(670, pty, 25, 20, "@<<");
        rb->callback((Fl_Callback*)backCB, this);

        arcLength = new Fl_Button(730, pty, 65, 20, "ArcLength");
        togglify(arcLength, 1);

        pty += 25;
        speed = new Fl_Value_Slider(655, pty, 140, 20, "speed");
        speed->range(0, 10);
        speed->value(1);
        speed->align(FL_ALIGN_LEFT);
        speed->type(FL_HORIZONTAL);

        pty += 30;

        // camera buttons - in a radio button group
        Fl_Group* camGroup = new Fl_Group(600, pty, 195, 20);
        camGroup->begin();
        worldCam = new Fl_Button(605, pty, 60, 20, "World");
        worldCam->type(FL_RADIO_BUTTON);         // radio button
        worldCam->value(1);                      // turned on
        worldCam->selection_color((Fl_Color)3);  // yellow when pressed
        worldCam->callback((Fl_Callback*)damageCB, this);
        trainCam = new Fl_Button(670, pty, 60, 20, "Train");
        trainCam->type(FL_RADIO_BUTTON);
        trainCam->value(0);
        trainCam->selection_color((Fl_Color)3);
        trainCam->callback((Fl_Callback*)damageCB, this);
        topCam = new Fl_Button(735, pty, 60, 20, "Top");
        topCam->type(FL_RADIO_BUTTON);
        topCam->value(0);
        topCam->selection_color((Fl_Color)3);
        topCam->callback((Fl_Callback*)damageCB, this);
        camGroup->end();

        pty += 30;

        // browser to select spline types
        splineBrowser = new Fl_Browser(605, pty, 120, 75, "Spline Type");
        splineBrowser->type(2);  // select
        splineBrowser->callback((Fl_Callback*)damageCB, this);
        splineBrowser->add("Linear");
        splineBrowser->add("Cardinal Cubic");
        splineBrowser->add("Cubic B-Spline");
        splineBrowser->select(2);

        bgmButton = new Fl_Button(735, pty, 60, 20, "BGM");
        togglify(bgmButton, 1);

        pty += 110;

        // add and delete points
        Fl_Button* ap = new Fl_Button(605, pty, 80, 20, "Add Point");
        ap->callback((Fl_Callback*)addPointCB, this);
        Fl_Button* dp = new Fl_Button(690, pty, 80, 20, "Delete Point");
        dp->callback((Fl_Callback*)deletePointCB, this);

        pty += 25;
        // reset the points
        resetButton = new Fl_Button(735, pty, 60, 20, "Reset");
        resetButton->callback((Fl_Callback*)resetCB, this);
        Fl_Button* loadb = new Fl_Button(605, pty, 60, 20, "Load");
        loadb->callback((Fl_Callback*)loadCB, this);
        Fl_Button* saveb = new Fl_Button(670, pty, 60, 20, "Save");
        saveb->callback((Fl_Callback*)saveCB, this);

        pty += 25;
        // roll the points
        Fl_Button* rx = new Fl_Button(605, pty, 30, 20, "R+X");
        rx->callback((Fl_Callback*)rpxCB, this);
        Fl_Button* rxp = new Fl_Button(635, pty, 30, 20, "R-X");
        rxp->callback((Fl_Callback*)rmxCB, this);
        Fl_Button* rz = new Fl_Button(670, pty, 30, 20, "R+Z");
        rz->callback((Fl_Callback*)rpzCB, this);
        Fl_Button* rzp = new Fl_Button(700, pty, 30, 20, "R-Z");
        rzp->callback((Fl_Callback*)rmzCB, this);

        pty += 30;

        // ---------- Lighting Buttons ----------
        directionalLightButton = new Fl_Button(605, pty, 60, 20, "Direct");
        togglify(directionalLightButton, 1);

        pointLightButton = new Fl_Button(670, pty, 60, 20, "Point");
        togglify(pointLightButton, 0);

        spotLightButton = new Fl_Button(735, pty, 60, 20, "Spot");
        togglify(spotLightButton, 0);

        pty += 30;

        // ---------- Tension Slider ----------
        tensionSlider = new Fl_Value_Slider(665, pty, 130, 20, "Tension");
        tensionSlider->range(0.1, 1);
        tensionSlider->value(0.5);
        tensionSlider->align(FL_ALIGN_LEFT);
        tensionSlider->type(FL_HORIZONTAL);
        tensionSlider->callback((Fl_Callback*)damageCB, this);

        pty += 30;

        // ---------- Shader Browser ----------
        shaderBrowser = new Fl_Browser(605, pty, 120, 75, "Shader");
        shaderBrowser->type(2);  // select
        shaderBrowser->callback((Fl_Callback*)damageCB, this);
        shaderBrowser->add("Simple Church");
        shaderBrowser->add("Colorful Church");
        shaderBrowser->add("Height Map Wave");
        shaderBrowser->add("Sine Wave");
        shaderBrowser->add("Water Reflect");

        shaderBrowser->select(2);

        pty += 110;

        // ---------- Pixelization, Toon, Paint, Smoke ----------
        pixelizeButton = new Fl_Button(605, pty, 60, 20, "Pixelize");
        togglify(pixelizeButton, 0);

        toonButton = new Fl_Button(675, pty, 60, 20, "Toon");
        togglify(toonButton, 0);

        pty += 25;

        paintButton = new Fl_Button(605, pty, 60, 20, "Paint");
        togglify(paintButton, 0);

        crosshatchButton = new Fl_Button(675, pty, 80, 20, "Hatch");
        togglify(crosshatchButton, 0);

        pty += 25;

        smokeButton = new Fl_Button(605, pty, 60, 20, "Smoke");
        togglify(smokeButton, 0);

        pty += 30;

        // ---------- Physics Button ----------
        physicsButton = new Fl_Button(605, pty, 60, 20, "Physics");
        togglify(physicsButton, 0);

        // ---------- Minecraft Button ----------
        minecraftButton = new Fl_Button(675, pty, 60, 20, "Minecraft");
        togglify(minecraftButton, 1);

        pty += 30;
// ---------- Reflection/Refraction Ratio Slider ----------
        reflectRefractSlider =
            new Fl_Value_Slider(700, pty, 95, 20, "Reflect/Refract");
        reflectRefractSlider->range(0.0, 1.0);
        reflectRefractSlider->value(0.7);
        reflectRefractSlider->align(FL_ALIGN_LEFT);
        reflectRefractSlider->type(FL_HORIZONTAL);
        reflectRefractSlider->callback((Fl_Callback*)damageCB, this);

#ifdef EXAMPLE_SOLUTION
        makeExampleWidgets(this, pty);
#endif

        // we need to make a little phantom widget to have things resize correctly
        Fl_Box* resizebox = new Fl_Box(600, 595, 200, 5);
        widgets->resizable(resizebox);

        widgets->end();
    }
    end();  // done adding to this widget

    // set up callback on idle
    Fl::add_idle((void (*)(void*))runButtonCB, this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::togglify(Fl_Button* b, int val)
//========================================================================
{
    b->type(FL_TOGGLE_BUTTON);        // toggle
    b->value(val);                    // turned off
    b->selection_color((Fl_Color)3);  // yellow when pressed
    b->callback((Fl_Callback*)damageCB, this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::damageMe()
//========================================================================
{
    if (trainView->selectedCube >= ((int)m_Track.points.size()))
        trainView->selectedCube = 0;
    trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::advanceTrain(float dir)
//========================================================================
{
    const size_t pointCount = m_Track.points.size();

    const float sliderSpeed = speed ? static_cast<float>(speed->value()) : 1.0f;
    const float baseStep = 0.1f;
    float delta = dir * sliderSpeed * baseStep;

    const bool useArcLength = arcLength && arcLength->value();
    const bool usePhysics =
        useArcLength && physicsButton && physicsButton->value() && trainView;

    if (usePhysics) {
        const Pnt3f forward = trainView->getTrainForward();
        float directionSign = 0.0f;

        // Determine the sign of the slope
        if (delta > 0.0f)
            directionSign = 1.0f;
        else if (delta < 0.0f)
            directionSign = -1.0f;
        else
            directionSign = (dir >= 0.0f) ? 1.0f : -1.0f;

        const float signedSlope = forward.y * directionSign;
        const float gravityGain = 0.12f;
        const float damping = 0.08f;
        const float minScale = 0.1f;
        const float maxScale = 10.0f;

        // Update the physics speed scale based on the slope and gravity
        physicsSpeedScale += (-signedSlope) * gravityGain;
        if (physicsSpeedScale < minScale)
            physicsSpeedScale = minScale;
        else if (physicsSpeedScale > maxScale)
            physicsSpeedScale = maxScale;

        // Apply damping to smooth out speed changes
        physicsSpeedScale += (1.0f - physicsSpeedScale) * damping;
        delta *= physicsSpeedScale;
    } else {
        physicsSpeedScale = 1.0f;
    }

    if (useArcLength) {
        const int splineMode = trainView->tw->splineBrowser->value();
        const size_t minPoints = (splineMode == 1) ? 2 : 4;
        if (pointCount < minPoints)
            return;
    }

    m_Track.trainU += delta;
    const float span = static_cast<float>(pointCount);
    while (m_Track.trainU >= span) {
        m_Track.trainU -= span;
    }
    while (m_Track.trainU < 0.0f) {
        m_Track.trainU += span;
    }

#ifdef EXAMPLE_SOLUTION
    // note - we give a little bit more example code here than normal,
    // so you can see how this works

    if (arcLength->value()) {
        float vel = ew.physics->value() ? physicsSpeed(this)
                                        : dir * (float)speed->value();
        world.trainU += arclenVtoV(world.trainU, vel, this);
    } else {
        world.trainU += dir * ((float)speed->value() * .1f);
    }

    float nct = static_cast<float>(world.points.size());
    if (world.trainU > nct)
        world.trainU -= nct;
    if (world.trainU < 0)
        world.trainU += nct;
#endif
}