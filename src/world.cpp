#include "world.h"
#include "noise.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

static const int32_t WATER_LEVEL = 0;

static inline double clamp01(double value) {
  if (value < 0.0) { return 0.0; }
  if (value > 1.0) { return 1.0; }
  return value;
}

static inline double normalizedNoise(glm::dvec4 loc) {
  return clamp01((simplexNoise4D(loc) + 1.0) * 0.5);
}

static inline double mountainHeight(int32_t x, int32_t z, int32_t w) {
  glm::dvec4 pos(x, z, w, 0);
  double continent = normalizedNoise(glm::dvec4(11, 17, 23, 31) + pos / 180.0);
  double broad = normalizedNoise(glm::dvec4(37, 41, 43, 47) + pos / 80.0);
  double detail = normalizedNoise(glm::dvec4(53, 59, 61, 67) + pos / 28.0);
  double ridgedRaw = simplexNoise4D(glm::dvec4(71, 73, 79, 83) + pos / 55.0);
  double ridges = 1.0 - std::abs(ridgedRaw);

  ridges = ridges * ridges;

  return -4.0 +
         10.0 * continent +
         18.0 * broad +
         34.0 * ridges +
         6.0 * detail;
}

static inline bool isTerrainSolid(int32_t x, int32_t y, int32_t z, int32_t w) {
  return y <= (int32_t) std::floor(mountainHeight(x, z, w));
}

static inline bool isSurface(int32_t x, int32_t y, int32_t z, int32_t w) {
  return isTerrainSolid(x, y, z, w) && !isTerrainSolid(x, y + 1, z, w);
}

glm::ivec4 World::chunkForPosition(glm::vec4 position) {
  return glm::ivec4((int32_t) std::floor(position.x / CHUNK_DIM),
                    (int32_t) std::floor(position.y / CHUNK_DIM),
                    (int32_t) std::floor(position.z / CHUNK_DIM),
                    (int32_t) std::floor(position.w / CHUNK_DIM));
}

HyperCubeTypes World::worldSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  if (isTerrainSolid(x, y, z, w)) {
    if (isSurface(x, y, z, w)) {
      if (y <= WATER_LEVEL + 1) { return HCT_SAND; }
      if (y >= 38) { return HCT_STONE; }
      return HCT_GRASS;
    }

    return HCT_STONE;
  }

  if (y < WATER_LEVEL) {
    return HCT_WATER;
  }

  return HCT_AIR;
}

void World::generateChunk(WorldData &data, int32_t chunkX, int32_t chunkY, int32_t chunkZ, int32_t chunkW) {
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
              data.grassLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_SAND:
            if (!SURROUNDED) {
              data.sandLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_STONE:
            if (!SURROUNDED) {
              data.stoneLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_WATER:
            if (!SURROUNDED) {
              data.waterLocs.push_back(glm::vec4(x, y, z, w));
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

World::World() : centerChunk(0x7fffffff), pendingCenterChunk(0x7fffffff) {
  initPerm();
}

World::WorldData World::generateAround(glm::ivec4 centerChunk) {
  WorldData data;

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
          generateChunk(data, cx, cy, cz, cw);
        }
      }
    }
  }

  std::cout << "Active tesseracts: "
            << "grass=" << data.grassLocs.size()
            << ", sand=" << data.sandLocs.size()
            << ", stone=" << data.stoneLocs.size()
            << ", water=" << data.waterLocs.size() << std::endl;

  return data;
}

void World::applyData(WorldData data) {
  stoneLocs = std::move(data.stoneLocs);
  grassLocs = std::move(data.grassLocs);
  sandLocs = std::move(data.sandLocs);
  waterLocs = std::move(data.waterLocs);
}

bool World::updateAround(glm::vec4 position) {
  if (pendingData.valid() &&
      pendingData.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    centerChunk = pendingCenterChunk;
    applyData(pendingData.get());
    pendingCenterChunk = glm::ivec4(0x7fffffff);
    return true;
  }

  glm::ivec4 newCenterChunk = chunkForPosition(position);

  if (newCenterChunk == centerChunk ||
      (pendingData.valid() && newCenterChunk == pendingCenterChunk)) {
    return false;
  }

  if (pendingData.valid()) {
    return false;
  }

  pendingCenterChunk = newCenterChunk;
  pendingData = std::async(std::launch::async, [newCenterChunk]() {
    return generateAround(newCenterChunk);
  });

  return false;
}

bool World::loadAround(glm::vec4 position) {
  glm::ivec4 newCenterChunk = chunkForPosition(position);

  if (pendingData.valid()) {
    pendingData.wait();
    pendingData.get();
    pendingCenterChunk = glm::ivec4(0x7fffffff);
  }

  if (newCenterChunk == centerChunk) {
    return false;
  }

  centerChunk = newCenterChunk;
  applyData(generateAround(centerChunk));
  return true;
}

bool World::isSolidAt(glm::vec4 position) {
  return worldSample((int32_t) std::floor(position.x),
                     (int32_t) std::floor(position.y),
                     (int32_t) std::floor(position.z),
                     (int32_t) std::floor(position.w)) >= HCT_SOLID_START;
}

glm::vec4 World::findSurfaceSpawn(glm::vec4 preferredPosition) {
  int32_t baseX = (int32_t) std::floor(preferredPosition.x);
  int32_t baseZ = (int32_t) std::floor(preferredPosition.z);
  int32_t baseW = (int32_t) std::floor(preferredPosition.w);

  for (int32_t radius=0; radius<=32; radius++) {
    for (int32_t dx=-radius; dx<=radius; dx++) {
      for (int32_t dz=-radius; dz<=radius; dz++) {
        for (int32_t dw=-radius; dw<=radius; dw++) {
          if (std::max(std::max(std::abs(dx), std::abs(dz)), std::abs(dw)) != radius) {
            continue;
          }

          int32_t x = baseX + dx;
          int32_t z = baseZ + dz;
          int32_t w = baseW + dw;

          for (int32_t y=96; y>=WATER_LEVEL; y--) {
            HyperCubeTypes block = worldSample(x, y, z, w);
            if (block >= HCT_SOLID_START && worldSample(x, y + 1, z, w) == HCT_AIR) {
              glm::vec4 spawn(x + 0.5f, y + 1.25f, z + 0.5f, w + 0.5f);
              std::cout << "Spawning player at "
                        << spawn.x << ", "
                        << spawn.y << ", "
                        << spawn.z << ", "
                        << spawn.w << std::endl;
              return spawn;
            }
          }
        }
      }
    }
  }

  glm::vec4 fallback(preferredPosition.x, WATER_LEVEL + 8.0f,
                     preferredPosition.z, preferredPosition.w);
  std::cout << "No surface spawn found, using fallback at "
            << fallback.x << ", "
            << fallback.y << ", "
            << fallback.z << ", "
            << fallback.w << std::endl;
  return fallback;
}

void World::draw() {
  std::cerr << "UNIMPLEMENTED\n";
  exit(-1);
}
