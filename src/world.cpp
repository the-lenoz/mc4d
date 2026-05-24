#include "world.h"
#include "noise.h"

static inline double noiseSample(glm::dvec4 loc) {
  double rawSimplex = simplexNoise4D(glm::dvec4(1, 1, 1, 1) + // This is kinda like a seed?
                                     (loc / glm::dvec4(WORLD_DIM)));

  return rawSimplex + loc.y/WORLD_DIM.y;
}

HyperCubeTypes World::worldSample(int32_t x, int32_t y, int32_t z, int32_t w) {
  if (x < 0 || y < 0 || z < 0 || w < 0) {
    return HCT_AIR;// TONE;
  } else if (x >= WD_X || y >= WD_Y || z >= WD_Z || w >= WD_W) {
    return HCT_AIR;
  } else {
    return hypercubes[x][y][z][w];
  }
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
          double sample = noiseSample(glm::dvec4(x, y, z, w));

          if (sample < 0.5) {
            hypercubes[x][y][z][w] = HCT_STONE;
          } else {
            hypercubes[x][y][z][w] = HCT_AIR;
          }
        }
      }
    }
  }
}

World::World() {
  std::cout << noiseSample(glm::ivec4(0, 0, 0, 0)) << "\n";

  // Ensure that the perm matrix is initialized
  initPerm();
  // Loop over the possible things in the world, chunk by chunk.
  std::cout << "Starting chunked world generation ("
            << CHUNKS_PER_AXIS << "^4 chunks, "
            << CHUNK_DIM << "^4 cells each)" << std::endl;
  int32_t x, y, z, w;
  for (int32_t cx=0; cx<CHUNKS_PER_AXIS; cx++) {
    for (int32_t cy=0; cy<CHUNKS_PER_AXIS; cy++) {
      for (int32_t cz=0; cz<CHUNKS_PER_AXIS; cz++) {
        for (int32_t cw=0; cw<CHUNKS_PER_AXIS; cw++) {
          generateChunk(cx, cy, cz, cw);
        }
      }
    }
    std::cout << cx + 1 << "/" << CHUNKS_PER_AXIS << " chunk slabs\n";
  }
  std::cout << "Done world generation" << std::endl;

  std::cout << "Growing grass" << std::endl;
  for (x=0; x<WORLD_DIM.x; x++) {
    for (y=0; y<WORLD_DIM.y; y++) {
      for (z=0; z<WORLD_DIM.z; z++) {
        for (w=0; w<WORLD_DIM.w; w++) {
          if (worldSample(x, y+1, z, w) == HCT_AIR && hypercubes[x][y][z][w] == HCT_STONE) {
            if (y >= WORLD_DIM.y / 2) {
              hypercubes[x][y][z][w] = HCT_GRASS;
            } else {
              hypercubes[x][y][z][w] = HCT_SAND;
            }
          }
        }
      }
    }
    std::cout << x << "/" << WORLD_DIM.x << "\n";
  }
  std::cout << "Done growing grass" << std::endl;

  std::cout << "Filling ponds" << std::endl;
  for (x=0; x<WORLD_DIM.x; x++) {
    for (y=0; y<WORLD_DIM.y / 2; y++) { // Only half filled with water
      for (z=0; z<WORLD_DIM.z; z++) {
        for (w=0; w<WORLD_DIM.w; w++) {
          if (hypercubes[x][y][z][w] == HCT_AIR) {
            hypercubes[x][y][z][w] = HCT_WATER;
          }
        }
      }
    }
    std::cout << x << "/" << WORLD_DIM.x << "\n";
  }
  std::cout << "Done filling ponds" << std::endl;

  std::cout << "Starting mesh generation" << std::endl;

  for (x=0; x<WORLD_DIM.x; x++) {
    for (y=0; y<WORLD_DIM.y; y++) {
      for (z=0; z<WORLD_DIM.z; z++) {
        for (w=0; w<WORLD_DIM.w; w++) {
          HyperCubeTypes hct = hypercubes[x][y][z][w];

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
        }
      }
    }
    std::cout << x << "/" << WORLD_DIM.x << "\n";
  }
  std::cout << "Done mesh generation" << std::endl;

}

void World::draw() {
  std::cerr << "UNIMPLEMENTED\n";
  exit(-1);
}
