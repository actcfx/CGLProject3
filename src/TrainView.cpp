/************************************************************************
     File:        TrainView.cpp

     Author:
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu

     Comment:
                                                The TrainView is the window that
actually shows the train. Its a GL display canvas (Fl_Gl_Window).  It is held
within a TrainWindow that is the outer window with all the widgets. The
TrainView needs to be aware of the window - since it might need to check the
widgets to see how to draw

          Note:        we need to have pointers to this, but maybe not know
                                                about it (beware circular
references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <windows.h>  // we will need OpenGL, and OpenGL needs windows.h
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ControlPoint.H"
#include "GL/glu.h"
#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#include "Stuffs/totemOfUndying.hpp"

#ifdef EXAMPLE_SOLUTION
#include "TrainExample/TrainExample.H"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DIVIDE_LINE 250.0f  // reduced for performance; was 1000
#define GUAGE 5.0f

//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::TrainView(int x, int y, int w, int h, const char* l)
    : Fl_Gl_Window(x, y, w, h, l)
//========================================================================
{
    mode(FL_RGB | FL_ALPHA | FL_DOUBLE | FL_STENCIL);

    resetArcball();

    church = new Church();
    water = new Water();
    skybox = new Skybox();
    totem = new TotemOfUndying();
    terrain = new Terrain();
    mcChest = new McChest(this);
    mcMinecart = new McMinecart(this);
    mcFox = new McFox(this);
    mcVillager = new McVillager(this);
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::resetArcball()
//========================================================================
{
    // Set up the camera to look at the world
    // these parameters might seem magical, and they kindof are
    // a little trial and error goes a long way
    arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
// ########################################################################
// TODO:
//       if you want to make the train respond to other events
//       (like key presses), you might want to hack this.
// ########################################################################
//========================================================================
int TrainView::handle(int event) {
    // see if the ArcBall will handle the event - if it does,
    // then we're done
    // note: the arcball only gets the event if we're in world view
    if (tw->worldCam->value())
        if (arcball.handle(event))
            return 1;

    // remember what button was used
    static int last_push;

    switch (event) {
        // Mouse button being pushed event
        case FL_PUSH:
            last_push = Fl::event_button();
            // if the left button be pushed is left mouse button
            if (last_push == FL_LEFT_MOUSE) {
                doPick();
                damage(1);
                return 1;
            };
            break;

            // Mouse button release event
        case FL_RELEASE:  // button release
            damage(1);
            last_push = 0;
            return 1;

        // Mouse button drag event
        case FL_DRAG:

            // Compute the new control point position
            if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
                ControlPoint* cp = &m_pTrack->points[selectedCube];

                double r1x, r1y, r1z, r2x, r2y, r2z;
                getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

                double rx, ry, rz;
                mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z,
                            static_cast<double>(cp->pos.x),
                            static_cast<double>(cp->pos.y),
                            static_cast<double>(cp->pos.z), rx, ry, rz,
                            (Fl::event_state() & FL_CTRL) != 0);

                cp->pos.x = (float)rx;
                cp->pos.y = (float)ry;
                cp->pos.z = (float)rz;

                // Terrain snapping: prevent control points from going underground
                if (terrain) {
                    float terrainHeight =
                        terrain->getHeightAtWorldPos(cp->pos.x, cp->pos.z);
                    const float minClearance =
                        3.0f;  // Minimum height above terrain for control points

                    if (cp->pos.y < terrainHeight + minClearance) {
                        cp->pos.y = terrainHeight + minClearance;
                    }
                }

                damage(1);
            }
            break;

        // in order to get keyboard events, we need to accept focus
        case FL_FOCUS:
            return 1;

        // every time the mouse enters this window, aggressively take focus
        case FL_ENTER:
            focus(this);
            break;

        case FL_KEYBOARD:
            int k = Fl::event_key();
            int ks = Fl::event_state();
            if (k == 'p') {
                // Print out the selected control point information
                if (selectedCube >= 0)
                    printf("Selected(%d) (%g %g %g) (%g %g %g)\n", selectedCube,
                           m_pTrack->points[selectedCube].pos.x,
                           m_pTrack->points[selectedCube].pos.y,
                           m_pTrack->points[selectedCube].pos.z,
                           m_pTrack->points[selectedCube].orient.x,
                           m_pTrack->points[selectedCube].orient.y,
                           m_pTrack->points[selectedCube].orient.z);
                else
                    printf("Nothing Selected\n");

                return 1;
            };
            break;
    }

    return Fl_Gl_Window::handle(event);
}

void TrainView::setLighting() {
    // enable the lighting
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // ---------- Directional Light ----------
    auto setDirectionalLight = []() -> void {
        float directionalAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0 };  // No ambient
        float directionalDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0 };
        float directionalLightPosition[] = { 0.0f, 1.0f, 1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, directionalAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, directionalDiffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, directionalLightPosition);
    };

    // ---------- Point Light ----------
    auto setPointLight = [this]() -> void {
        float pointAmbient[] = { 0.05f, 0.05f, 0.02f, 1.0f };
        float pointDiffuse[] = { 1.0f, 1.0f, 0.2f, 1.0f };

        Pnt3f pointPosition(10.0f, 10.0f, 10.0f);
        Pnt3f pointOrientation(0.0f, -1.0f, 0.0f);

        if (!m_pTrack->points.empty()) {
            int selectIndex = (selectedCube >= 0 &&
                               selectedCube < (int)m_pTrack->points.size())
                                  ? selectedCube
                                  : 0;
            pointPosition = m_pTrack->points[(size_t)selectIndex].pos;
            pointOrientation = m_pTrack->points[(size_t)selectIndex].orient;
            pointOrientation.normalize();
        }

        float pointLightPosition[] = { pointPosition.x, pointPosition.y,
                                       pointPosition.z, 1.0f };

        glLightfv(GL_LIGHT1, GL_AMBIENT, pointAmbient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, pointDiffuse);
        glLightfv(GL_LIGHT1, GL_POSITION, pointLightPosition);
    };

    // ---------- Spot Light ----------
    auto setSpotLight = [this]() -> void {
        float spotAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float spotDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };

        float spotLightPosition[] = { trainPosition.x, trainPosition.y,
                                      trainPosition.z, 1.0f };
        float spotLightDirection[] = { trainForward.x, trainForward.y,
                                       trainForward.z };

        glLightfv(GL_LIGHT2, GL_AMBIENT, spotAmbient);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, spotDiffuse);
        glLightfv(GL_LIGHT2, GL_POSITION, spotLightPosition);
        glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spotLightDirection);
        glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0f);
        glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 15.0f);
    };

    if (tw->topCam->value()) {
        // top view only needs one light
        setDirectionalLight();
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHT2);
    } else {
        if (tw->directionalLightButton->value()) {
            setDirectionalLight();
            glEnable(GL_LIGHT0);
        } else {
            glDisable(GL_LIGHT0);
        }

        if (tw->pointLightButton->value()) {
            setPointLight();
            glEnable(GL_LIGHT1);
        } else {
            glDisable(GL_LIGHT1);
        }

        if (tw->spotLightButton->value()) {
            setSpotLight();
            glEnable(GL_LIGHT2);
        } else {
            glDisable(GL_LIGHT2);
        }
    }

    if (tw->smokeButton->value()) {
        GLfloat fogColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glEnable(GL_FOG);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, smokeStartDistance);
        glFogf(GL_FOG_END, smokeEndDistance);
    }else {
        glDisable(GL_FOG);
    }
}

void TrainView::setUBO() {
    float wdt = this->pixel_w();
    float hgt = this->pixel_h();

    glm::mat4 viewMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &viewMatrix[0][0]);

    glm::mat4 projectionMatrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &projectionMatrix[0][0]);

    glBindBuffer(GL_UNIFORM_BUFFER, this->commonMatrices->ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                    &projectionMatrix[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                    &viewMatrix[0][0]);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrainView::initFrameBufferShader() {
    if (!pixelShader)
        pixelShader =
            new Shader("./shaders/pixelization.vert", nullptr, nullptr, nullptr,
                       "./shaders/pixelization.frag");
    if (!toonShader)
        toonShader = new Shader("./shaders/toon.vert", nullptr, nullptr,
                                nullptr, "./shaders/toon.frag");

    // Initialize Buffer if null
    if (!mainFrameBuffer) {
        mainFrameBuffer = new FrameBuffer(this->pixel_w(), this->pixel_h());
    }
    if (!tempFrameBuffer) {
        tempFrameBuffer = new FrameBuffer(this->pixel_w(), this->pixel_h());
    }
}

void TrainView::clearGlad() {
    this->shader = nullptr;
    this->commonMatrices = nullptr;
    this->plane = nullptr;
    this->texture = nullptr;
}

void TrainView::drawPlane() {
    if (!this->shader || !this->plane)
        return;

    setUBO();
    glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/ 0,
                      this->commonMatrices->ubo, 0, this->commonMatrices->size);

    //bind shader
    this->shader->Use();

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    if (tw->shaderBrowser->value() == 5) {
        float terrainWidth = terrain->getWidth() * terrain->getScaleXZ();
        float terrainDepth = terrain->getDepth() * terrain->getScaleXZ();
        float scaleX = terrainWidth;
        float scaleZ = terrainDepth;

        // Position the water plane at the configured water height so it aligns with terrain and clipping
        modelMatrix = glm::translate(modelMatrix,
                                     glm::vec3(0.0f, water->waterHeight, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleX, 1.0f, scaleZ));
    } else {
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 10.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(40.0f, 40.0f, 40.0f));
    }

    glUniformMatrix4fv(glGetUniformLocation(this->shader->Program, "u_model"),
                       1, GL_FALSE, &modelMatrix[0][0]);
    glUniform3fv(glGetUniformLocation(this->shader->Program, "u_color"), 1,
                 &glm::vec3(0.5f, 0.0f, 0.0f)[0]);

    if (this->texture) {
        this->texture->bind(0);
    } else {
        Texture2D::unbind(0);
    }

    glUniform1i(glGetUniformLocation(this->shader->Program, "u_texture"), 0);

    // Time uniform for wave animation
    static auto start = std::chrono::steady_clock::now();
    float t =
        std::chrono::duration<float>(std::chrono::steady_clock::now() - start)
            .count();
    glUniform1f(glGetUniformLocation(this->shader->Program, "u_time"), t);

    // Wave uniforms
    if (tw->shaderBrowser->value() == 3 || tw->shaderBrowser->value() == 4 ||
        tw->shaderBrowser->value() == 5) {
        int waveCount = (int)water->waveDirections.size();
        glUniform1i(glGetUniformLocation(this->shader->Program, "u_waveCount"),
                    waveCount);
        if (waveCount > 0) {
            glUniform2fv(
                glGetUniformLocation(this->shader->Program, "u_direction"),
                waveCount, &water->waveDirections[0][0]);
            glUniform1fv(
                glGetUniformLocation(this->shader->Program, "u_wavelength"),
                waveCount, &water->waveWavelengths[0]);
            glUniform1fv(
                glGetUniformLocation(this->shader->Program, "u_amplitude"),
                waveCount, &water->waveAmplitudes[0]);
            glUniform1fv(glGetUniformLocation(this->shader->Program, "u_speed"),
                         waveCount, &water->waveSpeeds[0]);
        }
        glUniform2fv(glGetUniformLocation(this->shader->Program, "u_scroll"), 1,
                     &water->heightMapScroll[0]);
        glUniform1f(
            glGetUniformLocation(this->shader->Program, "u_heightScale"),
            water->heightMapScale);

        // Reflection & Refraction textures for water shader
        if (water->reflectionTexture != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, water->reflectionTexture);
            glUniform1i(
                glGetUniformLocation(this->shader->Program, "u_reflectionTex"),
                1);
        }
        if (water->refractionTexture != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, water->refractionTexture);
            glUniform1i(
                glGetUniformLocation(this->shader->Program, "u_refractionTex"),
                2);
        }
        if (water->refractionDepthTexture != 0) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, water->refractionDepthTexture);
            glUniform1i(
                glGetUniformLocation(this->shader->Program, "u_depthTex"), 3);
        }

        glUniform1f(
            glGetUniformLocation(this->shader->Program, "u_waterHeight"),
            water->waterHeight);

        if (tw->shaderBrowser->value() == 5) {
            glUniform1f(glGetUniformLocation(this->shader->Program,
                                             "u_distortionStrength"),
                        0.0012f);
            glUniform1f(
                glGetUniformLocation(this->shader->Program, "u_normalStrength"),
                0.015f);
            glUniform3f(
                glGetUniformLocation(this->shader->Program, "u_waterColor"),
                0.02f, 0.32f, 0.52f);
            glUniform1f(glGetUniformLocation(this->shader->Program,
                                             "u_reflectRefractRatio"),
                        (float)tw->reflectRefractSlider->value());
        }
    }

    // Camera position and skybox for shader
    glm::mat4 viewMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &viewMatrix[0][0]);
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    glUniform3fv(glGetUniformLocation(this->shader->Program, "u_cameraPos"), 1,
                 &cameraPos[0]);

    GLint smokeLoc =
        glGetUniformLocation(this->shader->Program, "u_smokeParams");
    if (smokeLoc >= 0) {
        glUniform2f(smokeLoc, smokeStartDistance, smokeEndDistance);
    }

    GLint smokeEnabledLoc =
        glGetUniformLocation(this->shader->Program, "smokeEnabled");
    if (smokeEnabledLoc >= 0 && tw && tw->smokeButton) {
        glUniform1i(smokeEnabledLoc, tw->smokeButton->value() ? 1 : 0);
    }

    // Bind skybox for reflection
    if (skybox && skybox->getTexture() != 0) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->getTexture());
        GLint skyboxLoc =
            glGetUniformLocation(this->shader->Program, "u_skybox");
        if (skyboxLoc >= 0) {
            glUniform1i(skyboxLoc, 5);
        }
    }

    //bind VAO
    glBindVertexArray(this->plane->vao);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT,
                   0);

    glDisable(GL_BLEND);

    //unbind VAO
    glBindVertexArray(0);

    //unbind shader(switch to fixed pipeline)
    glUseProgram(0);
}

void TrainView::draw() {
    // ---------- Initialize GLAD ----------
    if (!glInited) {
        if (!gladLoadGL()) {
            throw std::runtime_error("Could not initialize GLAD!");
        }
        // Initialize Skybox once
        skybox->init();
        totem->init(this);
        terrain->init(tw);
        glInited = true;
    }

    // Check for Minecraft mode toggle
    static int lastMinecraftState = -1;
    int currentMinecraftState = tw->minecraftButton->value();
    if (currentMinecraftState != lastMinecraftState) {
        terrain->rebuild();
        lastMinecraftState = currentMinecraftState;
    }

    clearGlad();

    // Reinitialize content shaders depending on selection
    static int lastShader = -1;
    int shaderType = tw->shaderBrowser->value();

    if (shaderType != lastShader) {
        if (shaderType == 1) {
            church->initSimple();
        } else if (shaderType == 2) {
            church->initColorful();
        } else if (shaderType == 3) {
            water->initHeightMapWave();
        } else if (shaderType == 4) {
            water->initSineWave();
        } else if (shaderType == 5) {
            water->initReflectionWater(this);
        }
        lastShader = shaderType;
    }

    if (shaderType == 1) {
        this->shader = church->shader;
        this->plane = church->plane;
        this->texture = church->texture;
        this->commonMatrices = church->commonMatrices;
    } else if (shaderType == 2) {
        this->shader = church->shader;
        this->plane = church->plane;
        this->texture = church->texture;
        this->commonMatrices = church->commonMatrices;
    } else if (shaderType == 3) {
        this->shader = water->shader;
        this->plane = water->plane;
        this->texture = water->texture;
        this->commonMatrices = water->commonMatrices;
    } else if (shaderType == 4) {
        this->shader = water->shader;
        this->plane = water->plane;
        this->texture = water->texture;
        this->commonMatrices = water->commonMatrices;
    } else if (shaderType == 5) {
        this->shader = water->shader;
        this->plane = water->plane;
        this->texture = water->texture;
        this->commonMatrices = water->commonMatrices;
    } else {
        clearGlad();
    }

    // ---------- Initialize Framebuffer Shaders ----------
    initFrameBufferShader();
    mainFrameBuffer->resize(this->pixel_w(), this->pixel_h());
    tempFrameBuffer->resize(this->pixel_w(), this->pixel_h());

    // Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w(), h());

    // clear the window, be sure to clear the Z-Buffer too (single clear)
    glClearColor(0, 0, .3f, 0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // ---------- Bind framebuffer ----------
    if (tw->pixelizeButton->value() || tw->toonButton->value()) {
        mainFrameBuffer->bind();
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w(), h());
    }

    glClearColor(0, 0, .3f, 0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // ---------- Set up Projection and Lighting ----------
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    setProjection();

    setLighting();

    // ---------- Draw the skybox ----------
    glm::mat4 view_matrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
    glm::mat4 projection_matrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);

    skybox->draw(view_matrix, projection_matrix);

    // ---------- Draw the objects and shadows ----------
    glUseProgram(0);
    glEnable(GL_LIGHTING);
    setupObjects();

    drawStuff();

    // this time drawing is for shadows (except for top view)
    if (!tw->topCam->value()) {
        setupShadows();
        drawStuff(true);
        unsetupShadows();
    }

    // ---------- Render reflection & refraction for water ----------
    if (shaderType == 5) {
        // Store original viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        water->renderReflection(this);
        water->renderRefraction(this);

        // Restore viewport and matrices
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        setProjection();
        setLighting();
    }

    // ---------- Draw the totem billboard (with depth, before post-processing) ----------
    glm::mat4 totemViewMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &totemViewMatrix[0][0]);
    glm::mat4 totemProjectionMatrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &totemProjectionMatrix[0][0]);

    glm::mat4 invView = glm::inverse(totemViewMatrix);
    glm::vec3 totemCameraPos = glm::vec3(invView[3]);

    totem->draw(totemViewMatrix, totemProjectionMatrix, totemCameraPos,
                smokeStartDistance, smokeEndDistance);

    // ---------- Draw the terrain ----------
    terrain->draw(totemViewMatrix, totemProjectionMatrix, totemCameraPos);

    // ---------- Draw the plane ----------
    drawPlane();

    // ---------- Draw Minecraft Models ----------
    if (mcChest)
        mcChest->draw(glm::vec3(0, -10, 0));
    if (mcFox)
        mcFox->draw(glm::vec3(-20, -10, -20));
    
    // ---------- Post processing ----------
    glDisable(GL_DEPTH_TEST);
    if (tw->toonButton->value() && tw->pixelizeButton->value()) {
        // Step A: Apply Toon Shader (Main -> Temp)
        tempFrameBuffer->bind();  // Draw into temporary buffer
        glClear(GL_COLOR_BUFFER_BIT);

        toonShader->Use();
        glUniform1f(glGetUniformLocation(toonShader->Program, "width"),
                    (float)w());
        glUniform1f(glGetUniformLocation(toonShader->Program, "height"),
                    (float)h());
        glUniform1i(glGetUniformLocation(toonShader->Program, "screenTexture"),
                    0);

        mainFrameBuffer->bindTexture(0);  // Read from Main
        mainFrameBuffer->drawQuad();

        // Step B: Apply Pixel Shader (Temp -> Screen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Back to screen
        glViewport(0, 0, w(), h());
        glClear(GL_COLOR_BUFFER_BIT);

        pixelShader->Use();
        glUniform1f(glGetUniformLocation(pixelShader->Program, "pixelSize"),
                    5.0f);
        glUniform1f(glGetUniformLocation(pixelShader->Program, "screenWidth"),
                    (float)w());
        glUniform1f(glGetUniformLocation(pixelShader->Program, "screenHeight"),
                    (float)h());
        glUniform1i(glGetUniformLocation(pixelShader->Program, "screenTexture"),
                    0);

        tempFrameBuffer->bindTexture(
            0);  // Read from Temp (which has the Toon result)
        tempFrameBuffer->drawQuad();
    } else if (tw->pixelizeButton->value()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w(), h());
        glClear(GL_COLOR_BUFFER_BIT);

        pixelShader->Use();
        glUniform1f(glGetUniformLocation(pixelShader->Program, "pixelSize"),
                    5.0f);
        glUniform1f(glGetUniformLocation(pixelShader->Program, "screenWidth"),
                    (float)w());
        glUniform1f(glGetUniformLocation(pixelShader->Program, "screenHeight"),
                    (float)h());
        glUniform1i(glGetUniformLocation(pixelShader->Program, "screenTexture"),
                    0);

        mainFrameBuffer->bindTexture(0);
        mainFrameBuffer->drawQuad();
    } else if (tw->toonButton->value()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w(), h());
        glClear(GL_COLOR_BUFFER_BIT);

        toonShader->Use();
        glUniform1f(glGetUniformLocation(toonShader->Program, "width"),
                    (float)w());
        glUniform1f(glGetUniformLocation(toonShader->Program, "height"),
                    (float)h());
        glUniform1i(glGetUniformLocation(toonShader->Program, "screenTexture"),
                    0);

        mainFrameBuffer->bindTexture(0);
        mainFrameBuffer->drawQuad();
    }
    

    // Unbind
    glBindTexture(GL_TEXTURE_2D, 0);
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::setProjection()
//========================================================================
{
    // Compute the aspect ratio (we'll need it)
    float aspect = static_cast<float>(w()) / static_cast<float>(h());

    if (tw->worldCam->value()) {
        // ---------- World Cam ----------
        arcball.setProjection(false);
    } else if (tw->topCam->value()) {
        // ---------- Top Cam ----------
        float wi, he;
        if (terrain) {
            // Compute half extents of the terrain in world coordinates
            float terrainWidth = terrain->getWidth() * terrain->getScaleXZ();
            float terrainDepth = terrain->getDepth() * terrain->getScaleXZ();
            float halfW = terrainWidth * 0.5f;
            float halfH = terrainDepth * 0.5f;

            // Choose wi/he to ensure both width and depth are fully visible
            // mapping of wi/he: ortho spans [-wi, wi], [-he, he]
            wi = std::max(halfW, halfH * aspect);
            he = std::max(halfH, halfW / aspect);
        } else {
            // Fallback (previous behavior)
            if (aspect >= 1) {
                wi = 110;
                he = wi / aspect;
            } else {
                he = 110;
                wi = he * aspect;
            }
        }

        // Set up the top camera drop mode to be orthogonal and set
        // up proper projection matrix
        glMatrixMode(GL_PROJECTION);
        glOrtho(-wi, wi, -he, he, 200, -200);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(-90, 1, 0, 0);
    } else {
        // ---------- Train Cam ----------
        // Position camera behind and above the train using train's local frame
        const Pnt3f eyeOffset = trainForward * (-10.0f) + trainUp * 4.0f;
        const Pnt3f eye = trainPosition + eyeOffset;
        const Pnt3f center = trainPosition + trainForward * 40.0f;

        glMatrixMode(GL_PROJECTION);
        gluPerspective(60.0, aspect, 0.1, 5000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, trainUp.x,
                  trainUp.y, trainUp.z);
    }

#ifdef EXAMPLE_SOLUTION
    trainCamView(this, aspect);
#endif
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get
//       colored shadows). this gets called twice per draw
//       -- once for the objects, once for the shadows
//========================================================================
void TrainView::drawStuff(bool doingShadows) {
    // Draw the control points
    // don't draw the control points if you're driving
    // (otherwise you get sea-sick as you drive through them)
    if (!tw->trainCam->value()) {
        for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
            if (!doingShadows) {
                if (((int)i) != selectedCube)
                    glColor3ub(240, 60, 60);
                else
                    glColor3ub(240, 240, 30);
            }
            m_pTrack->points[i].draw();
        }
    }

    // draw the track
    drawTrack(doingShadows);

#ifdef EXAMPLE_SOLUTION
    drawTrack(this, doingShadows);
#endif

    // draw the train
    drawTrain(doingShadows);

    // draw the oden
    drawOden(doingShadows);

#ifdef EXAMPLE_SOLUTION
    // don't draw the train if you're looking out the front window
    if (!tw->trainCam->value())
        drawTrain(this, doingShadows);
#endif
}

float TrainView::currentTension(float fallback) const {
    if (tw && tw->tensionSlider)
        return static_cast<float>(tw->tensionSlider->value());
    return fallback;
}

void TrainView::buildBasisMatrix(int mode, float out[4][4]) const {
    float linearM[4][4] = { { 0.0f, 0.0f, 0.0f, 0.0f },
                            { 0.0f, 0.0f, -1.0f, 1.0f },
                            { 0.0f, 0.0f, 1.0f, 0.0f },
                            { 0.0f, 0.0f, 0.0f, 0.0f } };
    // keep classic cardinal values for reference but compute dynamically below if needed
    float baseCardinalM[4][4] = { { -0.5f, 1.0f, -0.5f, 0.0f },
                                  { 1.5f, -2.5f, 0.0f, 1.0f },
                                  { -1.5f, 2.0f, 0.5f, 0.0f },
                                  { 0.5f, -0.5f, 0.0f, 0.0f } };

    if (mode == 1) {
        memcpy(out, linearM, sizeof(linearM));
    } else if (mode == 2) {
        float tension = currentTension(0.5f);
        float cardinalM[4][4] = {
            { -tension, 2.0f * tension, -tension, 0.0f },
            { 2.0f - tension, tension - 3.0f, 0.0f, 1.0f },
            { tension - 2.0f, 3.0f - 2.0f * tension, tension, 0.0f },
            { tension, -tension, 0.0f, 0.0f }
        };
        memcpy(out, cardinalM, sizeof(cardinalM));
    } else {
        const float tension = 1.0f / 6.0f;
        float bSplineM[4][4] = {
            { -tension, 3.0f * tension, -3.0f * tension, tension },
            { 3.0f * tension, -6.0f * tension, 0.0f, 4.0f * tension },
            { -3.0f * tension, 3.0f * tension, 3.0f * tension, tension },
            { tension, 0.0f, 0.0f, 0.0f }
        };
        memcpy(out, bSplineM, sizeof(bSplineM));
    }
}

void TrainView::drawTrack(bool doingShadows) {
    const size_t pointCount = m_pTrack->points.size();
    if (pointCount < 2)
        return;

    float M[4][4] = { 0 };
    buildBasisMatrix(tw->splineBrowser->value(), M);

    if (!doingShadows) {
        //  Track color
        glColor3ub(200, 200, 200);
    }

    glLineWidth(8.0f);  // Track line width

    // Sampled geometry and frames along the spline
    std::vector<Pnt3f> trackCenters;
    std::vector<Pnt3f> trackTangents;
    std::vector<Pnt3f> trackOrients;
    std::vector<Pnt3f> rightVectors;
    std::vector<Pnt3f> upVectors;

    for (size_t cpIndex = 0; cpIndex < pointCount; ++cpIndex) {
        size_t idxPrev = (cpIndex + pointCount - 1) % pointCount;
        size_t idxCurr = cpIndex % pointCount;
        size_t idxNext = (cpIndex + 1) % pointCount;
        size_t idxNext2 = (cpIndex + 2) % pointCount;

        const Pnt3f& posPrev = m_pTrack->points[idxPrev].pos;
        const Pnt3f& posCurr = m_pTrack->points[idxCurr].pos;
        const Pnt3f& posNext = m_pTrack->points[idxNext].pos;
        const Pnt3f& posNext2 = m_pTrack->points[idxNext2].pos;

        const Pnt3f& orientPrev = m_pTrack->points[idxPrev].orient;
        const Pnt3f& orientCurr = m_pTrack->points[idxCurr].orient;
        const Pnt3f& orientNext = m_pTrack->points[idxNext].orient;
        const Pnt3f& orientNext2 = m_pTrack->points[idxNext2].orient;

        for (int k = 0; k <= (int)DIVIDE_LINE; ++k) {
            float t = static_cast<float>(k) / DIVIDE_LINE;
            float T[4] = { t * t * t, t * t, t, 1.0f };

            // Compute weights
            float weights[4];
            for (int r = 0; r < 4; ++r) {
                weights[r] = M[r][0] * T[0] + M[r][1] * T[1] + M[r][2] * T[2] +
                             M[r][3] * T[3];
            }

            // Compute position
            Pnt3f position = posPrev * weights[0] + posCurr * weights[1] +
                             posNext * weights[2] + posNext2 * weights[3];
            trackCenters.push_back(position);

            // Compute tangent (derivative of weights)
            float dT[4] = { 3.0f * t * t, 2.0f * t, 1.0f, 0.0f };
            float dWeights[4];
            for (int r = 0; r < 4; ++r) {
                dWeights[r] = M[r][0] * dT[0] + M[r][1] * dT[1] +
                              M[r][2] * dT[2] + M[r][3] * dT[3];
            }
            Pnt3f tangent = posPrev * dWeights[0] + posCurr * dWeights[1] +
                            posNext * dWeights[2] + posNext2 * dWeights[3];
            tangent.normalize();
            trackTangents.push_back(tangent);

            // Compute orientation
            Pnt3f orientation =
                orientPrev * weights[0] + orientCurr * weights[1] +
                orientNext * weights[2] + orientNext2 * weights[3];
            orientation.normalize();
            trackOrients.push_back(orientation);
        }
    }

    // ---------- Arc Length ----------
    // Build frames from tangent and orientation, ensuring continuity
    rightVectors.resize(trackCenters.size());
    upVectors.resize(trackCenters.size());

    if (!trackCenters.empty()) {
        // Initialize first frame
        Pnt3f tangent = trackTangents[0];
        Pnt3f orient = trackOrients[0];

        // Project orient perpendicular to tangent
        float dot =
            tangent.x * orient.x + tangent.y * orient.y + tangent.z * orient.z;
        Pnt3f up = Pnt3f(orient.x - dot * tangent.x, orient.y - dot * tangent.y,
                         orient.z - dot * tangent.z);
        float upLen = std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
        if (upLen < 1e-6f) {
            // Fallback if orient parallel to tangent
            Pnt3f fallback =
                (fabs(tangent.y) < 0.9f) ? Pnt3f(0, 1, 0) : Pnt3f(1, 0, 0);
            dot = tangent.x * fallback.x + tangent.y * fallback.y +
                  tangent.z * fallback.z;
            up = Pnt3f(fallback.x - dot * tangent.x,
                       fallback.y - dot * tangent.y,
                       fallback.z - dot * tangent.z);
            upLen = std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
        }
        up = Pnt3f(up.x / upLen, up.y / upLen, up.z / upLen);

        Pnt3f right = tangent * up;
        right.normalize();
        up = right * tangent;  // Ensure orthogonality
        up.normalize();

        upVectors[0] = up;
        rightVectors[0] = right;

        // Propagate frames, ensuring no sudden flips
        for (size_t i = 1; i < trackCenters.size(); ++i) {
            tangent = trackTangents[i];
            orient = trackOrients[i];

            // Project orient perpendicular to tangent
            dot = tangent.x * orient.x + tangent.y * orient.y +
                  tangent.z * orient.z;
            up = Pnt3f(orient.x - dot * tangent.x, orient.y - dot * tangent.y,
                       orient.z - dot * tangent.z);
            upLen = std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
            if (upLen < 1e-6f) {
                up = upVectors[i - 1];
            } else {
                up = Pnt3f(up.x / upLen, up.y / upLen, up.z / upLen);
            }

            // Check if up flipped relative to previous frame (dot product < 0)
            float continuityDot = up.x * upVectors[i - 1].x +
                                  up.y * upVectors[i - 1].y +
                                  up.z * upVectors[i - 1].z;
            if (continuityDot < 0.0f) {
                // Flip to maintain continuity
                up = Pnt3f(-up.x, -up.y, -up.z);
            }

            right = tangent * up;
            right.normalize();
            up = right * tangent;
            up.normalize();

            upVectors[i] = up;
            rightVectors[i] = right;
        }
    }

    if (tw->arcLength && tw->arcLength->value()) {
        const size_t sampleCount = trackCenters.size();
        if (sampleCount >= 2) {
            std::vector<float> cumLen(sampleCount, 0.0f);
            for (size_t i = 1; i < sampleCount; ++i) {
                Pnt3f delta = trackCenters[i] - trackCenters[i - 1];
                float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y +
                                       delta.z * delta.z);
                cumLen[i] = cumLen[i - 1] + dist;
            }
            float totalLen = cumLen.back();
            if (totalLen > 1e-6f) {
                std::vector<Pnt3f> resampledCenters(sampleCount);
                std::vector<Pnt3f> resampledUps(sampleCount);
                const float invSamples =
                    1.0f / static_cast<float>(sampleCount - 1);
                size_t baseIdx = 0;
                for (size_t sample = 0; sample < sampleCount; ++sample) {
                    float target = totalLen * sample * invSamples;
                    while (baseIdx + 1 < sampleCount &&
                           cumLen[baseIdx + 1] < target) {
                        ++baseIdx;
                    }
                    if (baseIdx >= sampleCount - 1)
                        baseIdx = sampleCount - 2;
                    size_t nextIdx = baseIdx + 1;
                    float segmentLen = cumLen[nextIdx] - cumLen[baseIdx];
                    float alpha = 0.0f;
                    if (segmentLen > 1e-6f) {
                        alpha = (target - cumLen[baseIdx]) / segmentLen;
                        if (alpha < 0.0f)
                            alpha = 0.0f;
                        else if (alpha > 1.0f)
                            alpha = 1.0f;
                    }
                    auto lerpPoint = [](const Pnt3f& a, const Pnt3f& b,
                                        float t) {
                        return Pnt3f(a.x + (b.x - a.x) * t,
                                     a.y + (b.y - a.y) * t,
                                     a.z + (b.z - a.z) * t);
                    };
                    resampledCenters[sample] = lerpPoint(
                        trackCenters[baseIdx], trackCenters[nextIdx], alpha);
                    resampledUps[sample] = lerpPoint(upVectors[baseIdx],
                                                     upVectors[nextIdx], alpha);
                }

                std::vector<Pnt3f> resampledTangents(sampleCount);
                std::vector<Pnt3f> resampledRights(sampleCount);
                Pnt3f prevUp(0.0f, 1.0f, 0.0f);
                Pnt3f prevRight(1.0f, 0.0f, 0.0f);
                for (size_t sample = 0; sample < sampleCount; ++sample) {
                    Pnt3f curr = resampledCenters[sample];
                    Pnt3f next = resampledCenters[(sample + 1) % sampleCount];
                    Pnt3f tangent = next - curr;
                    float tanLen = std::sqrt(tangent.x * tangent.x +
                                             tangent.y * tangent.y +
                                             tangent.z * tangent.z);
                    if (tanLen < 1e-6f) {
                        tangent = trackTangents[sample];
                    } else {
                        tangent = Pnt3f(tangent.x / tanLen, tangent.y / tanLen,
                                        tangent.z / tanLen);
                    }

                    Pnt3f up = resampledUps[sample];
                    float dotTU =
                        tangent.x * up.x + tangent.y * up.y + tangent.z * up.z;
                    up = Pnt3f(up.x - dotTU * tangent.x,
                               up.y - dotTU * tangent.y,
                               up.z - dotTU * tangent.z);
                    float upLen =
                        std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
                    if (upLen < 1e-6f)
                        up = prevUp;
                    else
                        up = Pnt3f(up.x / upLen, up.y / upLen, up.z / upLen);

                    float continuityDot =
                        up.x * prevUp.x + up.y * prevUp.y + up.z * prevUp.z;
                    if (continuityDot < 0.0f)
                        up = Pnt3f(-up.x, -up.y, -up.z);

                    Pnt3f right = tangent * up;
                    float rightLen =
                        std::sqrt(right.x * right.x + right.y * right.y +
                                  right.z * right.z);
                    if (rightLen < 1e-6f)
                        right = prevRight;
                    else
                        right = Pnt3f(right.x / rightLen, right.y / rightLen,
                                      right.z / rightLen);

                    up = right * tangent;
                    float finalUpLen =
                        std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
                    if (finalUpLen < 1e-6f)
                        up = prevUp;
                    else
                        up = Pnt3f(up.x / finalUpLen, up.y / finalUpLen,
                                   up.z / finalUpLen);

                    resampledTangents[sample] = tangent;
                    resampledRights[sample] = right;
                    resampledUps[sample] = up;
                    prevUp = up;
                    prevRight = right;
                }

                trackCenters = std::move(resampledCenters);
                trackTangents = std::move(resampledTangents);
                rightVectors = std::move(resampledRights);
                upVectors = std::move(resampledUps);
            }
        }
    }

    // Draw two parallel rails
    const float railOffset = GUAGE / 2.0f;  // Half gauge distance
    const float tieThickness = 0.8f;  // Same as tie thickness for alignment
    const float railHeight =
        tieThickness * 0.5f;  // Raise rails to sit on top of ties

    for (int rail = 0; rail < 2; ++rail) {
        float side = (rail == 0) ? -1.0f : 1.0f;

        if (!doingShadows) {
            glColor3ub(180, 180, 180);
        }

        for (size_t s = 0; s < pointCount; ++s) {
            glBegin(GL_LINE_STRIP);
            for (int k = 0; k <= (int)DIVIDE_LINE; ++k) {
                size_t idx = s * ((int)DIVIDE_LINE + 1) + k;

                // Use precomputed continuous frame
                const Pnt3f& right = rightVectors[idx];
                const Pnt3f& up = upVectors[idx];
                Pnt3f railPos = trackCenters[idx] +
                                right * (side * railOffset) + up * railHeight;
                glVertex3f(railPos.x, railPos.y, railPos.z);
            }
            glEnd();
        }
    }

    // Draw cross ties
    const int tieInterval = 40;
    if (!doingShadows) {
        glColor3ub(139, 90, 43);
    }

    for (size_t tieIndex = 0; tieIndex < trackCenters.size();
         tieIndex += tieInterval) {
        const Pnt3f& right = rightVectors[tieIndex];
        const Pnt3f& up = upVectors[tieIndex];

        // Extend the tie slightly beyond the rails
        Pnt3f leftEnd = trackCenters[tieIndex] + right * (-railOffset * 1.3f);
        Pnt3f rightEnd = trackCenters[tieIndex] + right * (railOffset * 1.3f);

        const float tieWidth = 1.5f;
        const float tieThickness = 0.8f;

        Pnt3f tangent = trackTangents[tieIndex];
        Pnt3f halfWidth =
            tangent *
            (tieWidth *
             0.5f);  // cross gives perpendicular; want along tangent -> implement scalar multiply instead
        // We need scalar multiply, but operator * is cross; so rebuild halfWidth explicitly:
        halfWidth =
            Pnt3f(tangent.x * (tieWidth * 0.5f), tangent.y * (tieWidth * 0.5f),
                  tangent.z * (tieWidth * 0.5f));
        Pnt3f thickness = up * tieThickness;  // lift using up

        // Calculate the 8 vertices of the tie (box)
        Pnt3f v1 = leftEnd - halfWidth - thickness;
        Pnt3f v2 = leftEnd + halfWidth - thickness;
        Pnt3f v3 = leftEnd + halfWidth;
        Pnt3f v4 = leftEnd - halfWidth;

        Pnt3f v5 = rightEnd - halfWidth - thickness;
        Pnt3f v6 = rightEnd + halfWidth - thickness;
        Pnt3f v7 = rightEnd + halfWidth;
        Pnt3f v8 = rightEnd - halfWidth;

        glBegin(GL_QUADS);
        // Top face
        glNormal3f(up.x, up.y, up.z);
        glVertex3f(v4.x, v4.y, v4.z);
        glVertex3f(v3.x, v3.y, v3.z);
        glVertex3f(v7.x, v7.y, v7.z);
        glVertex3f(v8.x, v8.y, v8.z);

        // Bottom face
        glNormal3f(-up.x, -up.y, -up.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v5.x, v5.y, v5.z);
        glVertex3f(v6.x, v6.y, v6.z);
        glVertex3f(v2.x, v2.y, v2.z);

        // Front face
        glNormal3f(tangent.x, tangent.y, tangent.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v6.x, v6.y, v6.z);
        glVertex3f(v7.x, v7.y, v7.z);
        glVertex3f(v3.x, v3.y, v3.z);

        // Back face
        glNormal3f(-tangent.x, -tangent.y, -tangent.z);
        glVertex3f(v5.x, v5.y, v5.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v4.x, v4.y, v4.z);
        glVertex3f(v8.x, v8.y, v8.z);

        // Left face
        glNormal3f(-right.x, -right.y, -right.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
        glVertex3f(v4.x, v4.y, v4.z);

        // Right face
        glNormal3f(right.x, right.y, right.z);
        glVertex3f(v6.x, v6.y, v6.z);
        glVertex3f(v5.x, v5.y, v5.z);
        glVertex3f(v8.x, v8.y, v8.z);
        glVertex3f(v7.x, v7.y, v7.z);
        glEnd();
    }

    // ---------- Draw Trestles (Support Pillars) ----------
    // Check each track point and draw pillars if track is above terrain
    const float minGapForTrestle = 5.0f;  // Minimum gap to trigger pillar
    const int trestleInterval = 40;       // Draw pillars every N samples

    if (!doingShadows && terrain) {
        glColor3ub(101, 67, 33);  // Dark brown for pillars

        for (size_t idx = 0; idx < trackCenters.size();
             idx += trestleInterval) {
            Pnt3f trackPos = trackCenters[idx];
            float terrainHeight =
                terrain->getHeightAtWorldPos(trackPos.x, trackPos.z);
            float gap = trackPos.y - terrainHeight;

            if (gap > minGapForTrestle) {
                // Draw a pillar from terrain to track
                const Pnt3f& right = rightVectors[idx];
                const Pnt3f& up = upVectors[idx];

                // Pillar dimensions
                const float pillarWidth = 1.2f;
                const float pillarDepth = 1.2f;

                // Create pillar at track center
                Pnt3f topCenter = trackPos - up * 1.0f;  // Slightly below track
                Pnt3f bottomCenter =
                    Pnt3f(trackPos.x, terrainHeight, trackPos.z);

                // Use tangent for depth direction
                Pnt3f tangent = trackTangents[idx];
                Pnt3f widthVec = right * (pillarWidth * 0.5f);
                Pnt3f depthVec = Pnt3f(tangent.x * (pillarDepth * 0.5f),
                                       tangent.y * (pillarDepth * 0.5f),
                                       tangent.z * (pillarDepth * 0.5f));

                // Four corners at top
                Pnt3f t1 = topCenter - widthVec - depthVec;
                Pnt3f t2 = topCenter + widthVec - depthVec;
                Pnt3f t3 = topCenter + widthVec + depthVec;
                Pnt3f t4 = topCenter - widthVec + depthVec;

                // Four corners at bottom
                Pnt3f b1 = bottomCenter - widthVec - depthVec;
                Pnt3f b2 = bottomCenter + widthVec - depthVec;
                Pnt3f b3 = bottomCenter + widthVec + depthVec;
                Pnt3f b4 = bottomCenter - widthVec + depthVec;

                glBegin(GL_QUADS);

                // Front face
                glNormal3f(-tangent.x, -tangent.y, -tangent.z);
                glVertex3f(b1.x, b1.y, b1.z);
                glVertex3f(b2.x, b2.y, b2.z);
                glVertex3f(t2.x, t2.y, t2.z);
                glVertex3f(t1.x, t1.y, t1.z);

                // Back face
                glNormal3f(tangent.x, tangent.y, tangent.z);
                glVertex3f(b4.x, b4.y, b4.z);
                glVertex3f(t4.x, t4.y, t4.z);
                glVertex3f(t3.x, t3.y, t3.z);
                glVertex3f(b3.x, b3.y, b3.z);

                // Left face
                glNormal3f(-right.x, -right.y, -right.z);
                glVertex3f(b4.x, b4.y, b4.z);
                glVertex3f(b1.x, b1.y, b1.z);
                glVertex3f(t1.x, t1.y, t1.z);
                glVertex3f(t4.x, t4.y, t4.z);

                // Right face
                glNormal3f(right.x, right.y, right.z);
                glVertex3f(b2.x, b2.y, b2.z);
                glVertex3f(b3.x, b3.y, b3.z);
                glVertex3f(t3.x, t3.y, t3.z);
                glVertex3f(t2.x, t2.y, t2.z);

                // Top (cap)
                Pnt3f upNorm = Pnt3f(0, 1, 0);
                glNormal3f(upNorm.x, upNorm.y, upNorm.z);
                glVertex3f(t1.x, t1.y, t1.z);
                glVertex3f(t2.x, t2.y, t2.z);
                glVertex3f(t3.x, t3.y, t3.z);
                glVertex3f(t4.x, t4.y, t4.z);

                // Bottom (cap)
                glNormal3f(-upNorm.x, -upNorm.y, -upNorm.z);
                glVertex3f(b1.x, b1.y, b1.z);
                glVertex3f(b4.x, b4.y, b4.z);
                glVertex3f(b3.x, b3.y, b3.z);
                glVertex3f(b2.x, b2.y, b2.z);

                glEnd();
            }
        }
    }

    glLineWidth(1.0f);
}

void TrainView::drawTrain(bool doingShadows) {
    const size_t pointCount = m_pTrack->points.size();
    const int splineMode = tw->splineBrowser->value();
    const size_t minPoints = (splineMode == 1) ? 2 : 4;
    if (pointCount < minPoints) {
        return;
    }

    const bool useMinecraftTrain =
        tw->minecraftButton && tw->minecraftButton->value();

    float rawParam = m_pTrack->trainU;
    if (rawParam < 0.0f) {
        rawParam = std::fmod(rawParam, static_cast<float>(pointCount)) +
                   static_cast<float>(pointCount);
    }
    float wrappedParam = std::fmod(rawParam, static_cast<float>(pointCount));
    if (wrappedParam < 0.0f) {
        wrappedParam += static_cast<float>(pointCount);
    }

    const auto wrapIndex = [](int index, size_t count) -> size_t {
        if (count == 0)
            return 0;
        int wrapped = index % static_cast<int>(count);
        if (wrapped < 0)
            wrapped += static_cast<int>(count);
        return static_cast<size_t>(wrapped);
    };

    const auto buildArcLengthTable = [&](int mode) {
        ArcLengthTable data;
        if ((mode == 1 && pointCount < 2) || (mode != 1 && pointCount < 4)) {
            return data;
        }

        data.segmentCount = pointCount;
        data.segOffset.assign(pointCount + 1, 0.0f);
        data.cumLen.assign(
            pointCount,
            std::vector<float>(TrainView::ARCLEN_SAMPLES + 1, 0.0f));
        data.tSamples.assign(
            pointCount,
            std::vector<float>(TrainView::ARCLEN_SAMPLES + 1, 0.0f));

        const auto evalPosition = [&](size_t si, float t) {
            if (pointCount == 0)
                return Pnt3f(0, 0, 0);

            if (mode == 1) {
                const Pnt3f& p0 = m_pTrack->points[si % pointCount].pos;
                const Pnt3f& p1 = m_pTrack->points[(si + 1) % pointCount].pos;
                return (1.0f - t) * p0 + t * p1;
            }

            size_t prevIdx = wrapIndex(static_cast<int>(si) - 1, pointCount);
            size_t nextIdx = (si + 1) % pointCount;
            size_t next2Idx = (si + 2) % pointCount;

            const Pnt3f& p0 = m_pTrack->points[prevIdx].pos;
            const Pnt3f& p1 = m_pTrack->points[si].pos;
            const Pnt3f& p2 = m_pTrack->points[nextIdx].pos;
            const Pnt3f& p3 = m_pTrack->points[next2Idx].pos;

            float basis[4][4] = { 0 };
            buildBasisMatrix(mode, basis);

            const float t2 = t * t;
            const float t3 = t2 * t;
            float Tvec[4] = { t3, t2, t, 1.0f };
            float weights[4];
            for (int r = 0; r < 4; ++r) {
                weights[r] = basis[r][0] * Tvec[0] + basis[r][1] * Tvec[1] +
                             basis[r][2] * Tvec[2] + basis[r][3] * Tvec[3];
            }

            return p0 * weights[0] + p1 * weights[1] + p2 * weights[2] +
                   p3 * weights[3];
        };

        for (size_t si = 0; si < pointCount; ++si) {
            Pnt3f prevPos = evalPosition(si, 0.0f);
            for (int sample = 1; sample <= TrainView::ARCLEN_SAMPLES;
                 ++sample) {
                float t =
                    static_cast<float>(sample) / TrainView::ARCLEN_SAMPLES;
                data.tSamples[si][sample] = t;
                Pnt3f current = evalPosition(si, t);
                Pnt3f diff = current - prevPos;
                float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y +
                                       diff.z * diff.z);
                data.cumLen[si][sample] = data.cumLen[si][sample - 1] + dist;
                prevPos = current;
            }
            float segmentLength = data.cumLen[si][TrainView::ARCLEN_SAMPLES];
            data.segOffset[si + 1] = data.segOffset[si] + segmentLength;
        }
        data.totalLen = data.segOffset.back();
        return data;
    };

    const auto mapLengthToSegment = [](const ArcLengthTable& data, float s,
                                       size_t& segment, float& outT) {
        if (data.segmentCount == 0 || data.totalLen <= 1e-6f) {
            segment = 0;
            outT = 0.0f;
            return;
        }

        float clamped = s;
        if (clamped <= 0.0f) {
            segment = 0;
            outT = 0.0f;
            return;
        }
        if (clamped >= data.totalLen) {
            segment = data.segmentCount - 1;
            outT = 1.0f;
            return;
        }

        size_t lo = 0;
        size_t hi = data.segmentCount;
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (data.segOffset[mid] <= clamped)
                lo = mid;
            else
                hi = mid;
        }
        segment = lo;
        float localLength = clamped - data.segOffset[segment];
        float segmentLength =
            data.segOffset[segment + 1] - data.segOffset[segment];
        if (segmentLength <= 1e-6f) {
            outT = 0.0f;
            return;
        }

        const auto& cum = data.cumLen[segment];
        const auto& ts = data.tSamples[segment];
        int loI = 0;
        int hiI = TrainView::ARCLEN_SAMPLES;
        while (loI < hiI) {
            int mid = (loI + hiI) / 2;
            if (cum[mid] < localLength)
                loI = mid + 1;
            else
                hiI = mid;
        }
        int idx =
            (loI < TrainView::ARCLEN_SAMPLES) ? loI : TrainView::ARCLEN_SAMPLES;
        int prevIdx = (idx > 0) ? idx - 1 : 0;
        float s0 = cum[prevIdx];
        float s1 = cum[idx];
        float t0 = ts[prevIdx];
        float t1 = ts[idx];
        float tau = 0.0f;
        float denom = s1 - s0;
        if (denom > 1e-6f)
            tau = (localLength - s0) / denom;
        if (tau < 0.0f)
            tau = 0.0f;
        else if (tau > 1.0f)
            tau = 1.0f;
        outT = t0 + tau * (t1 - t0);
    };

    size_t segmentIndex =
        static_cast<size_t>(std::floor(wrappedParam)) % pointCount;
    float localT = wrappedParam - std::floor(wrappedParam);

    if (tw->arcLength && tw->arcLength->value()) {
        ArcLengthTable arcData = buildArcLengthTable(splineMode);
        if (arcData.totalLen > 1e-6f) {
            float normalized = wrappedParam / static_cast<float>(pointCount);
            if (normalized < 0.0f)
                normalized += 1.0f;
            float targetLength = normalized * arcData.totalLen;
            mapLengthToSegment(arcData, targetLength, segmentIndex, localT);
        }
    }

    size_t idxPrev = wrapIndex(static_cast<int>(segmentIndex) - 1, pointCount);
    size_t idxCurr = segmentIndex;
    size_t idxNext = (segmentIndex + 1) % pointCount;
    size_t idxNext2 = (segmentIndex + 2) % pointCount;

    const ControlPoint& cpPrev = m_pTrack->points[idxPrev];
    const ControlPoint& cpCurr = m_pTrack->points[idxCurr];
    const ControlPoint& cpNext = m_pTrack->points[idxNext];
    const ControlPoint& cpNext2 = m_pTrack->points[idxNext2];

    float M[4][4] = { 0 };
    buildBasisMatrix(splineMode, M);

    const float t2 = localT * localT;
    const float t3 = t2 * localT;
    float T[4] = { t3, t2, localT, 1.0f };
    float weights[4];
    for (int r = 0; r < 4; ++r) {
        weights[r] =
            M[r][0] * T[0] + M[r][1] * T[1] + M[r][2] * T[2] + M[r][3] * T[3];
    }

    auto weightedSum = [&](const Pnt3f& a, const Pnt3f& b, const Pnt3f& c,
                           const Pnt3f& d) {
        return a * weights[0] + b * weights[1] + c * weights[2] +
               d * weights[3];
    };

    Pnt3f position =
        weightedSum(cpPrev.pos, cpCurr.pos, cpNext.pos, cpNext2.pos);
    Pnt3f up = weightedSum(cpPrev.orient, cpCurr.orient, cpNext.orient,
                           cpNext2.orient);

    float dT[4] = { 3.0f * t2, 2.0f * localT, 1.0f, 0.0f };
    float dWeights[4];
    for (int r = 0; r < 4; ++r) {
        dWeights[r] = M[r][0] * dT[0] + M[r][1] * dT[1] + M[r][2] * dT[2] +
                      M[r][3] * dT[3];
    }

    Pnt3f tangent = cpPrev.pos * dWeights[0] + cpCurr.pos * dWeights[1] +
                    cpNext.pos * dWeights[2] + cpNext2.pos * dWeights[3];

    float tangentLenSq =
        tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z;
    if (tangentLenSq > 1e-6f) {
        tangent.normalize();
    } else {
        tangent = Pnt3f(0.0f, 0.0f, 1.0f);
    }

    // In arc length mode, the derivative direction might not match
    // the arc length parameterization direction, so reverse if needed
    if (tw->arcLength && tw->arcLength->value()) {
        // Compute a small step forward in arc length space
        float testStep = 0.01f;
        float testParam = wrappedParam + testStep;
        while (testParam >= static_cast<float>(pointCount)) {
            testParam -= static_cast<float>(pointCount);
        }

        // Map the test parameter through arc length
        ArcLengthTable arcData = buildArcLengthTable(splineMode);
        if (arcData.totalLen > 1e-6f) {
            size_t testSegIdx = segmentIndex;
            float testLocalT = localT;

            float testNormalized = testParam / static_cast<float>(pointCount);
            if (testNormalized < 0.0f)
                testNormalized += 1.0f;
            float testTargetLength = testNormalized * arcData.totalLen;
            mapLengthToSegment(arcData, testTargetLength, testSegIdx,
                               testLocalT);

            // Compute position at test parameter using arc length mapping
            size_t tPrev =
                wrapIndex(static_cast<int>(testSegIdx) - 1, pointCount);
            size_t tCurr = testSegIdx;
            size_t tNext = (testSegIdx + 1) % pointCount;
            size_t tNext2 = (testSegIdx + 2) % pointCount;

            const float tt2 = testLocalT * testLocalT;
            const float tt3 = tt2 * testLocalT;
            float tT[4] = { tt3, tt2, testLocalT, 1.0f };
            float tWeights[4];
            for (int r = 0; r < 4; ++r) {
                tWeights[r] = M[r][0] * tT[0] + M[r][1] * tT[1] +
                              M[r][2] * tT[2] + M[r][3] * tT[3];
            }

            Pnt3f testPos = m_pTrack->points[tPrev].pos * tWeights[0] +
                            m_pTrack->points[tCurr].pos * tWeights[1] +
                            m_pTrack->points[tNext].pos * tWeights[2] +
                            m_pTrack->points[tNext2].pos * tWeights[3];

            // Check if tangent points towards test position
            Pnt3f toTest = testPos - position;
            float dot = tangent.x * toTest.x + tangent.y * toTest.y +
                        tangent.z * toTest.z;
            if (dot < 0.0f) {
                // Tangent points backwards, reverse it
                tangent = Pnt3f(-tangent.x, -tangent.y, -tangent.z);
            }
        }
    }

    float upLenSq = up.x * up.x + up.y * up.y + up.z * up.z;
    if (upLenSq < 1e-6f) {
        up = Pnt3f(0.0f, 1.0f, 0.0f);
    } else {
        up.normalize();
    }

    Pnt3f right = tangent * up;
    float rightLenSq =
        right.x * right.x + right.y * right.y + right.z * right.z;
    if (rightLenSq < 1e-6f) {
        right = Pnt3f(1.0f, 0.0f, 0.0f);
    } else {
        right.normalize();
    }

    up = right * tangent;
    up.normalize();

    // ---------- Terrain Snapping: Prevent train from going underground ----------
    if (terrain) {
        float terrainHeight =
            terrain->getHeightAtWorldPos(position.x, position.z);
        const float minClearance = 2.0f;  // Minimum distance above terrain

        if (position.y < terrainHeight + minClearance) {
            // Snap train above terrain
            position.y = terrainHeight + minClearance;
        }
    }

    const float wheelRadius = 3.25f;
    const float wheelWidth = 2.0f;
    const float twoPi = 2.0f * static_cast<float>(M_PI);
    const float bodyLift = 5.0f;
    const float wheelBodyGap = -2.0f;

    Pnt3f previousTrainPosition = trainPosition;
    float distanceMoved = 0.0f;
    if (wheelParamInitialized) {
        Pnt3f delta = position - previousTrainPosition;
        distanceMoved =
            std::sqrt(delta.x * delta.x + delta.y * delta.y +
                      delta.z * delta.z);
    } else {
        wheelParamInitialized = true;
    }

    if (distanceMoved > 1e-4f && wheelRadius > 1e-4f) {
        // Subtract to reverse visual rotation direction
        wheelAngle -= distanceMoved / wheelRadius;
        if (wheelAngle > twoPi || wheelAngle < -twoPi) {
            wheelAngle = std::fmod(wheelAngle, twoPi);
        }
    }

    const float halfExtent = 5.0f;
    Pnt3f halfUp = up * halfExtent;
    Pnt3f center = position + halfUp + up * bodyLift;

    Pnt3f updatedTrainPosition = position;
    if (!useMinecraftTrain) {
        updatedTrainPosition = center;
    }

    if (!useMinecraftTrain && !tw->trainCam->value()) {
        Pnt3f halfForward = tangent * halfExtent;
        Pnt3f halfRight = right * halfExtent;

        Pnt3f frontTopRight = center + halfForward + halfRight + halfUp;
        Pnt3f frontTopLeft = center + halfForward - halfRight + halfUp;
        Pnt3f frontBottomRight = center + halfForward + halfRight - halfUp;
        Pnt3f frontBottomLeft = center + halfForward - halfRight - halfUp;
        Pnt3f backTopRight = center - halfForward + halfRight + halfUp;
        Pnt3f backTopLeft = center - halfForward - halfRight + halfUp;
        Pnt3f backBottomRight = center - halfForward + halfRight - halfUp;
        Pnt3f backBottomLeft = center - halfForward - halfRight - halfUp;

        if (!doingShadows) {
            glColor3ub(255, 255, 255);
        }

        glBegin(GL_QUADS);
        // Front face
        if (!doingShadows) {
            glColor3ub(89, 110, 57);
        }
        glNormal3f(tangent.x, tangent.y, tangent.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(frontBottomLeft.x, frontBottomLeft.y, frontBottomLeft.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(frontBottomRight.x, frontBottomRight.y,
                   frontBottomRight.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);

        // Back face
        if (!doingShadows) {
            glColor3ub(255, 255, 255);
        }
        glNormal3f(-tangent.x, -tangent.y, -tangent.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(backBottomRight.x, backBottomRight.y, backBottomRight.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(backBottomLeft.x, backBottomLeft.y, backBottomLeft.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(backTopLeft.x, backTopLeft.y, backTopLeft.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(backTopRight.x, backTopRight.y, backTopRight.z);

        // Left face
        glNormal3f(-right.x, -right.y, -right.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(backBottomLeft.x, backBottomLeft.y, backBottomLeft.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(frontBottomLeft.x, frontBottomLeft.y, frontBottomLeft.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(backTopLeft.x, backTopLeft.y, backTopLeft.z);

        // Right face
        glNormal3f(right.x, right.y, right.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(frontBottomRight.x, frontBottomRight.y,
                   frontBottomRight.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(backBottomRight.x, backBottomRight.y, backBottomRight.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(backTopRight.x, backTopRight.y, backTopRight.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);

        // Top face
        glNormal3f(up.x, up.y, up.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(backTopRight.x, backTopRight.y, backTopRight.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(backTopLeft.x, backTopLeft.y, backTopLeft.z);

        // Bottom face
        glNormal3f(-up.x, -up.y, -up.z);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(backBottomLeft.x, backBottomLeft.y, backBottomLeft.z);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(backBottomRight.x, backBottomRight.y, backBottomRight.z);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(frontBottomRight.x, frontBottomRight.y,
                   frontBottomRight.z);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(frontBottomLeft.x, frontBottomLeft.y, frontBottomLeft.z);
        glEnd();

        const float axleInnerOffset = -1.5f; // negative moves wheels closer to center
        const float axleInset = halfExtent + wheelWidth * 0.5f + axleInnerOffset;
        Pnt3f bodyBottom = center - halfUp;
        Pnt3f axleCenter = bodyBottom - up * (wheelRadius + wheelBodyGap);
        const int rimSlices = 48;
        const int sectorSlices = 24;
        const float halfWheelWidth = wheelWidth * 0.5f;

        auto drawWheel = [&](const Pnt3f& center) {
            if (!doingShadows) {
                glColor3ub(45, 45, 45);
            }

            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i <= rimSlices; ++i) {
                float theta = (static_cast<float>(i) / rimSlices) * twoPi;
                float angle = theta + wheelAngle;
                float c = std::cos(angle);
                float s = std::sin(angle);
                Pnt3f radial = (up * c + tangent * s) * wheelRadius;
                Pnt3f normal = radial;
                normal.normalize();

                Pnt3f outer = center + radial + right * halfWheelWidth;
                Pnt3f inner = center + radial - right * halfWheelWidth;

                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(outer.x, outer.y, outer.z);
                glVertex3f(inner.x, inner.y, inner.z);
            }
            glEnd();

            auto drawCap = [&](float sign) {
                Pnt3f capCenter = center + right * (sign * halfWheelWidth);
                Pnt3f capNormal = right * sign;

                glBegin(GL_TRIANGLES);
                for (int i = 0; i < sectorSlices; ++i) {
                    float theta0 =
                        (static_cast<float>(i) / sectorSlices) * twoPi +
                        wheelAngle;
                    float theta1 = (static_cast<float>(i + 1) / sectorSlices) *
                                       twoPi + wheelAngle;

                    Pnt3f rim0 = capCenter +
                                 (up * std::cos(theta0) +
                                  tangent * std::sin(theta0)) * wheelRadius;
                    Pnt3f rim1 = capCenter +
                                 (up * std::cos(theta1) +
                                  tangent * std::sin(theta1)) * wheelRadius;

                    if (!doingShadows) {
                        if (i % 2 == 0) {
                            glColor3ub(70, 140, 255);
                        } else {
                            glColor3ub(255, 80, 95);
                        }
                    }

                    glNormal3f(capNormal.x, capNormal.y, capNormal.z);
                    glVertex3f(capCenter.x, capCenter.y, capCenter.z);
                    glVertex3f(rim0.x, rim0.y, rim0.z);
                    glVertex3f(rim1.x, rim1.y, rim1.z);
                }
                glEnd();
            };

            drawCap(1.0f);
            drawCap(-1.0f);
        };

        drawWheel(axleCenter + right * axleInset);
        drawWheel(axleCenter - right * axleInset);
    }

    // Update train state
    trainPosition = updatedTrainPosition;
    trainForward = tangent;
    trainUp = up;
    
    if (useMinecraftTrain && !tw->trainCam->value() && mcMinecart && mcVillager) {
        auto toGlm = [](const Pnt3f& p) { return glm::vec3(p.x, p.y, p.z); };

        const float heightOffset = 2.0f;
        Pnt3f raisedPosition = position + up * heightOffset;

        glm::mat4 basis(1.0f);
        basis[0] = glm::vec4(toGlm(right), 0.0f);
        basis[1] = glm::vec4(toGlm(up), 0.0f);
        basis[2] = glm::vec4(toGlm(tangent), 0.0f);
        basis[3] = glm::vec4(toGlm(raisedPosition), 1.0f);

        const glm::mat4 assetFix =
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 modelMatrix = basis * assetFix;
        glm::mat4 villagerOffset =
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
            
        mcVillager->draw(modelMatrix / assetFix * villagerOffset, doingShadows, smokeStartDistance,
                        smokeEndDistance);
        mcMinecart->draw(modelMatrix, doingShadows, smokeStartDistance,
                         smokeEndDistance);
    }
}

void TrainView::drawOden(bool doingShadows) {
    const float offsetX = 50.0f;
    const float offsetY = 0.0f;
    const float offsetZ = -30.0f;

    glPushMatrix();
    glTranslatef(offsetX, offsetY, offsetZ);

    // ----------- Tofu --------------
    const float tofuWidth = 20.0f;
    const float tofuDepth = 20.0f;
    const float tofuHeight = 25.0f;
    const float halfTofuWidth = tofuWidth / 2.0f;
    const float halfTofuDepth = tofuDepth / 2.0f;
    const float halfTofuHeight = tofuHeight / 2.0f;
    if (!doingShadows) {
        glColor3ub(150, 150, 150);
    }

    glPushMatrix();
    glTranslatef(0.0f, halfTofuHeight, 0.0f);
    glBegin(GL_QUADS);

    // Front face (+Z)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glVertex3f(halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, halfTofuDepth);
    glVertex3f(-halfTofuWidth, halfTofuHeight, halfTofuDepth);
    // Back face (-Z)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    glVertex3f(-halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    // Left face (-X)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glVertex3f(-halfTofuWidth, halfTofuHeight, halfTofuDepth);
    glVertex3f(-halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    // Right face (+X)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glVertex3f(halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, halfTofuDepth);
    // Top face (+Y)
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-halfTofuWidth, halfTofuHeight, halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, halfTofuDepth);
    glVertex3f(halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    glVertex3f(-halfTofuWidth, halfTofuHeight, -halfTofuDepth);
    // Bottom face (-Y)
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, -halfTofuHeight, -halfTofuDepth);
    glVertex3f(halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glVertex3f(-halfTofuWidth, -halfTofuHeight, halfTofuDepth);
    glEnd();

    glPopMatrix();

    // ---------- Pork ball --------------
    const float porkBallRadius = 10.0f;
    glPushMatrix();
    glTranslatef(0.0f, 0.6f * tofuHeight, 0.7f * tofuWidth);
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluSphere(quad, porkBallRadius, 16, 16);
    gluDeleteQuadric(quad);
    glPopMatrix();

    // ---------- Pig blood cake ----------
    const float pigBloodCakeWidth = 20.0f;
    const float pigBloodCakeDepth = 15.0f;
    const float pigBloodCakeHeight = 10.0f;
    const float halfPigBloodCakeWidth = pigBloodCakeWidth / 2.0f;
    const float halfPigBloodCakeDepth = pigBloodCakeDepth / 2.0f;
    const float halfPigBloodCakeHeight = pigBloodCakeHeight / 2.0f;

    glPushMatrix();
    glTranslatef(-0.7f * tofuWidth, 0.4f * tofuHeight, 0.0f);
    glBegin(GL_QUADS);

    // Front face (+Z)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    // Back face (-Z)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    // Left face (-X)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    // Right face (+X)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    // Top face (+Y)
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    // Bottom face (-Y)
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               -halfPigBloodCakeDepth);
    glVertex3f(halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glVertex3f(-halfPigBloodCakeWidth, -halfPigBloodCakeHeight,
               halfPigBloodCakeDepth);
    glEnd();

    glPopMatrix();

    glPopMatrix();
}

//
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
// ########################################################################
// TODO:
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change
// this
// ########################################################################
//========================================================================
void TrainView::doPick()
//========================================================================
{
    // since we'll need to do some GL stuff so we make this window as
    // active window
    make_current();

    // where is the mouse?
    int mx = Fl::event_x();
    int my = Fl::event_y();

    // get the viewport - most reliable way to turn mouse coords into GL coords
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Set up the pick matrix on the stack - remember, FlTk is
    // upside down!
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPickMatrix((double)mx, (double)(viewport[3] - my), 5, 5, viewport);

    // now set up the projection
    setProjection();

    // now draw the objects - but really only see what we hit
    GLuint buf[100];
    glSelectBuffer(100, buf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    // draw the cubes, loading the names as we go
    for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
        glLoadName((GLuint)(i + 1));
        m_pTrack->points[i].draw();
    }

    // go back to drawing mode, and see how picking did
    int hits = glRenderMode(GL_RENDER);
    if (hits) {
        // warning; this just grabs the first object hit - if there
        // are multiple objects, you really want to pick the closest
        // one - see the OpenGL manual
        // remember: we load names that are one more than the index
        selectedCube = buf[3] - 1;
    } else {
        // nothing hit, nothing selected
        selectedCube = -1;
    }

    printf("Selected Cube %d\n", selectedCube);
}

