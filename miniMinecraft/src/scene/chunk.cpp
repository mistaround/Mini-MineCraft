#include "chunk.h"
#include <QDebug>

Chunk::Chunk(OpenGLContext *context, int x, int z)
    : InterleavedDrawable(context), m_blocks(), minX(x), minZ(z),
      m_neighbors{
          {XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}} {
  std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

Chunk::~Chunk() { destroyVBOdata(); }

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    if (x < 0) {
        return m_neighbors.at(XNEG) ? m_neighbors.at(XNEG)->getBlockAt(15, y, z) : UNKNOWN;
    } else if (x >= 16) {
        return m_neighbors.at(XPOS) ? m_neighbors.at(XPOS)->getBlockAt( 0, y, z) : UNKNOWN;
    } else if (y < 0) {
        return UNKNOWN;
    } else if (y >= 256) {
        return EMPTY;
    } else if (z < 0) {
        return m_neighbors.at(ZNEG) ? m_neighbors.at(ZNEG)->getBlockAt(x, y, 15) : UNKNOWN;
    } else if (z >= 16) {
        return m_neighbors.at(ZPOS) ? m_neighbors.at(ZPOS)->getBlockAt(x, y,  0) : UNKNOWN;
    } else {
        return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
    }
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;

    // invalidate VBO
    validVBOonCPU = false;
    if (t == EMPTY) {
        // neighbors need to be updated
        if (x == 0 && m_neighbors.at(XNEG)) {
            m_neighbors.at(XNEG)->validVBOonCPU = false;
        } else if (x == 15 && m_neighbors.at(XPOS)) {
            m_neighbors.at(XPOS)->validVBOonCPU = false;
        }
        if (z == 0 && m_neighbors.at(ZNEG)) {
            m_neighbors.at(ZNEG)->validVBOonCPU = false;
        } else if (z == 15 && m_neighbors.at(ZPOS)) {
            m_neighbors.at(ZPOS)->validVBOonCPU = false;
        }
    }
}

const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        this->validVBOonCPU = false;
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
        neighbor->validVBOonCPU = false;
    }
}

void Chunk::generateChunk(int x_off, int z_off) {
    for(int x = 0; x < 16; x++) {
        for(int z = 0; z < 16; z++) {
            generateBlock(x, z, x_off, z_off);
        }
    }
}

bool Chunk::isCaveBlockInWater(int x, int y, int z) {
    if (getBlockAt(x, 129, z) == WATER) return true;
    if (getBlockAt(x, y+1, z) == WATER) return true;
    if (x > 0 && z > 0 && x < 15 && z < 16 && getBlockAt(x + 1, y, z) == WATER) return true;
    if (x > 1 && z > 0 && x < 16 && z < 16 && getBlockAt(x - 1, y, z) == WATER) return true;
    if (x > 0 && z > 0 && x < 16 && z < 15 && getBlockAt(x, y, z + 1) == WATER) return true;
    if (x > 0 && z > 1 && x < 16 && z < 16 && getBlockAt(x, y, z - 1) == WATER) return true;
    return false;
}

// Type: 0 Oak, 1 Dark, 2 Birch
void Chunk::plantATree(int x, int h, int z, int type) {
    BlockType log;
    BlockType leaf;
    if (type == 0) {
        log = OAK_LOG;
        leaf = OAK_LEAF;
    } else if (type == 1) {
        log = DARK_LOG;
        leaf = DARK_LEAF;
    } else {
        log = BIRCH_LOG;
        leaf = BIRCH_LEAF;
    }
    // Build the trunk
    for (int i = 0; i < 5; i++) {
        setBlockAt(x, h + i, z, log);
    }

    // Top layer of leaves
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx != 0 || dz != 0) { // Avoid placing a leaf block directly above the top log
                setBlockAt(x + dx, h + 5, z + dz, leaf);
            }
        }
    }

    // Second layer of leaves
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            setBlockAt(x + dx, h + 4, z + dz, leaf);
        }
    }

    // Third layer of leaves - wider spread
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            if (abs(dx) != 2 || abs(dz) != 2) {
                setBlockAt(x + dx, h + 3, z + dz, leaf);
            }
        }
    }

    // Fourth layer of leaves - just the corners
    setBlockAt(x - 2, h + 2, z - 2, leaf);
    setBlockAt(x - 2, h + 2, z + 2, leaf);
    setBlockAt(x + 2, h + 2, z - 2, leaf);
    setBlockAt(x + 2, h + 2, z + 2, leaf);

    // Add random leaves around the tree
    // for (int i = 0; i < numberOfRandomLeaves; i++) {
    //     int randomDx = random.nextInt(5) - 2; // Random offset between -2 and 2
    //     int randomDz = random.nextInt(5) - 2;
    //     setBlockAt(x + randomDx, h + 2, z + randomDz, OAK_LEAF);
    // }
}

void Chunk::generateBlock(int x, int z, int x_off, int z_off) {
    int x_world = x + x_off;
    int z_world = z + z_off;

    float weirdness = SimplexNoise(randomizeUV(glm::vec2(x_world, z_world), 0.25f, 200.f));
    float humidity = SimplexNoise(randomizeUV(glm::vec2(x_world, z_world), 0.33f, 128.f));

    BiomeType currentBiome;

    weirdness = weirdness * 0.5f + 0.5f;
    weirdness = glm::smoothstep(0.05f, 0.75f, weirdness);

    humidity = humidity * 0.5f + 0.5f;
    humidity = glm::smoothstep(0.1f, 0.8f, humidity);

    float grassH= midHeight(glm::vec2(x_world, z_world));
    float mountainH= peakHeight(glm::vec2(x_world, z_world));
    float desertH= lowHeight(glm::vec2(x_world, z_world));
    int h = int(glm::mix(mountainH, glm::mix(desertH, grassH, humidity), weirdness)) + 128;

    // Bedrock
    setBlockAt(x, 0, z, BEDROCK);

    // Stone Base
    for (int y = 1; y <= 127; y++) {
        setBlockAt(x, y, z, STONE);
    }

    // Water
    for (int y = 128; y <= 138; y++) {
        if (getBlockAt(x, y, z) == EMPTY) {
            setBlockAt(x, y, z, WATER);
        }
    }

    if (weirdness < 0.5f) {
        // MID
        currentBiome = MOUNTAIN;
        for (int y = 128; y < h; y++) {
            if (humidity < 0.5f) {
                setBlockAt(x, y, z, STONE);
            } else {
                setBlockAt(x, y, z, GRASS_BLOCK);
                currentBiome = BIRCH_FOREST;
            }

        }
        // PEAK
        if (weirdness < 0.4f && h > 175) {
            currentBiome = SNOWPEAK;
            setBlockAt(x, h-1, z, SNOW);
        }
    } else {
        // LOW
        if (humidity < 0.5f && h < 160) {
            currentBiome = DESERT;
            for (int y = 128; y < h; y++) {
                setBlockAt(x, y, z, SAND);
            }
        } else {
            currentBiome = PLAIN;
            for (int y = 128; y < h; y++) {
                if (getBlockAt(x, y, z) == WATER) {
                    setBlockAt(x, y, z, SAND);
                } else {
                    setBlockAt(x, y, z, GRASS_BLOCK);
                    if (humidity > 0.8f) {
                        currentBiome = OAK_FOREST;
                    }
                    if (humidity > 0.9f) {
                        currentBiome = DARK_FOREST;
                    }
                }
            }
            if (humidity > 0.98f && h < 150) {
                if (getBlockAt(x, h-1, z) != SAND) {
                    currentBiome = MARSH;
                    setBlockAt(x, h-1, z, MUSHLAND);
                    setBlockAt(x, h-2, z, MUSHLAND);
                    setBlockAt(x, h-3, z, MUSHLAND);
                }
            }
        }
    }

    // Cave
    for (int y = 128; y >= 1; y--) {
        glm::vec3 pos(x_world, y, z_world);
        float p = SimplexNoise(pos / 32.f) * 0.5f + 0.5f;
        if (p < 0.25f) {
            if (y <= 24) {
                setBlockAt(x, y, z, LAVA);
            } else {
                if (isCaveBlockInWater(x, y, z)) {
                    setBlockAt(x, y, z, WATER);
                } else {
                    setBlockAt(x, y, z, EMPTY);
                }

            }
        }
    }

    // Plants
    float density = (float) rand()/RAND_MAX;

    if (currentBiome == DESERT) {
        if (density > 0.9875f && getBlockAt(x, h, z) == EMPTY) {
            setBlockAt(x, h, z, CACTUS);
            // setBlockAt(x, h+1, z, CACTUS);
            // setBlockAt(x, h+2, z, CACTUS);
        }
    }

    else if (currentBiome == MARSH) {
        if (x + 1 >= 16 || x - 1 < 0 || z + 1 >= 16 || z - 1 < 0) {}
        else {
            density = (float) rand()/RAND_MAX;
            if (density > 0.9875f && getBlockAt(x, h + 2, z) == EMPTY) {
                setBlockAt(x, h, z, MUSHSTEM);
                setBlockAt(x, h+1, z, MUSHSTEM);
                setBlockAt(x, h+2, z, MUSHSTEM);
                setBlockAt(x, h+3, z, MUSHSTEM);

                setBlockAt(x, h+4, z, MUSHHEAD);

                // Should check boundary
                setBlockAt(x-1, h+4, z, MUSHHEAD);
                setBlockAt(x+1, h+4, z, MUSHHEAD);
                setBlockAt(x, h+4, z-1, MUSHHEAD);
                setBlockAt(x, h+4, z+1, MUSHHEAD);
                setBlockAt(x-1, h+4, z-1, MUSHHEAD);
                setBlockAt(x-1, h+4, z+1, MUSHHEAD);
                setBlockAt(x+1, h+4, z-1, MUSHHEAD);
                setBlockAt(x+1, h+4, z+1, MUSHHEAD);
            }
        }
    }

    else if (currentBiome == OAK_FOREST) {
        if (x + 2 >= 16 || x - 2 < 0 || z + 2 >= 16 || z - 2 < 0) {}
        else {
            density = (float) rand()/RAND_MAX;
            if (density > 0.99f && getBlockAt(x, h + 4, z) == EMPTY && getBlockAt(x, h - 1, z) != SAND) {
                plantATree(x, h, z, 0);
            }
        }
    }

    else if (currentBiome == DARK_FOREST) {
        if (x + 2 >= 16 || x - 2 < 0 || z + 2 >= 16 || z - 2 < 0) {}
        else {
            density = (float) rand()/RAND_MAX;
            if (density > 0.95f && getBlockAt(x, h + 4, z) == EMPTY && getBlockAt(x, h - 1, z) != SAND) {
                plantATree(x, h, z, 1);
            }
            density = (float) rand()/RAND_MAX;
            if (density > 0.99f && getBlockAt(x, h, z) == EMPTY && getBlockAt(x, h - 1, z) != SAND) {
                setBlockAt(x, h, z, PUMPKIN);
            }
        }
    }

    else if (currentBiome == BIRCH_FOREST) {
        if (x + 2 >= 16 || x - 2 < 0 || z + 2 >= 16 || z - 2 < 0) {}
        else {
            density = (float) rand()/RAND_MAX;
            if (density > 0.99f && getBlockAt(x, h + 4, z) == EMPTY) {
                plantATree(x, h, z, 2);
            }
        }
    }

    else if (currentBiome == SNOWPEAK) {
        density = (float) rand()/RAND_MAX;
        if (density > 0.9975f && getBlockAt(x, h, z) == EMPTY) {
            setBlockAt(x, h, z, LATERN);
        }
    }

    else if (currentBiome == PLAIN) {
        density = (float) rand()/RAND_MAX;
        if (density > 0.9995f && getBlockAt(x, h, z) == EMPTY) {
            setBlockAt(x, h, z, WATERMELON);
        }
    }

}

// ----------------------------------------------------------------------------
// VBO Generation -------------------------------------------------------------
// ----------------------------------------------------------------------------

struct BlockFace {
    Direction dir;
    std::array<glm::vec3, 4> pos;
    glm::vec3 nor;
    std::array<glm::vec2, 4> uv;
};

constexpr float UV = 1/16.f;

const static std::array<BlockFace, 6> neighboringFaces {
    BlockFace{
        XPOS,
        {glm::vec3{1, 0, 0}, glm::vec3{1, 1, 0}, glm::vec3{1, 1, 1}, glm::vec3{1, 0, 1}},
        glm::vec3{1, 0, 0},
        {glm::vec2{UV, 0}, glm::vec2{UV, UV}, glm::vec2{0, UV}, glm::vec2{0, 0}}
    },
    BlockFace{
        XNEG,
        {glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, glm::vec3{0, 1, 1}, glm::vec3{0, 1, 0}},
        glm::vec3{-1, 0, 0},
        {glm::vec2{0, 0}, glm::vec2{UV, 0}, glm::vec2{UV, UV}, glm::vec2{0, UV}}
    },
    BlockFace{
        YPOS,
        {glm::vec3{0, 1, 0}, glm::vec3{1, 1, 0}, glm::vec3{1, 1, 1}, glm::vec3{0, 1, 1}},
        glm::vec3{0, 1, 0},
        {glm::vec2{0, UV}, glm::vec2{UV, UV}, glm::vec2{UV, 0}, glm::vec2{0, 0}}
    },
    BlockFace{
        YNEG,
        {glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, glm::vec3{1, 0, 1}, glm::vec3{1, 0, 0}},
        glm::vec3{0, -1, 0},
        {glm::vec2{0, 0}, glm::vec2{0, UV}, glm::vec2{UV, UV}, glm::vec2{UV, 0}}
    },
    BlockFace{
        ZPOS,
        {glm::vec3{0, 0, 1}, glm::vec3{1, 0, 1}, glm::vec3{1, 1, 1}, glm::vec3{0, 1, 1}},
        glm::vec3{0, 0, 1},
        {glm::vec2{0, 0}, glm::vec2{UV, 0}, glm::vec2{UV, UV}, glm::vec2{0, UV}}
    },
    BlockFace{
        ZNEG,
        {glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}, glm::vec3{1, 1, 0}, glm::vec3{1, 0, 0}},
        glm::vec3{0, 0, -1},
        {glm::vec2{UV, 0}, glm::vec2{UV, UV}, glm::vec2{0, UV}, glm::vec2{0, 0}}
    }
};

const static std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> blockFaceUVs {
    {GRASS_BLOCK,
     {{XPOS, glm::vec2{ 3/16.f, 15/16.f}},
      {XNEG, glm::vec2{ 3/16.f, 15/16.f}},
      {YPOS, glm::vec2{ 8/16.f, 13/16.f}},
      {YNEG, glm::vec2{ 2/16.f, 15/16.f}},
      {ZPOS, glm::vec2{ 3/16.f, 15/16.f}},
      {ZNEG, glm::vec2{ 3/16.f, 15/16.f}}}},

    {DIRT,
     {{XPOS, glm::vec2{ 2/16.f, 15/16.f}},
      {XNEG, glm::vec2{ 2/16.f, 15/16.f}},
      {YPOS, glm::vec2{ 2/16.f, 15/16.f}},
      {YNEG, glm::vec2{ 2/16.f, 15/16.f}},
      {ZPOS, glm::vec2{ 2/16.f, 15/16.f}},
      {ZNEG, glm::vec2{ 2/16.f, 15/16.f}}}},

    {STONE,
     {{XPOS, glm::vec2{ 1/16.f, 15/16.f}},
      {XNEG, glm::vec2{ 1/16.f, 15/16.f}},
      {YPOS, glm::vec2{ 1/16.f, 15/16.f}},
      {YNEG, glm::vec2{ 1/16.f, 15/16.f}},
      {ZPOS, glm::vec2{ 1/16.f, 15/16.f}},
      {ZNEG, glm::vec2{ 1/16.f, 15/16.f}}}},

    {WATER,
     {{XPOS, glm::vec2{14/16.f,  3/16.f}},
      {XNEG, glm::vec2{14/16.f,  3/16.f}},
      {YPOS, glm::vec2{14/16.f,  3/16.f}},
      {YNEG, glm::vec2{14/16.f,  3/16.f}},
      {ZPOS, glm::vec2{14/16.f,  3/16.f}},
      {ZNEG, glm::vec2{14/16.f,  3/16.f}}}},

    {SNOW,
     {{XPOS, glm::vec2{ 2/16.f, 11/16.f}},
      {XNEG, glm::vec2{ 2/16.f, 11/16.f}},
      {YPOS, glm::vec2{ 2/16.f, 11/16.f}},
      {YNEG, glm::vec2{ 2/16.f, 11/16.f}},
      {ZPOS, glm::vec2{ 2/16.f, 11/16.f}},
      {ZNEG, glm::vec2{ 2/16.f, 11/16.f}}}},

    {BEDROCK,
     {{XPOS, glm::vec2{ 1/16.f, 14/16.f}},
      {XNEG, glm::vec2{ 1/16.f, 14/16.f}},
      {YPOS, glm::vec2{ 1/16.f, 14/16.f}},
      {YNEG, glm::vec2{ 1/16.f, 14/16.f}},
      {ZPOS, glm::vec2{ 1/16.f, 14/16.f}},
      {ZNEG, glm::vec2{ 1/16.f, 14/16.f}}}},

    {LAVA,
     {{XPOS, glm::vec2{14/16.f,  1/16.f}},
      {XNEG, glm::vec2{14/16.f,  1/16.f}},
      {YPOS, glm::vec2{14/16.f,  1/16.f}},
      {YNEG, glm::vec2{14/16.f,  1/16.f}},
      {ZPOS, glm::vec2{14/16.f,  1/16.f}},
      {ZNEG, glm::vec2{14/16.f,  1/16.f}}}},

    {SAND,
     {{XPOS, glm::vec2{2/16.f,  14/16.f}},
      {XNEG, glm::vec2{2/16.f,  14/16.f}},
      {YPOS, glm::vec2{2/16.f,  14/16.f}},
      {YNEG, glm::vec2{2/16.f,  14/16.f}},
      {ZPOS, glm::vec2{2/16.f,  14/16.f}},
      {ZNEG, glm::vec2{2/16.f,  14/16.f}}}},

    {MUSHLAND,
     {{XPOS, glm::vec2{14/16.f,  8/16.f}},
      {XNEG, glm::vec2{14/16.f,  8/16.f}},
      {YPOS, glm::vec2{14/16.f,  8/16.f}},
      {YNEG, glm::vec2{14/16.f,  8/16.f}},
      {ZPOS, glm::vec2{14/16.f,  8/16.f}},
      {ZNEG, glm::vec2{14/16.f,  8/16.f}}}},

    {MUSHHEAD,
     {{XPOS, glm::vec2{13/16.f,  8/16.f}},
      {XNEG, glm::vec2{13/16.f,  8/16.f}},
      {YPOS, glm::vec2{13/16.f,  8/16.f}},
      {YNEG, glm::vec2{13/16.f,  8/16.f}},
      {ZPOS, glm::vec2{13/16.f,  8/16.f}},
      {ZNEG, glm::vec2{13/16.f,  8/16.f}}}},

    {MUSHSTEM,
     {{XPOS, glm::vec2{13/16.f,  7/16.f}},
      {XNEG, glm::vec2{13/16.f,  7/16.f}},
      {YPOS, glm::vec2{13/16.f,  7/16.f}},
      {YNEG, glm::vec2{13/16.f,  7/16.f}},
      {ZPOS, glm::vec2{13/16.f,  7/16.f}},
      {ZNEG, glm::vec2{13/16.f,  7/16.f}}}},

    {CACTUS,
     {{XPOS, glm::vec2{6/16.f,  11/16.f}},
      {XNEG, glm::vec2{6/16.f,  11/16.f}},
      {YPOS, glm::vec2{5/16.f,  11/16.f}},
      {YNEG, glm::vec2{5/16.f,  11/16.f}},
      {ZPOS, glm::vec2{6/16.f,  11/16.f}},
      {ZNEG, glm::vec2{6/16.f,  11/16.f}}}},

    {OAK_LOG,
     {{XPOS, glm::vec2{4/16.f,  14/16.f}},
      {XNEG, glm::vec2{4/16.f,  14/16.f}},
      {YPOS, glm::vec2{5/16.f,  14/16.f}},
      {YNEG, glm::vec2{5/16.f,  14/16.f}},
      {ZPOS, glm::vec2{4/16.f,  14/16.f}},
      {ZNEG, glm::vec2{4/16.f,  14/16.f}}}},

    {OAK_LEAF,
     {{XPOS, glm::vec2{5/16.f,  12/16.f}},
      {XNEG, glm::vec2{5/16.f,  12/16.f}},
      {YPOS, glm::vec2{5/16.f,  12/16.f}},
      {YNEG, glm::vec2{5/16.f,  12/16.f}},
      {ZPOS, glm::vec2{5/16.f,  12/16.f}},
      {ZNEG, glm::vec2{5/16.f,  12/16.f}}}},

    {DARK_LOG,
     {{XPOS, glm::vec2{4/16.f,  8/16.f}},
      {XNEG, glm::vec2{4/16.f,  8/16.f}},
      {YPOS, glm::vec2{5/16.f,  14/16.f}},
      {YNEG, glm::vec2{5/16.f,  14/16.f}},
      {ZPOS, glm::vec2{4/16.f,  8/16.f}},
      {ZNEG, glm::vec2{4/16.f,  8/16.f}}}},

    {DARK_LEAF,
     {{XPOS, glm::vec2{4/16.f,  12/16.f}},
      {XNEG, glm::vec2{4/16.f,  12/16.f}},
      {YPOS, glm::vec2{4/16.f,  12/16.f}},
      {YNEG, glm::vec2{4/16.f,  12/16.f}},
      {ZPOS, glm::vec2{4/16.f,  12/16.f}},
      {ZNEG, glm::vec2{4/16.f,  12/16.f}}}},

    {BIRCH_LOG,
     {{XPOS, glm::vec2{5/16.f,  8/16.f}},
      {XNEG, glm::vec2{5/16.f,  8/16.f}},
      {YPOS, glm::vec2{5/16.f,  14/16.f}},
      {YNEG, glm::vec2{5/16.f,  14/16.f}},
      {ZPOS, glm::vec2{5/16.f,  8/16.f}},
      {ZNEG, glm::vec2{5/16.f,  8/16.f}}}},

    {BIRCH_LEAF,
     {{XPOS, glm::vec2{5/16.f,  12/16.f}},
      {XNEG, glm::vec2{5/16.f,  12/16.f}},
      {YPOS, glm::vec2{5/16.f,  12/16.f}},
      {YNEG, glm::vec2{5/16.f,  12/16.f}},
      {ZPOS, glm::vec2{5/16.f,  12/16.f}},
      {ZNEG, glm::vec2{5/16.f,  12/16.f}}}},

    {PUMPKIN,
     {{XPOS, glm::vec2{6/16.f,  8/16.f}},
      {XNEG, glm::vec2{6/16.f,  8/16.f}},
      {YPOS, glm::vec2{6/16.f,  9/16.f}},
      {YNEG, glm::vec2{6/16.f,  8/16.f}},
      {ZPOS, glm::vec2{6/16.f,  8/16.f}},
      {ZNEG, glm::vec2{6/16.f,  8/16.f}}}},

    {LATERN,
     {{XPOS, glm::vec2{8/16.f,  8/16.f}},
      {XNEG, glm::vec2{6/16.f,  8/16.f}},
      {YPOS, glm::vec2{6/16.f,  9/16.f}},
      {YNEG, glm::vec2{6/16.f,  8/16.f}},
      {ZPOS, glm::vec2{6/16.f,  8/16.f}},
      {ZNEG, glm::vec2{6/16.f,  8/16.f}}}},

    {WATERMELON,
     {{XPOS, glm::vec2{8/16.f,  7/16.f}},
      {XNEG, glm::vec2{8/16.f,  7/16.f}},
      {YPOS, glm::vec2{9/16.f,  7/16.f}},
      {YNEG, glm::vec2{8/16.f,  7/16.f}},
      {ZPOS, glm::vec2{8/16.f,  7/16.f}},
      {ZNEG, glm::vec2{8/16.f,  7/16.f}}}},

};

inline bool isTransparent(BlockType t) {
    return t == WATER || t == LAVA;
}

void Chunk::createVBOdata() {
    // use cached VBO data if possible
    if (validVBOonCPU) return;

    idxOpaque.clear();
    vboOpaque.clear();
    idxTransparent.clear();
    vboTransparent.clear();

    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {

                BlockType curr = getBlockAt(x, y, z);
                if (curr == EMPTY) continue;

                glm::vec3 blockWorldPos(x + minX, y, z + minZ);

                for (auto& bf : neighboringFaces) {

                    BlockType neighbor = getBlockAt(x + int(bf.nor.x), y + int(bf.nor.y), z + int(bf.nor.z));

                    if (curr == neighbor) continue;

                    if (neighbor == EMPTY || isTransparent(neighbor)) {
                        auto& idx = isTransparent(curr) ? idxTransparent : idxOpaque;
                        auto& vbo = isTransparent(curr) ? vboTransparent : vboOpaque;

                        int startIdx = vbo.size();
                        for (int i = 0; i < 4; i++) {
                            vbo.emplace_back(blockWorldPos + bf.pos[i], bf.nor, blockFaceUVs.at(curr).at(bf.dir) + bf.uv[i]);
                        }
                        idx.push_back(startIdx);
                        idx.push_back(startIdx + 1);
                        idx.push_back(startIdx + 2);
                        idx.push_back(startIdx);
                        idx.push_back(startIdx + 2);
                        idx.push_back(startIdx + 3);
                    }
                }

            }
        }
    }

    // cache VBO data
    validVBOonCPU = true;
    validVBOonGPU = false;
}

void Chunk::sendVBOdata() {
    // use cached VBO data if possible
    if (validVBOonGPU) return;

    m_countOpaque = idxOpaque.size();
    generateIdxOpaque();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpaque);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxOpaque.size() * sizeof(GLuint), idxOpaque.data(), GL_STATIC_DRAW);
    generateVboQpaque();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVboOpaque);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboOpaque.size() * sizeof(Vertex), vboOpaque.data(), GL_STATIC_DRAW);

    m_countTransparent = idxTransparent.size();
    generateIdxTransparent();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTransparent);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxTransparent.size() * sizeof(GLuint), idxTransparent.data(), GL_STATIC_DRAW);
    generateVboTransparent();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVboTransparent);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboTransparent.size() * sizeof(Vertex), vboTransparent.data(), GL_STATIC_DRAW);

    // cache VBO data
    validVBOonGPU = true;
}

void Chunk::destroyVBOdata() {
    InterleavedDrawable::destroyVBOdata();
    m_countOpaque = 0;
    idxOpaque.clear();
    vboOpaque.clear();
    m_countTransparent = 0;
    idxTransparent.clear();
    vboTransparent.clear();
    validVBOonCPU = false;
    validVBOonGPU = false;
}
