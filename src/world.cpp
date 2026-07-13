#include "world.h"
#include "noise.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

static const int32_t WATER_LEVEL = 0;
static const float CHUNK_PRELOAD_DISTANCE = CHUNK_DIM * 0.8f;
static const float PLAYER_RADIUS = 0.35f;
static const float PLAYER_EYE_HEIGHT = 1.62f;
static const float PLAYER_HEIGHT = 2.0f;
static const int32_t TREE_SPACING = 12;
static const int32_t WOOD_TESSERACT_SPACING = 32;
static const int32_t WOOD_TESSERACT_RADIUS = 3;

struct TreeBase {
  int32_t x;
  int32_t z;
  int32_t w;
  bool enabled;
};

struct WoodTesseractBase {
  int32_t x;
  int32_t y;
  int32_t z;
  int32_t w;
  bool enabled;
};

static inline double clamp01(double value) {
  if (value < 0.0) { return 0.0; }
  if (value > 1.0) { return 1.0; }
  return value;
}

static inline double normalizedNoise(glm::dvec4 loc) {
  return clamp01((simplexNoise4D(loc) + 1.0) * 0.5);
}

static inline double plainsHeight(int32_t x, int32_t z, int32_t w) {
  glm::dvec4 pos(x, z, w, 0);
  double broad = normalizedNoise(glm::dvec4(11, 17, 23, 31) + pos / 180.0);
  double rolling = normalizedNoise(glm::dvec4(37, 41, 43, 47) + pos / 70.0);
  double detail = normalizedNoise(glm::dvec4(53, 59, 61, 67) + pos / 32.0);

  return 3.0 +
         3.0 * broad +
         2.0 * rolling +
         1.0 * detail;
}

static inline int32_t floorDiv(int32_t value, int32_t divisor) {
  return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

static inline uint32_t hashCoords(int32_t x, int32_t z, int32_t w) {
  uint32_t h = 2166136261u;
  h = (h ^ (uint32_t) x) * 16777619u;
  h = (h ^ (uint32_t) z) * 16777619u;
  h = (h ^ (uint32_t) w) * 16777619u;
  h ^= h >> 16;
  h *= 2246822519u;
  h ^= h >> 13;
  h *= 3266489917u;
  h ^= h >> 16;
  return h;
}

static inline int32_t terrainHeight(int32_t x, int32_t z, int32_t w) {
  return (int32_t) std::floor(plainsHeight(x, z, w));
}

static inline TreeBase treeBaseForCell(int32_t cellX, int32_t cellZ, int32_t cellW) {
  uint32_t h = hashCoords(cellX, cellZ, cellW);
  if (h % 3 != 0) {
    return TreeBase{0, 0, 0, false};
  }

  int32_t x = cellX * TREE_SPACING;
  int32_t z = cellZ * TREE_SPACING;
  int32_t w = cellW * TREE_SPACING;

  if (terrainHeight(x, z, w) <= WATER_LEVEL + 1) {
    return TreeBase{0, 0, 0, false};
  }

  return TreeBase{x, z, w, true};
}

static inline WoodTesseractBase woodTesseractBaseForCell(int32_t cellX, int32_t cellZ, int32_t cellW) {
  if (std::abs(cellX + cellZ + cellW) % 2 != 0) {
    return WoodTesseractBase{0, 0, 0, 0, false};
  }

  int32_t x = cellX * WOOD_TESSERACT_SPACING + WOOD_TESSERACT_SPACING / 2;
  int32_t z = cellZ * WOOD_TESSERACT_SPACING + WOOD_TESSERACT_SPACING / 2;
  int32_t w = cellW * WOOD_TESSERACT_SPACING + WOOD_TESSERACT_SPACING / 2;
  int32_t y = terrainHeight(x, z, w);

  if (y <= WATER_LEVEL + 1) {
    return WoodTesseractBase{0, 0, 0, 0, false};
  }

  return WoodTesseractBase{x, y, z, w, true};
}

static inline WoodTesseractBase nearestWoodTesseractBase(int32_t x, int32_t z, int32_t w) {
  int32_t cellX = floorDiv(x, WOOD_TESSERACT_SPACING);
  int32_t cellZ = floorDiv(z, WOOD_TESSERACT_SPACING);
  int32_t cellW = floorDiv(w, WOOD_TESSERACT_SPACING);
  return woodTesseractBaseForCell(cellX, cellZ, cellW);
}

static inline bool isWoodTesseractDoor(WoodTesseractBase base, int32_t x, int32_t y, int32_t z, int32_t w) {
  int32_t minZ = base.z - WOOD_TESSERACT_RADIUS;
  return z == minZ &&
         x == base.x &&
         w == base.w &&
         y >= base.y + 1 &&
         y <= base.y + 2;
}

static inline bool isInsideWoodTesseractVolume(WoodTesseractBase base, int32_t x, int32_t y, int32_t z, int32_t w) {
  return base.enabled &&
         x >= base.x - WOOD_TESSERACT_RADIUS && x <= base.x + WOOD_TESSERACT_RADIUS &&
         y >= base.y && y <= base.y + 2 * WOOD_TESSERACT_RADIUS &&
         z >= base.z - WOOD_TESSERACT_RADIUS && z <= base.z + WOOD_TESSERACT_RADIUS &&
         w >= base.w - WOOD_TESSERACT_RADIUS && w <= base.w + WOOD_TESSERACT_RADIUS;
}

static inline HyperCubeTypes woodTesseractSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  WoodTesseractBase base = nearestWoodTesseractBase(x, z, w);
  if (!isInsideWoodTesseractVolume(base, x, y, z, w)) {
    return HCT_AIR;
  }

  int32_t minX = base.x - WOOD_TESSERACT_RADIUS;
  int32_t maxX = base.x + WOOD_TESSERACT_RADIUS;
  int32_t minY = base.y;
  int32_t maxY = base.y + 2 * WOOD_TESSERACT_RADIUS;
  int32_t minZ = base.z - WOOD_TESSERACT_RADIUS;
  int32_t maxZ = base.z + WOOD_TESSERACT_RADIUS;
  int32_t minW = base.w - WOOD_TESSERACT_RADIUS;
  int32_t maxW = base.w + WOOD_TESSERACT_RADIUS;

  bool shell = x == minX || x == maxX ||
               y == minY || y == maxY ||
               z == minZ || z == maxZ ||
               w == minW || w == maxW;
  if (!shell || isWoodTesseractDoor(base, x, y, z, w)) {
    return HCT_AIR;
  }

  return HCT_WOOD;
}

static inline bool woodTesseractClaimsAir(int32_t x, int32_t y, int32_t z, int32_t w) {
  WoodTesseractBase base = nearestWoodTesseractBase(x, z, w);
  if (!isInsideWoodTesseractVolume(base, x, y, z, w)) {
    return false;
  }

  return woodTesseractSample(x, y, z, w) == HCT_AIR;
}

static inline HyperCubeTypes treeSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  if (y < WATER_LEVEL + 2 || y > 20) {
    return HCT_AIR;
  }

  int32_t cellX = floorDiv(x + TREE_SPACING / 2, TREE_SPACING);
  int32_t cellZ = floorDiv(z + TREE_SPACING / 2, TREE_SPACING);
  int32_t cellW = floorDiv(w + TREE_SPACING / 2, TREE_SPACING);
  TreeBase base = treeBaseForCell(cellX, cellZ, cellW);
  if (!base.enabled) {
    return HCT_AIR;
  }

  int32_t baseY = terrainHeight(base.x, base.z, base.w);
  if (x == base.x && z == base.z && w == base.w &&
      y >= baseY + 1 && y <= baseY + 4) {
    return HCT_WOOD;
  }

  int32_t dx = std::abs(x - base.x);
  int32_t dy = std::abs(y - (baseY + 5));
  int32_t dz = std::abs(z - base.z);
  int32_t dw = std::abs(w - base.w);
  if (dy <= 1 && dx <= 2 && dz <= 2 && dw <= 2 && dx + dy + dz + dw <= 4) {
    return HCT_LEAVES;
  }

  return HCT_AIR;
}

static inline bool isTerrainSolid(int32_t x, int32_t y, int32_t z, int32_t w) {
  return y <= terrainHeight(x, z, w);
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

World::ChunkKey World::keyForChunk(glm::ivec4 chunk) {
  return ChunkKey{chunk.x, chunk.y, chunk.z, chunk.w};
}

HyperCubeTypes World::worldSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  HyperCubeTypes woodTesseractBlock = woodTesseractSample(x, y, z, w);
  if (woodTesseractBlock != HCT_AIR) {
    return woodTesseractBlock;
  }
  if (woodTesseractClaimsAir(x, y, z, w)) {
    return HCT_AIR;
  }

  HyperCubeTypes treeBlock = treeSample(x, y, z, w);
  if (treeBlock != HCT_AIR) {
    return treeBlock;
  }

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
          case HCT_WOOD:
            if (!SURROUNDED) {
              data.woodLocs.push_back(glm::vec4(x, y, z, w));
            }
            break;
          case HCT_LEAVES:
            if (!SURROUNDED) {
              data.leavesLocs.push_back(glm::vec4(x, y, z, w));
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

World::WorldData World::generateChunkData(ChunkKey key) {
  WorldData data;
  generateChunk(data, key[0], key[1], key[2], key[3]);
  return data;
}

std::vector<World::ChunkResult> World::generateChunks(std::vector<ChunkKey> keys) {
  std::vector<ChunkResult> result;
  result.reserve(keys.size());

  for (ChunkKey key : keys) {
    result.push_back(ChunkResult{key, generateChunkData(key)});
  }

  return result;
}

std::vector<World::ChunkKey> World::keysAround(glm::ivec4 centerChunk) {
  std::vector<ChunkKey> keys;
  keys.reserve((2 * ACTIVE_CHUNK_RADIUS_XZ + 1) *
               (2 * ACTIVE_CHUNK_RADIUS_Y + 1) *
               (2 * ACTIVE_CHUNK_RADIUS_XZ + 1) *
               (2 * ACTIVE_CHUNK_RADIUS_W + 1));

  for (int32_t cx=centerChunk.x - ACTIVE_CHUNK_RADIUS_XZ;
       cx<=centerChunk.x + ACTIVE_CHUNK_RADIUS_XZ; cx++) {
    for (int32_t cy=centerChunk.y - ACTIVE_CHUNK_RADIUS_Y;
         cy<=centerChunk.y + ACTIVE_CHUNK_RADIUS_Y; cy++) {
      for (int32_t cz=centerChunk.z - ACTIVE_CHUNK_RADIUS_XZ;
           cz<=centerChunk.z + ACTIVE_CHUNK_RADIUS_XZ; cz++) {
        for (int32_t cw=centerChunk.w - ACTIVE_CHUNK_RADIUS_W;
             cw<=centerChunk.w + ACTIVE_CHUNK_RADIUS_W; cw++) {
          keys.push_back(ChunkKey{cx, cy, cz, cw});
        }
      }
    }
  }

  return keys;
}

void World::applyData(WorldData data) {
  stoneLocs = std::move(data.stoneLocs);
  grassLocs = std::move(data.grassLocs);
  sandLocs = std::move(data.sandLocs);
  waterLocs = std::move(data.waterLocs);
  woodLocs = std::move(data.woodLocs);
  leavesLocs = std::move(data.leavesLocs);
}

void World::rebuildActiveData() {
  WorldData data;

  for (ChunkKey key : keysAround(centerChunk)) {
    auto it = chunks.find(key);
    if (it == chunks.end()) { continue; }

    WorldData &chunk = it->second;
    data.stoneLocs.insert(data.stoneLocs.end(), chunk.stoneLocs.begin(), chunk.stoneLocs.end());
    data.grassLocs.insert(data.grassLocs.end(), chunk.grassLocs.begin(), chunk.grassLocs.end());
    data.sandLocs.insert(data.sandLocs.end(), chunk.sandLocs.begin(), chunk.sandLocs.end());
    data.waterLocs.insert(data.waterLocs.end(), chunk.waterLocs.begin(), chunk.waterLocs.end());
    data.woodLocs.insert(data.woodLocs.end(), chunk.woodLocs.begin(), chunk.woodLocs.end());
    data.leavesLocs.insert(data.leavesLocs.end(), chunk.leavesLocs.begin(), chunk.leavesLocs.end());
  }

  std::cout << "Active tesseracts: "
            << "grass=" << data.grassLocs.size()
            << ", sand=" << data.sandLocs.size()
            << ", stone=" << data.stoneLocs.size()
            << ", wood=" << data.woodLocs.size()
            << ", leaves=" << data.leavesLocs.size()
            << ", water=" << data.waterLocs.size() << std::endl;

  applyData(std::move(data));
}

void World::pruneChunkCache() {
  const int32_t margin = 1;

  for (auto it=chunks.begin(); it!=chunks.end();) {
    const ChunkKey &key = it->first;
    bool keep = std::abs(key[0] - centerChunk.x) <= ACTIVE_CHUNK_RADIUS_XZ + margin &&
                std::abs(key[1] - centerChunk.y) <= ACTIVE_CHUNK_RADIUS_Y + margin &&
                std::abs(key[2] - centerChunk.z) <= ACTIVE_CHUNK_RADIUS_XZ + margin &&
                std::abs(key[3] - centerChunk.w) <= ACTIVE_CHUNK_RADIUS_W + margin;

    if (keep) {
      ++it;
    } else {
      it = chunks.erase(it);
    }
  }
}

glm::ivec4 World::preloadCenterForPosition(glm::vec4 position) {
  glm::ivec4 target = centerChunk;

  float minX = (centerChunk.x - ACTIVE_CHUNK_RADIUS_XZ) * CHUNK_DIM;
  float maxX = (centerChunk.x + ACTIVE_CHUNK_RADIUS_XZ + 1) * CHUNK_DIM;
  float minY = (centerChunk.y - ACTIVE_CHUNK_RADIUS_Y) * CHUNK_DIM;
  float maxY = (centerChunk.y + ACTIVE_CHUNK_RADIUS_Y + 1) * CHUNK_DIM;
  float minZ = (centerChunk.z - ACTIVE_CHUNK_RADIUS_XZ) * CHUNK_DIM;
  float maxZ = (centerChunk.z + ACTIVE_CHUNK_RADIUS_XZ + 1) * CHUNK_DIM;
  float minW = (centerChunk.w - ACTIVE_CHUNK_RADIUS_W) * CHUNK_DIM;
  float maxW = (centerChunk.w + ACTIVE_CHUNK_RADIUS_W + 1) * CHUNK_DIM;

  if (position.x < minX + CHUNK_PRELOAD_DISTANCE) { target.x--; }
  if (position.x > maxX - CHUNK_PRELOAD_DISTANCE) { target.x++; }
  if (position.y < minY + CHUNK_PRELOAD_DISTANCE) { target.y--; }
  if (position.y > maxY - CHUNK_PRELOAD_DISTANCE) { target.y++; }
  if (position.z < minZ + CHUNK_PRELOAD_DISTANCE) { target.z--; }
  if (position.z > maxZ - CHUNK_PRELOAD_DISTANCE) { target.z++; }
  if (position.w < minW + CHUNK_PRELOAD_DISTANCE) { target.w--; }
  if (position.w > maxW - CHUNK_PRELOAD_DISTANCE) { target.w++; }

  return target;
}

bool World::hasChunksFor(glm::ivec4 targetCenterChunk) {
  for (ChunkKey key : keysAround(targetCenterChunk)) {
    if (chunks.find(key) == chunks.end()) {
      return false;
    }
  }

  return true;
}

bool World::requestMissingChunks(glm::ivec4 targetCenterChunk) {
  if (pendingData.valid()) {
    return false;
  }

  std::vector<ChunkKey> missing;
  for (ChunkKey key : keysAround(targetCenterChunk)) {
    if (chunks.find(key) == chunks.end()) {
      missing.push_back(key);
    }
  }

  if (missing.empty()) {
    pendingKeys.clear();
    return false;
  }

  pendingKeys = missing;
  pendingCenterChunk = targetCenterChunk;
  std::cout << "Generating " << missing.size() << " missing chunks around "
            << targetCenterChunk.x << ", "
            << targetCenterChunk.y << ", "
            << targetCenterChunk.z << ", "
            << targetCenterChunk.w << std::endl;
  pendingData = std::async(std::launch::async, [missing]() {
    return generateChunks(missing);
  });

  return false;
}

bool World::updateAround(glm::vec4 position) {
  bool changed = false;

  if (pendingData.valid() &&
      pendingData.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    for (ChunkResult &result : pendingData.get()) {
      chunks[result.key] = std::move(result.data);
    }
    pendingKeys.clear();
    pendingCenterChunk = glm::ivec4(0x7fffffff);
  }

  glm::ivec4 newCenterChunk = chunkForPosition(position);

  if (newCenterChunk != centerChunk) {
    if (hasChunksFor(newCenterChunk)) {
      centerChunk = newCenterChunk;
      rebuildActiveData();
      pruneChunkCache();
      changed = true;
    } else {
      requestMissingChunks(newCenterChunk);
      return changed;
    }
  }

  glm::ivec4 preloadCenter = preloadCenterForPosition(position);
  if (!hasChunksFor(preloadCenter)) {
    requestMissingChunks(preloadCenter);
  }

  return changed;
}

bool World::loadAround(glm::vec4 position) {
  glm::ivec4 newCenterChunk = chunkForPosition(position);

  if (pendingData.valid()) {
    pendingData.wait();
    for (ChunkResult &result : pendingData.get()) {
      chunks[result.key] = std::move(result.data);
    }
    pendingCenterChunk = glm::ivec4(0x7fffffff);
    pendingKeys.clear();
  }

  centerChunk = newCenterChunk;

  std::vector<ChunkKey> missing;
  for (ChunkKey key : keysAround(centerChunk)) {
    if (chunks.find(key) == chunks.end()) {
      missing.push_back(key);
    }
  }

  if (!missing.empty()) {
    std::cout << "Loading " << missing.size() << " chunks around "
              << centerChunk.x << ", "
              << centerChunk.y << ", "
              << centerChunk.z << ", "
              << centerChunk.w << std::endl;
    for (ChunkResult &result : generateChunks(missing)) {
      chunks[result.key] = std::move(result.data);
    }
  }

  rebuildActiveData();
  pruneChunkCache();
  return true;
}

bool World::isSolidAt(glm::vec4 position) {
  return worldSample((int32_t) std::floor(position.x + 0.5f),
                     (int32_t) std::floor(position.y + 0.5f),
                     (int32_t) std::floor(position.z + 0.5f),
                     (int32_t) std::floor(position.w + 0.5f)) >= HCT_SOLID_START;
}

bool World::collidesWithPlayer(glm::vec4 eyePosition) {
  const float EPSILON = 0.001f;
  float minX = eyePosition.x - PLAYER_RADIUS;
  float maxX = eyePosition.x + PLAYER_RADIUS;
  float minY = eyePosition.y - PLAYER_EYE_HEIGHT;
  float maxY = minY + PLAYER_HEIGHT;
  float minZ = eyePosition.z - PLAYER_RADIUS;
  float maxZ = eyePosition.z + PLAYER_RADIUS;
  float minW = eyePosition.w - PLAYER_RADIUS;
  float maxW = eyePosition.w + PLAYER_RADIUS;

  int32_t blockMinX = (int32_t) std::ceil(minX - 0.5f + EPSILON);
  int32_t blockMaxX = (int32_t) std::floor(maxX + 0.5f - EPSILON);
  int32_t blockMinY = (int32_t) std::ceil(minY - 0.5f + EPSILON);
  int32_t blockMaxY = (int32_t) std::floor(maxY + 0.5f - EPSILON);
  int32_t blockMinZ = (int32_t) std::ceil(minZ - 0.5f + EPSILON);
  int32_t blockMaxZ = (int32_t) std::floor(maxZ + 0.5f - EPSILON);
  int32_t blockMinW = (int32_t) std::ceil(minW - 0.5f + EPSILON);
  int32_t blockMaxW = (int32_t) std::floor(maxW + 0.5f - EPSILON);

  for (int32_t x=blockMinX; x<=blockMaxX; x++) {
    for (int32_t y=blockMinY; y<=blockMaxY; y++) {
      for (int32_t z=blockMinZ; z<=blockMaxZ; z++) {
        for (int32_t w=blockMinW; w<=blockMaxW; w++) {
          if (worldSample(x, y, z, w) >= HCT_SOLID_START) { return true; }
        }
      }
    }
  }

  return false;
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
            if (isTerrainSolid(x, y, z, w) && !isTerrainSolid(x, y + 1, z, w)) {
              glm::vec4 landed(x, y + 0.5f + PLAYER_EYE_HEIGHT, z, w);
              glm::vec4 spawn = landed + glm::vec4(0, 4.0f, 0, 0);
              bool clearFallPath = true;
              for (float eyeY=landed.y; eyeY<=spawn.y; eyeY += 0.25f) {
                if (collidesWithPlayer(glm::vec4(spawn.x, eyeY, spawn.z, spawn.w))) {
                  clearFallPath = false;
                  break;
                }
              }

              if (!clearFallPath) {
                continue;
              }

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
