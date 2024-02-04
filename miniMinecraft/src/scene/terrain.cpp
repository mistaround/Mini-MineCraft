#include "terrain.h"
#include "cube.h"
#include "scene/FBMWorker.h"
#include "scene/VBOWorker.h"

#include <stdexcept>
#include <iostream>
#include <QThreadPool>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context), m_texture(context), m_normalMap(context)
{}

Terrain::~Terrain() {}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context, x, z);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = std::move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    m_texture.bind(0); m_normalMap.bind(1);

    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            bool has = hasChunkAt(x,z);
            if (hasChunkAt(x, z)) {
                const auto& chunk = getChunkAt(x, z);
                chunk->sendVBOdata();

                // proper X and Z translation done when creating VBO data
                shaderProgram->drawInterleavedOpaque(*chunk);
            }
        }
    }

    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const auto& chunk = getChunkAt(x, z);
                // proper X and Z translation done when creating VBO data
                shaderProgram->drawInterleavedTransparent(*chunk);
            }
        }
    }
}

void Terrain::CreateTestScene()
{
//    // TODO: DELETE THIS LINE WHEN YOU DELETE m_geomCube!
//    m_geomCube.createVBOdata();
//    for(int x = 0; x < 64; x += 16) {
//        for(int z = 0; z < 64; z += 16) {
//            m_generatedTerrain.insert(toKey(x, z));
//            Chunk *chunk = instantiateChunkAt(x, z);
//            chunk->generateChunk(x, z);
//        }
//    }

//    // Create the Chunks that will
//    // store the blocks for our
//    // initial world space
//    for(int x = 0; x < 64; x += 16) {
//        for(int z = 0; z < 64; z += 16) {
//            instantiateChunkAt(x, z);
//        }
//    }
//    // Tell our existing terrain set that
//    // the "generated terrain zone" at (0,0)
//    // now exists.
//    m_generatedTerrain.insert(toKey(0, 0));

//    // Create the basic terrain floor
//    for(int x = 0; x < 64; ++x) {
//        for(int z = 0; z < 64; ++z) {
//            if((x + z) % 2 == 0) {
//                setBlockAt(x, 128, z, STONE);
//            }
//            else {
//                setBlockAt(x, 128, z, DIRT);
//            }
//        }
//    }
//    // Add "walls" for collision testing
//    for(int x = 0; x < 64; ++x) {
//        setBlockAt(x, 129, 0, GRASS);
//        setBlockAt(x, 130, 0, GRASS);
//        setBlockAt(x, 129, 63, GRASS);
//        setBlockAt(0, 130, x, GRASS);
//    }
//    // Add a central column
//    for(int y = 129; y < 140; ++y) {
//        setBlockAt(32, y, 32, GRASS);
//    }
}

void Terrain::spawnVBOWorker(Chunk* c) {
    VBOWorker * worker = new VBOWorker(c, &m_chunksThatHaveBlockData, &m_chunksThatHaveBlockDataLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnVBOWorkers(const std::unordered_set<Chunk*> &chunksNeedingVBOs) {
    for (Chunk* c: chunksNeedingVBOs) {
        spawnVBOWorker(c);
    }
}

void Terrain::spawnFBMWorker(int64_t zone) {
    m_generatedTerrain.insert(zone);

    std::vector<Chunk*> chunksForWorker;
    glm::ivec2 coords = toCoords(zone);
    for (int x = coords.x; x < coords.x + 64; x += 16) {
        for (int z = coords.y; z < coords.y + 64; z += 16) {
            Chunk* c;
            if (hasChunkAt(x, z)) {
                c = getChunkAt(x, z).get();
            }
            else {
                c = instantiateChunkAt(x, z);
            }

            chunksForWorker.push_back(c);
        }
    }

    FBMWorker* worker = new FBMWorker(coords.x, coords.y, chunksForWorker,
                                      &m_chunksThatHaveBlockData, &m_chunksThatHaveBlockDataLock);

    QThreadPool::globalInstance()->start(worker);

}


void Terrain::checkThreadResults() {
    // for the new generated chunk, build their VBO data
    // by VBOWorkers
    m_chunksThatHaveBlockDataLock.lock();
    spawnVBOWorkers(m_chunksThatHaveBlockData);
    m_chunksThatHaveBlockData.clear();
    m_chunksThatHaveBlockDataLock.unlock();

    // Send the Chunk VBOData to GPU
    m_chunksThatHaveVBOsLock.lock() ;
    for (Chunk* &ck : m_chunksThatHaveVBOs) {
        ck->sendVBOdata();
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock() ;
}

// The terrian zone based on postion and radius
QSet<int64_t> Terrain::getTerrainZones(glm::ivec2 zoneCoords, unsigned int radius) {
    int step = static_cast<int>(radius) * 64;

    QSet<int64_t> result;
    for (int i = -step; i <= step; i+= 64) {
        for (int j = -step; j <= step; j+= 64) {
            result.insert(toKey(zoneCoords.x + i, zoneCoords.y + j));
        }
    }

    return result;
}

void Terrain::expandChunks(glm::vec3 playerPos, glm::vec3 playerPosPrev) {
    // multi thread for expansion
    glm::ivec2 currZone(64 * glm::floor(playerPos.x / 64.f),
                        64 * glm::floor(playerPos.z / 64.f));
    glm::ivec2 prevZone(64 * glm::floor(playerPosPrev.x / 64.f),
                        64 * glm::floor(playerPosPrev.z / 64.f));

    QSet<int64_t> terrainZonesCur = getTerrainZones(currZone, 4);
    QSet<int64_t> terrainZonesPrev = getTerrainZones(prevZone, 4);

    // destory the VBO Data when moving far enough
    // But the Chunk data is kept
    for (auto id : terrainZonesPrev) {
        if (!terrainZonesCur.contains(id)) {
            glm::ivec2 coord = toCoords(id);
            for (int x = coord.x; x < coord.x + 64; x += 16) {
                for (int z = coord.y; z < coord.y + 64; z += 16) {
                    if (hasChunkAt(x, z)) {
                        auto &chunk = getChunkAt(x, z);
                        chunk->destroyVBOdata();
                    }
                }
            }
        }
    }

    // For the current terrian zone, if no Chunk then generated it
    // If has Chunk but no VBOData inside, fill the VBOData
    for (auto id : terrainZonesCur) {
        glm::ivec2 coord = toCoords(id);
        if (m_generatedTerrain.find(id) != m_generatedTerrain.end()) {
            if (!terrainZonesPrev.contains(id)) {
                for (int x = coord.x; x < coord.x + 64; x += 16) {
                    for (int z = coord.y; z < coord.y + 64; z += 16) {
                        auto chunk = getChunkAt(x, z).get();
                        spawnVBOWorker(chunk);
                    }
                }
            }
        }
        else {
            spawnFBMWorker(id);
        }
    }

    // last step to fill data and send to GPU
    checkThreadResults();



}

void Terrain::instantiateTexture() {
    std::cout<< "working hahahhahaha" << std::endl;
    // Create the textures
    m_texture.create(":/textures/minecraft_textures_all.png");
    m_texture.load(0);
    m_normalMap.create(":/textures/minecraft_normals_all.png");
    m_normalMap.load(1);
}
