#include <GL/glew.h>
#include <GL/glfw.h>
#include <WindowManager.h>
#include <Renderer.h>
#include <Utils.h>

using namespace osp;

WindowManager::WindowManager(unsigned int _width,
                             unsigned int _height,
                             std::string _title)
                             :
                             width_(_width),
                             height_(_height),
                             title_(_title),
                             windowOpen_(false),
                             renderer_(NULL) { 
}    

WindowManager * WindowManager::New(unsigned int _width,
                                   unsigned int _height,
                                   std::string _title) {
  return new WindowManager(_width, _height, _title);
}

bool WindowManager::OpenWindow() {
  // Reset any errors
  glGetError();

  INFO("Opening " << width_ << " x " << height_ << " window");

  if (windowOpen_) {
    WARNING("Window already open");
    return true;
  }

  // Init GLFW
  if (glfwInit() != GL_TRUE) {
    ERROR("Failed to init GLFW");
    return false;
  }

  // Open GLFW window
  if (!glfwOpenWindow(width_, height_, 8, 8, 8, 8, 8, 8, GLFW_WINDOW)) {
    ERROR("WindowManager failed to open GLFW window");
    return false;
  }

  glfwSetWindowTitle(title_.c_str());
  windowOpen_ = true;

  // Check OpenGL version
  char *glVersion = (char*)glGetString(GL_VERSION);
  if (glVersion) {
    INFO("OpenGL version: " << glVersion);
  } else {
    WARNING("Failed to get OpenGL version");
  }

  // GLEW has to be initialized after context (window) has been set up
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    ERROR("Failed to initialize GLEW: " << glewGetErrorString(err));
    return false;
  } else {
    INFO("Using GLEW " << glewGetString(GLEW_VERSION));
  }

  // Check for errors and return
  CheckGLError("OpenWindow()");
  return true;
}

bool WindowManager::StartLoop() {

  glGetError();

  INFO("Starting rendering loop");

  if (!windowOpen_) {
    ERROR("Window not open");
    return false;
  }

  if (!renderer_) {
    ERROR("Renderer has not been set");
    return false;
  }

  float oldTime = glfwGetTime();
  float currentTime = oldTime;

  // Start the rendering loop
  while (true) {

    // Handle input
    if (glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS || 
        glfwGetWindowParam(GLFW_OPENED) == false) {
      Close();
      return false;
    }

    if (glfwGetKey('R') == GLFW_PRESS) {
      renderer_->SetKeyPressed("R", true);
    } else {
      renderer_->SetKeyPressed("R", false);
    }

    // Mouse
    int xMouse, yMouse;
    glfwGetMousePos(&xMouse, &yMouse);
    renderer_->SetMousePosition((float)xMouse, (float)yMouse);

    bool leftButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)== GLFW_PRESS;
    renderer_->SetMousePressed(leftButton, rightButton);

    // Update time
    oldTime = currentTime;
    currentTime = glfwGetTime();

    // Draw timestep
    if (!Draw(currentTime - oldTime)) {
      INFO("Rendering terminated, exiting");
      Close();
      return false;
    }

    CheckGLError("End of loop");
  }

  return true;
} 

bool WindowManager::Close() {
  glfwTerminate();
  INFO("GLFW terminated");
  return true;
}

bool WindowManager::Draw(float _timeStep) {
  if (!renderer_->Render(_timeStep)) {
    return false;
  }
  glfwSwapBuffers();
  return true;
}

void WindowManager::SetRenderer(Renderer *_renderer) {
  renderer_ = _renderer;
}





