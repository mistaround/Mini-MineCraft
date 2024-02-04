#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : m_count(-1), m_bufIdx(), m_bufPos(), m_bufNor(), m_bufCol(), m_bufUV(),
    m_idxGenerated(false), m_posGenerated(false), m_norGenerated(false), m_colGenerated(false), m_uvGenerated(false),
      mp_context(context)
{}

Drawable::~Drawable()
{}

void Drawable::destroyVBOdata()
{
    mp_context->glDeleteBuffers(1, &m_bufIdx);
    mp_context->glDeleteBuffers(1, &m_bufPos);
    mp_context->glDeleteBuffers(1, &m_bufNor);
    mp_context->glDeleteBuffers(1, &m_bufCol);
    m_idxGenerated = m_posGenerated = m_norGenerated = m_colGenerated = false;
    m_count = -1;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount()
{
    return m_count;
}

void Drawable::generateIdx()
{
    m_idxGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdx);
}

void Drawable::generatePos()
{
    m_posGenerated = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufPos);
}

void Drawable::generateNor()
{
    m_norGenerated = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufNor);
}

void Drawable::generateCol()
{
    m_colGenerated = true;
    // Create a VBO on our GPU and store its handle in bufCol
    mp_context->glGenBuffers(1, &m_bufCol);
}


void Drawable::generateUV()
{
    m_uvGenerated = true;
    // Create a VBO on our GPU and store its handle in bufUV
    mp_context->glGenBuffers(1, &m_bufUV);
}

bool Drawable::bindIdx()
{
    if(m_idxGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    }
    return m_idxGenerated;
}

bool Drawable::bindPos()
{
    if(m_posGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    }
    return m_posGenerated;
}

bool Drawable::bindNor()
{
    if(m_norGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor);
    }
    return m_norGenerated;
}

bool Drawable::bindCol()
{
    if(m_colGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufCol);
    }
    return m_colGenerated;
}

bool Drawable::bindUV()
{
    if(m_uvGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    }
    return m_uvGenerated;
}

InstancedDrawable::InstancedDrawable(OpenGLContext *context)
    : Drawable(context), m_numInstances(0), m_bufPosOffset(-1), m_offsetGenerated(false)
{}

InstancedDrawable::~InstancedDrawable(){}

int InstancedDrawable::instanceCount() const {
    return m_numInstances;
}

void InstancedDrawable::generateOffsetBuf() {
    m_offsetGenerated = true;
    mp_context->glGenBuffers(1, &m_bufPosOffset);
}

bool InstancedDrawable::bindOffsetBuf() {
    if(m_offsetGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOffset);
    }
    return m_offsetGenerated;
}


void InstancedDrawable::clearOffsetBuf() {
    if(m_offsetGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufPosOffset);
        m_offsetGenerated = false;
    }
}
void InstancedDrawable::clearColorBuf() {
    if(m_colGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufCol);
        m_colGenerated = false;
    }
}

InterleavedDrawable::InterleavedDrawable(OpenGLContext *context)
    : Drawable(context),
      m_countOpaque(-1), m_countTransparent(-1),
      m_bufIdxOpaque(-1), m_bufIdxTransparent(-1), m_bufVboOpaque(-1), m_bufVboTransparent(-1),
      m_idxOpaqueGenerated(false), m_idxTransparentGenerated(false), m_vboOpaqueGenerated(false), m_vboTransparentGenerated(false)
{}

InterleavedDrawable::~InterleavedDrawable(){}

void InterleavedDrawable::destroyVBOdata()
{
    Drawable::destroyVBOdata();
    mp_context->glDeleteBuffers(1, &m_bufIdxOpaque);
    mp_context->glDeleteBuffers(1, &m_bufIdxTransparent);
    mp_context->glDeleteBuffers(1, &m_bufVboOpaque);
    mp_context->glDeleteBuffers(1, &m_bufVboTransparent);
    m_idxOpaqueGenerated = m_idxTransparentGenerated = m_vboOpaqueGenerated = m_vboTransparentGenerated = false;
    m_countOpaque = m_countTransparent = -1;
}

int InterleavedDrawable::opaqueCount() const
{
    return m_countOpaque;
}

int InterleavedDrawable::transparentCount() const
{
    return m_countTransparent;
}

void InterleavedDrawable::generateIdxOpaque()
{
    m_idxOpaqueGenerated = true;
    mp_context->glGenBuffers(1, &m_bufIdxOpaque);
}

void InterleavedDrawable::generateIdxTransparent()
{
    m_idxTransparentGenerated = true;
    mp_context->glGenBuffers(1, &m_bufIdxTransparent);
}

void InterleavedDrawable::generateVboQpaque()
{
    m_vboOpaqueGenerated = true;
    mp_context->glGenBuffers(1, &m_bufVboOpaque);
}

void InterleavedDrawable::generateVboTransparent()
{
    m_vboTransparentGenerated = true;
    mp_context->glGenBuffers(1, &m_bufVboTransparent);
}

bool InterleavedDrawable::bindIdxOpaque()
{
    if(m_idxOpaqueGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpaque);
    }
    return m_idxOpaqueGenerated;
}

bool InterleavedDrawable::bindIdxTransparent()
{
    if(m_idxTransparentGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTransparent);
    }
    return m_idxTransparentGenerated;
}

bool InterleavedDrawable::bindVboOpaque()
{
    if(m_vboOpaqueGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVboOpaque);
    }
    return m_vboOpaqueGenerated;
}

bool InterleavedDrawable::bindVboTransparent()
{
    if(m_vboTransparentGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVboTransparent);
    }
    return m_vboTransparentGenerated;
}
