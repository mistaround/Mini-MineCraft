#include "biome.h"
#include <QDebug>

glm::vec2 smoothF(glm::vec2 uv) {
    return uv * uv * (3.0f - 2.0f * uv);
}

float noise(glm::vec2 uv) {
    const float k = 257.0f;
    glm::vec4 l = glm::vec4(glm::floor(uv), glm::fract(uv));
    float u = l.x + l.y * k;
    glm::vec4 v = glm::vec4(u, u + 1.0f, u + k, u + k + 1.0f);
    v = glm::fract(glm::fract(1.23456789f * v) * v / 0.987654321f);
    glm::vec2 zw = smoothF(glm::vec2(l.z, l.w));
    l.z = zw[0];
    l.w = zw[1];
    l.x = glm::mix(v.x, v.y, l.z);
    l.y = glm::mix(v.z, v.w, l.z);
    return glm::mix(l.x, l.y, l.w);
}

float fbm(glm::vec2 uv) {
    float a = 0.5f;
    float f = 5.0f;
    float n = 0.0f;
    int it = 8;
    for(int i = 0; i < 32; i++) {
        if(i < it) {
            n += noise(uv * f) * a;
            a *= 0.5f;
            f *= 2.0f;
        }
    }
    return n;
}

glm::vec2 random2(glm::vec2 p) {
    return glm::normalize(2.0f * glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1f, 311.7f)), glm::dot(p, glm::vec2(269.5f, 183.3f)))) * 43758.5453f) - 1.0f);
}

glm::vec3 random3(glm::vec3 p) {
    // Generate three pseudo-random numbers using sine and dot products
    float x = glm::fract(glm::sin(glm::dot(p, glm::vec3(127.1f, 311.7f, 74.7f))) * 43758.5453f);
    float y = glm::fract(glm::sin(glm::dot(p, glm::vec3(269.5f, 183.3f, 246.1f))) * 43758.5453f);
    float z = glm::fract(glm::sin(glm::dot(p, glm::vec3(419.2f, 371.9f, 218.3f))) * 43758.5453f);

    // Create a vector from these values and normalize it
    return glm::normalize(glm::vec3(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f));
}


float surflet(glm::vec2 P, glm::vec2 gridPoint) {
    float distX = glm::abs(P.x - gridPoint.x);
    float distY = glm::abs(P.y - gridPoint.y);
    float tX = 1.0f - 6.0f * glm::pow(distX, 5.0f) + 15.0f * glm::pow(distX, 4.0f) - 10.0f * glm::pow(distX, 3.0f);
    float tY = 1.0f - 6.0f * glm::pow(distY, 5.0f) + 15.0f * glm::pow(distY, 4.0f) - 10.0f * glm::pow(distY, 3.0f);

    glm::vec2 gradient = random2(gridPoint);
    glm::vec2 diff = P - gridPoint;
    float height = glm::dot(diff, gradient);
    return height * tX * tY;
}

float PerlinNoise(glm::vec2 uv) {
    glm::vec2 uvXLYL = glm::floor(uv);
    glm::vec2 uvXHYL = uvXLYL + glm::vec2(1.0f, 0.0f);
    glm::vec2 uvXHYH = uvXLYL + glm::vec2(1.0f, 1.0f);
    glm::vec2 uvXLYH = uvXLYL + glm::vec2(0.0f, 1.0f);
    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
}

glm::vec2 hash(glm::vec2 p) {
    p = glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)), glm::dot(p, glm::vec2(269.5, 183.3)));
    glm::vec2 fract = glm::fract(glm::vec2(53758.5453123 * glm::cos(p.x), 53758.5453123 * glm::cos(p.y)));
    return glm::vec2(-1.0 + 2.0 * fract.x, -1.0 + 2.0 * fract.y);
}

float step(float edge, float x) {
    return x < edge ? 0.f : 1.f;
}

glm::vec3 step(glm::vec3 edge, glm::vec3 x) {
    return glm::vec3(
        (x.x >= edge.x) ? 1.0f : 0.0f,
        (x.y >= edge.y) ? 1.0f : 0.0f,
        (x.z >= edge.z) ? 1.0f : 0.0f
        );
}

float SimplexNoise(glm::vec2 uv) {
    float K1 = 0.366025404;
    float K2 = 0.211324865;

    glm::vec2  i = glm::floor(uv + (uv.x + uv.y) * K1);
    glm::vec2  a = uv - i + (i.x + i.y) * K2;
    float m = step(float(a.y), float(a.x));
    glm::vec2  o = glm::vec2(m, 1.0 - m);
    glm::vec2  b = a - o + K2;
    glm::vec2  c = glm::vec2(a.x - 1.0 + 2.0 * K2, a.y - 1.0 + 2.0 * K2);
    glm::vec3  h = glm::max(glm::vec3(0.5) - glm::vec3(glm::dot(a,a), glm::dot(b,b), glm::dot(c,c)), glm::vec3(0.0));
    glm::vec3  n = h*h*h*h * glm::vec3(glm::dot(a, hash(i+glm::vec2(0.0))), glm::dot(b,hash(i+o)), glm::dot(c, hash(i+glm::vec2(1.0))));
    return glm::dot(n, glm::vec3(70.0));
}

glm::vec3 hash(glm::vec3 p) {
    return glm::vec3(
        glm::fract(glm::sin(glm::dot(p, glm::vec3(127.1, 311.7, 74.7))) * 43758.5453),
        glm::fract(glm::sin(glm::dot(p, glm::vec3(269.5, 183.3, 246.1))) * 43758.5453),
        glm::fract(glm::sin(glm::dot(p, glm::vec3(113.5, 271.9, 124.6))) * 43758.5453)
        );
}


float SimplexNoise(glm::vec3 pos) {
    const float F3 = 0.3333333f;
    const float G3 = 0.1666667f;
    // Find current tetrahedron T and its four vertices
    glm::vec3 s = glm::floor(pos + glm::dot(pos, glm::vec3(F3)));
    glm::vec3 x = pos - s + glm::dot(s, glm::vec3(G3));

    // Calculate i1 and i2
    glm::vec3 e = step(glm::vec3(0.0), x - glm::vec3(x.y, x.z, x.x));
    glm::vec3 i1 = e * (1.0f - glm::vec3(e.z, e.x, e.y));
    glm::vec3 i2 = 1.0f - glm::vec3(e.z, e.x, e.y) * (1.0f - e);

    // x1, x2, x3
    glm::vec3 x1 = x - i1 + G3;
    glm::vec3 x2 = x - i2 + 2.0f * G3;
    glm::vec3 x3 = x - 1.0f + 3.0f * G3;

    // Find four surflets and store them in d
    glm::vec4 w, d;

    // Calculate surflet weights
    w.x = glm::dot(x, x);
    w.y = glm::dot(x1, x1);
    w.z = glm::dot(x2, x2);
    w.w = glm::dot(x3, x3);
    w = glm::max(0.6f - w, 0.0f);

    // Calculate surflet components
    d.x = glm::dot(random3(s), x);
    d.y = glm::dot(random3(s + i1), x1);
    d.z = glm::dot(random3(s + i2), x2);
    d.w = glm::dot(random3(s + glm::vec3(1.0)), x3);

    // Multiply d by w^4
    w *= w;
    w *= w;
    d *= w;

    // Return the sum of the four surflets
    return glm::dot(d, glm::vec4(52.0));
}


float surflet3D(glm::vec3 p, glm::vec3 gridPoint) {
    // Compute the distance between p and the grid point along each axis
    glm::vec3 t2 = glm::abs(p - gridPoint);

    // Apply the quintic function to smooth the transition, element-wise
    glm::vec3 t = glm::vec3(1.f) - 6.f * glm::vec3(glm::pow(t2.x, 5.f), glm::pow(t2.y, 5.f), glm::pow(t2.z, 5.f))
                  + 15.f * glm::vec3(glm::pow(t2.x, 4.f), glm::pow(t2.y, 4.f), glm::pow(t2.z, 4.f))
                  - 10.f * glm::vec3(glm::pow(t2.x, 3.f), glm::pow(t2.y, 3.f), glm::pow(t2.z, 3.f));

    // Get the random vector for the grid point
    glm::vec3 gradient = random3(gridPoint) * 2.f - glm::vec3(1.f, 1.f, 1.f);

    // Get the vector from the grid point to P
    glm::vec3 diff = p - gridPoint;

    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);

    // Scale our height field by our polynomial falloff function
    return height * t.x * t.y * t.z;
}



float PerlinNoise(glm::vec3 pos) {
    // Floor the position to get the integer coordinates of the cube
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet3D(pos, glm::floor(pos) + glm::vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}

float WorleyNoise(glm::vec2 uv) {
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);
    float minDist = 1.0f;
    float secondMinDist = 1.0f;
    glm::vec2 closestPoint;
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y));
            glm::vec2 point = random2(uvInt + neighbor);
            glm::vec2 diff = neighbor + point - uvFract;
            float dist = glm::length(diff);
            if(dist < minDist) {
                secondMinDist = minDist;
                minDist = dist;
                closestPoint = point;
            } else if(dist < secondMinDist) {
                secondMinDist = dist;
            }
        }
    }
    return 0.5f * (minDist + secondMinDist);
}

glm::vec2 randomizeUV(glm::vec2 uv, float amp, float freq) {
    float world_x = uv.x;
    float world_z = uv.y;
    return glm::vec2(world_x * cos(M_PI * amp) - sin(M_PI * amp) * world_z,
                     world_x * sin(M_PI * amp) + cos(M_PI * amp) * world_z) / freq;
}

float peakHeight(glm::vec2 xz) {
    float h = 0.0f;
    float amp = 0.5f;
    float freq = 128.0f;
    for(int i = 0; i < 4; ++i) {
        glm::vec2 offset = glm::vec2(fbm(xz / 256.0f), fbm(xz / 300.0f) + 1000.0f);
        float h1 = PerlinNoise((xz + offset * 75.0f) / freq);
        h += h1 * amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    h = (h + 1.f) * 0.5f;
    h = glm::smoothstep(0.f, 0.75f, h);
    h = glm::floor(15.f + h * h * 100.0f);
    return h;
}

float midHeight(glm::vec2 xz) {
    float h = 0.0f;
    float amp = 0.5f;
    float freq = 128.0f;
    for(int i = 0; i < 4; ++i) {
        glm::vec2 offset = glm::vec2(fbm(xz / 256.0f), fbm(xz / 300.0f) + 1000.0f);
        float h1 = PerlinNoise((xz + offset * 75.0f) / freq);
        h += h1 * amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    h = glm::floor(15.f + h * 75.0f);
    return h;
}

float lowHeight(glm::vec2 xz) {
    float h = 0.0f;
    float amp = 0.5f;
    float freq = 128.0f;
    for(int i = 0; i < 4; ++i) {
        glm::vec2 offset = glm::vec2(fbm(xz / 256.0f), fbm(xz / 300.0f) + 1000.0f);
        float h1 = PerlinNoise((xz + offset * 75.0f) / freq);
        h += h1 * amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    h = glm::floor(15.f + h * 50.0f);
    return h;
}

