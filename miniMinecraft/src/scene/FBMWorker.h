#ifndef FBMWORKER_H
#define FBMWORKER_H

#include "chunk.h"
#include <QRunnable>
#include <QMutex>
#include <unordered_set>

class FBMWorker : public QRunnable
{
private:
    int m_xCorner, m_zCorner;
    std::vector<Chunk *> m_chunksToGenerate;
    std::unordered_set<Chunk *>* mp_chunksCompleteds;
    QMutex * m_chunkCompletedLock;
public:
    FBMWorker(int m_xCorner, int m_zCorner, std::vector<Chunk *> m_chunksToGenerate,
              std::unordered_set<Chunk *>* mp_chunksCompleteds, QMutex * m_chunkCompletedLock);

    void run() override;
};

#endif // FBMWORKER_H
