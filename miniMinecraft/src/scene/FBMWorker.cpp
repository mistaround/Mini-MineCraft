#include "FBMWorker.h"


FBMWorker::FBMWorker(int m_xCorner, int m_zCorner, std::vector<Chunk *> m_chunksToGenerate,
                                 std::unordered_set<Chunk *>* mp_chunksCompleteds, QMutex * m_chunkCompletedLock)
    : m_xCorner(m_xCorner), m_zCorner(m_zCorner), m_chunksToGenerate(m_chunksToGenerate), mp_chunksCompleteds(mp_chunksCompleteds),
    m_chunkCompletedLock(m_chunkCompletedLock){}


void FBMWorker::run() {
    for (Chunk * c:  m_chunksToGenerate){
        c->generateChunk(c->minX, c->minZ);
        m_chunkCompletedLock->lock();
        mp_chunksCompleteds->insert(c);
        m_chunkCompletedLock->unlock();
    }
}
