#ifndef VBOWORKER_H
#define VBOWORKER_H

#include "chunk.h"
#include <QRunnable>
#include <QMutex>
#include <unordered_set>


class VBOWorker : public QRunnable
{
private:
    Chunk * m_chunk;
    std::unordered_set<Chunk *>* m_VBOChunks;
    QMutex * m_VBOChunksLock;

public:
    VBOWorker(Chunk * c, std::unordered_set<Chunk *> * m_VBOChunks, QMutex * m_VBOChunksLock);

    void run() override;
};

#endif // VBOWORKER_H
