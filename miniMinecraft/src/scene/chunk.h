#pragma once
#include "drawable.h"
#include "smartpointerhelp.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include "biome.h"


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, UNKNOWN, GRASS_BLOCK, DIRT, STONE, WATER, SNOW, BEDROCK, LAVA, SAND,
    MUSHLAND, MUSHHEAD, MUSHSTEM,
    OAK_LOG, OAK_LEAF,
    DARK_LOG, DARK_LEAF,
    BIRCH_LOG, BIRCH_LEAF,
    PUMPKIN, WATERMELON, CACTUS, LATERN,
    // RED_MUSHROOM, BROWN_MUSHROOM, RED_FLOWER, YELLOW_FLOWER, GRASS
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

struct alignas(sizeof(float)) Vertex {
    glm::vec3 pos;
    glm::vec3 nor;
    glm::vec2 uv;

    Vertex(glm::vec3 pos, glm::vec3 nor, glm::vec2 uv) : pos(pos), nor(nor), uv(uv) {}
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public InterleavedDrawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;

    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    // render optimization ----------------------------
    bool validVBOonCPU = false;
    bool validVBOonGPU = false;
    // opaque vbo cache
    std::vector<GLuint> idxOpaque;
    std::vector<Vertex> vboOpaque;
    // transparent vbo cache
    std::vector<GLuint> idxTransparent;
    std::vector<Vertex> vboTransparent;
    // ------------------------------------------------

    bool isCaveBlockInWater(int x, int y, int z);

public:
    int minX, minZ;
    Chunk();
    Chunk(OpenGLContext* context, int x, int z);
    virtual ~Chunk();

    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    // Milestone 1
    void generateChunk(int x_off, int z_off);
    void generateBlock(int x, int z, int x_off, int z_off);
    virtual void createVBOdata() override;

    // Milestone 2
    void sendVBOdata();
    void destroyVBOdata();

    // Milestone 3
    void plantATree(int x, int h, int z, int type);

};

struct ChunkVBOData
{
    Chunk* mp_chunk;
    std::vector<float> m_vboDataOpaque, m_vboDataTransparent;
    std::vector<GLuint> m_idxDataOpaque, m_idxDataTransparent;


    ChunkVBOData(Chunk* c) : mp_chunk(c),
        m_vboDataOpaque{}, m_vboDataTransparent{},
        m_idxDataOpaque{}, m_idxDataTransparent{}
    {}
};
