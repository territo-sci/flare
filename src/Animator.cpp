#include <Animator.h>
#include <Utils.h>

using namespace osp;

Animator * Animator::New() {
  return new Animator();
}

Animator::Animator() 
  : numTimesteps_(0),
    currentTimestep_(0),
    paused_(false),
    elapsedTime_(0.f),
    refreshInterval_(0.f) {
}

void Animator::SetCurrentTimestep(unsigned int _timestep) {
  currentTimestep_ = _timestep;
}

void Animator::SetRefreshInterval(float _refreshInterval) {
  refreshInterval_ = _refreshInterval;
}

void Animator::Update(float _elapsedTime) {

  if (paused_) return;

  // Add time to the time that might have been left from last update
  elapsedTime_ += _elapsedTime;

  // Update as many times as needed
  while (elapsedTime_ > refreshInterval_) {
    elapsedTime_ -= refreshInterval_;
    if (currentTimestep_ < numTimesteps_-1) {
      currentTimestep_++;
    } else {
      currentTimestep_ = 0;
    }
  }

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
