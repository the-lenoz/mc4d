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
  GLuint buffer;
  size_t count;
  float indicator;

  BlockType(std::vector<glm::vec4> *pts, float ind) {
    count = pts->size();
    indicator = ind;

    std::cout << "Block type has " << count << " tesseracts\n";

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, buffer);
    glBufferData(GL_TEXTURE_BUFFER,
                 count * sizeof(glm::vec4),
                 pts->data(),
                 GL_STATIC_DRAW);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_BUFFER, tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer);
    GL_ERR_CHK;
  }

  void bind(GLuint texLoc, GLuint countLoc, GLuint indLoc) {
    // Bind the texture
    glUniform1i(texLoc, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_BUFFER, tex);

    // Bind the count
    glUniform1f(countLoc, count);

    // Bind the indicator
    glUniform1f(indLoc, indicator);
    GL_ERR_CHK;
  }
};

#endif // defined(__blocktype_h)
