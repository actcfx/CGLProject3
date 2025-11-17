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
#define GUAGE 5.0f

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

void TrainView::updateWave(const int& size) {
    if (!this->plane)
        return;

    const int division = 100;
    const float SIZE = 10.0f;

    // Define waves with same parameters as initialization
    struct Wave {
        glm::vec2 direction;
        float wavelength;
        float amplitude;
        float speed;
        float frequency;
    };

    std::vector<Wave> waves = { { { 1.0f, 0.0f }, 2.0f, 0.10f, 1.0f },
                                { { 0.7f, 0.7f }, 3.0f, 0.05f, 0.8f },
                                { { -0.6f, 0.8f }, 1.5f, 0.07f, 1.2f } };

    for (auto& wave : waves) {
        wave.frequency = 2.0f * M_PI / wave.wavelength;
    }

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> colors;

    // Recalculate vertex positions with time offset
    float half = SIZE / 2.0f;
    for (int j = 0; j <= division; ++j) {
        for (int i = 0; i <= division; ++i) {
            float x = -half + SIZE * (float)i / division;
            float z = -half + SIZE * (float)j / division;

            // Calculate height using wave functions with time offset
            float h = 0.0f;
            glm::vec3 normal(0.0f, 1.0f, 0.0f);

            for (const auto& w : waves) {
                // Normalize direction
                glm::vec2 dir = glm::normalize(w.direction);
                float dot = glm::dot(dir, glm::vec2(x, z));

                // Apply time-based phase shift for animation
                float phase = dot * w.frequency - waveTime * w.speed;
                h += w.amplitude * sin(phase);

                // Calculate normal gradient for better lighting
                float cosPhase = cos(phase);
                normal.x -= w.amplitude * w.frequency * dir.x * cosPhase;
                normal.z -= w.amplitude * w.frequency * dir.y * cosPhase;
            }

            normal = glm::normalize(normal);

            // Update vertex position
            vertices.push_back(x);
            vertices.push_back(h);
            vertices.push_back(z);

            // Update normals
            normals.push_back(normal.x);
            normals.push_back(normal.y);
            normals.push_back(normal.z);

            // Water color based on slope (foam on steep areas)
            float slope = glm::clamp(1.0f - normal.y, 0.0f, 1.0f);
            float foam = glm::smoothstep(0.25f, 0.8f, slope);
            glm::vec3 baseWater(0.02f, 0.30f, 0.45f);  // deep teal
            glm::vec3 foamColor(0.85f, 0.90f, 0.95f);  // light foam
            glm::vec3 finalColor = baseWater * (0.85f + 0.15f * normal.y);
            finalColor = glm::mix(finalColor, foamColor, foam * 0.6f);

            colors.push_back(finalColor.r);
            colors.push_back(finalColor.g);
            colors.push_back(finalColor.b);
        }
    }

    // Update VBO data
    glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat),
                    vertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(GLfloat),
                    normals.data());

    glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(GLfloat),
                    colors.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw() {
    const int division = 100;  // Number of subdivisions
    const float SIZE = 10.0f;  // Plane size

    struct Wave {
        glm::vec2 direction;
        float wavelength;
        float amplitude;
        float speed;
        float frequency;
    };

    std::vector<Wave> waves = { { { 1.0f, 0.0f }, 2.0f, 0.10f, 1.0f },
                                { { 0.7f, 0.7f }, 3.0f, 0.05f, 0.8f },
                                { { -0.6f, 0.8f }, 1.5f, 0.07f, 1.2f } };

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
            for (auto& wave : waves) {
                wave.frequency = 2.0f * M_PI / wave.wavelength;
            }

            std::vector<GLfloat> vertices;
            std::vector<GLfloat> normals;
            std::vector<GLfloat> textureCoordinates;
            std::vector<GLfloat> colors;
            std::vector<GLuint> elements;

            // Generate vertex data (initial flat plane)
            float half = SIZE / 2.0f;
            for (int j = 0; j <= division; ++j) {
                for (int i = 0; i <= division; ++i) {
                    float x = -half + SIZE * (float)i / division;
                    float z = -half + SIZE * (float)j / division;

                    // Start with flat surface - waves will be updated dynamically
                    float h = 0.0f;

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

                    // Water-like base tint (teal)
                    colors.push_back(0.02f);  // R
                    colors.push_back(0.30f);  // G
                    colors.push_back(0.45f);  // B
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

            // Vertex position (use GL_DYNAMIC_DRAW for animated data)
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                         vertices.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(0);

            // Normals (use GL_DYNAMIC_DRAW for animated data)
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                         normals.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(1);

            // Texture coordinates
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
            glBufferData(GL_ARRAY_BUFFER,
                         textureCoordinates.size() * sizeof(GLfloat),
                         textureCoordinates.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                  (GLvoid*)0);
            glEnableVertexAttribArray(2);

            // Colors (use GL_DYNAMIC_DRAW for animated data)
            glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
            glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
                         colors.data(), GL_DYNAMIC_DRAW);
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

        // Triangle
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

    waveTime += 0.016f;  // Update wave animation time (approximately 60 FPS)
    updateWave(100);     // Update wave vertices with new time

    this->shader->Use();  // Bind shader

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
    const size_t pointCount = m_pTrack->points.size();
    if (pointCount < 2)
        return;

    float M[4][4] = { 0 };

    if (tw->splineBrowser->value() == 1) {
        // Linear
        float linearM[4][4] = { { -1.0f, 1.0f, 0.0f, 0.0f },
                                { 1.0f, 0.0f, 0.0f, 0.0f },
                                { 0.0f, 0.0f, 0.0f, 0.0f },
                                { 0.0f, 0.0f, 0.0f, 0.0f } };
        memcpy(M, linearM, sizeof(linearM));

    } else if (tw->splineBrowser->value() == 2) {
        // Cardinal Cubic
        float cardinalM[4][4] = { { -0.5f, 1.0f, -0.5f, 0.0f },
                                  { 1.5f, -2.5f, 0.0f, 1.0f },
                                  { -1.5f, 2.0f, 0.5f, 0.0f },
                                  { 0.5f, -0.5f, 0.0f, 0.0f } };
        memcpy(M, cardinalM, sizeof(cardinalM));
    } else {
        // B-Spline
        float bSplineM[4][4] = { { -1.0f / 6.0f, 0.5f, -0.5f, 1.0f / 6.0f },
                                 { 0.5f, -1.0f, 0.0f, 2.0f / 3.0f },
                                 { -0.5f, 0.5f, 0.5f, 1.0f / 6.0f },
                                 { 1.0f / 6.0f, 0.0f, 0.0f, 0.0f } };
        memcpy(M, bSplineM, sizeof(bSplineM));
    }

    if (!doingShadows)
        glColor3ub(200, 200, 200);

    glLineWidth(8.0f);

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

    glLineWidth(1.0f);
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