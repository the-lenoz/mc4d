//===--- world.h - The Game World -------------------------------*- C++ -*-===//
//
//                              MC 4D Renderer
//                        Michael Layzell - CISC 454
//                        Queen's University - W2015
//
//===----------------------------------------------------------------------===//

#ifndef __world_h
#define __world_h

#include "tesseract.h"

#include "gl.h"

#include <array>
#include <future>
#include <map>
#include <vector>
#include <glm/glm.hpp>

static const int32_t CHUNK_DIM = 10;
static const int32_t ACTIVE_CHUNK_RADIUS_XZ = 3;
static const int32_t ACTIVE_CHUNK_RADIUS_W = 1;
static const int32_t ACTIVE_CHUNK_RADIUS_Y = 1;
static const int32_t CHUNKS_PER_AXIS = 2 * ACTIVE_CHUNK_RADIUS_XZ + 1;
static const int32_t DIM = CHUNK_DIM * CHUNKS_PER_AXIS;
static const int32_t WD_X = DIM;
static const int32_t WD_Y = DIM;
static const int32_t WD_Z = DIM;
static const int32_t WD_W = DIM;
static const glm::ivec4 WORLD_DIM(WD_X, WD_Y, WD_Z, WD_W);

class World {
  GLuint VAO;
  GLuint VBO;

  typedef std::array<int32_t, 4> ChunkKey;

  struct WorldData {
    std::vector<glm::vec4> stoneLocs;
    std::vector<glm::vec4> grassLocs;
    std::vector<glm::vec4> sandLocs;
    std::vector<glm::vec4> waterLocs;
    std::vector<glm::vec4> woodLocs;
    std::vector<glm::vec4> leavesLocs;
  };

  struct ChunkResult {
    ChunkKey key;
    WorldData data;
  };

  glm::ivec4 centerChunk;
  glm::ivec4 pendingCenterChunk;
  std::map<ChunkKey, WorldData> chunks;
  std::vector<ChunkKey> pendingKeys;
  std::future<std::vector<ChunkResult>> pendingData;

  static void generateChunk(WorldData &data, int32_t chunkX, int32_t chunkY, int32_t chunkZ, int32_t chunkW);
  static WorldData generateChunkData(ChunkKey key);
  static std::vector<ChunkResult> generateChunks(std::vector<ChunkKey> keys);
  static std::vector<ChunkKey> keysAround(glm::ivec4 centerChunk);
  static HyperCubeTypes worldSample(int32_t x, int32_t y, int32_t z, int32_t w);
  static glm::ivec4 chunkForPosition(glm::vec4 position);
  static ChunkKey keyForChunk(glm::ivec4 chunk);
  void applyData(WorldData data);
  void rebuildActiveData();
  void pruneChunkCache();
  glm::ivec4 preloadCenterForPosition(glm::vec4 position);
  bool hasChunksFor(glm::ivec4 targetCenterChunk);
  bool requestMissingChunks(glm::ivec4 targetCenterChunk);
public:
  std::vector<glm::vec4> stoneLocs;
  std::vector<glm::vec4> grassLocs;
  std::vector<glm::vec4> sandLocs;
  std::vector<glm::vec4> waterLocs;
  std::vector<glm::vec4> woodLocs;
  std::vector<glm::vec4> leavesLocs;

  World();
  bool loadAround(glm::vec4 position);
  bool updateAround(glm::vec4 position);
  bool isSolidAt(glm::vec4 position);
  bool collidesWithPlayer(glm::vec4 eyePosition);
  glm::vec4 findSurfaceSpawn(glm::vec4 preferredPosition);

  void draw();
};

#endif // defined(__world_h)
