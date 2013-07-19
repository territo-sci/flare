#include <Animator.h>
#include <Utils.h>
#include <Config.h>

using namespace osp;

Animator * Animator::New(Config *_config) {
  Animator *animator = new Animator(_config);
  animator->UpdateConfig();
  return animator;
}

Animator::Animator(Config *_config) 
  : numTimesteps_(0),
    currentTimestep_(0),
    fpsMode_(true),
    paused_(false),
    elapsedTime_(0.f),
    refreshInterval_(0.f),
    config_(_config) {
}

void Animator::SetCurrentTimestep(unsigned int _timestep) {
  currentTimestep_ = _timestep;
}

void Animator::UpdateConfig() {
  refreshInterval_ = config_->AnimatorRefreshInterval();
}

void Animator::Update(float _elapsedTime) {

  if (paused_) return;
 
  if (fpsMode_) {
    IncTimestep();
    return;
  }

  // Add time to the time that might have been left from last update
  elapsedTime_ += _elapsedTime;

  // Update as many times as needed
  while (elapsedTime_ > refreshInterval_) {
    elapsedTime_ -= refreshInterval_;
    IncTimestep();
  }

}

void Animator::ToggleFPSMode() {
  fpsMode_ = !fpsMode_;
}

void Animator::TogglePause() {
  paused_ = !paused_;
}

void Animator::SetNumTimesteps(unsigned int _numTimesteps) {
  numTimesteps_ = _numTimesteps;
}

void Animator::IncTimestep() {
  currentTimestep_++;
  if (currentTimestep_ == numTimesteps_) {
    currentTimestep_ = 0;
  }
}

void Animator::DecTimestep() {
  if (currentTimestep_ == 0) {
    currentTimestep_ = numTimesteps_-1;
  } else {
    currentTimestep_--;
  }
}
