#version 410

in vec3 pos;

out vec4 fragColor;

void main()
{
  float t = clamp(normalize(pos).y * 0.5 + 0.5, 0.0, 1.0);
  vec3 horizon = vec3(0.62, 0.84, 1.0);
  vec3 zenith = vec3(0.28, 0.58, 0.95);
  fragColor = vec4(mix(horizon, zenith, t), 1.0);
}
