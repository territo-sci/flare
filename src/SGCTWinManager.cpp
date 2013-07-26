/*
 *
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <SGCTWinManager.h>
#include <Renderer.h>
#include <Config.h>
#include <Utils.h>
#include <functional>

using namespace osp;

// Static variable definitions
SGCTWinManager * SGCTWinManager::instance_= NULL;
float SGCTWinManager::oldTime_ = 0.f;
float SGCTWinManager::currentTime_ = 0.f;
float SGCTWinManager::elapsedTime_ = 0.f;

SGCTWinManager * SGCTWinManager::Instance() {
  if (!instance_) {
    instance_ = new SGCTWinManager();
  }
  return instance_;
}

SGCTWinManager::SGCTWinManager() 
  : engine_(NULL), config_(NULL), renderer_(NULL) {
}

void SGCTWinManager::SetRenderer(Renderer *_renderer) {
  renderer_ = _renderer;
}

void SGCTWinManager::SetConfig(Config *_config) {
  config_ = _config;
}

SGCTWinManager::~SGCTWinManager() {
  if (engine_) {
    delete engine_;
  }
}

bool SGCTWinManager::InitEngine(int _argc, char **_argv, 
                               sgct::Engine::RunMode _runMode) {
  if (engine_) {
    ERROR("Engine already initialized");
    return false;
  }

  engine_ = new sgct::Engine(_argc, _argv);

  InitNavigation();

  engine_->setDrawFunction(Draw);

  if (engine_->init(_runMode)) {
    ERROR("Failed to init engine");
    return false;
  }

  return true;
}

bool SGCTWinManager::Render() {
  if (!config_) {
    ERROR("Win manager: No config");
    return false;
  } 

  if (!renderer_) {
    ERROR("Win Manager: No renderer");
    return false;
  }

  engine_->render();
  return true;
}


// Helper definitions
// ------------------------------------------------------------

void SGCTWinManager::InitNavigation() {
  keysToCheck.push_back('R');

  // Translate X
  keysToCheck.push_back('Q');
  keysToCheck.push_back('A');

  // Translate Y
  keysToCheck.push_back('E');
  keysToCheck.push_back('D');

  // Translate Z
  keysToCheck.push_back('W'); 
  keysToCheck.push_back('S');


  keysToCheck.push_back('F');
  keysToCheck.push_back(32);
  keysToCheck.push_back('Z');
  keysToCheck.push_back('X');
  keysToCheck.push_back('P');
  keysToCheck.push_back('T');
  keysToCheck.push_back('U');
}

void SGCTWinManager::UpdateNavigation() {
  for (auto it = keysToCheck.begin(); it != keysToCheck.end(); it++) {
    renderer_->SetKeyPressed(*it, (glfwGetKey(*it) == GLFW_PRESS));
  }

  int xMouse, yMouse;
  glfwGetMousePos(&xMouse, &yMouse);
  renderer_->SetMousePosition((float)xMouse, (float)yMouse);

  bool leftButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool rightButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  renderer_->SetMousePressed(leftButton, rightButton);
}

// Callback definitions
// --------------------------------------------------------------

void SGCTWinManager::Draw() {;

  // Update time
  oldTime_ = currentTime_;
  currentTime_ = static_cast<float>(glfwGetTime());
  elapsedTime_ = currentTime_ - oldTime_;

  // Handle user input
  Instance()->UpdateNavigation();

  // Render
  Instance()->renderer_->Render(elapsedTime_);
}


