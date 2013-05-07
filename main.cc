//
// aobench in OpenGL compute shader by syoyo.
//
// Thanks to http://wili.cc/blog/opengl-cs.html for OpenGL compute shader programming tutorial.
//
#include <GL/glew.h>
#include <GL/glfw.h>

#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#else
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif

#include <map>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fstream>

namespace {

GLuint  gTexID;
GLuint  gRenderProg;
GLuint  gComputeProg;
GLuint  gComputeShader;

void checkErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}


bool
LoadShader(
  GLenum shaderType,  // GL_VERTEX_SHADER or GL_FRAGMENT_SHADER(or maybe GL_COMPUTE_SHADER)
  GLuint& shader,
  const char* shaderSourceFilename)
{
  GLint val = 0;

  // free old shader/program
  if (shader != 0) glDeleteShader(shader);

  static GLchar srcbuf[16384];
  FILE *fp = fopen(shaderSourceFilename, "rb");
  if (!fp) {
    fprintf(stderr, "failed to load shader: %s\n", shaderSourceFilename);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  rewind(fp);
  len = fread(srcbuf, 1, len, fp);
  srcbuf[len] = 0;
  fclose(fp);

  static const GLchar *src = srcbuf;

  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    GLsizei msglen;
    glGetShaderInfoLog(shader, 4096, &msglen, log);
    printf("%s\n", log);
    assert(val == GL_TRUE && "failed to compile shader");
  }

  printf("Load shader [ %s ] OK\n", shaderSourceFilename);
  return true;
}

bool
LinkShader(
  GLuint& prog,
  GLuint& vertShader,
  GLuint& fragShader)
{
  GLint val = 0;
  
  if (prog != 0) {
    glDeleteProgram(prog);
  }

  prog = glCreateProgram();

  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");

  printf("Link shader OK\n");

  return true;
}

void GLFWCALL key(
  int key,
  int action)
{
  if (action != GLFW_PRESS) return;

  switch (key) {
  case GLFW_KEY_ESC:
    exit(1);
    break;
  default:
    return;
  }
}

void GLFWCALL reshape(
  int width,
  int height)
{
  GLfloat h = (GLfloat) height / (GLfloat) width;
  GLfloat xmax, znear, zfar;

  znear = 1.0f;
  zfar  = 1000.0f;
  xmax  = znear * 0.5f;

  glViewport( 0, 0, (GLint) width, (GLint) height );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glFrustum( -xmax, xmax, -xmax*h, xmax*h, znear, zfar );

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -5.0);
}

void updateTex(int frame) 
{
  glUseProgram(gComputeProg);
  glUniform1f(glGetUniformLocation(gComputeProg, "time"), (float)frame*0.01f);
  glDispatchCompute(512/16, 512/16, 1); // 512^2 threads in blocks of 16^2
  checkErrors("Dispatch compute shader");

}

void GLFWCALL draw()
{
  glClearColor(0.075, 0.075, 0.2, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(gRenderProg);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  checkErrors("Draw screen");
  //swapBuffers();

}

GLuint
myRenderShaderInit()
{
  GLuint progHandle = glCreateProgram();
  GLuint vp = glCreateShader(GL_VERTEX_SHADER);
  GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);

  const char *vpSrc[] = {
    "#version 420\n",
    "in vec2 pos;\
     out vec2 texCoord;\
     void main() {\
       texCoord = pos*0.5f + 0.5f;\
       gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);\
     }"
  };

  const char *fpSrc[] = {
    "#version 420\n",
    "uniform sampler2D srcTex;\
     in vec2 texCoord;\
     out vec4 color;\
     void main() {\
       vec3 rgb = texture(srcTex, texCoord).xyz;\
       color = vec4(rgb, 1.0);\
     }"
  };

    glShaderSource(vp, 2, vpSrc, NULL);
    glShaderSource(fp, 2, fpSrc, NULL);

    glCompileShader(vp);
    int rvalue;
    glGetShaderiv(vp, GL_COMPILE_STATUS, &rvalue);
    if (!rvalue) {
        fprintf(stderr, "Error in compiling vp\n");
        exit(30);
    }
    glAttachShader(progHandle, vp);

    glCompileShader(fp);
    glGetShaderiv(fp, GL_COMPILE_STATUS, &rvalue);
    if (!rvalue) {
        fprintf(stderr, "Error in compiling fp\n");
        exit(31);
    }
    glAttachShader(progHandle, fp);

  glBindFragDataLocation(progHandle, 0, "color");
    glLinkProgram(progHandle);

    glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
    if (!rvalue) {
        fprintf(stderr, "Error in linking sp\n");
        exit(32);
    }

  glUseProgram(progHandle);
  glUniform1i(glGetUniformLocation(progHandle, "srcTex"), 0);

  GLuint vertArray;
    glGenVertexArrays(1, &vertArray);
  glBindVertexArray(vertArray);

  GLuint posBuf;
  glGenBuffers(1, &posBuf);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf);
  float data[] = {
    -1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f
  };
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, data, GL_STREAM_DRAW);
  GLint posPtr = glGetAttribLocation(progHandle, "pos");
    glVertexAttribPointer(posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posPtr);

  return progHandle;
}

GLuint genTexture() {
  // Create floating point RGBA texture
  GLuint texHandle;
  glGenTextures(1, &texHandle);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);

  glBindImageTexture(0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  checkErrors("Gen Texture");
  return texHandle;
}



void
myInit()
{
  gTexID = genTexture();

  //
  // compute shader setup.
  //
  bool ret = LoadShader(GL_COMPUTE_SHADER, gComputeShader, "ao.comp");

  gComputeProg = glCreateProgram();
  glAttachShader(gComputeProg, gComputeShader);
  glLinkProgram(gComputeProg);

  GLint val;
  glGetProgramiv(gComputeProg, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");

  printf("Link shader OK\n");


  //
  // Render setup
  //
  gRenderProg = myRenderShaderInit();
}


} // namespace

// ---------------------------------------------


int main(int argc, char *argv[])
{

  glfwInit();
  glfwOpenWindow(0, 0, 0, 0, 0, 0, 32, 0, GLFW_WINDOW);
  glfwSetWindowTitle("Compute Shader Test");
  glfwSwapInterval(1);

  // glewInit() should be called after glfw initialization.
  if (GLEW_OK != glewInit()) {
    fprintf(stderr, "Failed to initialize GLEW. Exitting.\n");
    exit(1);
  }

  printf("GL_VENDOR                   : %s\n", glGetString(GL_VENDOR));
  printf("GL_RENDERER                 : %s\n", glGetString(GL_RENDERER));
  printf("GL_VERSION                  : %s\n", glGetString(GL_VERSION));
  printf("GL_SHADING_LANGUAGE_VERSION : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  myInit();

  glfwSetWindowSizeCallback(reshape);
  glfwSetKeyCallback(key);

  double startTime = glfwGetTime();
  int i = 0;
  while (glfwGetWindowParam(GLFW_OPENED)) {
    updateTex(i);
    draw();
    glfwSwapBuffers();

    i++;
    if (i >= 1024) {
      printf("Rendered 1024 frames.\n");
      double endTime = glfwGetTime();
      printf("Time to render 1024 frames: %f sec(s)\n", endTime - startTime);
      break;
    }
  }

  glfwTerminate();

  return 0;
}
