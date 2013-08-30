/*
 *
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <SGCTWinManager.h>
#include <Raycaster.h>
#include <Config.h>
#include <Utils.h>
#include <Animator.h>

using namespace osp;

// Static variable definitions
SGCTWinManager * SGCTWinManager::instance_= NULL;
float SGCTWinManager::oldTime_ = 0.f;
float SGCTWinManager::currentTime_ = 0.f;
sgct::SharedFloat SGCTWinManager::elapsedTime_;
sgct::SharedBool SGCTWinManager::animationPaused_;
sgct::SharedBool SGCTWinManager::fpsMode_;
sgct::SharedInt SGCTWinManager::manualTimestep_;
bool SGCTWinManager::leftMouseButton_ = false;
int SGCTWinManager::currentMouseX_ = 0;
int SGCTWinManager::currentMouseY_ = 0;
int SGCTWinManager::lastMouseX_ = 0;
int SGCTWinManager::lastMouseY_ = 0;
sgct::SharedFloat SGCTWinManager::pitch_;
sgct::SharedFloat SGCTWinManager::yaw_;
sgct::SharedFloat SGCTWinManager::roll_;
sgct::SharedFloat SGCTWinManager::translateX_;
sgct::SharedFloat SGCTWinManager::translateY_;
sgct::SharedFloat SGCTWinManager::translateZ_;
sgct::SharedBool SGCTWinManager::reloadFlag_;


SGCTWinManager * SGCTWinManager::Instance() {
  if (!instance_) {
    instance_ = new SGCTWinManager();
  }
  return instance_;
}

SGCTWinManager::SGCTWinManager() 
  : engine_(NULL), config_(NULL), raycaster_(NULL) {
}

void SGCTWinManager::SetRaycaster(Raycaster *_renderer) {
  raycaster_ = _renderer;
}

void SGCTWinManager::SetConfig(Config *_config) {
  config_ = _config;
}

void SGCTWinManager::SetAnimator(Animator *_animator) {
  animator_ = _animator;
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
  engine_->setPreSyncFunction(PreSync);
  engine_->setPostDrawFunction(PostDraw);
  engine_->setKeyboardCallbackFunction(Keyboard);
  engine_->setMouseButtonCallbackFunction(Mouse);
  sgct::SharedData::Instance()->setEncodeFunction(Encode);
  sgct::SharedData::Instance()->setDecodeFunction(Decode);

  if (!engine_->init(_runMode)) {
    ERROR("Failed to init engine");
    return false;
  }

  reloadFlag_.setVal(false);

  return true;
}

bool SGCTWinManager::Render() {
  if (!config_) {
    ERROR("Win manager: No config");
    return false;
  } 

  if (!raycaster_) {
    ERROR("Win Manager: No renderer");
    return false;
  }

  engine_->render();
  return true;
}

void SGCTWinManager::InitNavigation() {
  animationPaused_.setVal(false);

  // FPS mode should be OFF for cluster syncing
  fpsMode_.setVal(true);
  manualTimestep_.setVal(0);

  // Read initial values from config
  Config *c = Instance()->config_;

  pitch_.setVal(c->StartPitch());
  yaw_.setVal(c->StartYaw());
  roll_.setVal(c->StartRoll());

  translateX_.setVal(c->TranslateX());
  translateY_.setVal(c->TranslateY());
  translateZ_.setVal(c->TranslateZ());
}


void SGCTWinManager::Keyboard(int _key, int _action) {

  if (Instance()->engine_->isMaster()) {

    if (_action == GLFW_PRESS) {

    switch(_key) {
    case 32: // space bar
      // Toggle animation paused
      INFO("Pausing");
      animationPaused_.setVal(!animationPaused_.getVal());
      break;
    case 'Z':
    case 'z':
      // Decrease timestep
      // NOTE: Can't decrease timestep with double buffered approach atm
      //manualTimestep_.setVal(-1);
      break;
    case 'X':
    case 'x':
      // Increase timestep
      manualTimestep_.setVal(1);
      break;
    case 'D':
    case 'd':
      translateX_.setVal(translateX_.getVal() + 
        Instance()->config_->ZoomFactor());
      break;
    case 'A':
    case 'a':
      translateX_.setVal(translateX_.getVal() - 
        Instance()->config_->ZoomFactor());
      break;
    case 'W':
    case 'w':
      translateY_.setVal(translateY_.getVal() + 
        Instance()->config_->ZoomFactor());
      break;
    case 'S':
    case 's':
      translateY_.setVal(translateY_.getVal() - 
        Instance()->config_->ZoomFactor());
      break;
    case 'Q':
    case 'q':
      translateZ_.setVal(translateZ_.getVal() + 
        Instance()->config_->ZoomFactor());
      break;
    case 'E':
    case 'e':
      translateZ_.setVal(translateZ_.getVal() - 
        Instance()->config_->ZoomFactor());
      break;
    case 'R':
    case 'r':
      reloadFlag_.setVal(true);
      break;
    case 'F':
    case 'f':
      fpsMode_.setVal(!fpsMode_.getVal());
      if (fpsMode_.getVal()) {
        INFO("Updating animation ASAP");
      } else {
        INFO("Using refresh interval variable");
      }
      break;
    }

    }
  }
}

void SGCTWinManager::Mouse(int _button, int _action) {
  if (Instance()->engine_->isMaster()) { 

    switch (_button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      leftMouseButton_ = (_action == GLFW_PRESS) ? true : false;
      sgct::Engine::getMousePos(&lastMouseX_, &lastMouseY_);
    }

  }
}


void SGCTWinManager::PreSync() {
  if (Instance()->engine_->isMaster()) {

    // Update time
    oldTime_ = currentTime_;
    currentTime_ = static_cast<float>(sgct::Engine::getTime());
    elapsedTime_.setVal(currentTime_ - oldTime_);

    // Update automatic model transform
    if (!animationPaused_.getVal()) {
      pitch_.setVal(pitch_.getVal() +
                   Instance()->config_->PitchSpeed());
      roll_.setVal(roll_.getVal() +
                  Instance()->config_->RollSpeed());
      yaw_.setVal(yaw_.getVal() +
                  Instance()->config_->YawSpeed());
    }

    // Update mouse
    if (leftMouseButton_) {
      sgct::Engine::getMousePos(&currentMouseX_, &currentMouseY_);
      pitch_.setVal(pitch_.getVal() + 
                    Instance()->config_->MousePitchFactor() * 
                    static_cast<float>(currentMouseX_-lastMouseX_));
      roll_.setVal(roll_.getVal() +
                   Instance()->config_->MouseRollFactor() *
                   static_cast<float>(currentMouseY_-lastMouseY_));
    }
  }
}

void SGCTWinManager::PostDraw() {
  if (Instance()->engine_->isMaster()) {

    // Reset manual timestep
    manualTimestep_.setVal(0);

    // Reset reload flag
    reloadFlag_.setVal(false);
  }
}


void SGCTWinManager::Draw() {

  // Reload config if flag is set
  if (reloadFlag_.getVal()) {
    Instance()->raycaster_->Reload();
  }

  // Set model and view params
  Instance()->raycaster_->SetModelParams(pitch_.getVal(),
                                         yaw_.getVal(),
                                         roll_.getVal());
  Instance()->raycaster_->SetViewParams(translateX_.getVal(),
                                        translateY_.getVal(),
                                        translateZ_.getVal());
  // Render
  if (!Instance()->raycaster_->Render(elapsedTime_.getVal())) {
    exit(1);
  }
  
  // Save screenshot
  if (Instance()->config_->TakeScreenshot()) {
    Instance()->engine_->takeScreenshot();
  }



  // Update animator with synchronized time
  Instance()->animator_->SetPaused(animationPaused_.getVal());
  Instance()->animator_->SetFPSMode(fpsMode_.getVal());
  Instance()->animator_->Update(elapsedTime_.getVal());
  Instance()->animator_->ManualTimestep(manualTimestep_.getVal());

}

void SGCTWinManager::Encode() {
  sgct::SharedData::Instance()->writeBool(&animationPaused_);
  sgct::SharedData::Instance()->writeBool(&fpsMode_);
  sgct::SharedData::Instance()->writeFloat(&elapsedTime_);
  sgct::SharedData::Instance()->writeInt(&manualTimestep_);
  sgct::SharedData::Instance()->writeFloat(&pitch_);
  sgct::SharedData::Instance()->writeFloat(&yaw_);
  sgct::SharedData::Instance()->writeFloat(&roll_);
  sgct::SharedData::Instance()->writeFloat(&translateX_);
  sgct::SharedData::Instance()->writeFloat(&translateY_);
  sgct::SharedData::Instance()->writeFloat(&translateZ_);
  sgct::SharedData::Instance()->writeBool(&reloadFlag_);
}

void SGCTWinManager::Decode() {
  sgct::SharedData::Instance()->readBool(&animationPaused_);
  sgct::SharedData::Instance()->readBool(&fpsMode_);
  sgct::SharedData::Instance()->readFloat(&elapsedTime_);
  sgct::SharedData::Instance()->readInt(&manualTimestep_);
  sgct::SharedData::Instance()->readFloat(&pitch_);
  sgct::SharedData::Instance()->readFloat(&yaw_);
  sgct::SharedData::Instance()->readFloat(&roll_);
  sgct::SharedData::Instance()->readFloat(&translateX_);
  sgct::SharedData::Instance()->readFloat(&translateY_);
  sgct::SharedData::Instance()->readFloat(&translateZ_);
  sgct::SharedData::Instance()->readBool(&reloadFlag_);
}


