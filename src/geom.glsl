#version 410

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

// the 3d eye position
uniform vec3 eyePos3;
uniform int sliceMode;
uniform float sliceThickness;

in vec4 vcolor[];
out vec4 color;

in vec3 vpos3[];
in float vdepth4[];
in float vsliceCenterDist[];
out vec3 pos3;

out vec3 normal;

void main()
{
  if (sliceMode != 0 && abs(vsliceCenterDist[0]) > sliceThickness) {
    return;
  }

  if (sliceMode == 0 &&
      (vdepth4[0] <= 0.25 || vdepth4[1] <= 0.25 || vdepth4[2] <= 0.25)) {
    return;
  }

  // Calculate a normal
  vec3 a = vpos3[1] - vpos3[0];
  vec3 b = vpos3[2] - vpos3[0];
  normal = normalize(cross(a, b));

  // Ensure it is pointing toward the camera
  if (dot(normal, vpos3[0] - eyePos3) > 0) {
    normal = -normal;
  }

  for(int i = 0; i < gl_in.length(); i++)
  {
     // copy attributes
    gl_Position = gl_in[i].gl_Position;
    color = vcolor[i];
    pos3 = vpos3[i];

    // done with the vertex
    EmitVertex();
  }

  EndPrimitive();
}
