#include "player.h"
#include <QString>
#include <QDateTime>
#include <iostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
    mcr_camera(m_camera), m_flightMode(true), axis(-1), m_prevPos(pos), selectedBlockType(GRASS_BLOCK)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    m_prevPos = m_position;
    processInputs(input);
    computePhysics(dT, mcr_terrain, input);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.
    float rate = 1.f;
    if (m_flightMode) {
        // no gravity and collisions
        if (inputs.wPressed) {
            m_acceleration += rate * this->m_forward;
        }
        else if (inputs.sPressed) {
            m_acceleration += -rate * this->m_forward;
        }
        else if (inputs.dPressed) {
            m_acceleration += rate * this->m_right;
        }
        else if (inputs.aPressed) {
            m_acceleration += -rate * this->m_right;
        }
        else if (inputs.ePressed) {
            m_acceleration += rate * this->m_up;
        }
        else if (inputs.qPressed) {
            m_acceleration += -rate * this->m_up;
        }
        else {
            m_velocity = glm::vec3(0);
            m_acceleration = glm::vec3(0);
        }
    }
    else {
        bool isPressed = false;
        if (inputs.wPressed) {
            m_acceleration += rate * glm::normalize(glm::vec3(m_forward.x, 0, m_forward.z));
            isPressed = true;
        }
        if (inputs.sPressed) {
            m_acceleration += -rate * glm::normalize(glm::vec3(m_forward.x, 0, m_forward.z));
            isPressed = true;
        }
        if (inputs.dPressed) {
            m_acceleration += rate * glm::normalize(glm::vec3(m_right.x, 0, m_right.z));
            isPressed = true;
        }
        if (inputs.aPressed) {
            m_acceleration += -rate * glm::normalize(glm::vec3(m_right.x, 0, m_right.z));
            isPressed = true;
        }
        if (inputs.spacePressed) {
            if (getFeetBlockType(mcr_terrain) == WATER || getFeetBlockType(mcr_terrain) == LAVA) {
                m_velocity.y += 3.f * m_up.y;
            } else if (isOnGround(mcr_terrain, inputs)) {
                m_velocity.y = 10.f * m_up.y;
            }
            isPressed = true;
        }
        if (!isPressed) {
            m_velocity.x = 0;
            m_velocity.z = 0;
            m_acceleration.x = 0;
            m_acceleration.z = 0;
        }
    }

}

void Player::computePhysics(float dT, const Terrain &terrain, InputBundle & inputs) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    // Apply gravity if not in flight mode

    // Apply friction
    float frictionCoefficient = 0.95f; // Example value, can be adjusted
    m_velocity *= frictionCoefficient;
    m_velocity += dT * m_acceleration;

    glm::vec3 rayDirection = dT * m_velocity;
    glm::vec3 MAX_ACCELERATION = glm::vec3(4.f, 1000.f, 4.f);
    glm::vec3 MAX_VELOCITY = glm::vec3(10.f, 1000.f, 10.f);

    if (m_flightMode) {
        this->moveAlongVector(rayDirection);
    }
    else {
        if (!isOnGround(terrain, inputs)) {
            glm::vec3 gravity(0, -0.981f, 0); // Gravity vector
            if (getFeetBlockType(terrain) == WATER || getFeetBlockType(terrain) == LAVA) {
                if (!inputs.spacePressed) {
                    gravity *= 0.33f;
                    m_acceleration += gravity;
                    m_acceleration = glm::sign(m_acceleration) * glm::min(glm::abs(m_acceleration), MAX_ACCELERATION);
                    m_velocity += m_acceleration * dT;
                }
            } else {
                m_acceleration += gravity;
                m_acceleration = glm::sign(m_acceleration) * glm::min(glm::abs(m_acceleration), MAX_ACCELERATION);
                m_velocity += m_acceleration * dT;
            }
        }
        else if (isOnGround(terrain, inputs) && !inputs.spacePressed) {
            m_acceleration.y = 0.f;
            m_velocity.y = glm::max(m_velocity.y, 0.f);
        }
        m_velocity = glm::sign(m_velocity) * glm::min(glm::abs(m_velocity), MAX_VELOCITY);
        rayDirection = m_velocity * dT;
        detectCollision(&rayDirection, terrain);
        if (getFeetBlockType(terrain) == WATER || getFeetBlockType(terrain) == LAVA) {
            rayDirection *= 0.33f;
        }

        this->moveAlongVector(rayDirection);
    }
}

bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain,
                       float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // now all t values represent world dist

    float curr_t = 0.f;
    while (curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for (int i = 0; i < 3; ++i) { // Iterate over the three axes
            if (rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i]));

                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if (currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if (axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if (interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        axis = interfaceAxis;
        curr_t += min_t;
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0, 0, 0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If the currCell contains something other than empty, return curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);

        if (cellType != EMPTY && cellType != WATER && cellType != LAVA) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }

    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

void Player::detectCollision(glm::vec3 *rayDirection, const Terrain &terrain) {
    std::array<glm::vec3, 12> rayOrigins = {
        m_position + glm::vec3(0.5, 0, 0.5),
        m_position + glm::vec3(0.5, 0, -0.5),
        m_position + glm::vec3(-0.5, 0, -0.5),
        m_position + glm::vec3(-0.5, 0, 0.5),
        m_position + glm::vec3(0.5, 1, 0.5),
        m_position + glm::vec3(0.5, 1, -0.5),
        m_position + glm::vec3(-0.5, 1, -0.5),
        m_position + glm::vec3(-0.5, 1, 0.5),
        m_position + glm::vec3(0.5, 2, 0.5),
        m_position + glm::vec3(0.5, 2, -0.5),
        m_position + glm::vec3(-0.5, 2, -0.5),
        m_position + glm::vec3(-0.5, 2, 0.5)
    };

    glm::ivec3 out_blockHit = glm::ivec3();
    float out_dist = 0.f;

    for (auto rayOrigin : rayOrigins) {
        glm::vec3 x_ray = glm::vec3(rayDirection->x, 0.f, 0.f);
        glm::vec3 y_ray = glm::vec3(0.f, rayDirection->y, 0.f);
        glm::vec3 z_ray = glm::vec3(0.f, 0.f, rayDirection->z);
        if (gridMarch(rayOrigin, x_ray, terrain, &out_dist, &out_blockHit)) {
            // this offset is necessary, but it is not letting the player land after jumping (problem fixed)
            float distance = glm::min(out_dist - 0.005f, glm::abs(glm::length(this->m_position - glm::vec3(out_blockHit))));
            x_ray = distance * glm::normalize(x_ray);
        }
        if (gridMarch(rayOrigin, y_ray, terrain, &out_dist, &out_blockHit)) {
            float distance = glm::min(out_dist - 0.005f, glm::abs(glm::length(this->m_position - glm::vec3(out_blockHit))));
            y_ray = distance * glm::normalize(y_ray);
        }
        if (gridMarch(rayOrigin, z_ray, terrain, &out_dist, &out_blockHit)) {
            float distance = glm::min(out_dist - 0.005f, glm::abs(glm::length(this->m_position - glm::vec3(out_blockHit))));
            z_ray = distance * glm::normalize(z_ray);
        }
        // combine the three axis
        *rayDirection = glm::vec3(x_ray.x, y_ray.y, z_ray.z);

    }
}


bool Player::isOnGround(const Terrain &terrain, InputBundle &inputs) {
    glm::vec3 bottom1 = m_position - glm::vec3(0.5f, 0, 0.5f);
    // traverse the four bottom vertices
    for (int x = 0; x < 2; x++) {
        for (int z = 0; z < 2; z++) {
            glm::vec3 vertexPos = glm::vec3(floor(bottom1.x) + x, floor(bottom1.y - 0.1f),
                                            floor(bottom1.z) + z);
            // as long as one of the vertex is on a block that is not empty
            // player is on the ground
            BlockType currBlock = terrain.getBlockAt(vertexPos);
            if (currBlock != EMPTY && currBlock != WATER && currBlock != LAVA) {
//                inputs.isOnGround = true;
                if (!inputs.spacePressed) {
                    m_acceleration.y = 0.f;
                    m_velocity.y = 0.f;
                }
                return true;
            }
        }
    }
//    inputs.isOnGround = false;
    return false;
}

BlockType Player::getHeadBlockType(const Terrain &terrain) {
    glm::vec3 pos = m_camera.mcr_position;
    if (!terrain.hasChunkAt(floor(pos.x), floor(pos.z))) return EMPTY;
    return terrain.getBlockAt(pos);
}

BlockType Player::getFeetBlockType(const Terrain &terrain) {
    glm::vec3 pos = m_camera.mcr_position - glm::vec3(0.f, 1.f, 0.f);
    if (!terrain.hasChunkAt(floor(pos.x), floor(pos.z))) return EMPTY;
    return terrain.getBlockAt(pos);
}


void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}


BlockType Player::addBlock(Terrain* terrian) {
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = 3.f * glm::normalize(this->m_forward);
    float outDist = 0.f;
    glm::ivec3 outBlockHit = glm::ivec3();

    if (gridMarch(rayOrigin, rayDirection, *terrian, &outDist, &outBlockHit))
    {
        BlockType blockType = terrian->getBlockAt(outBlockHit.x-1 , outBlockHit.y, outBlockHit.z);
        if (blockType == EMPTY)
        {

            terrian->setBlockAt(outBlockHit.x-1 , outBlockHit.y, outBlockHit.z, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->sendVBOdata();
            return selectedBlockType;
        }
        blockType = terrian->getBlockAt(outBlockHit.x+1 , outBlockHit.y, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrian->setBlockAt(outBlockHit.x+1 , outBlockHit.y, outBlockHit.z, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->sendVBOdata();
            return selectedBlockType;
        }
        // check up
        blockType = terrian->getBlockAt(outBlockHit.x, outBlockHit.y-1, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrian->setBlockAt(outBlockHit.x, outBlockHit.y-1, outBlockHit.z, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->sendVBOdata();
            return selectedBlockType;
        }
        blockType = terrian->getBlockAt(outBlockHit.x, outBlockHit.y+1, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrian->setBlockAt(outBlockHit.x, outBlockHit.y+1, outBlockHit.z, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->sendVBOdata();
            return selectedBlockType;
        }
        // check right
        blockType = terrian->getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z-1);
        if (blockType == EMPTY)
        {
            terrian->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z-1, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->sendVBOdata();
            return selectedBlockType;
        }
        blockType = terrian->getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z+1);
        if (blockType == EMPTY)
        {
            terrian->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z+1, selectedBlockType);
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->destroyVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->createVBOdata();
            terrian->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->sendVBOdata();
            return selectedBlockType;
        }
    }
    return EMPTY;


}

BlockType Player::removeBlock(Terrain* terrian) {
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = 3.f * glm::normalize(this->m_forward);
    float outDist = 0.f;
    glm::ivec3 outBlockHit = glm::ivec3();

    if (gridMarch(rayOrigin, rayDirection, mcr_terrain, &outDist, &outBlockHit)) {
        BlockType blockType = mcr_terrain.getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z);

        terrian->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z, EMPTY);
        terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->destroyVBOdata();
        terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
        terrian->getChunkAt(outBlockHit.x, outBlockHit.z).get()->destroyVBOdata();
        std::cout << "remove block" << std::endl;
        return blockType;
    }
    return EMPTY;

}

