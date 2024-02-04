#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDateTime>
#include <QDebug>

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
    m_worldAxes(this),
    m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progPost(this), m_progSky(this),
    m_frameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
    m_postQuad(this), m_terrain(this), m_player(glm::vec3(48.f, 200.f, 48.f), m_terrain), m_inventoryIsOpen(false)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    // Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create the instance of the post quad
    m_postQuad.createVBOdata();

    // Create the instance of the frame buffer
    m_frameBuffer.create();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    // Create and set up the post effect shader
    m_progPost.create(":/glsl/post.vert.glsl", ":/glsl/post.frag.glsl");
    // Create and set up the sky shader
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    m_terrain.instantiateTexture();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)
    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);

    m_progSky.setViewProjMatrix(glm::inverse(viewproj));
    m_progSky.setDimensions(this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    m_progSky.setEye(m_player.mcr_camera.mcr_position);

    // glViewport(0, 0, w * this->devicePixelRatio(), h * this->devicePixelRatio());
    m_frameBuffer.resize(w, h, this->devicePixelRatio());
    m_frameBuffer.destroy();
    m_frameBuffer.create();

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    float dT = (QDateTime::currentMSecsSinceEpoch() - currentMSecsSinceEpoch) / 1000.f;
//    std::cout << "curtime is : " << curTime << std::endl;
//    std::cout << "currentMSecsSinceEpoch is : " << currentMSecsSinceEpoch << std::endl;
//    std::cout << "dt is :" << dT << std::endl;
    m_player.tick(dT, m_inputs);
    currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // paint to the frame buffer -----------------------------------------------
    m_frameBuffer.bindFrameBuffer();
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setModelMatrix(glm::mat4());
    m_progLambert.setTime(QDateTime::currentMSecsSinceEpoch());
    m_progPost.setTime(QDateTime::currentMSecsSinceEpoch());

    // Set the inverse of view-project matrix for mapping pixels back to world points
    m_progSky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    m_progSky.setTime(QDateTime::currentMSecsSinceEpoch());
    m_progSky.setEye(m_player.mcr_camera.mcr_position);
    m_progSky.draw(m_postQuad);

    m_terrain.expandChunks(m_player.mcr_position, m_player.m_prevPos);
    renderTerrain();

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);

    // post processing ---------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_frameBuffer.bindToTextureSlot(2);
    if (m_player.getHeadBlockType(m_terrain) == WATER) {
        m_progPost.setPostType(1);
    } else if (m_player.getHeadBlockType(m_terrain) == LAVA) {
        m_progPost.setPostType(2);
    } else {
        m_progPost.setPostType(0);
    }
    m_progPost.drawPostEffected(m_postQuad);
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    int x = 16 * static_cast<int>(glm::floor(m_player.mcr_position.x / 16.f));
    int z = 16 * static_cast<int>(glm::floor(m_player.mcr_position.z / 16.f));
    m_terrain.draw(x - 128, x + 128, z - 128, z + 128, &m_progLambert);
}


void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_F) {
        m_player.m_flightMode = !m_player.m_flightMode;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_F) {
//        m_player.m_flightMode = !m_player.m_flightMode;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    } else if (e->key() == Qt::Key_I) {
        m_inventoryIsOpen = !m_inventoryIsOpen;
        std::cout << "before sig_sendinventory" << std::endl;
        emit sig_sendInventory(m_inventoryIsOpen);
    }
}


void MyGL::mouseMoveEvent(QMouseEvent *e) {
    float dx = (this->width() * 0.5 - e->pos().x()) / width();
    float dy = (this->height() * 0.5 - e->pos().y()) / height();
    m_player.rotateOnUpGlobal(dx * 360 * 0.05f);
    m_player.rotateOnRightLocal(dy * 360 * 0.05f);
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    // TODO
    if (e->button() == Qt::LeftButton) {
        BlockType t = m_player.removeBlock(&m_terrain);
        emit sig_updateInventory(t, 1);

    } else if (e->button() == Qt::RightButton) {
//        if ()
        m_player.addBlock(&m_terrain);
        BlockType t = m_player.selectedBlockType;
        emit sig_updateInventory(t, -1);
    }
}

Player MyGL::getPlayer() {
    return this->m_player;
}

void MyGL::setCurBlockType(BlockType t) {
    this->m_player.selectedBlockType = t;
}
