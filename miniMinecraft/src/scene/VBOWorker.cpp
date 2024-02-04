#include "VBOWorker.h"


VBOWorker::VBOWorker(Chunk * c, std::unordered_set<Chunk *> * m_VBOChunks, QMutex * m_VBOChunksLock)
    : m_chunk(c), m_VBOChunks(m_VBOChunks), m_VBOChunksLock(m_VBOChunksLock)
{}


void VBOWorker::run() {
    m_chunk->createVBOdata();
    m_VBOChunksLock->lock();
    m_VBOChunks->insert(m_chunk);
    m_VBOChunksLock->unlock();
}
