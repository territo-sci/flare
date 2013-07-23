/*
 *
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <SGCTWinManager.h>
#include <Renderer.h>
#include <Config.h>
#include <Utils.h>

using namespace osp;

SGCTWinManager * SGCTWinManager::New(Config *_config) {
  SGCTWinManager *mgr = new SGCTWinManager(_config);
  mgr->UpdateConfig();
  return mgr;
}

SGCTWinManager::SGCTWinManager(Config *_config)
  : config_(_config), gEngine_(NULL), renderer_(NULL),
    winWidth_(0), winHeight_(0) {}

SGCTWinManager::~SGCTWinManager() {}

bool SGCTWinManager::UpdateConfig() {
  winWidth_ = config_->WinWidth();
  winHeight_ = config_->WinHeight();
  return true;
}

void InitOGL() {

  INFO("Initializing OpenGL");
  INFO("Initializing GLEW");
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    ERROR("Failed to init GLEW: " << glewGetErrorString(err));
  } else {
    INFO("Using GLEW " << glewGetString(GLEW_VERSION));
  }

}

bool SGCTWinManager::InitEngine() {

  // TODO temp hardcode
  int argc = 3;
  char** argv = new char*[argc];
  argv[0] = "FlareApp\0";
  argv[1] = "-config\0";
  argv[2] = "config.xml\0";

  gEngine_ = new sgct::Engine(argc, argv);

  gEngine_->setInitOGLFunction(InitOGL);

  if (!gEngine_->init(sgct::Engine::OpenGL_4_2_Core_Profile)) {
    ERROR("Failed to init SGCT engine");
    delete gEngine_;
    return false;
  }

  return true;
}

bool SGCTWinManager::Render() {
  gEngine_->render();
  return true;
}


