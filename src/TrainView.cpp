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

#include <cmath>
#include <iostream>
#include <vector>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
// #include "GL/gl.h"
#include <Fl/fl.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glu.h"
#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"

#ifdef EXAMPLE_SOLUTION
#include "TrainExample/TrainExample.H"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DIVIDE_LINE 1000.0f

void TrainView::setUBO() {
    float wdt = this->pixel_w();
    float hgt = this->pixel_h();

    glm::mat4 view_matrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
    // HMatrix view matrix;
    // this-›arcball -getMatrix(view_matrix);

    glm::mat4 projection_matrix;
    glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
    // projection_matrix = glm::perspective(glm::radians(this-›arcball.getFov()),
    // (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.f);

    glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                    &projection_matrix[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                    &view_matrix[0][0]);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

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

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw() {
    //*********************************************************************
    //
    // * Set up basic opengl informaiton
    //
    //**********************************************************************
    // initialized glad
    if (gladLoadGL()) {
        if (!this->shader) {
            this->shader = new Shader("./shaders/simple.vert", nullptr, nullptr,
                                      nullptr, "./shaders/simple.frag");
        }

        if (!this->common_matrices) {
            this->common_matrices = new UBO();
        }
        this->common_matrices->size = 2 * sizeof(glm::mat4);
        glGenBuffers(1, &this->common_matrices->ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
        glBufferData(GL_UNIFORM_BUFFER, this->common_matrices->size, NULL,
                     GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        if (!this->plane) {
            const int division = 100;  // Number of subdivisions
            const float SIZE = 10.0f;  // Plane size

            struct Wave {
                glm::vec2 direction;
                float wavelength;
                float amplitude;
                float speed;
                float frequency;
            };

            std::vector<Wave> waves = {
                { { 1.0f, 0.0f }, 2.0f, 0.10f, 1.0f },
                { { 0.7f, 0.7f }, 3.0f, 0.05f, 0.8f },
                { { -0.6f, 0.8f }, 1.5f, 0.07f, 1.2f }
            };

            for (auto& w : waves)
                w.frequency = 2.0f * M_PI / w.wavelength;

            std::vector<GLfloat> vertices;
            std::vector<GLfloat> normals;
            std::vector<GLfloat> textureCoordinates;
            std::vector<GLfloat> colors;
            std::vector<GLuint> elements;

            // Generate vertex data
            float half = SIZE / 2.0f;
            for (int j = 0; j <= division; ++j) {
                for (int i = 0; i <= division; ++i) {
                    float x = -half + SIZE * (float)i / division;
                    float z = -half + SIZE * (float)j / division;

                    // Calculate height using wave functions
                    float h = 0.0f;
                    for (const auto& w : waves) {
                        float dot = glm::dot(w.direction, glm::vec2(x, z));
                        h += w.amplitude * sin(dot * w.frequency);
                    }

                    // Vertex position
                    vertices.push_back(x);
                    vertices.push_back(h);
                    vertices.push_back(z);

                    // Simple normals (initially set to up)
                    normals.push_back(0.0f);
                    normals.push_back(1.0f);
                    normals.push_back(0.0f);

                    // Texture coordinates
                    textureCoordinates.push_back((float)i / division);
                    textureCoordinates.push_back((float)j / division);

                    // Color based on height
                    float blue = glm::clamp(0.4f + h * 4.0f, 0.0f, 1.0f);
                    colors.push_back(0.0f);  // R
                    colors.push_back(0.0f);  // G
                    colors.push_back(blue);  // B
                }
            }

            // Generate index data (two right triangles form a square)
            for (int j = 0; j < division; ++j) {
                for (int i = 0; i < division; ++i) {
                    GLuint topLeft = j * (division + 1) + i;
                    GLuint topRight = topLeft + 1;
                    GLuint bottomLeft = (j + 1) * (division + 1) + i;
                    GLuint bottomRight = bottomLeft + 1;

                    // Triangle 1
                    elements.push_back(topLeft);
                    elements.push_back(bottomLeft);
                    elements.push_back(topRight);

                    // Triangle 2
                    elements.push_back(topRight);
                    elements.push_back(bottomLeft);
                    elements.push_back(bottomRight);
                }
            }

            // Generate and bind VAO and VBOs
            this->plane = new VAO();
            this->plane->element_amount = elements.size();

            glGenVertexArrays(1, &this->plane->vao);
            glGenBuffers(4, this->plane->vbo);
            glGenBuffers(1, &this->plane->ebo);

            glBindVertexArray(this->plane->vao);

            // Vertex position
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                         vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            // Normals
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                         normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            // Texture coordinates
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(GLfloat),
                         textureCoordinates.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);

            // Colors
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
            glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                         colors.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(3);

            // Element indices
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         elements.size() * sizeof(GLuint), elements.data(),
                         GL_STATIC_DRAW);

            // Unbind VAO
            glBindVertexArray(0);
        }

        // if (!this->plane) {
        //     // Equilateral triangle - centered at origin
        //     GLfloat vertices[] = { -0.5f, 0.0f, -sqrt(3.0f) / 6.0f,
        //                            0.5f,  0.0f, -sqrt(3.0f) / 6.0f,
        //                            0.0f,  0.0f, sqrt(3.0f) / 3.0f };

        //     GLfloat normal[] = {
        //         0.0f, 0.0f, 1.0f,
        //         0.0f, 0.0f, 1.0f,
        //         0.0f, 0.0f, 1.0f
        //     };

        //     GLfloat texture_coordinate[] = {
        //         0.0f, 0.0f,
        //         1.0f, 0.0f,
        //         0.5f, 1.0f
        //     };

        //     GLuint element[] = { 0, 1, 2 };

        //     GLfloat colors[] = {
        //         1.0f, 0.0f, 0.0f,   // Red
        //         0.0f, 1.0f, 0.0f,   // Green
        //         0.0f, 0.0f, 1.0f    // Blue
        //     };

        //     this->plane = new VAO();
        //     this->plane->element_amount = sizeof(element) / sizeof(GLuint);
        //     glGenVertexArrays(1, &this->plane->vao);
        //     glGenBuffers(4, this->plane->vbo);  // Changed to 4 for color buffer
        //     glGenBuffers(1, &this->plane->ebo);

        //     glBindVertexArray(this->plane->vao);

        //     // Position attribute
        //     glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
        //     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        //                  GL_STATIC_DRAW);
        //     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
        //                           (GLvoid*)0);
        //     glEnableVertexAttribArray(0);

        //     // Normal attribute
        //     glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
        //     glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal,
        //                  GL_STATIC_DRAW);
        //     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
        //                           (GLvoid*)0);
        //     glEnableVertexAttribArray(1);

        //     // Texture Coordinate attribute
        //     glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
        //     glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate),
        //                  texture_coordinate, GL_STATIC_DRAW);
        //     glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
        //                           (GLvoid*)0);
        //     glEnableVertexAttribArray(2);

        //     // Element attribute
        //     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
        //     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element,
        //                  GL_STATIC_DRAW);

        //     // Color attribute
        //     glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
        //     glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors,
        //                  GL_STATIC_DRAW);
        //     glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
        //                           (GLvoid*)0);
        //     glEnableVertexAttribArray(3);

        //     // Unbind VAO
        //     glBindVertexArray(0);
        // }

        if (!this->texture) {
            // this->texture = new Texture2D("./images/church.png");
            this->texture = new Texture2D("./images/waterHeightMap.jpg");
        }
    } else {
        throw std::runtime_error("Could not initialize GLAD!");
    }

    // Set up the view port
    glViewport(0, 0, w(), h());

    // clear the window, be sure to clear the Z-Buffer too
    glClearColor(0, 0, .3f, 0);  // background should be blue

    // we need to clear out the stencil buffer since we'll use
    // it for shadows
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH);

    // Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // prepare for projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    setProjection();  // put the code to set up matrices here

    // ######################################################################
    //  TODO:
    //  you might want to set the lighting up differently. if you do,
    //  we need to set up the lights AFTER setting up the projection
    // ######################################################################

    // enable the lighting
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

    // top view only needs one light
    if (tw->topCam->value()) {
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHT2);
    } else {
        glEnable(GL_LIGHT1);
        // glEnable(GL_LIGHT2);
    }

    //*********************************************************************
    //
    // * set the light parameters
    //
    //**********************************************************************
    // GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
    // GLfloat lightPosition2[]	= {1, 0, 0, 0};
    // GLfloat lightPosition3[]	= {0, -1, 0, 0};
    // GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
    // GLfloat whiteLight[]	 	= {1.0f, 1.0f, 1.0f, 1.0};
    // GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
    // GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

    // glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
    // glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
    // glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

    // glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
    // glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

    // glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
    // glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

    // *********************************************************************
    // Directional light setup
    float noAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float directionalDiffuse[] = { 0.2f, 0.2f, 0.8f, 1.0f };

    /*
     * Directional light source (w = 0)
     * The light source is at an infinite distance,
     * all the ray are parallel and have the direction (x, y, z).
     */
    float directionalLightPosition[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, directionalDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, directionalLightPosition);

    // *********************************************************************
    // Point light setup — tie to a control point (follows its movement)
    float pointAmbient[] = { 0.02f, 0.02f, 0.05f, 1.0f };
    float pointDiffuse[] = { 1.0f, 0.2f, 0.2f, 1.0f };

    // Pick which control point drives the light: selected if any, otherwise 0
    Pnt3f lightPosCP(10.0f, 10.0f, 10.0f);
    Pnt3f lightDirCP(0.0f, -1.0f, 0.0f);
    if (!m_pTrack->points.empty()) {
        int idx =
            (selectedCube >= 0 && selectedCube < (int)m_pTrack->points.size())
                ? selectedCube
                : 0;
        lightPosCP = m_pTrack->points[(size_t)idx].pos;
        lightDirCP = m_pTrack->points[(size_t)idx].orient;
        lightDirCP.normalize();
    }
    float pointLightPosition[] = { lightPosCP.x, lightPosCP.y, lightPosCP.z,
                                   1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT, pointAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, pointDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, pointLightPosition);

    // *********************************************************************
    // // Spot light setup
    // float noAmbient[] = {0.0f, 0.0f, 0.2f, 1.0f}; //low ambient light
    // float diffuse[] = {0.0f, 0.0f, 1.0f, 1.0f};
    // float position[] = {0.0f, 0.0f, 0.0f, 1.0f};

    // //properties of the light
    // glLightfv(GL_LIGHT2, GL_AMBIENT, noAmbient);
    // glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuse);
    // glLightfv(GL_LIGHT2, GL_POSITION, position);

    // /*Spot properties*/
    // //spot direction
    // float direction[] = {5.0f, 8.0f, 1.0f};
    // glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, direction);
    // //angle of the cone light emitted by the spot : value between 0 to 180
    // float spotCutOff = 45;
    // glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, spotCutOff);

    // //exponent propertie defines the concentration of the light
    // glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 15.0f);
    // //light attenuation (default values used here : no attenuation with the
    // distance) glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    // glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.0f);
    // glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.0f);

    //*********************************************************************
    // now draw the ground plane
    //*********************************************************************
    // set to opengl fixed pipeline(use opengl 1.x draw function)
    glUseProgram(0);

    setupFloor();
    // glDisable(GL_LIGHTING);
    drawFloor(200, 10);

    //*********************************************************************
    // now draw the object and we need to do it twice
    // once for real, and then once for shadows
    //*********************************************************************
    glEnable(GL_LIGHTING);
    setupObjects();

    drawStuff();

    // this time drawing is for shadows (except for top view)
    if (!tw->topCam->value()) {
        setupShadows();
        drawStuff(true);
        unsetupShadows();
    }

    // *********************************************************************
    // draw a simple textured plane
    setUBO();
    glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/ 0,
                      this->common_matrices->ubo, 0,
                      this->common_matrices->size);

    // bind shader
    this->shader->Use();

    glm::mat4 model_matrix = glm::mat4();
    model_matrix = glm::translate(model_matrix, glm::vec3(0, 10.0f, 0.0f));
    model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
    glUniformMatrix4fv(glGetUniformLocation(this->shader->Program, "u_model"),
                       1, GL_FALSE, &model_matrix[0][0]);
    glUniform3fv(glGetUniformLocation(this->shader->Program, "u_color"), 1,
                 &glm::vec3(0.5f, 0.0f, 0.0f)[0]);

    this->texture->bind(0);
    glUniform1i(glGetUniformLocation(this->shader->Program, "u_texture"), 0);

    // bind VAO
    glBindVertexArray(this->plane->vao);

    glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT,
                   0);

    // unbind VAO
    glBindVertexArray(0);

    // unbind shader(switch to fixed pipeline)
    glUseProgram(0);
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

    // Check whether we use the world camp
    if (tw->worldCam->value())
        arcball.setProjection(false);
    // Or we use the top cam
    else if (tw->topCam->value()) {
        float wi, he;
        if (aspect >= 1) {
            wi = 110;
            he = wi / aspect;
        } else {
            he = 110;
            wi = he * aspect;
        }

        // Set up the top camera drop mode to be orthogonal and set
        // up proper projection matrix
        glMatrixMode(GL_PROJECTION);
        glOrtho(-wi, wi, -he, he, 200, -200);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(-90, 1, 0, 0);
    }
    // Or do the train view or other view here
    // ####################################################################
    // TODO:
    // put code for train view projection here!
    // ####################################################################
    else {
#ifdef EXAMPLE_SOLUTION
        trainCamView(this, aspect);
#endif
    }
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get
//       colored shadows). this gets called twice per draw
//       -- once for the objects, once for the shadows
// ########################################################################
// TODO:
// if you have other objects in the world, make sure to draw them
// ########################################################################
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

#ifdef EXAMPLE_SOLUTION
    // don't draw the train if you're looking out the front window
    if (!tw->trainCam->value())
        drawTrain(this, doingShadows);
#endif
}

void TrainView::drawTrack(bool doingShadows) {
    // If global arc-length mode is enabled, build a global s->(segment,t)
    // mapping and draw with uniform spacing along the full track length.
    if (tw->arcLength->value() == 1) {
        const size_t pointCount = m_pTrack->points.size();
        if (pointCount == 0)
            return;

        // Helper for wrapping indices in closed loop
        auto wrapIndex = [pointCount](int index) {
            int wrapped = index % static_cast<int>(pointCount);
            if (wrapped < 0)
                wrapped += static_cast<int>(pointCount);
            return static_cast<size_t>(wrapped);
        };

        // Build per-spline evaluators: position and orientation as a function of
        // segment index and local t in [0,1]
        const int mode =
            tw->splineBrowser
                ->value();  // 1: Linear, 2: Cardinal, else: B-Spline

        // Cardinal helpers
        auto evaluateCardinal = [](const Pnt3f& p0, const Pnt3f& p1,
                                   const Pnt3f& p2, const Pnt3f& p3, float t) {
            const float tension = 0.0f;  // Catmull-Rom
            const float tf = (1.0f - tension) * 0.5f;
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
            const float h10 = t3 - 2.0f * t2 + t;
            const float h01 = -2.0f * t3 + 3.0f * t2;
            const float h11 = t3 - t2;
            Pnt3f m1 = (p2 - p0) * tf;
            Pnt3f m2 = (p3 - p1) * tf;
            return p1 * h00 + m1 * h10 + p2 * h01 + m2 * h11;
        };

        // B-spline helper
        auto evaluateBSpline = [](const Pnt3f& p0, const Pnt3f& p1,
                                  const Pnt3f& p2, const Pnt3f& p3, float t) {
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float sixth = 1.0f / 6.0f;
            const float b0 = (-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * sixth;
            const float b1 = (3.0f * t3 - 6.0f * t2 + 4.0f) * sixth;
            const float b2 = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * sixth;
            const float b3 = t3 * sixth;
            return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
        };

        // Position evaluator
        auto evalPos = [&](size_t segIdx, float t) -> Pnt3f {
            if (mode == 1) {
                const Pnt3f& p0 = m_pTrack->points[segIdx].pos;
                const Pnt3f& p1 =
                    m_pTrack->points[(segIdx + 1) % pointCount].pos;
                return (1.0f - t) * p0 + t * p1;
            } else if (mode == 2) {
                const Pnt3f& p0 =
                    m_pTrack->points[wrapIndex(static_cast<int>(segIdx) - 1)]
                        .pos;
                const Pnt3f& p1 = m_pTrack->points[segIdx].pos;
                const Pnt3f& p2 =
                    m_pTrack->points[(segIdx + 1) % pointCount].pos;
                const Pnt3f& p3 =
                    m_pTrack->points[(segIdx + 2) % pointCount].pos;
                return evaluateCardinal(p0, p1, p2, p3, t);
            } else {
                const Pnt3f& p0 =
                    m_pTrack->points[wrapIndex(static_cast<int>(segIdx) - 1)]
                        .pos;
                const Pnt3f& p1 = m_pTrack->points[segIdx].pos;
                const Pnt3f& p2 =
                    m_pTrack->points[(segIdx + 1) % pointCount].pos;
                const Pnt3f& p3 =
                    m_pTrack->points[(segIdx + 2) % pointCount].pos;
                return evaluateBSpline(p0, p1, p2, p3, t);
            }
        };

        // Orientation evaluator (normalized)
        auto evalOrient = [&](size_t segIdx, float t) -> Pnt3f {
            Pnt3f o;
            if (mode == 1) {
                const Pnt3f& o0 = m_pTrack->points[segIdx].orient;
                const Pnt3f& o1 =
                    m_pTrack->points[(segIdx + 1) % pointCount].orient;
                o = (1.0f - t) * o0 + t * o1;
            } else if (mode == 2) {
                const Pnt3f& o0 =
                    m_pTrack->points[wrapIndex(static_cast<int>(segIdx) - 1)]
                        .orient;
                const Pnt3f& o1 = m_pTrack->points[segIdx].orient;
                const Pnt3f& o2 =
                    m_pTrack->points[(segIdx + 1) % pointCount].orient;
                const Pnt3f& o3 =
                    m_pTrack->points[(segIdx + 2) % pointCount].orient;
                o = evaluateCardinal(o0, o1, o2, o3, t);
            } else {
                const Pnt3f& o0 =
                    m_pTrack->points[wrapIndex(static_cast<int>(segIdx) - 1)]
                        .orient;
                const Pnt3f& o1 = m_pTrack->points[segIdx].orient;
                const Pnt3f& o2 =
                    m_pTrack->points[(segIdx + 1) % pointCount].orient;
                const Pnt3f& o3 =
                    m_pTrack->points[(segIdx + 2) % pointCount].orient;
                o = evaluateBSpline(o0, o1, o2, o3, t);
            }
            o.normalize();
            return o;
        };

        // Check minimum control points per mode
        if (mode == 1 && pointCount < 2)
            return;
        if ((mode == 2 || mode != 1) && pointCount < 4)
            return;

        // Build per-segment arc-length lookup tables
        const int ARCLEN_SAMPLES = 100;
        std::vector<float> segLen(pointCount, 0.0f);
        std::vector<std::vector<float>> segCumLen(
            pointCount, std::vector<float>(ARCLEN_SAMPLES + 1, 0.0f));
        std::vector<std::vector<float>> segTSamples(
            pointCount, std::vector<float>(ARCLEN_SAMPLES + 1, 0.0f));

        for (size_t si = 0; si < pointCount; ++si) {
            segTSamples[si][0] = 0.0f;
            segCumLen[si][0] = 0.0f;
            Pnt3f prev = evalPos(si, 0.0f);
            for (int k = 1; k <= ARCLEN_SAMPLES; ++k) {
                float t = static_cast<float>(k) / ARCLEN_SAMPLES;
                segTSamples[si][k] = t;
                Pnt3f cur = evalPos(si, t);
                Pnt3f d = cur - prev;
                float dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                segCumLen[si][k] = segCumLen[si][k - 1] + dl;
                prev = cur;
            }
            segLen[si] = segCumLen[si][ARCLEN_SAMPLES];
        }

        // Global segment offsets and total length
        std::vector<float> segOffset(pointCount + 1, 0.0f);
        for (size_t i = 0; i < pointCount; ++i)
            segOffset[i + 1] = segOffset[i] + segLen[i];
        float totalLen = segOffset[pointCount];
        if (totalLen <= 1e-6f)
            return;  // degenerate

        // Helper: map global s in [0,totalLen] to (segIdx, t)
        auto sToSegT = [&](float s, size_t& outSegIdx, float& outT) {
            // clamp
            if (s <= 0.0f) {
                outSegIdx = 0;
                outT = 0.0f;
                return;
            }
            if (s >= totalLen) {
                outSegIdx = pointCount - 1;
                outT = 1.0f;
                return;
            }

            // find segment by binary search on segOffset
            size_t lo = 0, hi = pointCount;
            while (lo + 1 < hi) {
                size_t mid = (lo + hi) / 2;
                if (segOffset[mid] <= s)
                    lo = mid;
                else
                    hi = mid;
            }
            outSegIdx = lo;
            float sLocal = s - segOffset[outSegIdx];
            float segTotal = segLen[outSegIdx];
            if (segTotal <= 1e-6f) {
                outT = 0.0f;
                return;
            }

            // within segment: binary search over cumLen to find t
            const auto& cum = segCumLen[outSegIdx];
            const auto& ts = segTSamples[outSegIdx];
            int loI = 0, hiI = ARCLEN_SAMPLES;
            while (loI < hiI) {
                int mid = (loI + hiI) / 2;
                if (cum[mid] < sLocal)
                    loI = mid + 1;
                else
                    hiI = mid;
            }
            int idx = (loI < ARCLEN_SAMPLES ? loI : ARCLEN_SAMPLES);
            int i0 = (idx > 0 ? idx - 1 : 0);
            float s0 = cum[i0], s1 = cum[idx];
            float t0 = ts[i0], t1 = ts[idx];
            float denom = (s1 - s0);
            float a = denom > 1e-6f ? (sLocal - s0) / denom : 0.0f;
            outT = t0 + a * (t1 - t0);
        };

        // Total samples around the loop to match previous density
        const size_t totalSamples =
            pointCount * static_cast<size_t>(DIVIDE_LINE);
        if (totalSamples < 2)
            return;
        size_t tieEvery = totalSamples / (pointCount * 10);
        if (tieEvery < 1)
            tieEvery = 1;

        // March uniformly in s and draw
        size_t segIdx0 = 0;
        float t0 = 0.0f;
        sToSegT(0.0f, segIdx0, t0);
        Pnt3f prev = evalPos(segIdx0, t0);
        for (size_t j = 1; j <= totalSamples; ++j) {
            float sTarget =
                (static_cast<float>(j) / static_cast<float>(totalSamples)) *
                totalLen;
            size_t segIdx;
            float t;
            sToSegT(sTarget, segIdx, t);
            Pnt3f curr = evalPos(segIdx, t);
            Pnt3f curOrient = evalOrient(segIdx, t);
            Pnt3f segDir = curr - prev;
            segDir.normalize();
            Pnt3f cross = segDir * curOrient;
            cross.normalize();
            cross = cross * 2.5f;

            // Rails
            glLineWidth(3);
            glBegin(GL_LINES);
            if (!doingShadows)
                glColor3ub(41, 42, 47);
            glVertex3f(prev.x + cross.x, prev.y + cross.y, prev.z + cross.z);
            glVertex3f(curr.x + cross.x, curr.y + cross.y, curr.z + cross.z);
            glVertex3f(prev.x - cross.x, prev.y - cross.y, prev.z - cross.z);
            glVertex3f(curr.x - cross.x, curr.y - cross.y, curr.z - cross.z);
            glEnd();
            glLineWidth(1);

            // Ties at regular global spacing
            if (j % tieEvery == 0) {
                Pnt3f tieCenter = (prev + curr) * 0.5f;
                Pnt3f halfTieThickness = segDir * 0.75f;
                Pnt3f tieHalfWidth = cross * 1.5f;
                Pnt3f tieRight = tieCenter + tieHalfWidth;
                Pnt3f tieLeft = tieCenter - tieHalfWidth;
                Pnt3f tieFR = tieRight + halfTieThickness;
                Pnt3f tieBR = tieRight - halfTieThickness;
                Pnt3f tieFL = tieLeft + halfTieThickness;
                Pnt3f tieBL = tieLeft - halfTieThickness;
                if (!doingShadows)
                    glColor3ub(94, 72, 44);
                glBegin(GL_QUADS);
                glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                glEnd();
            }

            prev = curr;
        }

        return;  // Done with global arc-length path
    }
    if (tw->splineBrowser->value() == 1) {
        // Linear
        const size_t pointCount = m_pTrack->points.size();
        if (pointCount == 0)
            return;

        // Precompute per-segment lengths and average length (for arc-length
        // sampling)
        float totalSegLen = 0.0f;
        std::vector<float> segLens(pointCount, 0.0f);
        for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const Pnt3f& p0 = m_pTrack->points[pointIndex].pos;
            const Pnt3f& p1 =
                m_pTrack->points[(pointIndex + 1) % pointCount].pos;
            Pnt3f d = p1 - p0;
            float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            segLens[pointIndex] = len;
            totalSegLen += len;
        }
        float avgSegLen = (pointCount > 0)
                              ? (totalSegLen / static_cast<float>(pointCount))
                              : 1.0f;
        if (avgSegLen < 1e-6f)
            avgSegLen = 1.0f;

        for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            Pnt3f p0 = m_pTrack->points[pointIndex].pos;
            Pnt3f p1 = m_pTrack->points[(pointIndex + 1) % pointCount].pos;
            Pnt3f o0 = m_pTrack->points[pointIndex].orient;
            Pnt3f o1 = m_pTrack->points[(pointIndex + 1) % pointCount].orient;

            auto evalPos = [&](float t) {
                return (1.0f - t) * p0 + t * p1;
            };
            auto evalOrient = [&](float t) {
                Pnt3f o = (1.0f - t) * o0 + t * o1;
                o.normalize();
                return o;
            };

            if (!(tw->arcLength->value() == 1)) {
                float step = 1.0f / DIVIDE_LINE;
                float t = 0.0f;
                Pnt3f currentPos = evalPos(t);
                for (size_t i = 0; i < DIVIDE_LINE; ++i) {
                    Pnt3f prev = currentPos;
                    t += step;
                    currentPos = evalPos(t);
                    Pnt3f nextPos = currentPos;
                    Pnt3f curOrient = evalOrient(t);
                    Pnt3f segDir = currentPos - prev;
                    segDir.normalize();
                    Pnt3f cross = segDir * curOrient;
                    cross.normalize();
                    cross = cross * 2.5f;

                    glLineWidth(3);
                    glBegin(GL_LINES);
                    if (!doingShadows)
                        glColor3ub(41, 42, 47);
                    glVertex3f(prev.x + cross.x, prev.y + cross.y,
                               prev.z + cross.z);
                    glVertex3f(currentPos.x + cross.x, currentPos.y + cross.y,
                               currentPos.z + cross.z);
                    glVertex3f(prev.x - cross.x, prev.y - cross.y,
                               prev.z - cross.z);
                    glVertex3f(currentPos.x - cross.x, currentPos.y - cross.y,
                               currentPos.z - cross.z);
                    glEnd();
                    glLineWidth(1);

                    if (i % static_cast<size_t>(DIVIDE_LINE / 10) == 0) {
                        Pnt3f tieCenter = (prev + nextPos) * 0.5f;
                        Pnt3f halfTieThickness = segDir * 0.75f;
                        Pnt3f tieHalfWidth = cross * 1.5f;
                        Pnt3f tieRight = tieCenter + tieHalfWidth;
                        Pnt3f tieLeft = tieCenter - tieHalfWidth;
                        Pnt3f tieFR = tieRight + halfTieThickness;
                        Pnt3f tieBR = tieRight - halfTieThickness;
                        Pnt3f tieFL = tieLeft + halfTieThickness;
                        Pnt3f tieBL = tieLeft - halfTieThickness;
                        if (!doingShadows)
                            glColor3ub(94, 72, 44);
                        glBegin(GL_QUADS);
                        glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                        glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                        glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                        glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                        glEnd();
                    }
                }
            } else {
                // Distribute samples for this straight segment proportional to its
                // length
                float segLen = segLens[pointIndex];
                size_t samplesThisSeg = static_cast<size_t>(
                    std::floor((segLen / avgSegLen) * DIVIDE_LINE + 0.5f));
                if (samplesThisSeg < 1)
                    samplesThisSeg = 1;
                size_t tieEvery = samplesThisSeg / 10;
                if (tieEvery < 1)
                    tieEvery = 1;

                Pnt3f prev = evalPos(0.0f);
                for (size_t i = 0; i < samplesThisSeg; ++i) {
                    // For linear, parameter t is proportional to arc length: t = s
                    float tCurr = static_cast<float>(i + 1) /
                                  static_cast<float>(samplesThisSeg);
                    Pnt3f curr = evalPos(tCurr);
                    Pnt3f curOrient = evalOrient(tCurr);
                    Pnt3f segDir = curr - prev;
                    segDir.normalize();
                    Pnt3f cross = segDir * curOrient;
                    cross.normalize();
                    cross = cross * 2.5f;

                    glLineWidth(3);
                    glBegin(GL_LINES);
                    if (!doingShadows)
                        glColor3ub(41, 42, 47);
                    glVertex3f(prev.x + cross.x, prev.y + cross.y,
                               prev.z + cross.z);
                    glVertex3f(curr.x + cross.x, curr.y + cross.y,
                               curr.z + cross.z);
                    glVertex3f(prev.x - cross.x, prev.y - cross.y,
                               prev.z - cross.z);
                    glVertex3f(curr.x - cross.x, curr.y - cross.y,
                               curr.z - cross.z);
                    glEnd();
                    glLineWidth(1);

                    if (i % tieEvery == 0) {
                        Pnt3f tieCenter = (prev + curr) * 0.5f;
                        Pnt3f halfTieThickness = segDir * 0.75f;
                        Pnt3f tieHalfWidth = cross * 1.5f;
                        Pnt3f tieRight = tieCenter + tieHalfWidth;
                        Pnt3f tieLeft = tieCenter - tieHalfWidth;
                        Pnt3f tieFR = tieRight + halfTieThickness;
                        Pnt3f tieBR = tieRight - halfTieThickness;
                        Pnt3f tieFL = tieLeft + halfTieThickness;
                        Pnt3f tieBL = tieLeft - halfTieThickness;

                        if (!doingShadows)
                            glColor3ub(94, 72, 44);
                        glBegin(GL_QUADS);
                        glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                        glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                        glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                        glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                        glEnd();
                    }

                    prev = curr;
                }
            }
        }
    } else if (tw->splineBrowser->value() == 2) {
        // Cardinal Cubic
        const size_t pointCount = m_pTrack->points.size();
        if (pointCount < 4) {
            return;
        }

        const float tension = 0.0f;  // Catmull-Rom (Cardinal with zero tension)
        const float tensionFactor = (1.0f - tension) * 0.5f;

        auto evaluateCardinal = [tensionFactor](
                                    const Pnt3f& p0, const Pnt3f& p1,
                                    const Pnt3f& p2, const Pnt3f& p3, float t) {
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
            const float h10 = t3 - 2.0f * t2 + t;
            const float h01 = -2.0f * t3 + 3.0f * t2;
            const float h11 = t3 - t2;

            Pnt3f m1 = (p2 - p0) * tensionFactor;
            Pnt3f m2 = (p3 - p1) * tensionFactor;

            Pnt3f term1 = p1 * h00;
            Pnt3f term2 = m1 * h10;
            Pnt3f term3 = p2 * h01;
            Pnt3f term4 = m2 * h11;
            return term1 + term2 + term3 + term4;
        };

        const int ARCLEN_SAMPLES = 100;
        float totalSegLen = 0.0f;
        for (size_t pi = 0; pi < pointCount; ++pi) {
            const ControlPoint& a0 =
                m_pTrack->points[(pi + pointCount - 1) % pointCount];
            const ControlPoint& a1 = m_pTrack->points[pi];
            const ControlPoint& a2 = m_pTrack->points[(pi + 1) % pointCount];
            const ControlPoint& a3 = m_pTrack->points[(pi + 2) % pointCount];
            auto segEval = [&](float t) {
                return evaluateCardinal(a0.pos, a1.pos, a2.pos, a3.pos, t);
            };
            Pnt3f prevS = segEval(0.0f);
            float segLen = 0.0f;
            for (int k = 1; k <= ARCLEN_SAMPLES; ++k) {
                float tt = static_cast<float>(k) / ARCLEN_SAMPLES;
                Pnt3f curS = segEval(tt);
                Pnt3f d = curS - prevS;
                segLen += std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                prevS = curS;
            }
            totalSegLen += segLen;
        }
        float avgSegLen = (pointCount > 0)
                              ? (totalSegLen / static_cast<float>(pointCount))
                              : 1.0f;
        if (avgSegLen < 1e-6f)
            avgSegLen = 1.0f;

        for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const ControlPoint& cp0 =
                m_pTrack->points[(pointIndex + pointCount - 1) % pointCount];
            const ControlPoint& cp1 = m_pTrack->points[pointIndex];
            const ControlPoint& cp2 =
                m_pTrack->points[(pointIndex + 1) % pointCount];
            const ControlPoint& cp3 =
                m_pTrack->points[(pointIndex + 2) % pointCount];

            auto evalPos = [&](float t) {
                return evaluateCardinal(cp0.pos, cp1.pos, cp2.pos, cp3.pos, t);
            };
            auto evalOrient = [&](float t) {
                Pnt3f o = evaluateCardinal(cp0.orient, cp1.orient, cp2.orient,
                                           cp3.orient, t);
                o.normalize();
                return o;
            };

            if (!(tw->arcLength->value() == 1)) {
                float step = 1.0f / DIVIDE_LINE;
                float t = 0.0f;
                Pnt3f prev = evalPos(0.0f);
                for (size_t i = 0; i < DIVIDE_LINE; ++i) {
                    t += step;
                    if (i + 1 == static_cast<size_t>(DIVIDE_LINE))
                        t = 1.0f;
                    Pnt3f curr = evalPos(t);
                    Pnt3f segDir = curr - prev;
                    segDir.normalize();
                    Pnt3f curOrient = evalOrient(t);
                    Pnt3f cross = segDir * curOrient;
                    cross.normalize();
                    cross = cross * 2.5f;

                    glLineWidth(3);
                    glBegin(GL_LINES);
                    if (!doingShadows)
                        glColor3ub(41, 42, 47);
                    glVertex3f(prev.x + cross.x, prev.y + cross.y,
                               prev.z + cross.z);
                    glVertex3f(curr.x + cross.x, curr.y + cross.y,
                               curr.z + cross.z);
                    glVertex3f(prev.x - cross.x, prev.y - cross.y,
                               prev.z - cross.z);
                    glVertex3f(curr.x - cross.x, curr.y - cross.y,
                               curr.z - cross.z);
                    glEnd();
                    glLineWidth(1);

                    if (i % static_cast<size_t>(DIVIDE_LINE / 10) == 0) {
                        Pnt3f tieCenter = (prev + curr) * 0.5f;
                        Pnt3f halfTieThickness = segDir * 0.75f;
                        Pnt3f tieHalfWidth = cross * 1.5f;
                        Pnt3f tieRight = tieCenter + tieHalfWidth;
                        Pnt3f tieLeft = tieCenter - tieHalfWidth;
                        Pnt3f tieFR = tieRight + halfTieThickness;
                        Pnt3f tieBR = tieRight - halfTieThickness;
                        Pnt3f tieFL = tieLeft + halfTieThickness;
                        Pnt3f tieBL = tieLeft - halfTieThickness;
                        if (!doingShadows)
                            glColor3ub(94, 72, 44);
                        glBegin(GL_QUADS);
                        glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                        glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                        glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                        glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                        glEnd();
                    }

                    prev = curr;
                }
            } else {
                float cumLen[ARCLEN_SAMPLES + 1];
                float tSamples[ARCLEN_SAMPLES + 1];
                cumLen[0] = 0.0f;
                tSamples[0] = 0.0f;
                Pnt3f prevS = evalPos(0.0f);
                for (int k = 1; k <= ARCLEN_SAMPLES; ++k) {
                    float t = static_cast<float>(k) / ARCLEN_SAMPLES;
                    tSamples[k] = t;
                    Pnt3f curS = evalPos(t);
                    Pnt3f d = curS - prevS;
                    float dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                    cumLen[k] = cumLen[k - 1] + dl;
                    prevS = curS;
                }
                float segLen = cumLen[ARCLEN_SAMPLES];
                size_t samplesThisSeg = static_cast<size_t>(
                    std::floor((segLen / avgSegLen) * DIVIDE_LINE + 0.5f));
                if (samplesThisSeg < 1)
                    samplesThisSeg = 1;
                size_t tieEvery = samplesThisSeg / 10;
                if (tieEvery < 1)
                    tieEvery = 1;

                float totalLen = segLen;
                auto sToT = [&](float s01) {
                    if (totalLen <= 1e-6f)
                        return s01;
                    float target = s01 * totalLen;
                    int lo = 0, hi = ARCLEN_SAMPLES;
                    while (lo < hi) {
                        int mid = (lo + hi) / 2;
                        if (cumLen[mid] < target)
                            lo = mid + 1;
                        else
                            hi = mid;
                    }
                    int idx = (lo < ARCLEN_SAMPLES ? lo : ARCLEN_SAMPLES);
                    int i0 = (idx > 0 ? idx - 1 : 0);
                    float s0 = cumLen[i0], s1 = cumLen[idx];
                    float t0 = tSamples[i0], t1 = tSamples[idx];
                    float denom = (s1 - s0);
                    float a = denom > 1e-6f ? (target - s0) / denom : 0.0f;
                    return t0 + a * (t1 - t0);
                };

                Pnt3f prev = evalPos(0.0f);
                for (size_t i = 0; i < samplesThisSeg; ++i) {
                    float sPrev = static_cast<float>(i) / samplesThisSeg;
                    float sCurr = static_cast<float>(i + 1) / samplesThisSeg;
                    float tPrev = sToT(sPrev);
                    float tCurr = sToT(sCurr);
                    Pnt3f curr = evalPos(tCurr);
                    Pnt3f segDir = curr - prev;
                    segDir.normalize();
                    Pnt3f curOrient = evalOrient(tCurr);
                    Pnt3f cross = segDir * curOrient;
                    cross.normalize();
                    cross = cross * 2.5f;

                    glLineWidth(3);
                    glBegin(GL_LINES);
                    if (!doingShadows)
                        glColor3ub(41, 42, 47);
                    glVertex3f(prev.x + cross.x, prev.y + cross.y,
                               prev.z + cross.z);
                    glVertex3f(curr.x + cross.x, curr.y + cross.y,
                               curr.z + cross.z);
                    glVertex3f(prev.x - cross.x, prev.y - cross.y,
                               prev.z - cross.z);
                    glVertex3f(curr.x - cross.x, curr.y - cross.y,
                               curr.z - cross.z);
                    glEnd();
                    glLineWidth(1);

                    if (i % tieEvery == 0) {
                        Pnt3f tieCenter = (prev + curr) * 0.5f;
                        Pnt3f halfTieThickness = segDir * 0.75f;
                        Pnt3f tieHalfWidth = cross * 1.5f;
                        Pnt3f tieRight = tieCenter + tieHalfWidth;
                        Pnt3f tieLeft = tieCenter - tieHalfWidth;
                        Pnt3f tieFR = tieRight + halfTieThickness;
                        Pnt3f tieBR = tieRight - halfTieThickness;
                        Pnt3f tieFL = tieLeft + halfTieThickness;
                        Pnt3f tieBL = tieLeft - halfTieThickness;
                        if (!doingShadows)
                            glColor3ub(94, 72, 44);
                        glBegin(GL_QUADS);
                        glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                        glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                        glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                        glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                        glEnd();
                    }

                    prev = curr;
                }
            }
        }

    } else {
        // B-Spline
        const size_t pointCount = m_pTrack->points.size();
        if (pointCount < 4) {
            return;
        }

        auto evaluateBSpline = [](const Pnt3f& p0, const Pnt3f& p1,
                                  const Pnt3f& p2, const Pnt3f& p3, float t) {
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float sixth = 1.0f / 6.0f;
            const float b0 = (-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * sixth;
            const float b1 = (3.0f * t3 - 6.0f * t2 + 4.0f) * sixth;
            const float b2 = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * sixth;
            const float b3 = t3 * sixth;
            return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
        };

        const int ARCLEN_SAMPLES = 100;
        float totalSegLen = 0.0f;
        for (size_t pi = 0; pi < pointCount; ++pi) {
            const ControlPoint& a0 =
                m_pTrack->points[(pi + pointCount - 1) % pointCount];
            const ControlPoint& a1 = m_pTrack->points[pi];
            const ControlPoint& a2 = m_pTrack->points[(pi + 1) % pointCount];
            const ControlPoint& a3 = m_pTrack->points[(pi + 2) % pointCount];
            auto segEval = [&](float t) {
                return evaluateBSpline(a0.pos, a1.pos, a2.pos, a3.pos, t);
            };
            Pnt3f prevS = segEval(0.0f);
            float segLen = 0.0f;
            for (int k = 1; k <= ARCLEN_SAMPLES; ++k) {
                float tt = static_cast<float>(k) / ARCLEN_SAMPLES;
                Pnt3f curS = segEval(tt);
                Pnt3f d = curS - prevS;
                segLen += std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                prevS = curS;
            }
            totalSegLen += segLen;
        }
        float avgSegLen = (pointCount > 0)
                              ? (totalSegLen / static_cast<float>(pointCount))
                              : 1.0f;
        if (avgSegLen < 1e-6f)
            avgSegLen = 1.0f;

        for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const ControlPoint& cp0 =
                m_pTrack->points[(pointIndex + pointCount - 1) % pointCount];
            const ControlPoint& cp1 = m_pTrack->points[pointIndex];
            const ControlPoint& cp2 =
                m_pTrack->points[(pointIndex + 1) % pointCount];
            const ControlPoint& cp3 =
                m_pTrack->points[(pointIndex + 2) % pointCount];

            auto evalPos = [&](float t) {
                return evaluateBSpline(cp0.pos, cp1.pos, cp2.pos, cp3.pos, t);
            };
            auto evalOrient = [&](float t) {
                Pnt3f o = evaluateBSpline(cp0.orient, cp1.orient, cp2.orient,
                                          cp3.orient, t);
                o.normalize();
                return o;
            };

            if (!(tw->arcLength->value() == 1)) {
                float step = 1.0f / DIVIDE_LINE;
                float t = 0.0f;
                Pnt3f prev = evalPos(0.0f);
                for (size_t i = 0; i < DIVIDE_LINE; ++i) {
                    t += step;
                    if (i + 1 == static_cast<size_t>(DIVIDE_LINE))
                        t = 1.0f;
                    Pnt3f curr = evalPos(t);
                    Pnt3f segDir = curr - prev;
                    float segLenSq = segDir.x * segDir.x + segDir.y * segDir.y +
                                     segDir.z * segDir.z;
                    if (segLenSq > 1e-6f) {
                        segDir.normalize();
                        Pnt3f curOrient = evalOrient(t);
                        Pnt3f cross = segDir * curOrient;
                        float cLenSq = cross.x * cross.x + cross.y * cross.y +
                                       cross.z * cross.z;
                        if (cLenSq > 1e-6f) {
                            cross.normalize();
                            cross = cross * 2.5f;
                            glLineWidth(3);
                            glBegin(GL_LINES);
                            if (!doingShadows)
                                glColor3ub(41, 42, 47);
                            glVertex3f(prev.x + cross.x, prev.y + cross.y,
                                       prev.z + cross.z);
                            glVertex3f(curr.x + cross.x, curr.y + cross.y,
                                       curr.z + cross.z);
                            glVertex3f(prev.x - cross.x, prev.y - cross.y,
                                       prev.z - cross.z);
                            glVertex3f(curr.x - cross.x, curr.y - cross.y,
                                       curr.z - cross.z);
                            glEnd();
                            glLineWidth(1);

                            if (i % static_cast<size_t>(DIVIDE_LINE / 10) ==
                                0) {
                                Pnt3f tieCenter = (prev + curr) * 0.5f;
                                Pnt3f halfTieThickness = segDir * 0.75f;
                                Pnt3f tieHalfWidth = cross * 1.5f;
                                Pnt3f tieRight = tieCenter + tieHalfWidth;
                                Pnt3f tieLeft = tieCenter - tieHalfWidth;
                                Pnt3f tieFR = tieRight + halfTieThickness;
                                Pnt3f tieBR = tieRight - halfTieThickness;
                                Pnt3f tieFL = tieLeft + halfTieThickness;
                                Pnt3f tieBL = tieLeft - halfTieThickness;
                                if (!doingShadows)
                                    glColor3ub(94, 72, 44);
                                glBegin(GL_QUADS);
                                glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                                glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                                glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                                glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                                glEnd();
                            }
                        }
                    }
                    prev = curr;
                }
            } else {
                float cumLen[ARCLEN_SAMPLES + 1];
                float tSamples[ARCLEN_SAMPLES + 1];
                cumLen[0] = 0.0f;
                tSamples[0] = 0.0f;
                Pnt3f prevS = evalPos(0.0f);
                for (int k = 1; k <= ARCLEN_SAMPLES; ++k) {
                    float t = static_cast<float>(k) / ARCLEN_SAMPLES;
                    tSamples[k] = t;
                    Pnt3f curS = evalPos(t);
                    Pnt3f d = curS - prevS;
                    float dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                    cumLen[k] = cumLen[k - 1] + dl;
                    prevS = curS;
                }
                float segLen = cumLen[ARCLEN_SAMPLES];
                size_t samplesThisSeg = static_cast<size_t>(
                    std::floor((segLen / avgSegLen) * DIVIDE_LINE + 0.5f));
                if (samplesThisSeg < 1)
                    samplesThisSeg = 1;
                size_t tieEvery = samplesThisSeg / 10;
                if (tieEvery < 1)
                    tieEvery = 1;

                float totalLen = segLen;
                auto sToT = [&](float s01) {
                    if (totalLen <= 1e-6f)
                        return s01;
                    float target = s01 * totalLen;
                    int lo = 0, hi = ARCLEN_SAMPLES;
                    while (lo < hi) {
                        int mid = (lo + hi) / 2;
                        if (cumLen[mid] < target)
                            lo = mid + 1;
                        else
                            hi = mid;
                    }
                    int idx = (lo < ARCLEN_SAMPLES ? lo : ARCLEN_SAMPLES);
                    int i0 = (idx > 0 ? idx - 1 : 0);
                    float s0 = cumLen[i0], s1 = cumLen[idx];
                    float t0 = tSamples[i0], t1 = tSamples[idx];
                    float denom = (s1 - s0);
                    float a = denom > 1e-6f ? (target - s0) / denom : 0.0f;
                    return t0 + a * (t1 - t0);
                };

                Pnt3f prev = evalPos(0.0f);
                for (size_t i = 0; i < samplesThisSeg; ++i) {
                    float sPrev = static_cast<float>(i) / samplesThisSeg;
                    float sCurr = static_cast<float>(i + 1) / samplesThisSeg;
                    float tPrev = sToT(sPrev);
                    float tCurr = sToT(sCurr);
                    Pnt3f curr = evalPos(tCurr);
                    Pnt3f segDir = curr - prev;
                    float segLenSq = segDir.x * segDir.x + segDir.y * segDir.y +
                                     segDir.z * segDir.z;
                    if (segLenSq > 1e-6f) {
                        segDir.normalize();
                        Pnt3f curOrient = evalOrient(tCurr);
                        Pnt3f cross = segDir * curOrient;
                        float cLenSq = cross.x * cross.x + cross.y * cross.y +
                                       cross.z * cross.z;
                        if (cLenSq > 1e-6f) {
                            cross.normalize();
                            cross = cross * 2.5f;
                            glLineWidth(3);
                            glBegin(GL_LINES);
                            if (!doingShadows)
                                glColor3ub(41, 42, 47);
                            glVertex3f(prev.x + cross.x, prev.y + cross.y,
                                       prev.z + cross.z);
                            glVertex3f(curr.x + cross.x, curr.y + cross.y,
                                       curr.z + cross.z);
                            glVertex3f(prev.x - cross.x, prev.y - cross.y,
                                       prev.z - cross.z);
                            glVertex3f(curr.x - cross.x, curr.y - cross.y,
                                       curr.z - cross.z);
                            glEnd();
                            glLineWidth(1);

                            if (i % tieEvery == 0) {
                                Pnt3f tieCenter = (prev + curr) * 0.5f;
                                Pnt3f halfTieThickness = segDir * 0.75f;
                                Pnt3f tieHalfWidth = cross * 1.5f;
                                Pnt3f tieRight = tieCenter + tieHalfWidth;
                                Pnt3f tieLeft = tieCenter - tieHalfWidth;
                                Pnt3f tieFR = tieRight + halfTieThickness;
                                Pnt3f tieBR = tieRight - halfTieThickness;
                                Pnt3f tieFL = tieLeft + halfTieThickness;
                                Pnt3f tieBL = tieLeft - halfTieThickness;
                                if (!doingShadows)
                                    glColor3ub(94, 72, 44);
                                glBegin(GL_QUADS);
                                glVertex3f(tieFL.x, tieFL.y, tieFL.z);
                                glVertex3f(tieFR.x, tieFR.y, tieFR.z);
                                glVertex3f(tieBR.x, tieBR.y, tieBR.z);
                                glVertex3f(tieBL.x, tieBL.y, tieBL.z);
                                glEnd();
                            }
                        }
                    }
                    prev = curr;
                }
            }
        }
    }
}

void TrainView::drawTrain(bool doingShadows) {
    const size_t pointCount = m_pTrack->points.size();
    if (pointCount < 4) {
        return;
    }

    float rawParam = m_pTrack->trainU;
    if (rawParam < 0.0f) {
        rawParam = std::fmod(rawParam, static_cast<float>(pointCount)) +
                   static_cast<float>(pointCount);
    }
    float wrappedParam = std::fmod(rawParam, static_cast<float>(pointCount));
    if (wrappedParam < 0.0f) {
        wrappedParam += static_cast<float>(pointCount);
    }

    const size_t baseIndex =
        static_cast<size_t>(std::floor(wrappedParam)) % pointCount;
    float localT = wrappedParam - std::floor(wrappedParam);

    auto wrapIndex = [pointCount](int index) {
        int wrapped = index % static_cast<int>(pointCount);
        if (wrapped < 0) {
            wrapped += static_cast<int>(pointCount);
        }
        return static_cast<size_t>(wrapped);
    };

    Pnt3f position;
    Pnt3f tangent;
    Pnt3f up;

    if (tw->splineBrowser->value() == 1) {
        const ControlPoint& cp1 = m_pTrack->points[baseIndex];
        const ControlPoint& cp2 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) + 1)];

        position = (1.0f - localT) * cp1.pos + localT * cp2.pos;
        up = (1.0f - localT) * cp1.orient + localT * cp2.orient;
        up.normalize();

        tangent = cp2.pos - cp1.pos;
        float tangentLenSq = tangent.x * tangent.x + tangent.y * tangent.y +
                             tangent.z * tangent.z;
        if (tangentLenSq > 1e-6f) {
            tangent.normalize();
        } else {
            tangent = Pnt3f(0.0f, 0.0f, 1.0f);
        }
    } else if (tw->splineBrowser->value() == 2) {
        const ControlPoint& cp0 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) - 1)];
        const ControlPoint& cp1 = m_pTrack->points[baseIndex];
        const ControlPoint& cp2 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) + 1)];
        const ControlPoint& cp3 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) + 2)];

        const float tension = 0.0f;
        const float tensionFactor = (1.0f - tension) * 0.5f;
        const float t2 = localT * localT;
        const float t3 = t2 * localT;

        Pnt3f m1 = (cp2.pos - cp0.pos) * tensionFactor;
        Pnt3f m2 = (cp3.pos - cp1.pos) * tensionFactor;

        const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        const float h10 = t3 - 2.0f * t2 + localT;
        const float h01 = -2.0f * t3 + 3.0f * t2;
        const float h11 = t3 - t2;

        position = cp1.pos * h00 + m1 * h10 + cp2.pos * h01 + m2 * h11;

        Pnt3f orientM1 = (cp2.orient - cp0.orient) * tensionFactor;
        Pnt3f orientM2 = (cp3.orient - cp1.orient) * tensionFactor;
        up = cp1.orient * h00 + orientM1 * h10 + cp2.orient * h01 +
             orientM2 * h11;
        up.normalize();

        const float dh00 = 6.0f * t2 - 6.0f * localT;
        const float dh10 = 3.0f * t2 - 4.0f * localT + 1.0f;
        const float dh01 = -6.0f * t2 + 6.0f * localT;
        const float dh11 = 3.0f * t2 - 2.0f * localT;
        tangent = cp1.pos * dh00 + m1 * dh10 + cp2.pos * dh01 + m2 * dh11;
        float tangentLenSq = tangent.x * tangent.x + tangent.y * tangent.y +
                             tangent.z * tangent.z;
        if (tangentLenSq > 1e-6f) {
            tangent.normalize();
        } else {
            tangent = Pnt3f(0.0f, 0.0f, 1.0f);
        }
    } else {
        const ControlPoint& cp0 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) - 1)];
        const ControlPoint& cp1 = m_pTrack->points[baseIndex];
        const ControlPoint& cp2 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) + 1)];
        const ControlPoint& cp3 =
            m_pTrack->points[wrapIndex(static_cast<int>(baseIndex) + 2)];

        const float t2 = localT * localT;
        const float t3 = t2 * localT;
        const float sixth = 1.0f / 6.0f;
        const float b0 = (-t3 + 3.0f * t2 - 3.0f * localT + 1.0f) * sixth;
        const float b1 = (3.0f * t3 - 6.0f * t2 + 4.0f) * sixth;
        const float b2 =
            (-3.0f * t3 + 3.0f * t2 + 3.0f * localT + 1.0f) * sixth;
        const float b3 = t3 * sixth;

        position = cp0.pos * b0 + cp1.pos * b1 + cp2.pos * b2 + cp3.pos * b3;
        up = cp0.orient * b0 + cp1.orient * b1 + cp2.orient * b2 +
             cp3.orient * b3;
        up.normalize();

        const float db0 = (-3.0f * t2 + 6.0f * localT - 3.0f) * sixth;
        const float db1 = (9.0f * t2 - 12.0f * localT) * sixth;
        const float db2 = (-9.0f * t2 + 6.0f * localT + 3.0f) * sixth;
        const float db3 = (3.0f * t2) * sixth;
        tangent = cp0.pos * db0 + cp1.pos * db1 + cp2.pos * db2 + cp3.pos * db3;
        float tangentLenSq = tangent.x * tangent.x + tangent.y * tangent.y +
                             tangent.z * tangent.z;
        if (tangentLenSq > 1e-6f) {
            tangent.normalize();
        } else {
            tangent = Pnt3f(0.0f, 0.0f, 1.0f);
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

    const float halfExtent = 5.0f;
    Pnt3f halfForward = tangent * halfExtent;
    Pnt3f halfRight = right * halfExtent;
    Pnt3f halfUp = up * halfExtent;

    Pnt3f center = position + halfUp;

    Pnt3f frontTopRight = center + halfForward + halfRight + halfUp;
    Pnt3f frontTopLeft = center + halfForward - halfRight + halfUp;
    Pnt3f frontBottomRight = center + halfForward + halfRight - halfUp;
    Pnt3f frontBottomLeft = center + halfForward - halfRight - halfUp;
    Pnt3f backTopRight = center - halfForward + halfRight + halfUp;
    Pnt3f backTopLeft = center - halfForward - halfRight + halfUp;
    Pnt3f backBottomRight = center - halfForward + halfRight - halfUp;
    Pnt3f backBottomLeft = center - halfForward - halfRight - halfUp;

    if (!doingShadows) {
        // set train color
        // glColor3ub(200, 40, 40);
        glColor3ub(255, 255, 255);
    }

    glBegin(GL_QUADS);
    // Front face
    if (!doingShadows) {
        glColor3ub(89, 110, 57);
    }  // front face colored
    glNormal3f(tangent.x, tangent.y, tangent.z);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(frontBottomLeft.x, frontBottomLeft.y, frontBottomLeft.z);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(frontBottomRight.x, frontBottomRight.y, frontBottomRight.z);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);

    // Back face
    if (!doingShadows) {
        glColor3ub(255, 255, 255);
    }  // other faces default
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
    if (!doingShadows) {
        glColor3ub(255, 255, 255);
    }
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
    if (!doingShadows) {
        glColor3ub(255, 255, 255);
    }
    glNormal3f(right.x, right.y, right.z);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(frontBottomRight.x, frontBottomRight.y, frontBottomRight.z);
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
    glVertex3f(frontBottomRight.x, frontBottomRight.y, frontBottomRight.z);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(frontBottomLeft.x, frontBottomLeft.y, frontBottomLeft.z);
    glEnd();
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