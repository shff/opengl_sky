#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GL(line) do { line; assert(glGetError() == GL_NO_ERROR); } while(0)
#define GLSL(str) (const char*)"#version 330\n" #str

// Regular Shaders

const char* vertShader = GLSL(
  layout(location = 0) in vec3 position;
  layout(location = 1) in vec2 vUV;
  uniform mat4 P;
  uniform mat4 V;
  out vec2 fUV;

  void main()
  {
    gl_Position = P * V * vec4(position, 1);
    fUV = vUV;
  }
);

const char* fragShader = GLSL(
  in vec2 fUV;
  out vec4 color;
  uniform sampler2D tex;

  void main()
  {
    color = texture(tex, fUV);
  }
);

// Sky Shaders

const char* skyVertShader = GLSL(
  out vec3 pos;
  out vec3 fsun;
  uniform mat4 P;
  uniform mat4 V;
  uniform float time = 0.0;

  const vec2 data[4] = vec2[](
    vec2(-1.0,  1.0), vec2(-1.0, -1.0),
    vec2( 1.0,  1.0), vec2( 1.0, -1.0));

  void main()
  {
    gl_Position = vec4(data[gl_VertexID], 0.0, 1.0);
    pos = transpose(mat3(V)) * (inverse(P) * gl_Position).xyz;
    fsun = vec3(0.0, sin(time * 0.01), cos(time * 0.01));
  }
);

const char* skyFragShader = GLSL(
  in vec3 pos;
  in vec3 fsun;
  out vec4 color;
  uniform float time = 0.0;
  uniform float cirrus = 0.4;
  uniform float cumulus = 0.8;

  const float Br = 0.0025;
  const float Bm = 0.0003;
  const float g =  0.9800;
  const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
  const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
  const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

  float hash(float n)
  {
    return fract(sin(n) * 43758.5453123);
  }

  float noise(vec3 x)
  {
    vec3 f = fract(x);
    float n = dot(floor(x), vec3(1.0, 157.0, 113.0));
    return mix(mix(mix(hash(n +   0.0), hash(n +   1.0), f.x),
                   mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
               mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                   mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
  }

  const mat3 m = mat3(0.0, 1.60,  1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
  float fbm(vec3 p)
  {
    float f = 0.0;
    f += noise(p) / 2; p = m * p * 1.1;
    f += noise(p) / 4; p = m * p * 1.2;
    f += noise(p) / 6; p = m * p * 1.3;
    f += noise(p) / 12; p = m * p * 1.4;
    f += noise(p) / 24;
    return f;
  }

  void main()
  {
    if (pos.y < 0)
      discard;

    // Atmosphere Scattering
    float mu = dot(normalize(pos), normalize(fsun));
    float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
    vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

    vec3 day_extinction = exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0 ) * 4.0) * exp(-pos.y * 2.0) * 4.0;
    vec3 night_extinction = vec3(1.0 - exp(fsun.y)) * 0.2;
    vec3 extinction = mix(day_extinction, night_extinction, -fsun.y * 0.2 + 0.5);
    color.rgb = rayleigh * mie * extinction;

    // Cirrus Clouds
    float density = smoothstep(1.0 - cirrus, 1.0, fbm(pos.xyz / pos.y * 2.0 + time * 0.05)) * 0.3;
    color.rgb = mix(color.rgb, extinction * 4.0, density * max(pos.y, 0.0));

    // Cumulus Clouds
    for (int i = 0; i < 3; i++)
    {
      float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * pos.xyz / pos.y + time * 0.3));
      color.rgb = mix(color.rgb, extinction * density * 5.0, min(density, 1.0) * max(pos.y, 0.0));
    }

    // Dithering Noise
    color.rgb += noise(pos * 1000) * 0.01;
  }
);

// Post-Processing Shaders

const char *postVertShader = GLSL(
  out vec2 UV;

  const vec2 data[4] = vec2[](
    vec2(-1.0,  1.0), vec2(-1.0, -1.0),
    vec2( 1.0,  1.0), vec2( 1.0, -1.0));

  void main()
  {
    gl_Position = vec4(data[gl_VertexID], 0.0, 1.0);
    UV = gl_Position.xy * 0.5 + 0.5;
  }
);

const char *postFragShader = GLSL(
  in vec2 UV;
  out vec4 color;
  uniform sampler2D tex[2];

  void main()
  {
    color = texture(tex[0], UV);
    float depth = texture(tex[1], UV).r;

    // Ambient Occlusion
    vec2 r = 4.0 / textureSize(tex[0], 0);
    float occlusion = 0.0;
    for (int i = -2; i < 3; i++)
    {
      for (int j = -2; j < 3; j++)
      {
        occlusion += 1.0 / (1.0 + pow(10.0 * min(depth - texture(tex[1], UV + vec2(i, j) * r).r, 0.0), 2.0)) / 24.0;
      }
    }
    color.rgb *= occlusion;

    // Gamma Correction
    color.rgb = pow(1.0 - exp(-1.3 * color.rgb), vec3(1.3));
  }
);

// Constants

float floorCoords[] = {
   30.0f, -1.0f, -30.0f, 5.0f, 0.0f, 0.0f,
  -30.0f, -1.0f, -30.0f, 0.0f, 0.0f, 0.0f,
   30.0f, -1.0f,  30.0f, 5.0f, 5.0f, 0.0f,
  -30.0f, -1.0f,  30.0f, 0.0f, 5.0f, 0.0f,
};

// Structures

typedef struct { float x, y, z; } vector;
typedef struct { float m[16]; } matrix;
typedef struct __attribute__((packed)) { char magic[2]; unsigned int size, reserved, offset, hsize, width, height, colors, compression, image_size, h_res, v_res, palletes, colors2; } bmp_header;

typedef struct { float x, y, z, r, r2; double px, py; } gamestate;
typedef struct { unsigned int vao, buffer, vertices, program, textures[256], fb; int depth_test, texcount, P, V, M, tex, time; } entity;
typedef struct { entity* entities; unsigned int entity_count; gamestate state; } scene;

// Math Functions

matrix getProjectionMatrix(int w, int h)
{
  float fov = 65.0f;
  float aspect = (float)w / (float)h;
  float near = 1.0f;
  float far = 1000.0f;

  return (matrix) { .m = {
    [0] = 1.0f / (aspect * tanf(fov * 3.14f / 180.0f / 2.0f)),
    [5] = 1.0f / tanf(fov * 3.14f / 180.0f / 2.0f),
    [10] = -(far + near) / (far - near),
    [11] = -1.0f,
    [14] = -(2.0f * far * near) / (far - near)
  }};
}

matrix getViewMatrix(float x, float y, float z, float a, float p)
{
  float cosy = cosf(a), siny = sinf(a), cosp = cosf(p), sinp = sinf(p);

  return (matrix) { .m = {
    [0] = cosy,
    [1] = siny * sinp,
    [2] = siny * cosp,
    [5] = cosp,
    [6] = -sinp,
    [8] = -siny,
    [9] = cosy * sinp,
    [10] = cosp * cosy,
    [12] = -(cosy * x - siny * z),
    [13] = -(siny * sinp * x + cosp * y + cosy * sinp * z),
    [14] = -(siny * cosp * x - sinp * y + cosp * cosy * z),
    [15] = 1.0f,
  }};
}

// OpenGL Helpers

void glAssert(unsigned int obj, GLenum statusType, void (*ivFun)(GLuint, GLenum, GLint*),
  void (*infoLogFun)(GLuint, GLsizei, GLsizei*, GLchar*))
{
  GLint statusCode = GL_FALSE;
  ivFun(obj, statusType, &statusCode);
  if (statusCode == GL_TRUE)
  {
    return;
  }

  GLint length = 0;
  ivFun(obj, GL_INFO_LOG_LENGTH, &length);

  char error_log[length];
  infoLogFun(obj, length, &length, &error_log[0]);

  fprintf(stderr, "%s\n", error_log);
  exit(0);
}

unsigned int makeShader(const char* code, GLenum shaderType)
{
  unsigned int shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &code, NULL);
  glCompileShader(shader);

  glAssert(shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog);

  return shader;
}

unsigned int makeProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
  unsigned int vertexShader = vertexShaderSource ? makeShader(vertexShaderSource, GL_VERTEX_SHADER) : 0;
  unsigned int fragmentShader = fragmentShaderSource ? makeShader(fragmentShaderSource, GL_FRAGMENT_SHADER) : 0;

  unsigned int program = glCreateProgram();
  if (vertexShader) { glAttachShader(program, vertexShader); }
  if (fragmentShader) { glAttachShader(program, fragmentShader); }
  glLinkProgram(program);

  glAssert(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog);

  if (vertexShader) { glDetachShader(program, vertexShader); }
  if (vertexShader) { glDeleteShader(vertexShader); }
  if (fragmentShader) { glDetachShader(program, fragmentShader); }
  if (fragmentShader) { glDeleteShader(fragmentShader); }

  return program;
}

unsigned int loadTexture(char* filename)
{
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  bmp_header h;
  FILE* bmp = fopen(filename, "r");
  fread(&h, sizeof(bmp_header), 1, bmp);
  if (h.magic[0] != 'B' || h.magic[1] != 'M') { return texture; }
  char buffer[h.image_size];
  fread(&buffer, h.image_size, 1, bmp);
  fclose(bmp);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, (int)h.width, (int)h.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &buffer);

  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  return texture;
}

unsigned int blankTexture(int w, int h, int format)
{
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, (GLenum)format, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  return texture;
}

unsigned int makeFramebuffer(unsigned int* renderTexture, unsigned int* depthTexture, int w, int h)
{
  *renderTexture = blankTexture(w, h, GL_RGBA);
  *depthTexture = blankTexture(w, h, GL_DEPTH_COMPONENT);

  unsigned int framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *renderTexture, 0);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *depthTexture, 0);
  glDrawBuffers(2, (GLenum[]) { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT });
  return framebuffer;
}

unsigned int makeBuffer(GLenum target, size_t size, void* data)
{
  unsigned int buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, (long)size, data, GL_STATIC_DRAW);
  return buffer;
}

// Entities

entity makeEntity(scene *s, const char* vs, const char* fs, int texcount, char textures[][40],
  void* data, unsigned int vertices, unsigned int layouts, int is_framebuffer, int depth_test,
  int w, int h)
{
  entity e = { .vertices = vertices, .texcount = texcount, .depth_test = depth_test };

  // Create VAO
  glGenVertexArrays(1, &e.vao);
  glBindVertexArray(e.vao);

  // Create Buffer
  e.buffer = makeBuffer(GL_ARRAY_BUFFER, sizeof(float) * vertices * layouts * 3, data);
  glBindBuffer(GL_ARRAY_BUFFER, e.buffer);

  // Load Attribute Pointers
  for (unsigned int i = 0; i < layouts; i++)
  {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, (int)(sizeof(float) * layouts * 3), (void*)(sizeof(float) * i * 3));
  }

  // Load Program
  e.program = makeProgram(vs, fs);
  e.P = glGetUniformLocation(e.program, "P");
  e.V = glGetUniformLocation(e.program, "V");
  e.M = glGetUniformLocation(e.program, "M");
  e.tex = glGetUniformLocation(e.program, "tex");
  e.time = glGetUniformLocation(e.program, "time");

  // Load Textures
  if (!is_framebuffer)
    for (int i = 0; i < texcount; i++)
      if (textures[i][0] > 0)
        e.textures[i] = loadTexture(textures[i]);

  // Create a framebuffer if applicable
  if (is_framebuffer)
    e.fb = makeFramebuffer(&e.textures[0], &e.textures[1], w, h);

  s->entities = realloc(s->entities, ++s->entity_count * sizeof(entity));
  memcpy(&s->entities[s->entity_count - 1], &e, sizeof(entity));

  return e;
}

void renderEntity(entity e, matrix P, matrix V, float time)
{
  glUseProgram(e.program);
  for(int i = 0; i < (e.fb ? 2 : e.texcount); i++)
    glActiveTexture(GL_TEXTURE0 + (unsigned int)i), glBindTexture(GL_TEXTURE_2D, e.textures[i]), glUniform1i(e.tex + i, i);
  glUniformMatrix4fv(e.P, 1, GL_FALSE, P.m);
  glUniformMatrix4fv(e.V, 1, GL_FALSE, V.m);
  glUniform1f(e.time, time);

  if (e.fb)
    glBindFramebuffer(GL_FRAMEBUFFER, 0), glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (e.depth_test == 0)
    glDisable(GL_DEPTH_TEST);
  glBindVertexArray(e.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, (int)e.vertices);
  if (e.depth_test == 0)
    glEnable(GL_DEPTH_TEST);
}

void deleteEntity(entity e)
{
  glDeleteProgram(e.program);
  glDeleteTextures(e.texcount, e.textures);
  glDeleteBuffers(1, &e.buffer);
  glDeleteFramebuffers(1, &e.fb);
  glDeleteVertexArrays(1, &e.vao);
}

// Scene

scene makeScene()
{
  scene s = { .entity_count = 0, .entities = 0, .state = { .x = 0.0f, .y = 2.0f, .z = -3.0f, .r = 3.14f, .r2 = 0.0f } };
  return s;
}

void renderScene(scene s, int w, int h, float time)
{
  matrix p = getProjectionMatrix(w, h);
  matrix v = getViewMatrix(s.state.x, s.state.y, s.state.z, s.state.r, s.state.r2);
  for (unsigned int i = 0; i < s.entity_count; i++)
    if (!s.entities[i].fb)
      renderEntity(s.entities[i], p, v, time);
  for (unsigned int i = 0; i < s.entity_count; i++)
    if (s.entities[i].fb)
      renderEntity(s.entities[i], p, v, 0.0f);
}

void deleteScene(scene s)
{
  for (unsigned int i = 0; i < s.entity_count; i++)
    deleteEntity(s.entities[i]);
  free(s.entities);
}

// Main Loop

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwMakeContextCurrent(window);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  scene s = makeScene();
  makeEntity(&s, skyVertShader, skyFragShader, 0, NULL, NULL, 4, 0, 0, 0, 0, 0);
  makeEntity(&s, postVertShader, postFragShader, 0, NULL, NULL, 4, 0, 1, 0, 800, 600);

  glfwGetCursorPos(window, &s.state.px, &s.state.py);
  while(!glfwWindowShouldClose(window))
  {
    // Move Cursor
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    s.state.r -= (float)(mx - s.state.px) * 0.01f;
    s.state.r2 -= (float)(my - s.state.py) * 0.01f;
    s.state.px = (float)mx;
    s.state.py = (float)my;

    // Clear Framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, s.entities[1].fb);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the Scene
    float time = (float)glfwGetTime() * 0.2f - 0.0f;
    renderScene(s, 800, 600, time);

    // Swap
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  deleteScene(s);

  glfwTerminate();
  return 0;
}
