#include "world.h"
#include "noise.h"

#include <cmath>

static const double WORLD_NOISE_SCALE = 40.0;
static const int32_t WATER_LEVEL = 0;

static inline double noiseSample(glm::dvec4 loc) {
  double rawSimplex = simplexNoise4D(glm::dvec4(1, 1, 1, 1) +
                                     (loc / glm::dvec4(WORLD_NOISE_SCALE)));

  return rawSimplex + loc.y / WORLD_NOISE_SCALE;
}

glm::ivec4 World::chunkForPosition(glm::vec4 position) {
  return glm::ivec4((int32_t) std::floor(position.x / CHUNK_DIM),
                    (int32_t) std::floor(position.y / CHUNK_DIM),
                    (int32_t) std::floor(position.z / CHUNK_DIM),
                    (int32_t) std::floor(position.w / CHUNK_DIM));
}

HyperCubeTypes World::worldSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  bool solid = noiseSample(glm::dvec4(x, y, z, w)) < 0.5;

  if (solid) {
    bool solidAbove = noiseSample(glm::dvec4(x, y + 1, z, w)) < 0.5;
    if (!solidAbove) {
      return y >= WATER_LEVEL ? HCT_GRASS : HCT_SAND;
    }

    return HCT_STONE;
  }

  if (y < WATER_LEVEL) {
    return HCT_WATER;
  }

  return HCT_AIR;
}

void World::generateChunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ, int32_t chunkW) {
  for (int32_t lx=0; lx<CHUNK_DIM; lx++) {
    for (int32_t ly=0; ly<CHUNK_DIM; ly++) {
      for (int32_t lz=0; lz<CHUNK_DIM; lz++) {
        for (int32_t lw=0; lw<CHUNK_DIM; lw++) {
          int32_t x = chunkX * CHUNK_DIM + lx;
          int32_t y = chunkY * CHUNK_DIM + ly;
          int32_t z = chunkZ * CHUNK_DIM + lz;
          int32_t w = chunkW * CHUNK_DIM + lw;
          HyperCubeTypes hct = worldSample(x, y, z, w);

#define SURROUNDED (worldSample(x-1, y, z, w) >= HCT_SOLID_START && \
                    worldSample(x+1, y, z, w) >= HCT_SOLID_START && \
                    worldSample(x, y-1, z, w) >= HCT_SOLID_START && \
                    worldSample(x, y+1, z, w) >= HCT_SOLID_START && \
                    worldSample(x, y, z-1, w) >= HCT_SOLID_START && \
                    worldSample(x, y, z+1, w) >= HCT_SOLID_START && \
                    worldSample(x, y, z, w-1) >= HCT_SOLID_START && \
                    worldSample(x, y, z, w+1) >= HCT_SOLID_START)

          switch (hct) {
          case HCT_AIR:
            break;
          case HCT_GRASS:
            if (!SURROUNDED) {
              grassLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_SAND:
            if (!SURROUNDED) {
              sandLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_STONE:
            if (!SURROUNDED) {
              stoneLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_WATER:
            if (!SURROUNDED) {
              waterLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          default:
            std::cerr << "INVALID BLOCK TYPE " << hct << "\n";
            exit(-1);
          }

#undef SURROUNDED
        }
      }
    }
  }
}

World::World() : centerChunk(0x7fffffff) {
  initPerm();
  updateAround(glm::vec4(0));
}

bool World::updateAround(glm::vec4 position) {
  glm::ivec4 newCenterChunk = chunkForPosition(position);

  if (newCenterChunk == centerChunk) {
    return false;
  }

  centerChunk = newCenterChunk;
  stoneLocs.clear();
  grassLocs.clear();
  sandLocs.clear();
  waterLocs.clear();

  std::cout << "Generating chunks around "
            << centerChunk.x << ", "
            << centerChunk.y << ", "
            << centerChunk.z << ", "
            << centerChunk.w << std::endl;

  for (int32_t cx=centerChunk.x - ACTIVE_CHUNK_RADIUS;
       cx<=centerChunk.x + ACTIVE_CHUNK_RADIUS; cx++) {
    for (int32_t cy=centerChunk.y - ACTIVE_CHUNK_RADIUS;
         cy<=centerChunk.y + ACTIVE_CHUNK_RADIUS; cy++) {
      for (int32_t cz=centerChunk.z - ACTIVE_CHUNK_RADIUS;
           cz<=centerChunk.z + ACTIVE_CHUNK_RADIUS; cz++) {
        for (int32_t cw=centerChunk.w - ACTIVE_CHUNK_RADIUS;
             cw<=centerChunk.w + ACTIVE_CHUNK_RADIUS; cw++) {
          generateChunk(cx, cy, cz, cw);
        }
      }
    }
  }

  std::cout << "Active tesseracts: "
            << "grass=" << grassLocs.size()
            << ", sand=" << sandLocs.size()
            << ", stone=" << stoneLocs.size()
            << ", water=" << waterLocs.size() << std::endl;

  return true;
}

void World::draw() {
  std::cerr << "UNIMPLEMENTED\n";
  exit(-1);
}
