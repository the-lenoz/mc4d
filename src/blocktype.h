//===--- blocktype.h - A block type for rendering purposes ------*- C++ -*-===//
//
//                              MC 4D Renderer
//                        Michael Layzell - CISC 454
//                        Queen's University - W2015
//
//===----------------------------------------------------------------------===//

#ifndef __blocktype_h
#define __blocktype_h

#include "gl.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <vector>

struct BlockType {
  GLuint tex;
  size_t count;
  float indicator;

  BlockType(std::vector<glm::vec4> *pts, float ind) {
    indicator = ind;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    update(pts);
    GL_ERR_CHK;
  }

  void update(std::vector<glm::vec4> *pts) {
    count = pts->size();
    std::cout << "Block type has " << count << " tesseracts\n";

    size_t uploadCount = count > 0 ? count : 1;
    int width = uploadCount < 1024 ? (int) uploadCount : 1024;
    int height = (int) ((uploadCount + width - 1) / width);
    std::vector<glm::vec4> upload(width * height, glm::vec4(0));

    for (size_t i=0; i<count; i++) {
      upload[i] = (*pts)[i];
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
                 width, height,
                 0, GL_RGBA, GL_FLOAT,
                 upload.data());
    GL_ERR_CHK;
  }

  void bind(GLuint texLoc, GLuint countLoc, GLuint indLoc) {
    // Bind the texture
    glUniform1i(texLoc, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Bind the count
    glUniform1f(countLoc, count);

    // Bind the indicator
    glUniform1f(indLoc, indicator);
    GL_ERR_CHK;
  }
};

#endif // defined(__blocktype_h)
