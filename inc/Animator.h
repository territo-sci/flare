#ifndef ANIMATOR_H
#define ANIMATOR_H

/*
 * Author: Victor Sand (victor.sand@gmail.com) 
 * Encapsulates animation logic 
 *
 */

namespace osp {

class Animator {
public:
  static Animator * New();

  // Signals the animator to update its state if needed
  void Update(float _elapsedTime);
  // Pauses if unpaused, unpauses if paused
  void TogglePause();
  // If FPS mode is on, don't wait. Update every timestep.
  void ToggleFPSMode();

  unsigned int CurrentTimestep() const { return currentTimestep_; }

  void SetCurrentTimestep(unsigned int _timestep);
  void SetRefreshInterval(float _refreshInterval);
  void SetNumTimesteps(unsigned int _numTimesteps);
  void IncTimestep();
  void DecTimestep();

private:
  Animator();
  Animator(const Animator&) { }
  
  // Number of timesteps before looping occurs
  unsigned int numTimesteps_;
  // Current timestep, the output of the Animator
  unsigned int currentTimestep_;
  bool fpsMode_;
  bool paused_;
  // Keeps track of elapsed time between timestep updates 
  float elapsedTime_;
  // Time before timestep gets updates
  float refreshInterval_;
};

}

#endif

